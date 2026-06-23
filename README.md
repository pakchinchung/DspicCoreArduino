# dspicArduino Core

Run **standard Arduino sketches on Microchip dsPIC** microcontrollers
(dsPIC33CK and dsPIC33AK). You write ordinary Arduino code; the core handles the
rest — including C++ on dsPIC, which Microchip's stock compiler doesn't enable.

## Install (Arduino IDE)

1. **File ▸ Preferences ▸ Additional Boards Manager URLs**, add:
   ```
   https://raw.githubusercontent.com/pakchinchung/DspicCoreArduino/master/package_dspicArduino_index.json
   ```
2. **Tools ▸ Board ▸ Boards Manager** → search **dspic** → install **dspicArduino Core**.
3. Select **Tools ▸ Board ▸ dsPIC33CK256MP508 (generic)**.

### Prerequisites (free Microchip downloads)
- **MPLAB XC-DSC compiler** — assembler, libraries, target headers.
- **MPLAB X IDE** — device packs (`<xc.h>`, linker scripts) and the `ipecmd`
  programmer used by Upload.

The board package ships our GPL C++ compiler + the Arduino core; the Microchip
pieces above are installed by you (they are not redistributable).

## Use — pins by native name

Pick pins by their datasheet name (`RA0`…`RE15`):

```cpp
#define LED_USER  RE6          // your LED's pin

void setup() { pinMode(LED_USER, OUTPUT); }
void loop()  {
    digitalWrite(LED_USER, HIGH); delay(500);
    digitalWrite(LED_USER, LOW);  delay(500);
}
```

See `examples/Blink`.

## Program / Upload

Tools ▸ Programmer ▸ **On-board PICkit (PKOB)**, **PICkit 4**, or **PICkit 5**,
then press **Upload** (runs MPLAB IPE `ipecmd` headless — no GUI, no bootloader).

## Boards
- `dsPIC33CK256MP508 (generic)` — fully supported.
- `dsPIC33AK128MC106 (experimental)` — compiles/links; C++ runtime not yet
  hardware-verified.

## More
- Testing & publishing notes: [`docs/TESTING_AND_RELEASE.md`](docs/TESTING_AND_RELEASE.md).
- The bundled C++ compiler is a GPL rebuild of Microchip's XC-DSC GCC (C++
  enabled). Corresponding modified source + build scripts are available on
  request and in the development repository.
