# Building the C++-enabled XC-DSC compiler

> ✅ **Done successfully** (XC-DSC v3.31.01 / GCC 8.3.1) on **WSL Ubuntu** with a
> **gcc-11 host**, and verified on the MPLAB X Simulator for both dsPIC33C and
> dsPIC33A. The blow-by-blow record (every issue + fix, symptom table, new-version
> checklist) is in **[`BUILD_JOURNEY.md`](BUILD_JOURNEY.md)**.

XC-DSC ships its GCC with the C++ front-end disabled. We rebuild it from
Microchip's GPL source with `--enable-languages=c,c++` — legal under GPLv3
(Microchip's EULA defers to the OSS licenses for the GCC binaries). The two
lessons that made it work:

1. **Build on Linux/WSL**, not native Windows/MSYS2 (avoids shell/path/quoting issues).
2. **Use a gcc-11/12 host**, not gcc-15/16 (new libstdc++ collides with GCC 8.3 source).

## All WSL/Ubuntu dependencies (complete list)

```bash
sudo apt update
# (a) build the Linux C++ compiler:
sudo apt install -y build-essential gcc-11 g++-11 bison flex \
     libgmp-dev libmpfr-dev libmpc-dev libisl-dev texinfo libexpat1-dev
# (b) ALSO build the native WINDOWS compiler (Canadian cross):
sudo apt install -y gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64
```

| Package | Why |
|---|---|
| `build-essential` | make + the build-side toolchain |
| `gcc-11` / `g++-11` | **host compiler for the build** — NOT gcc-15/16 (libstdc++ skew) |
| `bison`, `flex` | GCC parser/lexer generation |
| `libgmp-dev`, `libmpfr-dev`, `libmpc-dev`, `libisl-dev` | GCC math deps (Linux build) |
| `texinfo` | `makeinfo` (docs step in the build) |
| `libexpat1-dev` | Microchip's `xc-covtool-comm.c` needs `expat.h` |
| `gcc-mingw-w64-x86-64`, `g++-mingw-w64-x86-64` | Canadian cross → native Windows `cc1plus.exe` |

(For the Windows cross, `gmp`/`mpfr`/`mpc` are cross-built for mingw from the XC-DSC
source tree by `build-windows-compiler.sh` — no extra apt packages needed.)

## Quick start (WSL Ubuntu)

```bash
# Linux compiler (pass the GPL source root; omit for the v3.31 default):
HOST_CC=gcc-11 HOST_CXX=g++-11 bash tools/build/build-on-wsl.sh \
     [/mnt/c/path/to/<ver>/src/xc16_gcc_NN]

# Native Windows compiler (cc1plus.exe + elf-g++.exe -> C:\dspic-cpp-toolchain-win):
bash tools/build/build-windows-compiler.sh
```

Produces `~/xcdsc-build/out/bin/elf-g++` + `cc1plus`. It builds **GCC only** (the
binutils in the GPL drop are stock, no pic30 target) — assemble/link with the
Windows XC-DSC binutils, which support pic30.

## Verify (no hardware)

```bash
tools/test/run-cpp-sim-test.sh 33CK256MP508     # dsPIC33C
tools/test/run-cpp-sim-test.sh 33AK256MPS205    # dsPIC33A
```
Compiles a multi-object C++ test, links with the XC-DSC binutils, runs on the
MPLAB X Simulator, and prints PASS/FAIL.

## New XC compiler version — checklist

1. ☐ Download the new GPL source; note the GCC version (`gcc/BASE-VER`).
2. ☐ Pick a host gcc no more than ~3 versions newer (gcc-11/12).
3. ☐ Confirm `gcc/config/pic30` exists and `gcc/config.gcc` matches `pic30*-*-elf*`.
4. ☐ `bash tools/build/build-on-wsl.sh /mnt/c/.../xc16_gcc_NN`
5. ☐ `tools/test/run-cpp-sim-test.sh <device>` → expect PASS.
6. ☐ Work the symptom table in [`BUILD_JOURNEY.md`](BUILD_JOURNEY.md) for anything new.
7. ☐ **GPLv3:** publish the modified source you built from (copyleft).

## GPLv3 obligation

Redistributing the modified compiler binaries requires publishing the complete
corresponding source (build scripts + the pic30 back-end). Host your fork +
`tools/build/build-on-wsl.sh` publicly; a 3-year written offer is the alternative.
