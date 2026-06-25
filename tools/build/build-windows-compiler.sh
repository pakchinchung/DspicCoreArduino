#!/usr/bin/env bash
###############################################################################
# build-windows-compiler.sh — produce a NATIVE WINDOWS C++ compiler (cc1plus.exe
# + elf-g++.exe) for dsPIC, via a Canadian cross built in WSL/Linux.
#
#   build = x86_64-linux-gnu   (we build here)
#   host  = x86_64-w64-mingw32 (the compiler binaries run on WINDOWS)
#   target= pic30-elf          (the dsPIC they compile for)
#
# Building on Linux avoids every Windows-shell build problem; the OUTPUT is a
# Windows .exe that end users run with NO WSL. Run once per XC version.
#
# Prereq (one sudo): the mingw-w64 cross toolchain
#   sudo apt install -y gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64
#
# Usage (inside WSL):  bash tools/build/build-windows-compiler.sh
###############################################################################
set -euo pipefail

HOSTTRIP=x86_64-w64-mingw32
TARGET=pic30-elf
BUILD_CC=gcc-11; BUILD_CXX=g++-11          # build-side compiler (avoids skew)
SRCROOT=/mnt/c/xcdsc-src/v3.31.01.src/src/xc16_gcc_831
GCC_SRC="$HOME/xcdsc-build/src/xc16_gcc_831/gcc"   # patched gcc source from build-on-wsl.sh
RES_DIR="$HOME/xcdsc-build/src/c30_resource/src/c30"
WORK="$HOME/winbuild"
DEPS="$WORK/mingw-deps"                     # gmp/mpfr/mpc built FOR mingw (host)
PREFIX="$WORK/out-win"                      # the Windows toolchain we ship
JOBS="$(nproc)"
WINOUT=/mnt/c/dspic-cpp-toolchain-win

log(){ printf '\n\033[1;36m>> %s\033[0m\n' "$*"; }
die(){ printf '\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

command -v ${HOSTTRIP}-g++ >/dev/null || die "mingw cross missing — run: sudo apt install -y gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64"
[ -d "$GCC_SRC/gcc/cp" ] || die "patched gcc source not found ($GCC_SRC) — run build-on-wsl.sh first (it copies + patches the source)"
[ -f "$RES_DIR/resource_info.h" ] || die "c30_resource not found ($RES_DIR)"

mkdir -p "$WORK"

# --- 1. cross-build gmp / mpfr / mpc for the mingw HOST ----------------------
build_dep(){ # $1=srcdir-glob  $2=name  extra-args...
    local glob="$1" name="$2"; shift 2
    [ -f "$DEPS/lib/lib${name}.a" ] && { log "$name already built for mingw"; return; }
    local src; src="$(ls -d "$SRCROOT"/$glob 2>/dev/null | head -1)"; [ -d "$src" ] || die "no source for $name ($glob)"
    log "Cross-building $name for $HOSTTRIP"
    rm -rf "$WORK/b-$name"; mkdir -p "$WORK/b-$name"; cd "$WORK/b-$name"
    # --build MUST be explicit (WSL runs .exe via interop -> autoconf otherwise
    # mis-detects a native build). CC_FOR_BUILD must be the Linux gcc so build-time
    # helper tools are runnable Linux binaries, not mingw .exe.
    CC_FOR_BUILD="$BUILD_CC" CXX_FOR_BUILD="$BUILD_CXX" \
    "$src/configure" --build=x86_64-linux-gnu --host="$HOSTTRIP" --prefix="$DEPS" \
        --disable-shared --enable-static "$@" >/dev/null \
        || die "$name configure failed"
    make -j"$JOBS" >/dev/null && make install >/dev/null || die "$name build failed"
}
build_dep "gmp-*"  gmp
build_dep "mpfr-*" mpfr --with-gmp="$DEPS"
build_dep "mpc-*"  mpc  --with-gmp="$DEPS" --with-mpfr="$DEPS"
build_dep "expat-*" expat --without-docbook   # Microchip's xc-covtool-comm.c needs expat.h

# --- 2. Canadian-cross build GCC (c,c++) for pic30-elf, hosted on Windows -----
MCHP_DEFINES="-DLONG_MODIFIER -D_BUILD_C30_ -D_BUILD_MCHP_ -DRESOURCE_MISMATCH_OK -I$RES_DIR"
log "Configuring Canadian-cross GCC (host=$HOSTTRIP, target=$TARGET)"
rm -rf "$WORK/obj-gcc"; mkdir -p "$WORK/obj-gcc" "$PREFIX"; cd "$WORK/obj-gcc"
CC_FOR_BUILD="$BUILD_CC" CXX_FOR_BUILD="$BUILD_CXX" \
CFLAGS="-O2 $MCHP_DEFINES -I$DEPS/include" CXXFLAGS="-O2 $MCHP_DEFINES -I$DEPS/include" \
LDFLAGS="-L$DEPS/lib -static -static-libgcc -static-libstdc++" \
"$GCC_SRC/configure" \
    --build=x86_64-linux-gnu --host="$HOSTTRIP" --target="$TARGET" \
    --prefix="$PREFIX" --program-prefix=elf- \
    --with-gmp="$DEPS" --with-mpfr="$DEPS" --with-mpc="$DEPS" \
    --enable-stage1-languages=c --enable-languages=c,c++ \
    --disable-decimal-float --disable-multilib --disable-bootstrap \
    --disable-libffi --disable-libgomp --disable-libmudflap --disable-libquadmath \
    --enable-libssp --disable-libstdcxx-pch --disable-libstdcxx-verbose --disable-lto \
    --disable-maintainer-mode --disable-nls --disable-shared --disable-sim \
    --disable-threads --disable-tls --disable-gdb --disable-werror \
    --enable-interwork --enable-plugins --enable-target-optspace \
    --enable-sjlj-exceptions --with-gnu-as --with-gnu-ld \
    --without-isl --without-cloog --without-headers --with-dwarf2 \
    --with-pkgversion="dspicArduino C++ (GPL rebuild, win)" \
    MAKEINFO=makeinfo \
    || die "configure failed (see $WORK/obj-gcc/config.log)"

log "Building (make -j$JOBS all-gcc) — produces Windows cc1plus.exe / elf-g++.exe"
make -j"$JOBS" all-gcc || die "build failed"
make install-gcc || die "install failed"

# --- 3. stage the Windows toolchain on C: ------------------------------------
CC1PLUS="$(find "$PREFIX" -name 'cc1plus.exe' | head -1)"
GPP="$(ls "$PREFIX/bin/"*g++*.exe 2>/dev/null | head -1)"
[ -n "$CC1PLUS" ] && [ -n "$GPP" ] || die "cc1plus.exe / g++.exe not produced"
log "Staging Windows toolchain -> $WINOUT"
rm -rf "$WINOUT"; mkdir -p "$WINOUT"; cp -r "$PREFIX/"* "$WINOUT/"
echo ""
echo "=============================================================="
echo " Windows C++ compiler built:"
echo "   driver : $WINOUT/bin/$(basename "$GPP")"
echo "   cc1plus: ${CC1PLUS/$PREFIX/$WINOUT}"
echo " Use it on Windows with the XC-DSC binutils (no WSL needed)."
echo "=============================================================="
