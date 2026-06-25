# dspicArduino — prebuilt native Windows C++ compiler

This directory holds the **prebuilt, native Windows** C++ compiler for dsPIC,
packaged as the artifact the Arduino Boards Manager downloads. End users never
build it and never need WSL — they install it through the board manager.

## Artifact

| | |
|---|---|
| File | `dspic-cpp-toolchain-win-3.31.01.tar.bz2` |
| Size | `24348195` bytes (23.2 MB) |
| SHA-256 | `dffe3837cf8d970b92f08502804a44e520b1d82adb9672ba336ff5babf6c5acd` |
| Unpacks to | `dspic-cpp-toolchain-win/` |
| Compiler | GCC 8.3.1, target `pic30-elf`, C + C++ enabled |
| Host | `x86_64-w64-mingw32` (native Windows, static-linked — no DLLs needed) |

Key binaries inside:
- `bin/elf-g++.exe` — the C++ driver
- `libexec/gcc/pic30-elf/8.3.1/cc1plus.exe` — the C++ compiler proper

## What it is / why it exists

Microchip's XC-DSC ships with C++ **disabled**. This is the same GCC source
(XC-DSC v3.31.01 GPL drop) rebuilt with `--enable-languages=c,c++`. Rebuilding
the GPL compiler parts is legal; we do **not** redistribute Microchip's
proprietary device files — users install XC-DSC themselves for those (DFP
headers, `.gld` linker scripts, binutils, libs). See the licensing notes in the
repo docs.

## How it was built

Canadian cross, built in WSL/Linux, hosted on Windows:
`build = x86_64-linux-gnu`, `host = x86_64-w64-mingw32`, `target = pic30-elf`.
Scripts: `tools/build/build-windows-compiler.sh` + `tools/build/finish-windows-build.sh`.
Rebuild for a new XC-DSC version by pointing those scripts at the new GPL source.

## How the toolchain is used (no WSL)

The end user also installs Microchip XC-DSC (for device files + binutils). Our
compiler produces the C++ object/assembly; the stock `xc-dsc-gcc` assembles,
links (with the device `.gld`) and `xc-dsc-bin2hex` makes the `.hex`. The build
recipe is encoded in `tools/test/run-cpp-sim-test-win.sh` and will move into the
core's `platform.txt` recipes. Two flags are always injected for C++:
- `-include cores/dspic/dspic_cpp_compat.h` (lets `<xc.h>` parse in C++)
- `-I <XC-DSC>/include` (target `stdint.h` etc.; our compiler is `--without-headers`)

## Distribution note

For publishing, this archive should be uploaded as a **GitHub Release asset**
(or any HTTP host) and referenced by URL + this SHA-256 + size in
`package_dspicArduino_index.json`. It is committed here as well so the verified
artifact is never lost, but the board-manager index should point at the hosted
copy, not at the git blob.
