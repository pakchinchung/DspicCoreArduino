# Phase 1 — simulator verification results

The C++-enabled dsPIC compiler (built per `docs/BUILD_JOURNEY.md`) was verified on
**Microchip's MPLAB X Simulator** (driven headless via `mdb.bat` — Microchip's own
cycle-accurate dsPIC simulator, no third-party emulation). No hardware required.

## Test 1 — `cpp_test.cpp` (constructor + vtable basics)
Confirmed in assembly + runtime: a global object's constructor runs before `main()`
(registered in `.ctors`, which the dsPIC `crt0` walks) and a virtual call dispatches.

## Test 2 — `multi_object_test.cpp` (multiple objects, inheritance, polymorphism)

Two classes (`Counter`, `StepCounter : Counter`), **three** global objects with
independent state, virtual `next()` overridden in the derived class, dispatched
polymorphically through base pointers. Results read from the device RAM array `R[]`:

| Read  | Value (hex) | Decimal | Proves |
|-------|-------------|---------|--------|
| R[0]  | 0x000A | 10  | `Counter a(10)` constructed before main |
| R[1]  | 0x000B | 11  | `a.next()` → `Counter::next` (+1) |
| R[2]  | 0x0064 | 100 | `StepCounter b(100,2)` constructed |
| R[3]  | 0x0066 | 102 | `b.next()` → `StepCounter::next` **override** (+2) |
| R[4]  | 0x0007 | 7   | `StepCounter c(0,7)` after a polymorphic call (+7) |
| R[5]  | 0x007B | 123 | **polymorphic sum** over base pointers: 12 + 104 + 7 |
| R[6]  | 0x0003 | 3   | all three objects iterated |
| R[7]  | 0xABCD | —   | signature: program ran to completion with all values set |

**8/8 correct.** This is conclusive: constructors-before-main, per-object state,
inheritance, and virtual/polymorphic dispatch all work on the dsPIC.

## How to reproduce

The compiler is Linux-hosted (WSL); binutils + simulator are the Windows XC-DSC tools.

```bash
# 1. (WSL) compile C++ -> dsPIC assembly with the rebuilt compiler
elf-g++ -B<prefix>/libexec/gcc/pic30-elf/8.3.1 -mcpu=33CK256MP508 \
        -mdfp="<DFP>/xc16" -S -g -O1 -fno-exceptions -fno-rtti \
        multi_object_test.cpp -o multi_object_test.s

# 2. (Windows) assemble + link with the XC-DSC binutils (they support pic30)
xc-dsc-gcc -mcpu=33CK256MP508 -mdfp="<DFP>/xc16" -omf=elf -g \
           -T<DFP>/xc16/support/dsPIC33C/gld/p33CK256MP508.gld \
           multi_object_test.s -o multi_object_test.elf

# 3. (Windows) run on the simulator
mdb.bat multi_object_test.mdb     # see multi_object_test.mdb
```

### mdb gotchas learned (for future runs)
- Break at the **`done()` line** (an `extern "C"` no-op called after all writes), not
  a guessed line — code shifts move line numbers; breaking too early reads stale data.
- mdb only reads memory via `print <symbol>`; use **`/datasize:2`** (dsPIC `int` = 2 bytes).
- Store results in a **single array** `R[]` — mdb resolves one array symbol cleanly,
  whereas individual globals from Linux-compiled DWARF sometimes resolve to wrong
  addresses. Assign array elements in index order (mdb's scripted `run` is async).
