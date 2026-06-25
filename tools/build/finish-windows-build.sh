#!/usr/bin/env bash
# Finish/resume the Windows (Canadian-cross) GCC build: re-link static, build
# cc1plus.exe, install, and stage to C:. Safe to re-run. Run inside WSL.
set -euo pipefail
W="$HOME/winbuild"; DEPS="$W/mingw-deps"; PREFIX="$W/out-win"
LDF="-L$DEPS/lib -static -static-libgcc -static-libstdc++"
WINOUT=/mnt/c/dspic-cpp-toolchain-win

cd "$W/obj-gcc"
# Canadian-cross quirk: the `specs` target runs the host xgcc to -dumpspecs, but a
# Windows xgcc.exe hangs under WSL interop for -dumpspecs. The specs file is
# OPTIONAL (the installed compiler uses built-in specs), so pre-satisfy the target
# with an empty placeholder (made newest) so make skips it. Requires xgcc.exe built.
( cd gcc
  # Remove any leftover no-.exe wrappers/symlinks, and any driver .exe that got
  # clobbered into a text script, so make re-links them as real PE binaries.
  for d in xgcc xg++ gcc-cross g++-cross cpp; do
    rm -f "$d"
    [ -f "$d.exe" ] && file -b "$d.exe" 2>/dev/null | grep -qi "text\|script" && rm -f "$d.exe"
  done
  # Pre-satisfy build steps that RUN the host compiler (hang/127 under WSL): specs
  # (-dumpspecs) + self-tests. Optional for the final compiler. Future-date the
  # stamps so a re-linked xgcc.exe can't make them look out-of-date.
  : > specs; for s in s-selftest-c s-selftest-c++ s-selftest; do : > "$s"; done
  touch -d 2099-01-01 specs s-selftest-c s-selftest-c++ s-selftest 2>/dev/null \
    || touch specs s-selftest-c s-selftest-c++ s-selftest
)
echo ">> make all-gcc (static link)"
make -j"$(nproc)" all-gcc LDFLAGS="$LDF" CXXFLAGS_FOR_BUILD="-O2" || { echo "MAKE all-gcc FAILED"; exit 1; }
echo ">> install-gcc"
make install-gcc LDFLAGS="$LDF" || { echo "INSTALL FAILED"; exit 1; }

echo ">> stage to $WINOUT"
rm -rf "$WINOUT"; mkdir -p "$WINOUT"; cp -r "$PREFIX/"* "$WINOUT/"
CC1PLUS="$(find "$WINOUT" -name cc1plus.exe | head -1)"
GPP="$(ls "$WINOUT"/bin/*g++*.exe 2>/dev/null | head -1)"
echo "RESULT cc1plus=$CC1PLUS"
echo "RESULT g++=$GPP"
[ -n "$CC1PLUS" ] && [ -n "$GPP" ] && echo "WINDOWS COMPILER OK" || echo "WINDOWS COMPILER INCOMPLETE"
