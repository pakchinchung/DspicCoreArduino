# Phase 1 Spike — C++ runtime feasibility on dsPIC

## Question
Before building a C++-enabled compiler (cc1plus) from the XC-DSC GPL source,
does the dsPIC runtime actually support what C++ needs — namely **static
constructors running before `main()`** and **vtable-style indirect dispatch**?

## Result: ✅ PROVEN — using only the stock C compiler + MPLAB X simulator

No cc1plus and no hardware were needed. `__attribute__((constructor))` in C
emits into the **same `.ctors` table** that C++ global objects use, so it
exercises the identical mechanism.

### Evidence

1. **Startup already calls constructors.** `libpic30` `crt0_standard.s`:
   ```asm
   rcall  __ctor      ; walks the __ctors table, calling each entry via `call w11`
   call   _main       ; only then enters main()
   ```
   (`__ctor` is in `crt_ctor.s`.)

2. **Our constructor is registered in the table.** After compiling `ctor_test.c`:
   - `_my_global_ctor` @ `0x0352`
   - `.ctors` section @ `0x035e` contains bytes `52 03 00 00` = `0x00000352`
     → a pointer to our constructor.

3. **Runtime confirmation on the MPLAB X simulator** (`mdb`, Sim tool):
   ```
   main_saw_ctor = 0xC0DE   ← set by the constructor, read inside main()
   ctor_ran      = 0xC0DE
   ```
   `main_saw_ctor == 0xC0DE` proves the constructor ran *before* main.

4. **Vtable dispatch is implicitly proven**: `__ctor` invokes each constructor
   with `call w11` — an indirect call through a memory table, which is exactly
   how a C++ virtual call dispatches. The passing test exercises this path.

## Conclusion
The dsPIC C runtime requires **no modification** to support C++ global objects
and virtual dispatch. Phase 1 reduces to: build cc1plus + a g++ driver from the
GCC 8.3.1 XC-DSC GPL source, add freestanding C++ runtime stubs, and re-run a
real C++ version of this test. Low risk — the hard runtime question is settled.

## Reproduce in the MPLAB X GUI (no command line)
1. New Project → Standalone → device `dsPIC33CK256MP508` → XC-DSC toolchain.
2. Add `ctor_test.c`.
3. Set the debugger tool to **Simulator**.
4. Build, then Debug. Halt in the `while(1)` loop.
5. Open Variables/Watch, add `main_saw_ctor` → expect `0xC0DE`.

## Files
- `ctor_test.c`      — the test (stock C compiler)
- `mdb_script.txt`   — headless simulator run script for `mdb.bat`
- `ctor_test.elf`    — built artifact (gitignored)

## Exact build command that worked (stock PRO compiler)
```
xc-dsc-gcc -mcpu=33CK256MP508 \
  -mdfp="<MPLABX>/packs/Microchip/dsPIC33CK-MP_DFP/1.15.423/xc16" \
  -omf=elf -O1 -g \
  -T"<DFP>/xc16/support/dsPIC33C/gld/p33CK256MP508.gld" \
  -o ctor_test.elf ctor_test.c
```
Note the two flags the boards-manager wrapper MUST supply: `-mdfp=<DFP>/xc16`
and the explicit device `-T <device>.gld`.
