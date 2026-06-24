# dspicArduino Core

An Arduino core that lets **standard, unmodified Arduino sketches run on Microchip
dsPIC** microcontrollers. The same `blink` or button sketch that works on an Uno
or an STM32 board compiles and runs on a dsPIC33 — the user writes ordinary
Arduino code and never has to know what happens underneath.

> **Status: early development.** The toolchain and core scaffold are in place and
> the C++ feasibility is proven on silicon (see below). The first end-to-end goal
> is **blink an LED + respond to a button press**.

---

## Why this is non-trivial (and how it's solved)

The Arduino API is C++ (`Serial`, `Wire`, `LiquidCrystal`, `String`, classes with
virtual methods…). Microchip's dsPIC compiler — **XC16 / XC-DSC** — ships with the
**C++ front-end disabled**, which is why no dsPIC Arduino core has existed.

This project's approach:

1. **Rebuild the compiler with C++ enabled.** XC-DSC is GCC (v3.31 = GCC 8.3.1).
   The C++ front-end is present in Microchip's GPL source but turned off; we
   rebuild it with `--enable-languages=c,c++`. This is legal under the GPLv3 that
   covers the GCC components (Microchip's EULA §4 defers to the OSS licenses for
   those binaries). See [`PLAN.md`](PLAN.md) and
   [`docs/BUILDING_THE_COMPILER.md`](docs/BUILDING_THE_COMPILER.md).
2. **Don't redistribute Microchip's proprietary parts.** Device headers, runtime,
   and linker scripts stay in the user's own XC-DSC install; our wrappers locate
   them at build time. We ship only the GPL compiler + our Arduino core.
3. **Implement the Arduino API** (`pinMode`, `digitalWrite`, `millis`,
   `HardwareSerial`, `Wire`, `SPI`, …) against dsPIC peripherals so stock sketches
   and libraries compile unchanged.

### Proven: the dsPIC runtime already supports C++

A Phase 1 spike confirmed on the MPLAB X simulator (using only the stock compiler)
that Microchip's `crt0` already calls static constructors before `main()` and that
vtable-style indirect dispatch works — so global objects and virtual methods will
work once `cc1plus` is built. Details in [`spike/phase1/README.md`](spike/phase1/README.md).

---

## Repository layout

```
boards.txt / platform.txt / programmers.txt   Arduino boards-manager definitions
package_dspicArduino_index.json               Boards Manager URL manifest
cores/dspic/                                   The Arduino API implementation
variants/dspic33ck256mp508/                    Per-board pin map + clock/config
tools/xc16pp/bin/                              Compiler wrappers (locate XC-DSC + DFP)
tools/build/                                   Scripts to build the C++ compiler
spike/phase1/                                  C++ feasibility proof + acceptance test
docs/BUILDING_THE_COMPILER.md                  How to (re)build the C++ compiler
PLAN.md                                         Full phased roadmap
```

---

## Getting started (developers)

Currently you build the C++-enabled compiler, then use the core from the Arduino IDE.

### Prerequisites
- Windows with **MPLAB XC-DSC** and **MPLAB X IDE** installed (provides the device
  packs, runtime libraries, and the simulator/programmer).
- A dsPIC33CK board or the MPLAB X **Simulator** (no hardware required to test).

### Build the C++ compiler (Phase 1)
Build the compiler in WSL Ubuntu (gcc-11 host), then verify on the simulator:

```bash
# one-time deps:
sudo apt install -y build-essential gcc-11 g++-11 bison flex \
     libgmp-dev libmpfr-dev libmpc-dev libisl-dev texinfo libexpat1-dev
# build the C++-enabled compiler:
HOST_CC=gcc-11 HOST_CXX=g++-11 bash tools/build/build-on-wsl.sh
# verify on the MPLAB X simulator (no hardware):
tools/test/run-cpp-sim-test.sh 33CK256MP508
```

Success looks like:
```
=== PASS — C++ verified on 33CK256MP508 ===
```

Full details (every issue + fix, new-version checklist) in
[`docs/BUILD_JOURNEY.md`](docs/BUILD_JOURNEY.md) and
[`docs/BUILDING_THE_COMPILER.md`](docs/BUILDING_THE_COMPILER.md).

Full instructions: [`docs/BUILDING_THE_COMPILER.md`](docs/BUILDING_THE_COMPILER.md).

---

## Roadmap

| Phase | Goal | Status |
|------:|------|--------|
| 1 | Build C++-enabled compiler; prove on silicon | runtime proven; build pending |
| 2 | Core + **blink** (clock, Timer1, `pinMode`/`digitalWrite`) | scaffolded |
| 3 | **Button** (`digitalRead`, `INPUT_PULLUP`) — MVP target | scaffolded |
| 4 | `HardwareSerial` / `Print` / `String` | stubs |
| 5 | `analogRead` / `analogWrite` | stubs |
| 6 | `Wire` (I²C) / `SPI` | planned |
| 7 | Boards-manager packaging + release | planned |
| 8 | More boards/variants | planned |

See [`PLAN.md`](PLAN.md) for the detailed plan.

---

## Licensing

- The Arduino core, wrappers, and build scripts in this repository: see repository
  license.
- The C++-enabled compiler is built from Microchip's **GPLv3** GCC sources;
  redistributing it carries the GPLv3 obligation to publish the corresponding
  modified source.
- Microchip's proprietary components (device headers, runtime, linker scripts) are
  **not** included here — users obtain them via their own XC-DSC installation.

This project is community-developed and is not an official Microchip or Arduino
product.
