#!/usr/bin/env bash
###############################################################################
# build-on-wsl.sh — build a C++-enabled dsPIC (pic30-elf) toolchain in WSL/Linux.
#
# GCC's Unix build system runs cleanly on Linux (no MSYS path/quoting issues),
# so we build here to PROVE Phase 1. Builds binutils + GCC(c,c++) for pic30-elf
# from Microchip's GPL source, mirroring build_XC16_831's flags + c++.
#
# Run inside WSL Ubuntu (optionally pass the GPL source root for a NEW version):
#   bash tools/build/build-on-wsl.sh [/mnt/c/path/to/<ver>/src/xc16_gcc_<n>]
#
# For a new XC compiler version: download its GPL source, extract under C:, and
# pass the 'xc16_gcc_*' dir (the one containing gcc/ and binutils/) as arg 1.
#
# Prereqs (install once, needs sudo): use gcc-11 host (NOT gcc-15/16 — too new):
#   sudo apt update && sudo apt install -y build-essential gcc-11 g++-11 bison \
#        flex libgmp-dev libmpfr-dev libmpc-dev libisl-dev texinfo libexpat1-dev
###############################################################################
set -euo pipefail

# --- locations (build in native WSL fs for speed; /mnt/c is slow) ----------
# Source root on C: — override by passing it as arg 1 (for a new XC version).
SRC_WIN_ROOT="${1:-/mnt/c/xcdsc-src/v3.31.01.src/src/xc16_gcc_831}"
WORK="${WORK:-$HOME/xcdsc-build}"                               # native WSL fs
SRC="$WORK/src/xc16_gcc_831"
BINU_SRC="$SRC/binutils"            # binutils-2.32 tree (Microchip pic30)
GCC_SRC="$SRC/gcc"                  # gcc 8.3.1 tree
PREFIX="$WORK/out"                  # toolchain install prefix
TARGET="pic30-elf"
JOBS="$(nproc)"

log(){ printf '\n\033[1;36m>> %s\033[0m\n' "$*"; }
warn(){ printf '\033[1;33mWARN: %s\033[0m\n' "$*" >&2; }
die(){ printf '\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

# --- 0. dep check ----------------------------------------------------------
for t in make bison flex makeinfo; do command -v $t >/dev/null || die "missing build tool '$t' — run the apt install in the header"; done

# Host compiler: GCC 8.3.1 source needs a host gcc that is NOT too new (gcc-15/16
# libstdc++ collides with the old source). Prefer an older installed gcc.
HOST_CC="${HOST_CC:-$(command -v gcc-11 || command -v gcc-12 || command -v gcc-13 || command -v gcc)}"
HOST_CXX="${HOST_CXX:-$(command -v g++-11 || command -v g++-12 || command -v g++-13 || command -v g++)}"
[ -n "$HOST_CC" ] && [ -n "$HOST_CXX" ] || die "no host gcc/g++ found"
export CC="$HOST_CC" CXX="$HOST_CXX"
log "Host compiler: $CC / $CXX  ($($CC -dumpversion 2>/dev/null))"
case "$($CC -dumpversion 2>/dev/null)" in
    1[4-9]*|[2-9][0-9]*) warn "host gcc $($CC -dumpversion) is very new for GCC 8.3.1 — expect libstdc++ collisions; install gcc-11/g++-11" ;;
esac

# Header check via the compiler's real search path (Ubuntu multiarch puts gmp.h
# under /usr/include/x86_64-linux-gnu, so a fixed /usr/include path check is wrong).
for h in gmp mpfr mpc; do
    echo "#include <$h.h>" | "$HOST_CC" -E -x c - >/dev/null 2>&1 || die "missing lib$h-dev (compiler can't find $h.h)"
done

# --- 1. copy source from C: into native WSL fs (fast) ----------------------
if [ ! -d "$GCC_SRC/gcc/cp" ]; then
    [ -d "$SRC_WIN_ROOT/gcc/gcc/cp" ] || die "source not found at $SRC_WIN_ROOT (is C: mounted? is it extracted?)"
    log "Copying source from C: into $SRC (one-time, a few minutes)"
    mkdir -p "$SRC"
    cp -r "$SRC_WIN_ROOT/gcc" "$SRC/" 2>/dev/null || die "copy gcc failed"
    cp -r "$SRC_WIN_ROOT/binutils" "$SRC/" 2>/dev/null || die "copy binutils failed"
    # c30_resource lives one level up in the source tree
    mkdir -p "$WORK/src/c30_resource"
    cp -r "$SRC_WIN_ROOT/../c30_resource/." "$WORK/src/c30_resource/" 2>/dev/null || true
fi

RES_DIR="$(cd "$WORK/src/c30_resource/src/c30" 2>/dev/null && pwd || true)"
[ -n "$RES_DIR" ] && [ -f "$RES_DIR/resource_info.h" ] || die "c30_resource/resource_info.h not found under $WORK/src"
log "pic30 resource dir: $RES_DIR"

MCHP_DEFINES="-DLONG_MODIFIER -D_BUILD_C30_ -D_BUILD_MCHP_ -DRESOURCE_MISMATCH_OK -I$RES_DIR"

# binutils' stock config.sub (2.32) doesn't recognize the 'pic30' machine, but
# gcc's Microchip-patched config.sub does (-> pic30-unknown-elf). Copy the
# pic30-aware one over every config.sub in the binutils tree. Idempotent.
if [ -f "$GCC_SRC/config.sub" ] && ! bash "$BINU_SRC/config.sub" pic30-elf >/dev/null 2>&1; then
    find "$BINU_SRC" -name config.sub -exec cp -f "$GCC_SRC/config.sub" {} \;
    log "Patched binutils config.sub with pic30-aware version"
fi

# --- 2. binutils ------------------------------------------------------------
# The binutils 2.32 source in Microchip's GPL drop is STOCK (no pic30 target in
# bfd/config.bfd, gas/configure.tgt, ld/configure.tgt). So we CANNOT build Linux
# pic30 binutils from it. We don't need to: this build produces cc1plus/elf-g++,
# and we generate dsPIC ASSEMBLY (-S) for proof, then assemble/link/simulate with
# the Windows XC-DSC binutils (which do support pic30). Skip binutils here.
if grep -q "pic30" "$BINU_SRC/bfd/config.bfd" 2>/dev/null; then
    log "Configuring binutils ($TARGET)"
    rm -rf "$WORK/obj-binutils"; mkdir -p "$WORK/obj-binutils"; cd "$WORK/obj-binutils"
    CFLAGS="-O2 -DPIC30ELF -DPIC30 -DRESOURCE_MISMATCH_OK" \
    "$BINU_SRC/configure" --target="$TARGET" --prefix="$PREFIX" \
        --disable-nls --disable-werror --disable-gdb --disable-sim \
        || die "binutils configure failed"
    make -j"$JOBS" || die "binutils build failed"
    make install || die "binutils install failed"
    export PATH="$PREFIX/bin:$PATH"
else
    warn "Skipping binutils: this GPL source's binutils-2.32 is stock (no pic30 in"
    warn "bfd/config.bfd). cc1plus/elf-g++ still build; assemble/link the test with"
    warn "the Windows XC-DSC binutils (they support pic30)."
fi

# --- 3. use system gmp/mpfr/mpc (move in-tree aside) -----------------------
for d in gmp mpfr mpc isl; do
    [ -d "$GCC_SRC/$d" ] && [ ! -e "$GCC_SRC/$d.nobuild" ] && mv "$GCC_SRC/$d" "$GCC_SRC/$d.nobuild" && log "moved in-tree $d aside"
done || true

# --- 4. build gcc (c,c++) for pic30-elf ------------------------------------
log "Configuring GCC (c,c++) for $TARGET"
rm -rf "$WORK/obj-gcc"; mkdir -p "$WORK/obj-gcc"; cd "$WORK/obj-gcc"
CFLAGS="-O2 $MCHP_DEFINES" \
CXXFLAGS="-O2 $MCHP_DEFINES" \
"$GCC_SRC/configure" \
    --target="$TARGET" --prefix="$PREFIX" --program-prefix=elf- \
    --with-gmp=/usr --with-mpfr=/usr --with-mpc=/usr \
    --enable-stage1-languages=c --enable-languages=c,c++ \
    --disable-decimal-float --disable-multilib --disable-bootstrap \
    --disable-libffi --disable-libgomp --disable-libmudflap --disable-libquadmath \
    --enable-libssp --disable-libstdcxx-pch --disable-libstdcxx-verbose --disable-lto \
    --disable-maintainer-mode --disable-nls --disable-shared --disable-sim \
    --disable-threads --disable-tls --disable-gdb --disable-werror \
    --enable-interwork --enable-plugins --enable-target-optspace \
    --enable-sjlj-exceptions --with-gnu-as --with-gnu-ld \
    --without-isl --without-cloog --without-headers --with-dwarf2 \
    --with-pkgversion="dspicArduino C++ (GPL rebuild)" \
    MAKEINFO=makeinfo \
    || die "gcc configure failed (see $WORK/obj-gcc/config.log)"

log "Building GCC compilers (make -j$JOBS all-gcc)"
make -j"$JOBS" all-gcc || die "gcc build failed"
make install-gcc || die "gcc install failed"

GPP="$(ls "$PREFIX/bin/"*g++* 2>/dev/null | head -1)"
CC1PLUS="$(find "$WORK/obj-gcc" "$PREFIX" -name 'cc1plus' 2>/dev/null | head -1)"
[ -n "$CC1PLUS" ] || die "cc1plus not produced"
log "SUCCESS — C++ compiler built"
echo "  cc1plus : $CC1PLUS"
echo "  g++     : $GPP"
echo "  binutils: $PREFIX/bin/$TARGET-as"
echo ""
echo "Next: compile spike/phase1/cpp_test.cpp with $GPP and verify on the simulator."
