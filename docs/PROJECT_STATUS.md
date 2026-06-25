# dspicArduino — Project Status & Session Handoff

**Read this first if you're a new session picking up this project.** It captures
where things stand, the hard-won technical knowledge, and the prioritized plan.
Last updated: 2026-06-24 (**v0.1.5 RELEASED** — see §2 / §5). This session also:
AK **I2C/Wire** ported & HW-verified (Curiosity LCD ACKs at **0x27** via AkI2cScanner
on I2C2 alt pins RD7/RD8, `ALTI2C2=ON`); cut & published the **0.1.5** release end-to-end
(zip build → local pre-flight → `gh release` on origin → post-release arduino-cli
install/compile from the raw-GitHub index). Earlier: Done this session: **#34** (map/random/yield,
HW), **#35** (`attachInterrupt` via Change Notification, HW), **#36** SPI + UART1/2
self-loopback (HW; PWM still needs a scope). **#40** AK bring-up: 🎉 **FIRST
LIGHT on real AK silicon** — AkBlink runs, Timer1/millis interrupts HW-confirmed,
real board pin map applied, generic+Curiosity variant split w/ Tools menu;
remaining = AK clock/PLL (8 MHz EC) + porting the HAL (Serial…) to AK. **#38**
EEPROM (flash-emulated) ✅ DONE + HW-confirmed on CK (boot counter persists across
resets). Requested: **#41** clock/PLL selection menu (CK **and AK** — both HW-confirmed: CK 50/100 MIPS,
AK 100 MIPS FRC+PLL). AK now has a precise 100 MHz clock → Serial port to AK is
next (#40 cont.). See §6 for detail.

---

## 1. What this project is

An Arduino **board package** that lets standard, unmodified Arduino sketches
(including **C++**) compile and run on Microchip **dsPIC33** MCUs. The headline
trick: Microchip's stock XC-DSC compiler ships with the C++ front-end disabled, so
we rebuild GCC with C++ enabled (GPL-legal) and implement the Arduino HAL on top.

- **Primary target:** dsPIC33CK256MP508 (e.g. DM330030 Curiosity board).
- **Experimental:** dsPIC33AK128MC106.
- **Pin model:** by native datasheet name — `#define LED RE6` then `pinMode(LED, OUTPUT)`.
  Encoding `(port<<4)|bit` in `cores/dspic/dspic_pins.h`; per-port SFR table in the
  variant's `variant.cpp`.

Distribution model: the public package ships **only GPL parts** (our C++ compiler +
Arduino core + wrappers). The user separately installs **XC-DSC** (assembler/libs/
C headers) and **MPLAB X** (device packs = `<xc.h>` + `.gld` + `ipecmd`). NEVER
redistribute Microchip proprietary files (DFP, ipecmd, datasheet PDFs).

---

## 2. Current state — v0.1.4 (shipped & live)

Full multi-instance HAL, all compile-verified via arduino-cli; **I²C confirmed on
real hardware** (a 16×2 LCD on the stock LiquidCrystal_I2C library):

| Area | Status |
|------|--------|
| digital I/O, `millis/micros/delay` | done |
| `Serial` / `Serial1` / `Serial2` (UART1/2/3) | done (Serial confirmed HW) |
| `Wire` / `Wire1` / `Wire2` (I²C1/2/3, master) | **done, HW-confirmed** |
| `SPI` / `SPI_2` / `SPI_3` (SPI1/2/3, master) | done (compile only) |
| `analogRead` (ADC), `analogWrite` (PWM), `dacWrite` | done (ADC+DAC HW-confirmed earlier) |
| Stock-library compatibility shims | done |
| Upload via MPLAB IPE; flash erase; mass-program tools | done |
| Sketch size report | done (fixed) |

**Latest release: `dspicArduino-dspic-0.1.6.zip`** (size 26094129, sha256
`88f8f996ae97f0da3b58c01c7ec4982d61554907d441d1103ff1576b91b5db9e`) — published
2026-06-25 on the **origin** repo (GitHub Release `v0.1.6` + index at
`origin/master`), verified end-to-end via arduino-cli from the raw-GitHub index
(CK Blink + AK AkEepromTest @200 MIPS compiled). **Completes the AK HAL**: PWM,
dacWrite, attachInterrupt, EEPROM, plus the 200 MIPS clock option; README matrix
now committed. Prior: 0.1.5 (sha256 `f58bcb55…`), 0.1.4 (sha256 `4f0f9007…`).
NOTE: after publishing, delete the local cached index
(`~/AppData/Local/Arduino15/package_dspicArduino_index.json`) and/or wait ≤5 min —
the raw.githubusercontent CDN caches the old index briefly (a `?t=<n>` cache-bust
query forces a fresh edge copy).

---

## 3. Environment (Windows, git-bash)

- **Repo root:** `C:\Users\A76570\OneDrive - Microchip Technology Inc\Desktop\Work\ArduinoDspic`
- **arduino-cli (bundled in IDE):**
  `C:\Users\A76570\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe`
- **XC-DSC:** `C:\Program Files\Microchip\xc-dsc\v3.31.01` (GCC 8.3.1 based)
- **DFP:** `C:\Program Files\Microchip\MPLABX\v6.30\packs\Microchip\dsPIC33CK-MP_DFP\1.15.423`
  - device header: `…\xc16\support\dsPIC33C\h\p33CK256MP508.h`
  - pin/SFR map (XML): `…\1.15.423\edc\DSPIC33CK256MP508.PIC`
- **ipecmd:** `C:\Program Files\Microchip\MPLABX\v6.30\mplab_platform\mplab_ipe\ipecmd.exe`
- **Active dev test install** (sync changed core files here, then compile):
  `C:\Users\A76570\AppData\Local\Arduino15\packages\dspicArduino\hardware\dspic\0.1.3\`
  (the dir is named 0.1.3 but holds the latest synced files — it's the working
  install I edit + compile against; a clean 0.1.4 was verified separately under
  `C:\dspic-pkg-build\acli014`).
- **FQBN:** `dspicArduino:dspic:dspic33ck256mp508` (menus: `:reset=hold`, `:i2c1pins=primary`).
- **Compile:** `arduino-cli compile -b dspicArduino:dspic:dspic33ck256mp508 examples/Blink`
  (add `--clean` after core changes; arduino-cli caches the core lib build).

### Dev loop  (compile + self-test, no hardware)
1. Edit `cores/dspic/*` (or variant/boards/platform).
2. **`cp` the changed files to the active install dir** above (compile reads the
   *installed* copy, not the repo). Sync ALL files you touched.
3. `arduino-cli compile --clean -b dspicArduino:dspic:dspic33ck256mp508 examples/<x>`.
   - Filter the noise: pipe to
     `grep -iE "error|Sketch uses|Global var|undefined|multiple def|maximum"`.
     A clean build prints `Sketch uses N bytes … Global variables use M bytes`.
   - The repeated `Could not open resource file: …c30_device.info` lines are
     **harmless** (size-report objdump pass) — not errors.
4. Hardware run — see the flash + serial self-test below.

### Flash + serial self-test  (autonomous, when the board is on PKOB4)
The assistant **can** flash and read back over serial without the user, *when a
board is connected via PKOB4* (confirmed working 2026-06-24: DM330030 on **COM6**;
`map/random/yield` HW-verified this way). PKOB4 exposes a virtual UART COM port
(here **COM6**) wired to the dsPIC's `Serial` (UART1), separate from its programming
interface — so you can flash *and* monitor on the same cable.

1. **Flash** (compile + upload in one; PKOB4, default `reset=run` so it runs after):
   ```
   arduino-cli compile --clean -u -p COM6 -b dspicArduino:dspic:dspic33ck256mp508 examples/<x>
   ```
   Success prints `Connecting to MPLAB PKoB4` / `Device Id = 0x7c740000` /
   `Programming/Verify complete`. (`-p COM6` satisfies the CLI; the `ipe` recipe
   uses tool code `PKOB4`, not the port. Plain Upload in the IDE works the same.)
2. **Read serial** (pyserial 3.5 is installed under the Windows Python):
   ```
   "/c/Users/A76570/AppData/Local/Programs/Python/Python312/python" \
       tools/test/read_serial.py COM6 9600 <seconds>
   ```
   `tools/test/read_serial.py PORT BAUD SECONDS` opens the port, prints every line
   for N seconds, closes. The sketch loops forever (prints every 500 ms), so it's
   fine to attach *after* the post-flash reset — you just miss the first line or two.
   - Worked example — `examples/MathRandom` (#34 HW proof): output
     `duty=21 r1=<0..99> r2=<10..19>` streaming every 500 ms confirmed `map()`
     (stable scaled ADC), `random(max)`/`random(min,max)` (in range), and that
     `yield()` links + the loop doesn't hang.
   - Other tools: user can also flash from the IDE (Upload), or batch-flash the
     newest cached `.ino.hex` with `tools/production/dspic-tool.bat`.

> **Two boards connected at once (CK + AK):** both PKOB4 programmers enumerate, so
> `ipecmd` errors "More than one PKOB4 connected" unless you select one by S/N. Use
> **Tools ▸ "Programmer (PKOB S/N)"** (menu `prog` → `upload.tool_sn` → `ipecmd -TS`),
> or set the `IPE_TOOL_SN` env var (wrapper honors both; menu/arg wins). This setup's
> S/Ns (list yours via `ipecmd -P<dev> -TPPKOB4 -OK -v4`): **CK Curiosity =
> `BUR233012076`, AK GP DIM = `020085204RYN000728`**. CLI: append to the FQBN
> comma-separated, e.g. `...:dspic33ak128mc106:clock=f100,prog=ak`. COM ports with
> both plugged: **CK serial = COM6**, **AK serial = COM24** (MCP2221A UART consoles;
> COM23/COM25 are the PKOB back-channels, COM9 is an unrelated CP210x).

> Still NO autonomous *programmer-driven debug* (no mdb/ICD stepping here) and no
> scope/logic-analyzer — peripheral *accuracy* (PWM duty, SPI/UART loopback timing)
> still needs the user. But anything observable over `Serial` is now
> assistant-verifiable end-to-end via the two steps above.

> **⚠ STALE `core.a` CACHE — bites every core edit.** arduino-cli caches the
> compiled core archive at
> `C:\Users\A76570\AppData\Local\arduino\cores\<board-hash>\core.a` and **reuses
> it regardless of source mtime**. `--clean` only wipes the *sketch* build dir, NOT
> this global core cache — so edits to `cores/dspic/*.c{,pp}` (hooks.c, main.cpp,
> WMath.cpp, …) can silently link against a stale core (e.g. cost us a phantom
> `undefined reference to '_yield'`). To force a core rebuild, invalidate that
> `core.a`. **`rm -rf` is blocked by the sandbox policy**, so move it aside instead:
> `mv "<…>/core.a" "<…>/core.a.stale-bak"`. (Editing a *header* like `Arduino.h`
> does trigger a rebuild via the dep tracking; editing a `.c/.cpp` source does not.)
> A leftover `core.a.stale-bak` currently sits in the `…_reset_run,i2c1pins_alt_*`
> cache dir — harmless; arduino-cli ignores anything not named `core.a`.

---

## 4. Technical knowledge (the gotchas — don't relearn these)

### Compiler / build
- **`-Wl,--gc-sections` MUST NOT be used** — it garbage-collects C++ global
  constructors (`.init/.ctors`), so global objects never construct.
- `xc.h` has unguarded generic typedefs `SPI`/`UART`/`PSPI`/`PUART` that collide
  with Arduino object names → `cores/dspic/xc_compat.h` renames them around the
  `<xc.h>` include. **Use `#include "xc_compat.h"` instead of `<xc.h>`** in any TU
  that names an Arduino object/type.
- C++ needs `dspic_cpp_compat.h` force-included (`#pragma config` / `__prog__`
  address-space keyword). Already wired in platform.txt.
- All `.bat` wrappers use **positional args** — `cmd.exe` splits batch args on `=`.
- **arduino-cli does NOT auto-define `ARDUINO`** — platform.txt now passes
  `-DARDUINO={runtime.ide.version} -DARDUINO_ARCH_DSPIC -DARDUINO_{build.board}
  -DF_CPU={build.f_cpu}` (without `-DARDUINO`, libs guarding `#if ARDUINO>=100`
  fall back to `WProgram.h` and fail).
- **Weak default functions (`yield`, future hooks) must live in an
  ALWAYS-LINKED TU — `main.cpp`, NOT `hooks.c`.** The sketch supplies *strong*
  `setup()`/`loop()`, so `hooks.o` is never extracted from `core.a`; and this
  binutils (GCC 8.3.1 base) will **not** pull an archive member solely to satisfy
  a reference whose only definition is **weak**. Result: a weak `yield()` in
  `hooks.c` → `undefined reference to '_yield'`. Putting it in `main.cpp` (always
  linked, defines `main`) guarantees the weak default is present yet still
  overridable by a library's strong `yield()`. (`setup`/`loop` weak stubs stay in
  `hooks.c` because main.cpp *references* them, which pulls hooks.o for those.)
- **`#include <stdlib.h>` (and other libc headers that declare `abs`/`round`/`div`)
  must come BEFORE `"Arduino.h"`** in any TU — Arduino.h `#define`s `abs`/`round`
  as macros that otherwise mangle the libc prototypes (`error: expected
  unqualified-id before 'int'`).
- **Overloaded functions can't be `extern "C"`** — e.g. `random(long)` /
  `random(long,long)` are declared in Arduino.h's C++ section, not the `extern "C"`
  block. Single-signature C-callable funcs (`map`, `yield`) stay in `extern "C"`.

### dsPIC33CK silicon facts
- **FCY = F_CPU/2 = 50 MHz** (F_CPU=100 MHz=FOSC). UART BRG and I²C BRG use FCY.
  (The `initClock` comment saying "FCY=100MHz" is wrong; the math uses F_CPU/2.)
- **ADC:** set `ADCON1Lbits.ADON=1` BEFORE powering cores (`CxPWR`), else `SHRRDY`
  never sets. The **shared core** (all AN2+) needs a real sample time —
  `ADCON2Hbits.SHRSAMC` (we set 31); default ~2 TAD is too short for a pot and the
  reading slews slowly (looks "laggy"). AN0/AN1 are dedicated cores.
- **"New" UART** status bits are in `UxSTAH` (`UTXBF`/`URXBE`), not `UxSTA`.
- **No classic Timer2** — uses SCCP modules CCP1–CCP9 as timers (CCP2 timer ISR =
  `_CCT2Interrupt`, IFS1.CCT2IF, IEC1.CCT2IE).
- **PPS:** SPI (SDO/SDI/SCK), UART (TX/RX) are remappable (RPORx/RPINRx). Helper
  `rpor_set(rp, fn)` in SPI.cpp/HardwareSerial.cpp. Output func codes: SDO1/2/3 =
  5/8/11, SCK1/2/3OUT = 6/9/12, U1/2/3TX = 1/3/27. Input selects: `_SDIxR`, `_UxRXR`.
- **PPS remappable-pin set = ports B/C/D ONLY (48 pins, RP32–RP79). Ports A & E are
  NOT remappable** (verified from the `.PIC`: exactly 48 `RPn`↔pin pairs). Formula:
  **`RBn=RP(32+n)`, `RCn=RP(48+n)`, `RDn=RP(64+n)`** (so RD3=RP67, RD4=RP68, matching
  the working UART). ⇒ a pin's RPn = `32 + port_index_BCD*16 + bit`, where port B=0.
  **Consequence:** anything needing a PPS *input* (SPI SDI, UART RX, external-INT
  INT1/2/3) cannot use an RA/RE pin — including the DM330030 buttons on RE7/8/9.
- **External interrupts:** `INT0` (pin **fixed**, not remappable) + `INT1/INT2/INT3`
  (input-remappable via `_INT1R`=RPINR0, `_INT2R`/`_INT3R`=RPINR1 — assign an RPn).
  Edge polarity `INTCON2bits.INTxEP` (0=rising, 1=falling; no built-in CHANGE).
  Per-line `_INTxIF/_INTxIE/_INTxIP`, vectors `_INT0Interrupt`…`_INT3Interrupt`.
  Because INTx needs PPS input, it can't reach RA/RE → not usable for RE buttons.
- **Change Notification (CN) — the better `attachInterrupt` primitive here.** Works
  on ANY pin (incl. RA/RE) and supports edge selection. Per-port regs `CNCONx`,
  `CNEN0x`, `CNEN1x`, `CNFx`, `CNSTATx`, `CNPUx`, `CNPDx` (x = A..E). `CNCONx.ON`
  (bit 15) enables the module; **`CNCONx.CNSTYLE=1` = edge-detect style** → then
  `CNEN0x` bit = **rising**-edge enable, `CNEN1x` bit = **falling**-edge enable
  (set BOTH = CHANGE), and `CNFx` is the per-pin "this pin fired" flag (clear it in
  the ISR). One interrupt **per port**: `_CNAIE`…`_CNEIE` / `_CNAIF`…`_CNEIF` /
  vectors `_CNAInterrupt`…`_CNEInterrupt`. (CNSTYLE=0 is the legacy mismatch/level
  style — don't use it; edge style is what maps cleanly to RISING/FALLING/CHANGE.)
- **I²C is NOT PPS-remappable** and has no runtime pin-select register. Each module
  has a fixed primary pair and an alternate pair, chosen ONLY by the **FDEVOPT
  `ALTI2Cx` config bit** (compile-time `#pragma config`). On this board the LCD is
  on I²C1 **alternate** pins **RC8 (SDA)/RC9 (SCL)** → `variant.cpp` defaults
  `ALTI2C1=ON`; `-DDSPIC_I2C1_PRIMARY` (Tools ▸ "I2C1 pins" menu) selects RB9/RB8.
  Pin→function map came from the `.PIC` file: RC8=ASDA1, RC9=ASCL1, RB9=SDA1,
  RB8=SCL1, RB5/RB6=SDA2/SCL2, RB7/RB2=SDA3/SCL3.

### dsPIC33AK (AK) silicon facts — for the #40 bring-up
- **DFP:** `dsPIC33AK128MC106` lives in **`dsPIC33AK-MC_DFP`** (MC, not MP),
  support subdir **`dsPIC33A`**, header `p33AK128MC106.h`, gld `p33AK128MC106.gld`.
  The build wrapper (`xc16pp-env.bat`) already maps `AK`→`dsPIC33A` and a name with
  `MC`→`-MC_DFP`, so `arduino-cli compile -b …:dspic33ak128mc106` finds it. Only the
  **CK** datasheet is on disk (`dsPIC33CK256MP508-…DS70005349H.pdf`) — there is NO
  AK datasheet locally, so AK clock/electrical specs must come from the board/web.
- **32-bit architecture:** GPIO/timer SFRs are `uint32_t` (`dspic_sfr_t` =
  `uint32_t` under `__dsPIC33A__`). A 32-bit load is **atomic**, so `millis()` needs
  no DISI guard (unlike the 16-bit CK path). Ports implemented: **A..D**.
- **Hardware FPU (AK only):** dsPIC33A has a hardware floating-point unit (CK does
  NOT). `float`/`double` math is fast on AK — relevant for `map()`-heavy / DSP /
  trig sketches. **TODO:** confirm our GCC 8.3.1 pic30 build actually emits FPU
  instructions for the 33A ISA (check the `-mcpu=33AK*` default / any `-mfpu`-style
  flag); if a flag is needed, add it to platform.txt for AK builds so float isn't
  software-emulated. (No action needed for the Arduino HAL itself.)
- **Timer1 (verified from header):** `T1CON`/`TMR1`/`PR1` are 32-bit. **Enable bit =
  `T1CONbits.ON` (bit 15), NOT `TON`.** `TCKPS` = 2 bits @ pos 4 (1 ⇒ 1:8). T1
  interrupt bits are in **`IFS1`/`IEC1`/`IPC6`** (`_T1IF`=IFS1, `_T1IE`=IEC1,
  `_T1IP`=IPC6) — NOT IFS0/IEC0/IPC0 like CK. ISR vector name is still
  **`_T1Interrupt`** (DFP template confirms). The `cc1plus: warning: No interrupt
  vector names defined for 33AK128MC106` is **benign** (CK emits the identical
  warning and its ISRs work). → implemented in `wiring.cpp` `#if __dsPIC33A__`.
  **Timer1 is clocked by the STANDARD-speed peripheral clock = FPB/2 = FCY/2
  (= F_CPU/4), NOT FCY** — so `PR1 = F_CPU/4/8/1000 − 1` for a 1 ms tick (one extra
  ÷2 vs CK). Caught by a `millis()`-vs-host-timestamp self-test that ran exactly 2×
  slow with the FCY assumption; the corrected formula gives device-ms == wall-clock.
- **Relocatable IVT:** AK uses a register-based IVT (`IVTBASE` @0x88, table @
  0x800004) instead of CK's fixed vector slots — but the named-vector
  `__attribute__((interrupt)) _XxxInterrupt` convention still works.
- **Clock/PLL — RESOLVED from the AK datasheet (DS70005539 §12, now on disk):**
  - **FCY = FPLLO = FOSC (1:1 — NO /2, unlike CK!).** 100 MHz FPLLO ⇒ 100 MIPS.
    (Our `F_CPU` convention is `F_CPU = 2·FCY`, so 100 MIPS ⇒ `F_CPU=200000000`.)
  - Formula (same divider names as CK): `FVCO = FPLLI·M/N1`, `FPLLO = FVCO/(N2·N3)`,
    M=`PLL1DIV.PLLFBDIV`, N1=`PLLPRE`, N2=`POSTDIV1`, N3=`POSTDIV2` (all direct
    values). VCO range up to **1600 MHz**; FPLLI 8 MHz typical.
  - **Clock generators:** `CLKGEN1`(CLK1CON/CLK1DIV) = system clock (FOSC). NOSC
    codes: **0001=FRC, 0010=BFRC, 0011=POSC, 0101=PLL1 FOUT, 0111=PLL1 VCODIV**.
    `PLL1CON.NOSC` selects the PLL *input* (FRC=1, POSC=3). `OSCCFG.POSCMD`: 0=EC,
    1=XT(3.5-10M), 2=HS(10-32M), 3=off (DIM's 8 MHz EC clock ⇒ POSCMD=0).
  - **Switch sequence (no key; poll the request bit) — datasheet Example 12-4
    (FRC+PLL→100 MHz), VERIFIED on HW (see §6 #41):** set `PLL1CON.ON=1`,
    `PLL1CON.NOSC=1`(FRC), `PLL1DIV`={PLLPRE 1, PLLFBDIV 125, POSTDIV1 5, POSTDIV2 2},
    then `PLLSWEN=1`(wait clear), `FOUTSWEN=1`(wait clear), `OSWEN=1`(wait clear),
    wait `OSCCTRL.PLL1RDY`. Then point the CPU at it: `CLK1CON.NOSC=5`(PLL1 FOUT),
    `CLK1CON.OSWEN=1`(wait clear), wait `CLK1CON.CLKRDY`. (FVCO=8·125=1000 MHz;
    FPLLO=1000/(5·2)=100 MHz.) `OSCCTRL.CLKLOCK` (bit23) resets to 0 = unlocked.
- **Clock tree — multi-generator overview:** AK has a complex multi-generator
  tree: `OSCCTRL` (enable/ready bits for FRC/BFRC/POSC/LPRC/**PLL1**/**PLL2**),
  `CLK1CON..CLK13CON` + `CLK1DIV..`, `PLL1CON`/`PLL2CON`, `FRCTUN`. `initClock()`
  is NOT implemented — the part runs on its **default FRC**, so `millis/delay` rate
  is approximate (Timer1 still fires → no hang). **Unknowns needing the AK
  datasheet/board:** default FRC frequency and whether **FCY = FOSC or FOSC/2** (the
  CK assumption FCY=F_CPU/2 is hard-coded in PR1 for now). `F_CPU` in the AK
  `pins_arduino.h` is a placeholder to correct once the clock is validated.
- **`__prog__` shim is a no-op on AK** (AK uses no named-address-space keywords —
  see [[dspic-build-facts]]); the C++ compiler handles AK (sim-proven, DSPIC33A_STATUS.md).
- **UART ✅ WORKS on AK (HardwareSerial `#elif __dsPIC33A__` branch; HW-confirmed
  2026-06-24 on COM24 @ 9600).** It's a DIFFERENT peripheral than CK (DS70005539
  §18): `U1CON`/`U1STAT`/`U1BRG`/`U1TXB`/`U1RXB` (32-bit), NOT CK's `U1MODE/U1STAH`.
  3 UARTs. Config: `U1CON.MODE=0` (8N1), `CLKSEL=0` (FUART = standard peripheral
  clock = FPB/2 = FCY/2 = **F_CPU/4**), `BRGS=0`/`CLKMOD=0` ⇒ `BRG = FUART/(16·baud)
  −1 = F_CPU/(64·baud)−1`. Status: `U1STAT.TXBF`(b20)/`TXBE`(b21)/`RXBE`(b17, RX
  empty). Data: write `U1TXB`, read `U1RXB`. Regs are 32-bit (CLKSEL in the high
  word) ⇒ the AK path uses direct `U1CONbits`/cast access, not CK's uint16_t ptrs.
  **THREE gotchas that each caused total silence until fixed (debug history):**
  1. **PMD** — must clear the module's Peripheral-Module-Disable bit
     (`_U1MD=0`, PMD1) or the UART is unclocked and `U1CON` writes do nothing.
  2. **ANSEL/TRIS** — PPS does NOT make an analog pin digital. The UART `begin()`
     bypasses `pinMode()`, so it must clear `ANSELD` for the TX/RX pins and set
     `TRISD` (TX out, RX in) itself.
  3. **Board net names are from the MCU's PERSPECTIVE** — so `UART_USB_TX`=RD1 is
     the *MCU TX* and `UART_USB_RX`=RD3 is the *MCU RX*. ⇒ **MCU TX=RD1(RP50),
     RX=RD3(RP52)** for the MCP2221A COM (COM24). (Had them swapped first → silence.)
  Internal loopback (RX mapped to the TX pin) PASSES for ANY baud (TX/RX share BRG),
  so it proves the driver works but NOT the absolute baud — use the host COM +
  `read_serial.py` (now prefixes host-elapsed seconds) to confirm baud AND timing.
- **AK PPS (no unlock needed):** `IOLOCK` (RPCON bit11) **resets unlocked**, and the
  PAC defaults to writes-allowed (all WR=1/LK=0 at reset) — so PPS + clock regs are
  writable out of reset with NO key sequence (that's why initClock worked bare).
  Input select `_U1RXR = <RPn>` (RPINR9); output `_RP<n>R = 9` (U1TX func code = 9).
  Board UART pins: **COM25/PKOB = MCU TX RP59(RD10) / RX RP58(RD9)**; COM24/MCP2221A
  = TX RP52(RD3)/RX RP50(RD1). RPOR reg for pin = `(rp-1)/4`, field `(rp-1)%4`·8, 7-bit.
- **ADC ✅ WORKS on AK** (`wiring_analog.cpp` `#elif __dsPIC33A__`; HW-confirmed —
  Curiosity pot on AD1AN6 swept 0..4095). Per-channel ADC (DS70005539 §15): module
  `AD1CON` (`ON`, `ADRDY`), per-channel `AD1CHxCON` (`MODE`=0 single, `TRG1SRC`=1
  software, `PINSEL`=AN#, `SAMC`=sample time), trigger `AD1SWTRG.CHxTRG=1`, wait
  `AD1STAT.CHxRDY`, read `AD1CHxDATA` (12-bit, right-aligned). **TWO must-dos:**
  clear `_ADC1MD` (PMD3); and the ADC needs its own clock generator **CLKGEN6**
  (`CLK6CON`) — we source it from FRC (NOSC=1) so it's independent of the system
  clock. `analogRead(ch)` = analog input number ANch (pot = `analogRead(6)`).
- **`analogWrite` / PWM ✅ WORKS on AK** (`wiring_pwm.cpp` `#elif __dsPIC33A__`;
  HW-CONFIRMED 2026-06-25 — `AkPwmTest`/`AkRgbFade` fade the RGB LED R→G→B, user
  verified). RGB common-CATHODE. **Generators PG1/PG2/PG3, output routed by PPS:**
  RC2(RP35)←PWM1H=Red, RD0(RP49)←PWM2H=Green, RD2(RP51)←PWM3H=Blue. The exact
  recipe (reverse-engineered from the HW-verified `MplabProject/Training.X`, after
  Demo_2's `0xC0200`/no-PPS/PWM4H values turned out to be a non-working variant):
  - **`PGxCON = 0x41000008`** — indep-edge, CLKSEL=master, **MPERSEL=1** (period
    comes from the shared **`MPER`**, not `PGxPER`). ON (bit15) set LAST.
  - **`PGxIOCON = 0x02080200`** — **PENH=1 + PPSEN=1** (PWM output goes through PPS;
    PPSEN is essential). PENL=0.
  - **`PGxEVT = 0x00000008`** — **UPDTRG=1**: a write to `PGxDC` latches the new
    duty. ⚠ THE KEY MISSING PIECE earlier — without it the duty is frozen at its
    first value (looked dark when init DC=0, or constant white when init DC=mid).
  - **`MDC=0; MPER=0xF9F0`** (master period). Duty = `PGxDC` (mask `& 0xFFFF0`).
  - **PPS routing needs IOLOCK unlock:** `RPCONbits.IOLOCK=0;` set `_RP35R=1`
    (PWM1H), `_RP49R=3` (PWM2H), `_RP51R=5` (PWM3H); `RPCONbits.IOLOCK=1`. Output
    func codes: PWM1H=1, PWM2H=3, **PWM3H=5**, PWM4H=7 (datasheet p479 RPnR table).
  - **PWM master clock = Clock Generator 5**, `PCLKCON=0x1` (MCLKSEL=1). Source CLK5
    from PLL1 Out: `CLK5CON=0x129500` (for THIS generator NOSC=2 = "PLL1 Out"; the
    NOSC encoding is per-generator!), `CLK5DIV=0`, then `CLK5CONbits.OSWEN=1`. Needs
    the PLL up → **Tools > Clock = "100 MIPS"**. PWM freq = FOUT/MPER ≈ 1.56 kHz.
  - Init ORDER (mirrors `PWM_Initialize`): clear `_PWMMD`; CLK5; TRIS+PPS; configure
    all PGx (CON/IOCON/EVT/DC, ON=0); `MPER`; `PCLKCON=0x1`; then `PGxCONbits.ON=1`
    for all. Examples: `AkPwmTest` (per-channel fade + Serial labels), `AkRgbFade`.
- **SPI ✅ WORKS on AK** (`SPI.cpp` `#elif __dsPIC33A__`; HW-confirmed autonomously —
  internal PPS loopback returned `SPI loopback: 8/8 PASS` over COM24). AK SPI uses a
  single 32-bit **`SPI1CON1`** (MSTEN/CKP/CKE/MODE16/ON), `SPI1BRG`, `SPI1BUF`,
  `SPI1STAT.SPIRBF` (=exchange done) — classic-style, NOT CK's CON1L/CON1H split.
  Clear `_SPI1MD` (PMD1). PPS OUTPUTS: **SDO1=13, SCK1OUT=14** (RPOR), SDI via
  `_SDI1R` (RPINR10). Uses the peripheral clock directly (no clock-generator needed,
  like the UART — that's why it worked first try, unlike PWM). Only SPI1 (`SPI`)
  wired; `SPI_2`/`SPI_3` (SPI2/3) are AK-TODO. Pins by native name (`SPI.setPins`).
- **I2C (`Wire`) ✅ WORKS on AK** (`Wire.cpp` `#elif __dsPIC33A__`; HW-confirmed — the
  Curiosity LCD ACKs at **0x27**, found by `AkI2cScanner` over COM24). AK I2C is the
  newer module: single 32-bit `I2CxCON1` (same master bits SEN/RSEN/PEN/RCEN/ACKEN/
  ACKDT, enable = **ON** not I2CEN), `I2CxSTAT1` (TRSTAT/ACKSTAT), `I2CxTRN`/`I2CxRCV`,
  and split baud **`I2CxHBRG`+`I2CxLBRG`** (≈ I2Cclk/(2·FSCL) each; ~38 = 100 kHz @
  8 MHz). Clear `_I2CxMD` (PMD). **AK has only I2C1 + I2C2** (no I2C3; Wire2 aliases
  I2C2). I2C is NOT PPS — pin pair is fixed; **alternate I2C2 (ASDA2=RD8/ASCL2=RD7)
  selected by `#pragma config ALTI2C2 = ON`** in variant.cpp (the Curiosity LCD is on
  I2C2 = `Wire1`). No clock-generator needed (peripheral clock, like UART/SPI).
- **`dacWrite` ✅ WORKS on AK** (`wiring_dac.cpp` `#elif __dsPIC33A__`; HW-verified
  6/6 via `AkDacAdcLoopback`). AK has 3 comparator-DACs; we drive **DAC1**. Enable:
  clear `_DACMD` (PMD), then `DACCTRL1bits.ON=1` (FCLKDIV=0, clock=FP), then
  `DAC1CONbits.DACOEN=1` (route to output pad) + `DAC1CONbits.DACEN=1`. Write the
  12-bit value to **`DAC1DATbits.DACDAT`** (the UPPER 16-bit field of the 32-bit
  `DAC1DAT`; the low field `DACLOW` is the slope/low threshold). **DACOUT1 pin =
  RA1** (`PGC2/DACOUT1/AD1AN7/AD2AN3/CMP1D/...`) — DACOUT1 and **AD1AN7 are the same
  pad**, so `dacWrite(0,v)` + `analogRead(7)` is a no-wiring internal loopback. Set
  the pad analog (`ANSELA1=1`, `TRISA1=1`). Ref = AVDD.
- **`attachInterrupt` ✅ WORKS on AK** (`WInterrupts.cpp` `#elif __dsPIC33A__`;
  HW-verified — `IntSelfTest` counts exactly 20 ISR fires for 20 edges, 0 after
  detach). CN ports **A..D only** (32-bit SFRs); same edge-detect design as CK:
  `CNCONx.ON`(bit15)|`CNSTYLE`(bit11), `CNEN0x`=rising / `CNEN1x`=falling, flag in
  `CNFx`; per-port `_CNxIF/IE/IP`; vectors **`_CNAInterrupt`.._CNDInterrupt**. No PMD.
- **⚠ CRITICAL CN GOTCHA (cost an hour): AK Change Notification does NOT flag
  self-driven OUTPUT-pad changes.** Verified directly: with `CNCONC=0x8800` and the
  pin toggling 12/12 in PORT, neither `CNFx` (edge) nor `CNSTATx` (mismatch) ever
  set. So the "make the pin an output and toggle LAT, watch CN on the same pin"
  self-test trick **does not work on AK — NOR on CK** (confirmed on both: CK RE6
  output toggled 12/12 but `CNFE` stayed 0). This is general dsPIC CN behaviour.
  It only flags REAL input transitions. **Autonomous CN self-test that DOES
  work:** pick a pin that *floats* on the board (no load) and swing the MCU's
  internal pull-up/pull-down (`CNPUx`/`CNPDx`) to create a genuine input edge — see
  `IntSelfTest` (uses **RA2**). Floating/pull-controllable AK Curiosity pins found
  by scan: RA2-5, RA8-11, RB0-3, RB6-11, RC0, RC11, RD0, RD2, RD9, RD11, RD12.
- **`EEPROM` ✅ WORKS on AK** (`libraries/EEPROM/EEPROM.cpp` `#elif __dsPIC33A__`;
  HW-verified — `AkEepromTest` boot counter climbs across software resets, persists
  in flash). AK flash NVM is very different from CK: **no NVMKEY** (the PAC leaves
  register writes enabled at reset, so just set NVMOP/WREN and pulse WR — same
  reason our clock writes need no unlock). Program granularity = **128-bit quad-word**
  (`NVMDATA0..3` + `NVMADR`, `NVMOP=0x1`); erase = **4 KB page** (`NVMOP=0x3`). A
  `static const __attribute__((aligned(0x1000))) uint32_t s_flash[1024]` reserves one
  page; its **data pointer IS the NVM physical address** (lands at 0x802000) and
  `const` flash arrays read directly (no tblrd). `asm volatile("reset")` works on AK
  for software reset (used by the test).
- **AK 200 MIPS option** (Tools ▸ Clock = "200 MIPS"): `f_cpu=400000000UL`; initClock
  sets PLL1 POSTDIV2=1 (vs 2) → FPLLO=FCY=200 MHz. HW-verified: EEPROM + Serial +
  millis all correct at 200 MHz.
- **AK HAL: COMPLETE** — digital I/O, millis/delay (Timer1), Serial/Serial1,
  analogRead, analogWrite/PWM, SPI, Wire/I2C, dacWrite, attachInterrupt, EEPROM.
- **Advanced I/O ✅ DONE on BOTH families (HW-verified):**
  - `shiftOut`/`shiftIn` (`wiring_shift.cpp`) — canonical bit-bang on digitalWrite/Read.
  - `pulseIn`/`pulseInLong` (`wiring_pulse.cpp`) — micros()-based width measure (both
    identical here; sub-us resolution). Verified: tracks PWM duty exactly (50%->40/40us,
    25%->20/60us on AK; tone loopback on CK).
  - `tone`/`noTone` (`wiring_tone.cpp`) — non-blocking square wave via the **SCCP
    module CCP1 as a timer**, period-match `_CCT1Interrupt` toggles the pin. CCP1/CCT1
    exist on both families but register names DIFFER: AK = 32-bit `CCP1CON1`/`CCP1PR`/
    `CCP1TMR` + `ON` bit; CK = 16-bit `CCP1CON1L`/`CCP1PRL`/`CCP1TMRL` + `CCPON` bit
    (macro'd `T_CON`/`T_ON`/`T_PR`/`T_TMR` per family). **SCCP timebase (CLKSEL=Tcy)
    differs: AK = FCY/2 (F_CPU/4), CK = FCY (F_CPU/2)** — measured on HW. Verified AK
    1000/2000/4000 Hz, CK 1000/2000 Hz via pulseIn loopback.
- **⚠ FIXED latent CK `micros()` 32-bit overflow** (`wiring.cpp`): the CK formula
  `tmr*16000000UL/F_CPU` overflows for `tmr > ~268` (tmr ranges 0..~6250), so micros()
  returned garbage — `delay()` never exposed it (it uses millis()), but `pulseIn`'s
  timeout fired instantly -> always 0. Fixed to `tmr*16UL/(F_CPU/1000000UL)`.

### dsPIC33AK Curiosity board pin map (from DS70005556 DIM sheet + demo board PDF)
Board = **Curiosity Platform Development Board (EV74H48A)** + **dsPIC33AK128MC106
GP DIM (EV02G02A)**, 64-pin device. The two PDFs are in the repo root
(`dspic33ak_demoBoard.pdf`, `dsPIC33AK128MC106-General-Purpose-DIM-Info-Sheet-DS70005556.pdf`).
Net `P##_x` on the demo board = DIM connector pin ##; the DIM sheet maps DIM pin →
MCU pin. Resolved (now in `variants/dspic33ak128mc106/pins_arduino.h`):
- **8 green LEDs LED0..LED7** = **RC3, RC4, RC5, RC6, RC7, RC8, RC9, RC10**
  (active-high). `LED_BUILTIN` = LED0 = **RC3**.
- **RGB LED**: R=**RC2**, G=**RD0**, B=**RD2** (PWM-capable: PWM4H/PWM2H/PWM1H).
- **Buttons S1/S2/S3** = **RB5 / RB4 / RA6**, **active-LOW** w/ external pull-up
  (press = 0; FALLING-edge for attachInterrupt once CN is ported to AK).
- **Potentiometer** = **RA7** (AD1AN6).
- **Two USB-serial back-channels** (both enumerate as COM ports; `COM9` is NOT the
  board — it's a Silicon Labs CP210x. The two Microchip VID 04D8 ports are the
  board): **COM24 = MCP2221A** "USB-UART" (PID 0x00DD; MCU **TX=RD3/RP52,
  RX=RD1/RP50**); **COM25 = PKOB4** back-channel (PID 0x810B; MCU **TX=RD10/RP59,
  RX=RD9/RP58**). Use these RPn when porting `Serial` to AK.
- **CLOCK SOURCE (resolves the #40 unknown):** the DIM carries an **8.000 MHz
  external CLOCK OSCILLATOR** (DSC6011, a MEMS osc — single-ended, EC mode) into
  **CLKI = RC1** (pin 38). So initClock() should select **EC @ 8 MHz** and PLL up to
  the target FOSC — no internal-FRC guessing. (Exact AK PLL1CON dividers + the
  FCY=FOSC vs FOSC/2 question still need the AK datasheet, not on disk yet.)
- **ICSP** uses **PGD2/PGC2 = RA0/RA1** (so RC3 etc. are free for LEDs).

### Multi-instance HAL pattern (how all 3 drivers work)
Each class ctor takes **pointers to its module's registers** + a **module id**.
Hot paths read/write through `volatile uint16_t*`; bit fields are reached by
**casting the pointer to module-1's bitfield struct type** (e.g.
`((SPI1CON1LBITS*)_con1l)->SPIEN = 1`, `((I2C1CONLBITS*)_conl)->SEN`,
`((U1STAHBITS*)_staH)->URXBE`). One code path, named fields, no addresses. The
few genuinely per-module symbols (PPS input selects `_SDIxR`/`_UxRXR`, interrupt
enables `_UxRXIE`) use a small `switch(_module)`. UART has **one RX ISR per module**
(`_U1/2/3RXInterrupt`) each calling `instanceN.rxHandler()`; each instance owns its
ring buffer.

### Serial pin selection at runtime, by native pin NAME (CK + AK)
Because UART TX/RX are PPS-remappable, the pins are chosen at runtime (ESP32-style):
`Serial.setPins(txPin, rxPin)` before `begin()`, or `Serial.begin(baud, txPin, rxPin)`.
**Pins are native NAMES** (e.g. `RD4`), not raw RPn — consistent with the pin model.
`HardwareSerial` stores `_txPin/_rxPin` (pin ids) and `begin()` does the work:
- `pinToRP(pin)` converts name→PPS RPn, family-specific (CK: RB=32+/RC=48+/RD=64+,
  ports A/E not remappable → 0xFF; AK: RA=1+/RB=17+/RC=33+/RD=49+).
- `serialPinCfg(pin,dir)` clears `ANSELx` + sets `TRISx` via `g_ports[]` (PPS won't
  make an analog pin digital).
- TX output func: CK U1/2/3 = 1/3/27 via `rpor_set`; AK = 9/11/13 via `ak_rpor_set`
  (AK RPOR reg numbers have gaps RPOR3/7/11 → explicit switch, not `&RPOR0+idx`).
- RX input select `_UxRXR = rxRP` per module; CK wraps writes in
  `__builtin_write_RPCON` (AK PPS is unlocked at reset). Invalid pin (0xFF) skips.
Defaults: CK Serial TX=RD4/RX=RD3 (PKoB COM); AK Serial TX=RD1/RX=RD3 (MCP2221A COM).
HW-verified on AK (COM24). **The pin→RPn + ANSEL/TRIS helpers are shared in
`dspic_pins.h`** (`dspic_pin_to_rp()`, `dspic_pin_pps_cfg()`) — used by BOTH
HardwareSerial and SPI. **SPI now also takes native pin NAMES** —
`SPI.setPins(sckPin, sdoPin, sdiPin)` (default SCK=RD6/SDO=RD5/SDI=RD7);
CK compile-verified (HW-reverify when a CK board is back). AK SPI is still a stub
(driver not ported) — its setPins is a no-op until AK SPI is implemented.

### Naming collision (IMPORTANT)
`<xc.h>` declares SFR accessor variables **`SPI1`/`SPI2`/`SPI3`** and self-`#define`s
them, so those names **cannot** be used for objects (the `SPI` *typedef* can be
renamed, the *variables* can't). Therefore SPI buses are named **`SPI` (SPI1),
`SPI_2` (SPI2), `SPI_3` (SPI3)**. `Wire`/`Wire1`/`Wire2` and `Serial`/`Serial1`/
`Serial2` are fine (no SFR named `Wire*`/`Serial*`).

### Stock-library compatibility (added in 0.1.4)
- `cores/dspic/binary.h` — full `B0..B11111111` incl. zero-padded forms.
- `cores/dspic/avr/pgmspace.h` — PROGMEM no-op; `pgm_read_*` = direct reads
  (dsPIC reads const/flash with normal loads). Included by `Arduino.h`.
- The `-DARDUINO…` defines above.

### Size report
`tools/xc16pp/bin/xc16pp-size.bat` sums objdump section sizes into decimal
`Program bytes:`/`Data bytes:`. Two bugs were fixed: (1) objdump needs
`-mdfp=<DFP>` for device-correct program-memory units (~2× too big without);
(2) the DFP path has spaces, so `-mdfp` must be **quoted** or objdump silently
ignores it. platform.txt size regex parses those decimal lines.

### Upload / erase
- Upload = MPLAB IPE headless (`ipecmd -P<dev> -TP<tool> -F<hex> -M -OL`). Plain
  **Upload** works with no programmer selected because boards set
  `upload.protocol=ipe`, `upload.tp=PKOB4`. External programmer →
  **Upload Using Programmer**. Tool codes: PKOB4/PK4/PK5/ICD4/SNAP.
- **Erase** = repurposed **Tools ▸ Burn Bootloader** (`ipecmd -E`; no bootloader
  on this core; the IDE menu label can't be renamed). Standalone:
  `tools/production/dspic-erase.bat`.
- **Mass-program (no recompile):** `tools/production/dspic-tool.bat` finds the
  newest `.ino.hex` in the IDE build cache and flashes board after board.

### Misc gotchas
- **OneDrive path with spaces** is the repo root — quote paths.
- In git-bash, **bash tools** accept `/c/...` paths but **Windows python** needs
  `C:/...`. `cmd //c` needs absolute paths (CWD with spaces won't resolve relative).
- Files commit with CRLF warnings (harmless).
- **Boards Manager index cache:** a new published version won't show until the
  IDE's cached `…/Arduino15/package_dspicArduino_index.json` is cleared (remove/
  re-add the URL, or `core update-index`). Also raw.githubusercontent CDN-caches
  the served index ~5 min.

---

## 5. Repos & release process

- **`dev`** remote = `github.com/pakchinchung/DspicArduinoCore` (full repo, where
  work happens; local `master` tracks this).
- **`origin`** remote = `github.com/pakchinchung/DspicCoreArduino` (PUBLIC; serves
  the index + hosts release assets; curated subset — no `spike/`, no dev-only
  docs, no datasheet PDFs, and the 66 MB toolchain binary is NOT in git, only in
  the release zip).
- **Served index URL:**
  `https://raw.githubusercontent.com/pakchinchung/DspicCoreArduino/master/package_dspicArduino_index.json`

- **`gh` CLI is now installed** at `/c/Program Files/GitHub CLI/gh.exe` (winget
  `GitHub.cli`, v2.95), **authenticated as `pakchinchung`** (token in the OS keyring;
  `gh auth login --web` device-code flow — re-auth only if the token is revoked).
  Extracting the git token via `git credential fill` is **blocked** by the sandbox;
  use `gh` for releases, not raw API+token.

### To publish a new version (PROVEN flow, used for 0.1.5 — 2026-06-24)
1. Bump `version=` in `platform.txt`. Update `README.md` (status line + the "What
   works" matrix) and add a newest-first platform entry to
   `package_dspicArduino_index.json` (placeholder url/checksum/size for now).
2. Build staging: `cp -r` the previous `C:\dspic-pkg-build\dspicArduino-dspic-<prev>`
   to `...-<v>` (inherits the 67 MB compiler under `tools/xc16pp` for free), then
   overwrite the curated source from the repo worktree: README, boards/platform/
   programmers.txt, cores, docs, examples, variants, `tools/production`,
   `tools/xc16pp/bin/*` (wrappers only — keep `dspic-cpp-toolchain-win/`), and
   `libraries/` (shipped in the zip since 0.1.5). Do NOT copy spike/CLAUDE.md/PDFs.
3. Zip with a single top-level dir `dspicArduino-dspic-<v>/`
   (`python -c "import shutil;shutil.make_archive(...)"`). 67 MB on disk → ~26 MB
   zipped (compiler compresses ~2.7:1; that is why the artifact is 25–26 MB).
4. `sha256sum` + `stat -c %s` the zip → write real checksum/size into the index entry.
5. **Pre-flight locally** before publishing: serve the build dir over
   `python -m http.server <port> --bind 127.0.0.1`, write a `test-index-<v>.json`
   whose url points at the local zip, then `arduino-cli --config-dir <throwaway>
   core install dspicArduino:dspic --additional-urls <local index>` (verifies the
   checksum) and `compile` a CK + an AK example against the installed tree.
6. Commit + push `dev` (full source + index).
7. Create the release with `gh` (asset BEFORE the public index goes live, else 404):
   `gh release create v<v> <zip> --repo pakchinchung/DspicCoreArduino --target master
   --title v<v> --notes "..."`. Verify: `curl -sL <asset url> | sha256sum` ==
   the index checksum, and byte size matches.
8. **Sync the FULL package source to `origin/master`** (board owner's standing rule:
   "always sync full source to the public repo" — NOT a surgical index-only update,
   and NOT a blind mirror of dev either). origin must contain everything an end user
   needs: `cores/`, `variants/`, ALL `examples/`, `libraries/`, `docs/`, `boards.txt`,
   `platform.txt`, `programmers.txt`, `tools/production` + `tools/xc16pp/bin` wrappers,
   `README.md`, `package_dspicArduino_index.json`. EXCLUDE dev-only items: **`spike/`,
   `CLAUDE.md`** (and the C++ compiler binaries under `tools/xc16pp/dspic-cpp-toolchain-win`
   stay gitignored — they ship ONLY inside the release zip). Procedure:
   `git worktree add <wt> origin/master`; `cd <wt>`; `git read-tree dev/master`
   (index := full dev tree); `git rm -rq CLAUDE.md spike`; `git commit`;
   `git push origin HEAD:master`; `git worktree remove <wt> --force`.
9. **Post-release E2E test**: `core update-index` + `core install` + `compile` from the
   REAL raw-GitHub index URL into a clean `--config-dir` (raw.githubusercontent CDN
   can lag ≤5 min). 0.1.5 passed: CK Blink + AK AkBlink both compiled.

---

## 6. Plan / TODO (full, prioritized) — tasks #34–#40

Do these roughly in order; #36 can run in parallel whenever a board is free. Each
item lists the **approach**, **files**, and **verification**. "Verify" without
hardware = `arduino-cli compile` clean + (where possible) a loopback/self-test
sketch the user flashes.

### #34 — `map()` / `random()` / `randomSeed()` / `yield()`  ✅ DONE (2026-06-24)
- **Why:** countless sketches/libraries call `map()`; it's currently absent and a
  hard compile error. Cheapest high-impact win.
- **Shipped:** new `cores/dspic/WMath.cpp` (`map`, `random(max)`, `random(min,max)`,
  `randomSeed` wrapping libc `rand()/srand()`); `yield()` weak no-op; prototypes in
  `cores/dspic/Arduino.h`; demo `examples/MathRandom/MathRandom.ino`.
- **Verify:** compile-clean (`examples/MathRandom`, 7956 bytes) with the canonical
  idiom `analogWrite(0, map(analogRead(0),0,4095,0,255))` + `random()` + `yield()`.
  **HW-CONFIRMED (2026-06-24)** on DM330030/COM6: serial streamed
  `duty=21 r1=<0..99> r2=<10..19>` — map scaling, both `random()` forms in range,
  loop runs (yield links, no hang). See §3 "Flash + serial self-test".
- **Three gotchas hit (all now captured in §4 — read before touching this code):**
  1. `random()` is **overloaded** → its prototypes must live **outside** the
     `extern "C"` block (C linkage forbids overloads). `map`/`yield` stay inside.
     XC-DSC `stdlib.h` declares `long random(void)` but only under
     `_XOPEN_SOURCE`/`_GNU_SOURCE`/`_BSD_SOURCE` (none defined → no collision).
  2. `WMath.cpp` must `#include <stdlib.h>` **before** `Arduino.h` — Arduino.h's
     `abs()`/`round()` macros otherwise mangle stdlib's declarations.
  3. `yield()` weak default lives in **`main.cpp`, not `hooks.c`** (see §4 linker
     note) — otherwise `undefined reference to '_yield'` at link time.

### #35 — `attachInterrupt()` / `detachInterrupt()`  ✅ DONE + HW-CONFIRMED (2026-06-24)
- **Why:** gates many libraries (encoders, sensors).
- **DECISION (2026-06-24): use Change Notification (CN) edge-style, NOT INTx.**
  Rationale (see §4 silicon facts for the register detail): INT1/2/3 need a PPS
  *input* map, and **PPS only covers ports B/C/D** — so INTx physically cannot
  reach RA/RE pins, including the DM330030 buttons on **RE7/8/9**. CN works on any
  pin and, with `CNCONx.CNSTYLE=1`, gives per-pin edge selection
  (`CNEN0x`=rising, `CNEN1x`=falling, both=CHANGE; `CNFx`=which-pin flag). This is
  the single primitive that covers the whole API (RISING/FALLING/CHANGE) on every
  pin. (INTx remains a possible future fast-path for B/C/D clean-edge use.)
- **Approach:** `attachInterrupt(pin, isr, mode)` (accept the dsPIC pin id;
  `digitalPinToInterrupt(p)` = identity macro so both call forms work):
  decode pin→(port,bit) via the existing `dspic_pins.h` model; index a per-port CN
  register table (mirrors `g_ports[]`); set `CNCONx.CNSTYLE=1`, `ON=1`; per mode set
  `CNEN0x`/`CNEN1x` bit; store callback in a `[port][bit]` table; enable `_CNxIE`.
  One ISR per port (`_CNAInterrupt`…`_CNEInterrupt`) scans `CNFx`, dispatches stored
  callbacks, clears the flags. `detachInterrupt(pin)` clears the enable bits +
  callback. Pull-ups: respect existing `pinMode(...,INPUT_PULLUP)` (CNPUx).
- **Files:** new `cores/dspic/WInterrupts.c` (or `.cpp`); a per-port CN table in
  `variant.cpp` (or derive from `g_ports[]`); `Arduino.h` decls +
  `RISING/FALLING/CHANGE` mode constants (LOW/HIGH already defined).
- **Verify:** DM330030 button (RE7/8/9, ext pull-up) toggles an LED (RE5/6) from
  the ISR — and serial-printable, so HW-verifiable via §3 (flash + read COM6).
- **SHIPPED (compile-clean, awaiting button-press HW confirm) 2026-06-24:**
  - `cores/dspic/WInterrupts.cpp` — CN edge-style impl, per-port reg pointer
    tables `CN_CON/CN_EN0/CN_EN1/CN_F[A..E]`, callback table `cn_cb[port][bit]`,
    ISRs `_CNAInterrupt.._CNEInterrupt` → `cn_dispatch(port)`. Kept the per-port
    IE/IF/IP bit-twiddling in `switch(port)` (bit accessors are scalar `#define`s).
  - `cores/dspic/Arduino.h` — `CHANGE=1/FALLING=2/RISING=3` (AVR values; LOW/HIGH
    level-trigger NOT supported — CN is edge-only), `typedef voidFuncPtr`,
    `digitalPinToInterrupt(p)=(p)` (our pin id IS the interrupt id),
    `attachInterrupt(uint8_t,voidFuncPtr,int)` / `detachInterrupt(uint8_t)`.
  - `examples/PinInterrupt/PinInterrupt.ino` — RE7 button (FALLING) toggles RE6 LED
    + prints a press counter. Compiles to 8146 bytes.
  - Decision NOT to override the user's pull-up in attachInterrupt (only forces the
    pin to digital-input via ANSEL/TRIS); board buttons have external pull-ups.
- **HW-CONFIRMED (2026-06-24, DM330030/COM6):** pressing RE7 fired the CN ISR — the
  RE6 LED toggled (user-observed) and the press counter incremented live on COM6
  (`button presses: 4…14` over ~10 presses, one count per press, no obvious bounce
  double-counts). Confirms CN edge-style on a **non-PPS port E** pin, FALLING edge,
  callback dispatch, and LED toggle from inside the ISR all work on silicon.
  Validated CN-register semantics: `CNEN1x` bit = falling-edge enable (a HIGH→LOW
  press), `CNFx` = per-pin fired flag, `_CNEInterrupt` is the port-E vector.

### #36 — Hardware-verify PWM / SPI / UART  ← START HERE (SPI+UART done; PWM left)
- **KEY trick — PPS self-loopback (no jumper, fully assistant-verifiable):** map a
  module's PPS *input* select to the SAME RPn its PPS *output* drives → the pin
  loops the signal back internally. Works for SPI (SDI←SDO pin) and UART (RX←TX
  pin). `examples/SpiLoopback` and `examples/Serial1Loopback` do exactly this, so
  both are checkable via §3 (flash + read COM6) with zero wiring.
- **SPI:** ✅ **HW-CONFIRMED 2026-06-24** — `examples/SpiLoopback` (`SPI`, SCK=RP70,
  SDO=SDI=RP69) streamed `SPI loopback: 8/8  PASS` on COM6. (Multi-instance SPI_2/3
  still only compile-tested.)
- **UART:** `Serial`/UART1 already HW-confirmed (it's the COM6 console). `Serial1`
  (UART2) ✅ **HW-CONFIRMED 2026-06-24** — `examples/Serial1Loopback` (U2TX=U2RX=RP72
  /RD8, PPS self-loopback, write→read-back) streamed `Serial1 loopback: 8/8  PASS`.
  `Serial2`/UART3 still compile-only (same trick would confirm it).
- **PWM:** ⏳ still needs the **user** (scope/LED) — `examples/PwmSweep` (PG1→PWM1H on
  **RB14**). PWM out is a fixed peripheral pin (not PPS) so it can't self-loopback
  to an input; an Input-Capture self-measure is a possible future autonomous route.

### #37 — `tone()` / `noTone()` / `shiftOut()` / `pulseIn()`
- **Approach:** `tone(pin,freq[,dur])` drives a square wave via a free SCCP/OC
  module (or a timer ISR toggling the pin); `noTone()` stops it. `shiftOut`/
  finish `shiftIn` = simple bit-bang loops over `digitalWrite`/`digitalRead`.
  `pulseIn` = bounded busy-wait timing using `micros()`.
- **Files:** `cores/dspic/Tone.cpp` (new), `cores/dspic/wiring_shift.c` (new) or
  fold into existing; `Arduino.h` decls.
- **Verify:** `tone` on a piezo; `shiftOut` to a 74HC595; `pulseIn` on an HC-SR04.

### #38 — EEPROM library (flash-emulated)  ✅ DONE + HW-CONFIRMED (2026-06-24)
- **SHIPPED:** `libraries/EEPROM/{EEPROM.h,EEPROM.cpp,library.properties,
  examples/Eeprom_BootCounter}`. RAM-shadowed reserved Flash page; standard Arduino
  API (read/write/update/get/put/length/operator[]/iteration) + `commit()` to
  persist (ESP/SAMD convention — writes are buffered; `commit()` does the page
  erase + double-word reprogram). CK-only NVM code (`#if __dsPIC33C__`); RAM-only
  stub elsewhere. EEPROM_SIZE = 1024 B in the first 512 words of the page.
- **HW-CONFIRMED on dsPIC33CK/COM6:** `Eeprom_BootCounter` boot count climbed
  **1→6 across software (`asm reset`) resets**, each boot reading back the
  committed value (`raw read = 0xFFFFFFFF` blank → `0x1` → … → `0x5`), every
  `commit = OK`. Proves page-erase + double-word program + read-back + persistence
  across reset all work — the NVMCON sequence from the datasheet was correct on the
  first hardware try.
- **C++ gotchas hit (now in §4):** GNU array range-init `[0...N]=v` is invalid C++
  (parsed as a lambda) → fill the reserved page via repetition macros; and
  `EERef(idx) = v;` is a most-vexing-parse (declares a var) → wrap as `(EERef(idx))`.
- **Original research notes (kept for reference):**
- **Why:** the device has no real EEPROM; many sketches expect the `EEPROM` API.
- **Approach:** bundle `libraries/EEPROM/` with the Arduino `EEPROM` API
  (`read/write/update/get/put/length/operator[]`), backed by a reserved dsPIC
  flash page using the page-erase + double-word/row write NVM sequence (`NVMCON`).
  Buffer a page in RAM, write-back on `update`/`put`. Mind flash endurance.
- **Files:** `libraries/EEPROM/{EEPROM.h,EEPROM.cpp,library.properties,examples/}`;
  ensure the linker reserves the page (variant/gld note).
- **Verify (FULLY AUTONOMOUS — no user):** write/read-back over serial, AND
  persistence across reset via a self-incrementing boot counter printed on COM6
  (self-reset with `asm("reset")` or a watchdog; no reflash needed — note a *reflash*
  bulk-erases the page, a plain *reset* does not).
- **RESEARCH FINDINGS (2026-06-24):**
  - **`libpic30.h`'s `_erase_flash`/`_write_flash16`/`_FLASH_PAGE`/`_FLASH_ROW` do
    NOT cover `__dsPIC33C__`** — the geometry `#if` chain stops at 30F/33F/24H/24F/
    24FK/33E/24E. Those helpers use legacy NVMOP codes (e.g. `0x4042`) that are wrong
    for CK. ⇒ **must hand-code the NVMCON sequence for dsPIC33CK.** (The `libpic30`
    `_memcpy_p2dN()` *read* helpers are family-agnostic and DO work for reading
    flash; `__builtin_tblrd*` is the alternative.)
  - NVM SFRs present: `NVMCON` (`NVMOP<3:0>`, `WREN`=bit14, `WR`=bit15, `WRERR`=bit13),
    `NVMADR`+`NVMADRU` (24-bit target), `NVMKEY`. Unlock = write 0x55 then 0xAA to
    `NVMKEY`, then set `NVMCON.WR` (use `__builtin_write_NVM()`).
  - **NVMOP codes RESOLVED from the CK datasheet (DS70005349H §5, on disk):**
    `NVMOP[3:0]`: `0011`=**page erase**, `0010`=row program (128 instr), `0001`=
    **double-word program** (2 instr), `1110`=bulk erase. So `NVMCON=0x4003` (WREN|
    page-erase), `NVMCON=0x4001` (WREN|double-word). **Geometry:** Flash = rows of
    **128 instr (384 B)**; a **page = 8 rows = 1024 instr (3072 B)** = the erase
    unit. Program-memory addresses step by 2 per instr word, so a page spans 0x800
    address units. **Write sequence** (datasheet Example 5-1): `NVMCON=0x4001`;
    `TBLPAG=0xFA` (write-latch upper addr); set `NVMADR`/`NVMADRU` = target; load 2
    latches with `__builtin_tblwtl(0,lo)/tblwth(0,hi)` + `(2,...)`; `disi #5`;
    `__builtin_write_NVM()`; wait `_WR==0`. Double-word writes land on a 4-word
    boundary (target addr 0x...2/6/A...). **Read** via `__builtin_tblrdl/tblrdh`
    (set `TBLPAG=__builtin_tblpage(&var)`, offset `__builtin_tbloffset(&var)`), or
    `_memcpy_p2d16` (family-agnostic). NVM IRQ optional; processor stalls during the
    self-timed op so polling `_WR` is fine.

### #39 — `Wire` I²C slave mode
- **Approach:** add `TwoWire::begin(uint8_t addr)`, `onReceive(cb)`,
  `onRequest(cb)` driven by the `SI2Cx` slave interrupt. The in-sketch slave in
  `examples/WireLoopback` already proves the register path (address match, D_A/R_W
  handling, `SCLREL`). Generalize it into the class with per-instance callbacks +
  RX/TX buffers; one `_SI2Cx`Interrupt per module dispatches to the instance.
- **Files:** `cores/dspic/Wire.{h,cpp}`.
- **Verify:** master (`Wire`) ↔ slave (`Wire1`) on one chip, or against a second board.

### #40 — dsPIC33AK128MC106 bring-up + parity  (in progress 2026-06-24 — board pending)
- **Approach:** C++ is already sim-proven on the AK family (see
  `DSPIC33A_STATUS.md`). Bring `variants/dspic33ak128mc106` to parity: correct pin
  table, clock/`#pragma config` (AK differs — **no `__prog__` shim**, different
  config words/clock tree), and validate the HAL on real AK hardware. Check AK's
  I²C/SPI/UART instance count + alt-pin scheme and add menus if needed.
- **Files:** `variants/dspic33ak128mc106/*`, possibly per-family `#if` in core.
- **Verify:** Blink + Serial + an I²C device on a real AK board.
- **DONE + HW-CONFIRMED on real AK silicon (2026-06-24, board plugged in):**
  - 🎉 **FIRST LIGHT on dsPIC33AK** — `examples/AkBlink` (8 green LEDs RC3..RC10)
    blinks on the Curiosity GP DIM board. Confirms our **C++ core + toolchain run on
    real AK silicon** (not just the sim): digital I/O, clock, startup/ctors all work.
  - ✅ **Timer1 interrupt WORKS on AK** — with a `millis()`-driven `delay()`, the
    LEDs blink (they'd freeze if the ISR never fired). So `_T1Interrupt` **vectors
    correctly through the relocatable IVT with NO extra IVTBASE setup** from us, and
    `millis()/micros()/delay()` are interrupt-driven on AK (NOT busy-wait). Register
    specifics in §4. (Earlier I'd reverted to busy-wait fearing the IVT wasn't set
    up — that was wrong; the real "no blink" was the `RE6` example-pin bug below.)
  - **AK default clock ≈ 10 MHz FOSC / ~5 MHz FCY** (measured: a `delay(500)` ran
    ~5 s at the old 50 MHz-FCY assumption → ~10× slow). `F_CPU` recalibrated to
    **10 MHz** (boards.txt `build.f_cpu` + both AK `pins_arduino.h`) so timing is
    ~right pending a real PLL setup.
  - **Real board pin map applied** (DIM sheet DS70005556 — see §4): LED0..7, RGB,
    SW1..3, POT in `pins_arduino.h`. `LED_BUILTIN = RC3`.
  - **Generic vs board variant split + Tools menu** (per user request): a generic
    bare-chip variant (`variants/dspic33ak128mc106`, pins by name) AND a board
    variant (`variants/dspic33ak128mc106_curiosity`, named LEDs/buttons/POT),
    selected by **Tools ▸ "Pin mapping"** (`menu.pins`: Curiosity [default] /
    Generic → swaps `build.variant`). Both compile.
  - **Bug found:** stock `examples/Blink` hardcodes `LED_USER=RE6` (a CK port-E pin)
    → silent no-op on AK (ports A..D only). Use `examples/AkBlink` for AK.
- **REMAINING:**
  1. ✅ **`initClock()` for AK — DONE + HW-confirmed** (FRC+PLL → 100 MHz, #41).
  2. **Port the rest of the HAL to AK.** ✅ `Serial` (UART1) + ✅ `analogRead` (ADC)
     + ✅ `SPI` (SPI1) DONE + HW-confirmed. ⏳ `analogWrite`(PWM) IN PROGRESS (see
     above). Remaining: Wire(I²C)/`dacWrite`/`attachInterrupt`(CN); AK
     `Serial1`/`Serial2`/`SPI_2`/`SPI_3` (instances 2/3) need their own regs+pins.
  3. ✅ Blink rate, PLL clock, Serial, and accurate timing all HW-verified.

> **AK status:** Blink + Serial + PLL clock (100 MIPS) all run on real silicon with
> accurate timing; generic+Curiosity variants and Clock/Pin-mapping menus in place.
> Remaining AK work is porting the other HAL drivers (SPI/Wire/ADC/PWM/CN) — each
> needs an `#elif __dsPIC33A__` branch (AK SFRs differ from CK, as the UART did).

### #41 — Clock / PLL selection menu (CK + AK)  ✅ DONE + HW-CONFIRMED both (2026-06-24)
- **CK ✅ DONE + HW-CONFIRMED (2026-06-24):** Tools ▸ "Clock (CPU speed)" menu with
  **f50** = FOSC 100 MHz/FCY 50 MHz (default), **f100** = FOSC 200 MHz/FCY 100 MHz
  (max), **frc** = 8 MHz FRC (no PLL). The menu sets `build.f_cpu` (= FOSC, flows via
  `-DF_CPU`); `variant.cpp initClock()` `#if`s on `F_CPU` to pick PLL dividers
  (PLLPRE/PLLFBDIV/POST1/POST2). `pins_arduino.h` now guards F_CPU with `#ifndef` so
  the menu `-D` wins (else identical-macro-redefinition rule breaks). **Verified:**
  flashed f100 then f50 on CK/COM6 — serial stayed clean at 9600 at both (proves the
  actual FCY matched F_CPU; a wrong clock would garble the baud). **CK PLL formula
  (reconciled, see §4):** FCY = 8 MHz·M/(N1·N2·N3)/4 (the extra /2 is FOSC=FPLLO/2 in
  PLL modes); VCO=8·M/N1 must be 400–1600 MHz.
- **AK ✅ DONE + HW-CONFIRMED (2026-06-24):** Tools ▸ "Clock (CPU speed)" on the AK
  board with **por** = power-on clock (~5 MHz FCY, no PLL, default) and **f100** =
  FRC+PLL → **FCY 100 MHz (100 MIPS)**. `initClock()` in both AK variants runs the
  datasheet Example 12-4 sequence (FRC→PLL1, M=125/N1=1/N2=5/N3=2 → FVCO 1 GHz,
  FPLLO=FCY 100 MHz; then CLKGEN1.NOSC=5 PLL1 FOUT), with bounded waits. F_CPU=
  200000000 for f100 (our F_CPU=2·FCY convention; dsPIC33A is FCY=FOSC 1:1).
  **Verified:** flashed `AkBlink` with `:clock=f100` — blink stayed ~1 s, which
  proves FCY really hit 100 MHz (a failed PLL would run ~20× slow / ~20 s, since the
  Timer1 period assumes 100 MHz). The full AK clock register/sequence detail is in §4.

### #41 (original note) — Clock / PLL selection menu (CK + AK)  ← requested 2026-06-24
- **Why (user request):** both dsPICs should let the user pick the CPU clock /
  enable the PLL **from a Tools menu** (e.g. `menu.clock` = "FRC default", "PLL
  100 MHz", "PLL max", "ext clock + PLL"), rather than hard-coding it in
  `variant.cpp`. Currently CK `initClock()` hard-codes FRC+PLL; AK runs on its
  ~10 MHz power-on clock (no PLL).
- **Approach:** add `menu.clock=Clock` (global) + per-board options that pass a
  `-DDSPIC_CLOCK_xxx` flag (and/or set `build.f_cpu`); `initClock()` in each
  variant `#if`s on that flag to program the PLL and the matching F_CPU. Keep an
  FRC/EC-default option that needs no PLL. **AK PLL is datasheet-gated** (need
  PLL1CON dividers, OSCCTRL EC-select, FCY=FOSC vs FOSC/2); AK DIM has an 8 MHz EC
  clock on CLKI/RC1 to PLL from. F_CPU must stay consistent between boards.txt
  `build.f_cpu`, the menu flag, and `pins_arduino.h` (they currently must match —
  identical-macro redefinition, else compile error).
- **Files:** `boards.txt` (menu), `variants/*/variant.cpp` (`initClock` per option),
  `platform.txt` if a clock define is needed globally.

### Known gaps not yet ticketed
- More variants/boards; `Servo`/`Stepper` library validation; `analogReference`
  modes beyond AVDD; deeper `SPI`/`Wire` transaction edge cases; `SoftwareSerial`/
  soft-I²C (decided against for now — hardware-first via PPS). The `tools/build`
  compiler-build scripts are dev-only (not shipped in the package).

---

## 7. Key files

- Core: `cores/dspic/{Arduino.h, dspic_pins.h, wiring*.cpp, HardwareSerial.*,
  SPI.*, Wire.*, Print.*, Stream.h, WString.h, main.cpp, hooks.c, new.cpp,
  xc_compat.h, dspic_cpp_compat.h, binary.h, avr/pgmspace.h}`
- Variants: `variants/dspic33ck256mp508/{variant.cpp, pins_arduino.h}` (+ AK).
- Build wrappers: `tools/xc16pp/bin/*` (`.sh` for mac/linux, `.bat` for Windows;
  `xc16pp-env.bat` = shared discovery).
- Bench tools: `tools/production/{dspic-tool,dspic-prog,dspic-erase}.bat`.
- Packaging: `boards.txt`, `platform.txt`, `programmers.txt`,
  `package_dspicArduino_index.json`.
- Docs: `docs/{TESTING_AND_RELEASE, BUILDING_THE_COMPILER, BUILD_JOURNEY,
  DSPIC33A_STATUS}.md`, and this file (the plan/status source of truth).
- Memory files (`.claude/.../memory/MEMORY.md` + entries) hold the same gotchas in
  recall-friendly form — keep them in sync with this doc.
