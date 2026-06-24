# Production / bench tools (no Arduino IDE recompile)

These standalone Windows scripts program and erase dsPIC chips using **MPLAB IPE
(`ipecmd`)** directly. They need MPLAB X installed (for `ipecmd.exe`) but **do
not** use the Arduino IDE and **do not recompile** — ideal for flashing the same
firmware onto many boards.

## 1. Build the `.hex` once

Pick whichever you prefer:

- **Arduino IDE:** open the sketch → **Sketch ▸ Export Compiled Binary**.
  The `.hex` lands in the sketch folder under `build/dspicArduino.dspic.<board>/<sketch>.ino.hex`.
- **arduino-cli:**
  ```
  arduino-cli compile --export-binaries -b dspicArduino:dspic:dspic33ck256mp508 <sketch>
  ```
  Same `build/.../<sketch>.ino.hex` output.

## Quickest: the interactive tool

Double-click **`dspic-tool.bat`** for a menu:

```
   [1] Burn LAST compiled hex (no recompile)
   [2] Mass-program last hex (loop, many boards)
   [3] Erase device
   [4] Burn a chosen .hex file
   [5] Change device / tool code
```

"Last compiled hex" is found automatically in the Arduino build cache, so the
workflow is: **Verify/Compile the sketch once in the IDE**, then pick `[1]` (one
board) or `[2]` (loop for production) here — no recompiling per board. `[3]` is
the clearly-labelled erase (the IDE can't rename its "Burn Bootloader" menu, so
use this for an obvious "Erase Device" action).

The scriptable equivalents below are handy for automation / drag-and-drop.

## 2. Mass-program from that `.hex`

```
dspic-prog.bat "build\dspicArduino.dspic.dspic33ck256mp508\Blink.ino.hex"
```
or just **drag the `.hex` onto `dspic-prog.bat`** in Explorer.

It programs the chip, then prompts *"Swap to the next board, press ENTER…"* so
you can flash board after board. Type `Q` + ENTER to stop. A running ok/failed
tally is shown.

Defaults are device `33CK256MP508` and tool `PKOB4` (the on-board PICkit). For a
different device or an external programmer, pass them as args:
```
dspic-prog.bat "MyApp.ino.hex" 33CK256MP508 PK4
```

## 3. Erase a chip

```
dspic-erase.bat                 (defaults: 33CK256MP508, PKOB4)
dspic-erase.bat 33AK128MC106 PK4
```

> Note: programming with `dspic-prog.bat` already auto-erases the program region,
> so you only need `dspic-erase.bat` to blank a chip without reprogramming it.

## Tool codes (`-TP`)

| Programmer            | code     |
|-----------------------|----------|
| On-board PKOB / Curiosity | `PKOB4` |
| PICkit 4              | `PK4`    |
| PICkit 5              | `PK5`    |
| ICD 4                 | `ICD4`   |
| MPLAB SNAP            | `SNAP`   |
