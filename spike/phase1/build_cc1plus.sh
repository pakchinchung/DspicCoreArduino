#!/usr/bin/env bash
# build_cc1plus.sh — build a C++-enabled compiler from the XC-DSC v3.31 GPL source.
#
# Target: GCC 8.3.1 (the base of XC-DSC v3.31.01), dsPIC/pic30 back-end.
# Output: cc1plus + an xc-dsc-g++ driver that reuse the STOCK xc-dsc binutils,
#         device packs (DFP) and runtime libs — none of which we redistribute.
#
# This is the ONE genuinely heavy step of the project. The Phase 1 spike already
# proved the dsPIC runtime supports C++ (static ctors + indirect dispatch), so
# the remaining work is purely building the front-end.
#
# PREREQUISITES (host build environment — e.g. MSYS2/MinGW on Windows, or Linux):
#   - gcc/g++, make, makeinfo, flex, bison
#   - GMP, MPFR, MPC, ISL (GCC build deps)
#   - The XC-DSC v3.31 GPL SOURCE drop from Microchip's compiler source page
#     (the install ships only LIBRARY sources, NOT the compiler source).
#
# !! Verify Microchip's EXACT configure line first (see step 2). Reproducing
#    their build config is what keeps our cc1plus ABI-compatible with their
#    elf-gcc / elf-cc1 objects and libraries.

set -euo pipefail

# ---- config ----------------------------------------------------------
SRC="${1:?usage: build_cc1plus.sh <path-to-xc-dsc-gpl-source> [install-prefix]}"
PREFIX="${2:-$PWD/cc1plus-build/install}"
BUILD="$PWD/cc1plus-build/obj"

# The XC-DSC GCC target triple. Confirm from the source's config.guess/Makefile;
# historically Microchip's 16-bit target is 'pic30' (elf flavor in v3.x).
TARGET="${XC_TARGET:-pic30-elf}"

mkdir -p "$BUILD" "$PREFIX"

# ---- step 1: sanity check the source tree ----------------------------
[ -d "$SRC/gcc" ]            || { echo "ERROR: $SRC has no gcc/ — not a GCC source tree"; exit 1; }
[ -d "$SRC/gcc/cp" ]         || { echo "ERROR: gcc/cp (C++ front-end) missing from source"; exit 1; }
[ -d "$SRC/gcc/config/pic30" ] || echo "WARN: gcc/config/pic30 not found — check back-end dir name"

# ---- step 2: recover Microchip's own configure flags -----------------
# Their build args are the safest baseline. Look here, then ADD c++:
echo ">> Microchip's configure args (if recorded in the source):"
grep -rhoE -- '--enable-languages=[^ ]+|--target=[^ ]+|--with[a-z-]+' \
    "$SRC"/*.log "$SRC"/Makefile* "$SRC"/gcc/config.log 2>/dev/null | sort -u || true
echo ">> (Mirror those flags below; the key change is adding c++ to --enable-languages)"

# ---- step 3: configure with C++ enabled, freestanding ----------------
cd "$BUILD"
"$SRC/configure" \
    --target="$TARGET" \
    --prefix="$PREFIX" \
    --enable-languages=c,c++ \
    --disable-libssp \
    --disable-libstdcxx-pch \
    --without-headers \
    --disable-shared \
    --disable-nls \
    --disable-libstdcxx \
    MAKEINFO=makeinfo

# Rationale:
#  --enable-languages=c,c++   : THE change — builds cc1plus + g++ driver
#  --without-headers          : freestanding; no hosted libc assumptions
#  --disable-libstdcxx        : no full C++ stdlib on a 16-bit Harvard MCU;
#                               we supply freestanding stubs instead (step 5)
#  (exceptions/RTTI are disabled per-compile via -fno-exceptions -fno-rtti in
#   platform.txt, not here)

# ---- step 4: build just the C++ front-end + driver -------------------
# Full 'make' would try to build target libs we don't want. Build the compilers.
make -j"$(nproc)" all-gcc

echo ""
echo ">> Built compilers:"
find "$BUILD/gcc" -maxdepth 1 -name 'cc1plus*' -o -maxdepth 1 -name 'g++*' 2>/dev/null
echo ""
echo "NEXT:"
echo "  - Copy cc1plus next to the xc16pp wrappers (so OUR_BIN finds it)."
echo "  - Provide it as 'xc-dsc-g++' driver, or symlink, matching detect_prefix()."
echo "  - Add freestanding C++ runtime stubs (step 5) and re-run the C++ spike:"
echo "      a class with a virtual method + a global object, expect 0xC0DE."
echo ""
echo "  Stub set to provide (check libcppcfl-elf.a first — it may have some):"
echo "    operator new / operator delete"
echo "    __cxa_pure_virtual, __cxa_atexit, __cxa_guard_acquire/release/abort"
echo "    __dso_handle (crt_ctor.s already defines ___dso_handle)"
