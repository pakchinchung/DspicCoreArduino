# dspicArduino Core

An Arduino core that lets **standard, unmodified Arduino sketches run on Microchip
dsPIC33** microcontrollers — including **C++** sketches and libraries, which
Microchip's stock compiler normally disables. Write ordinary Arduino code, pick
your pins by their datasheet names, and it compiles and runs on a dsPIC33.

> **Status: working (v0.1.4).** Full Arduino HAL implemented and verified on
> hardware (digital I/O, `Serial`, `Wire`/I²C, `SPI`, `analogRead`, `analogWrite`
> PWM, and DAC). Standard libraries compile against it — e.g. a 16×2 I²C LCD runs
> on the stock **LiquidCrystal_I2C** library.

---

## Features

- **Standard Arduino API** — `pinMode`/`digitalWrite`/`digitalRead`, `millis`/
  `micros`/`delay`, `Serial` (`HardwareSerial`/`Print`/`String`), `Wire` (I²C),
  `SPI`, `analogRead`, `analogWrite` (PWM), `dacWrite`, `tone`-style timing, etc.
- **Real C++** — classes, virtual methods, global constructors all work (the core
  ships a C++-enabled build of Microchip's own GCC; see *Licensing*).
- **Pins by native name** — `#define LED RE6; pinMode(LED, OUTPUT);`. No
  board-specific remap to memorize — any pin on the chip, by its datasheet name.
- **Multiple peripherals** — every UART/SPI/I²C instance is exposed:
  `Serial`/`Serial1`/`Serial2`, `Wire`/`Wire1`/`Wire2`, `SPI`/`SPI_2`/`SPI_3`.
- **Stock-library compatible** — `ARDUINO`/`ARDUINO_ARCH_DSPIC` defines, `binary.h`,
  and an `avr/pgmspace.h` shim, so most libraries that use `Wire`/`SPI`/`PROGMEM`
  compile unchanged.
- **Upload with no bootloader** — flashes via MPLAB IPE (`ipecmd`) using the
  on-board PKoB4 or an external PICkit 4/5 / SNAP / ICD 4.
- **Bench tooling** — chip erase + "build once, flash many" mass-programming
  scripts in [`tools/production/`](tools/production/).

## Supported boards

| Board | MCU | Notes |
|-------|-----|-------|
| dsPIC33CK256MP508 (generic) | dsPIC33CK256MP508 | Primary target (e.g. DM330030 Curiosity) |
| dsPIC33AK128MC106 (experimental) | dsPIC33AK128MC106 | Compiles/links; C++-on-hardware still being validated |

---

## Install

You need three things — the Arduino package (this repo) plus two **free** Microchip
downloads it relies on (their proprietary parts are *not* redistributed here):

1. **MPLAB XC-DSC compiler** — provides the assembler, libraries, and target C
   headers. <https://www.microchip.com/xcdsc>
2. **MPLAB X IDE** — provides the **Device Family Packs** (the `<xc.h>` headers,
   `.gld` linker scripts) and `ipecmd` used for upload. <https://www.microchip.com/mplabx>
3. **This board package** — in Arduino IDE:
   - **File ▸ Preferences ▸ Additional Boards Manager URLs**, add:
     ```
     https://raw.githubusercontent.com/pakchinchung/DspicCoreArduino/master/package_dspicArduino_index.json
     ```
   - **Tools ▸ Board ▸ Boards Manager**, search **dspic**, install **dspicArduino Core**.

> Tip: after a new version is published the IDE may keep a cached index. If the
> latest version doesn't appear, remove and re-add the URL (or delete the cached
> `…/Arduino15/package_dspicArduino_index.json`) and reopen Boards Manager.

---

## Quick start

```cpp
#define LED RE6              // any pin, by datasheet name

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(9600);        // PKoB4 virtual COM
}
void loop() {
  digitalWrite(LED, HIGH); delay(500);
  digitalWrite(LED, LOW);  delay(500);
  Serial.println("blink");
}
```

1. **Tools ▸ Board** → *dsPIC33CK256MP508 (generic)*.
2. **Verify** to compile, **Upload** to flash (on-board PKoB4 — no port/programmer
   selection needed). For an external programmer: **Tools ▸ Programmer**, then
   **Sketch ▸ Upload Using Programmer**.

### Peripheral pins
- **I²C** pins are fixed per module (not remappable). `Wire` (I²C1) defaults to the
  **alternate** pins **RC8 (SDA) / RC9 (SCL)** on this board; a **Tools ▸ "I2C1
  pins"** menu switches it to RB9/RB8. `Wire1`/`Wire2` use I²C2/I²C3.
- **UART** and **SPI** pins are remappable (PPS). `Serial` defaults to the PKoB4
  COM pins; for `Serial1`/`Serial2` and `SPI_2`/`SPI_3` call `setPins(...)` before
  `begin()`.

### Examples (File ▸ Examples)
`Blink`, `SerialEcho`, `CppShowcase`, `PotRGB`, `PwmSweep`, `DacAdcLoopback`,
`SpiLoopback`, `WireLoopback`, `I2cScanner`, `LcdHelloWorld`, `HalSanity`.

---

## Upload & erase

- **Upload**: MPLAB IPE headless via the on-board PKoB4 (default) or a selected
  external programmer. **Tools ▸ After Upload** chooses run vs. hold-in-reset.
- **Erase**: **Tools ▸ Burn Bootloader** performs a full chip erase (this core has
  no bootloader; the IDE just can't rename that menu item). A standalone
  `tools/production/dspic-erase.bat` does the same outside the IDE.
- **Mass-programming**: `tools/production/dspic-tool.bat` flashes the last
  compiled `.hex` to many boards without recompiling — see
  [`tools/production/README.md`](tools/production/README.md).

---

## Licensing

- The Arduino core, wrappers, and scripts here: see the repository license.
- The bundled C++-enabled compiler is built from Microchip's **GPLv3** GCC sources;
  redistribution carries the GPLv3 obligation to publish the corresponding source.
- Microchip's proprietary components (device headers, runtime, linker scripts,
  `ipecmd`) are **not** included — users obtain them via their own XC-DSC / MPLAB X
  installation.

This project is community-developed and is **not** an official Microchip or Arduino
product.
