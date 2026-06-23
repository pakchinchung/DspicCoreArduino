# Testing & Releasing dspicArduino

## What an end user must install (real-world)

The published package contains ONLY our GPL parts (the C++ compiler + the
Arduino core + the build wrappers). Two Microchip pieces are NOT bundled
(proprietary — must be user-installed):

1. **MPLAB XC-DSC compiler** (free): provides the assembler, libraries, and
   target C headers (`stdint.h`, …). Install from microchip.com.
2. **MPLAB X IDE** (free): provides the **Device Family Packs (DFP)** — the
   device headers `<xc.h>`, the `.gld` linker scripts, `c30_device.info` — and
   `ipecmd` used by the Upload button. The standalone XC-DSC does **not** include
   device files, so the DFP (hence MPLAB X, or a manually downloaded `.atpack`
   into `~/.mchp_packs`) is required.

So: **Arduino package (Boards Manager) + XC-DSC + MPLAB X.**

## How to TEST (Boards Manager flow)

1. In Arduino IDE: **File ▸ Preferences ▸ Additional Boards Manager URLs**, add:
   ```
   https://raw.githubusercontent.com/pakchinchung/DspicCoreArduino/master/package_dspicArduino_index.json
   ```
   (This works only after you publish the release below. For a purely local
   test, see "Local test" further down.)
2. **Tools ▸ Board ▸ Boards Manager**, search "dspic", install **dspicArduino Core**.
3. **Tools ▸ Board** ▸ select **dsPIC33CK256MP508 (generic)**.
4. Open `examples/Blink/Blink.ino` (or File ▸ Examples once installed). Set the
   LED pin by its native name, e.g. `#define LED_USER RE6`.
5. **Verify (compile)** — it should build to a `.hex` (Sketch ▸ Verify).
6. **Upload** — Tools ▸ Programmer ▸ pick *On-board PICkit (PKOB)* / *PICkit 4* /
   *PICkit 5*, connect the board, press Upload. This runs MPLAB IPE's `ipecmd`
   headless (no GUI, no bootloader).

### Pin model
Pins are referenced by datasheet name: `RA0`…`RE15` (and `RG*` where present).
`#define LED_USER RE6` then `pinMode(LED_USER, OUTPUT); digitalWrite(LED_USER, HIGH);`.
No board-specific pin map — works for any wiring on the chip.

## How to PUBLISH (you do this once)

The platform archive is already built and verified at:
```
C:\dspic-pkg-build\dspicArduino-dspic-0.1.0.zip
   SHA-256: ae513be821bac40e8b0ff87d4d5a4200737c9c2a371083cd43de8076bd757ac9
   size:    25920586 bytes
```
`package_dspicArduino_index.json` (committed at the repo root) already points at
the GitHub Release URL with this checksum/size. To publish:

1. Push the repo (the index + sources).
2. Create a GitHub Release tagged **v0.1.0** in `pakchinchung/DspicCoreArduino`.
3. Upload `dspicArduino-dspic-0.1.0.zip` as a **release asset** (drag-drop in the
   GitHub Releases web UI, or `gh release create v0.1.0 C:\dspic-pkg-build\dspicArduino-dspic-0.1.0.zip`).
4. Done — the raw `package_dspicArduino_index.json` URL (step 1 of TEST) now works.

If you ever rebuild the zip, update the `checksum` and `size` in
`package_dspicArduino_index.json` to match (sha256sum + byte size).

## Local test WITHOUT publishing (what was used to verify)

Serve the staging dir over HTTP and point a throwaway index at it:
```bash
cd /c/dspic-pkg-build && python -m http.server 8731 --bind 127.0.0.1 &
# test-index.json there already points at http://127.0.0.1:8731/...zip
ARDUINO_CLI="…/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
"$ARDUINO_CLI" --config-dir /c/dspic-pkg-build/acli core install dspicArduino:dspic \
    --additional-urls http://127.0.0.1:8731/test-index.json
"$ARDUINO_CLI" --config-dir /c/dspic-pkg-build/acli compile \
    -b dspicArduino:dspic:dspic33ck256mp508 examples/Blink \
    --additional-urls http://127.0.0.1:8731/test-index.json
```
(arduino-cli rejects `file://` for the archive download, so a local HTTP server
is needed — GitHub Releases is HTTP, so this matches production.)

## Known cosmetic issues (do not affect the .hex)

- The link step prints several `Could not open resource file … c30_device.info`
  lines — benign toolchain resource-lookup noise; the device info IS found via
  `-mdfp` and a correct `.hex` is produced.
- "Sketch uses 0 bytes" in the size report — `-ffunction-sections` splits the
  code into many `.text.*` sections that the simple size regex doesn't total.
  The binary is fine; only the size display is wrong.
- `warning: No interrupt vector names defined` / `-fno-short-double ignored` —
  expected from the GPL `--without-headers` rebuild; codegen is correct.
