# dsPIC33A (dsPIC33AK) C++ status — ✅ WORKS

Tested with the rebuilt C++ compiler (GCC 8.3.1, `pic30-elf`) against
`dsPIC33AK256MPS205` (DFP `dsPIC33AK-MP_DFP`) on the MPLAB X Simulator.

## Result: C++ runs correctly on dsPIC33A

| Capability | dsPIC33C (33CK256MP508) | dsPIC33A (33AK256MPS205) |
|---|---|---|
| Plain C compiles + runs | ✅ | ✅ |
| C++ compiles (vtables, ctors, ISA) | ✅ | ✅ |
| C++ links to a valid ELF | ✅ | ✅ |
| **C++ runs correctly on the sim** | ✅ verified | ✅ **verified** |

`scalar_33a_test.cpp` on `33AK256MPS205` reads back **`rv_poly=2`** (polymorphic
dispatch through a base pointer), **`rv_tag=99`** (inheritance + constructor
chaining `Derived()→Base(99)`), **`rv_base=1`** (base virtual), **`rv_sig=0xABCD`**
(ran to completion). The same compiler handles 33C and 33A — 33A is just a wider
32-bit ISA (`mov.l`/`movs.l`) selected by `-mcpu`.

## The earlier "traps before main" conclusion was WRONG — it was a test-harness issue

Stepping (`stepi`) from reset shows the 33A C++ program executes cleanly through
startup and `main`, parking in its final `while(1)` — **no trap, no hang.** The
confusion came from two **mdb/simulator quirks specific to 33A C++ binaries** (not
the compiler or the code):

1. **`run` + breakpoint does not halt** on 33A C++ binaries (it does for trivial C).
   So reads taken after `run` happened while the program was still running → `null`.
2. **mdb returns `null` reading C++ *array* elements** on 33A (Linux-compiled DWARF
   vs Windows mdb). Simple scalar globals read fine (like a plain C global does).

### Working method to verify C++ on dsPIC33A
- Use **simple scalar `volatile int` globals** for results (not an array).
- Drive execution with **`reset` then a batch of `stepi`** to reach the final loop,
  instead of `run`+breakpoint. (~400 `stepi` reaches `main`'s end for a small test.)
- Read each scalar with `print /x /datasize:2 <name>`.

See `spike/phase1/scalar_33a_test.cpp`. `tools/test/run-cpp-sim-test.sh` auto-uses
this method for `33A*` devices and the array+breakpoint method for others.

## Bottom line
C++ (classes, multiple objects, inheritance, virtual/polymorphic dispatch) is
**verified working on both dsPIC33C and dsPIC33A** with the rebuilt compiler.
