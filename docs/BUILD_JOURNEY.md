# Building a C++-enabled dsPIC compiler — the complete, reproducible record

This documents **everything** required to rebuild Microchip's XC-DSC GCC with C++
enabled, including every problem hit and its fix. It exists so the build can be
reproduced on a **future XC compiler version** without rediscovering all of this.

> **Result achieved (XC-DSC v3.31.01 = GCC 8.3.1):** a working `cc1plus` + `elf-g++`
> that compiles C++ for `pic30-elf` and emits correct dsPIC code — verified: it
> generates a **vtable** (`__ZTV7Derived`) and registers a **global constructor**
> in the **`.ctors`** table that the dsPIC `crt0` runs before `main()`.

Authoritative script (encodes all fixes): `tools/build/build-on-wsl.sh` (the path
that worked). Verify with `tools/test/run-cpp-sim-test.sh <device>`.

---

## 1. The one decision that matters most: host compiler version

GCC 8.3.1 source **will not build cleanly with a host gcc that is too new**
(gcc-15/16). Their libstdc++ uses `abort()` etc. inside standard headers in ways
GCC 8.3's source can't absorb → cascading errors in `prefix.c`, `diagnostic-color.c`, …

**Use gcc-11 (or gcc-12) as the host compiler.** This was THE fix that made the
build go through. On a new XC version, match the bundled GCC's era: use a host gcc
no more than ~3–4 major versions newer than the source.

| Host gcc | GCC 8.3.1 source builds? |
|---|---|
| gcc-15 / gcc-16 | ❌ libstdc++ collisions (abort macro, locale_facets, …) |
| gcc-11 / gcc-12 | ✅ clean |

---

## 2. Verified-working environment

- **WSL Ubuntu** (Linux build — avoids all the Windows/MSYS shell+path issues; it's
  also how Microchip builds). A native Windows/MSYS2 build is possible but hit many
  extra issues (see §6).
- **Host compiler: `gcc-11` / `g++-11`** — `sudo apt install -y gcc-11 g++-11`
- **Build deps:**
  ```bash
  sudo apt install -y build-essential bison flex \
       libgmp-dev libmpfr-dev libmpc-dev libisl-dev texinfo libexpat1-dev
  ```
- **Source:** XC-DSC GPL source drop. Layout:
  - GCC tree: `…/<ver>.src/src/xc16_gcc_831/gcc`  (has `configure`, `gcc/cp`, `gcc/config/pic30`)
  - binutils: `…/xc16_gcc_831/binutils`  (⚠️ stock 2.32 — see §5)
  - resource headers: `…/<ver>.src/src/c30_resource/src/c30/resource_info.h`
  - Microchip's own build recipe: `…/<ver>.src/src_build.sh` → `build_XC16_831`

---

## 3. The build recipe (mirror Microchip, change ONE flag)

Recovered from `src_build.sh` → `build_XC16_831`. The **only** change vs Microchip
is `--enable-languages=c` → `--enable-languages=c,c++`.

```bash
export CC=gcc-11 CXX=g++-11          # host compiler (critical, see §1)

# REQUIRED CFLAGS/CXXFLAGS defines (pic30 back-end won't compile without them):
DEFS="-DLONG_MODIFIER -D_BUILD_C30_ -D_BUILD_MCHP_ -DRESOURCE_MISMATCH_OK -I<c30_resource>/src/c30"

CFLAGS="-O2 $DEFS" CXXFLAGS="-O2 $DEFS" \
<gccsrc>/configure \
  --target=pic30-elf --prefix=<prefix> --program-prefix=elf- \
  --with-gmp=/usr --with-mpfr=/usr --with-mpc=/usr \
  --enable-stage1-languages=c --enable-languages=c,c++ \
  --disable-decimal-float --disable-multilib --disable-bootstrap \
  --disable-libffi --disable-libgomp --disable-libmudflap --disable-libquadmath \
  --enable-libssp --disable-libstdcxx-pch --disable-libstdcxx-verbose --disable-lto \
  --disable-maintainer-mode --disable-nls --disable-shared --disable-sim \
  --disable-threads --disable-tls --disable-gdb --disable-werror \
  --enable-interwork --enable-plugins --enable-target-optspace \
  --enable-sjlj-exceptions --with-gnu-as --with-gnu-ld \
  --without-isl --without-cloog --without-headers --with-dwarf2

make -j$(nproc) all-gcc      # builds cc1, cc1plus, the driver (NOT target libs)
make install-gcc
```

`all-gcc` is enough — it builds the compiler. Target libraries (libgcc, …) are not
needed to prove C++ and would require working pic30 binutils (which we don't build).

---

## 4. Source patches required

Applied to the GCC source before configuring (idempotent; baked into the scripts):

### 4a. `gcc/cppdefault.c` — `NATIVE_SYSTEM_HEADER_DIR` fallback
Microchip's `#ifdef _BUILD_MCHP_` block uses `NATIVE_SYSTEM_HEADER_DIR`, but cross
builds (`CROSS_DIRECTORY_STRUCTURE`) leave it undefined → *"not declared"* error.
Add, right before `const char cpp_NATIVE_SYSTEM_HEADER_DIR[]`:
```c
#ifndef NATIVE_SYSTEM_HEADER_DIR
#define NATIVE_SYSTEM_HEADER_DIR "/include"   /* unused for a cross compiler */
#endif
```

### 4b. (Only if host gcc is too new — avoid by using gcc-11) `gcc/system.h`
Pre-include `<memory> <string> <vector> <algorithm> <utility>` (guarded by
`__cplusplus && !GENERATOR_FILE`) before the `abort()` macro override. This only
partially mitigates the gcc-15/16 skew — **prefer fixing the root cause: use gcc-11.**

---

## 5. binutils: do NOT try to build it from this source

The binutils-2.32 tree in the GPL drop is **stock** — `bfd/config.bfd`,
`gas/configure.tgt`, `ld/configure.tgt` contain **no pic30 target**. Symptoms:
`GAS does not support target CPU pic30`, `BFD does not support target pic30-unknown-elf`.
(Its `config.sub` also rejects `pic30` — gcc's Microchip-patched `config.sub` accepts
it — but patching config.sub isn't enough since the target tables lack pic30.)

**Conclusion:** skip building binutils. Use the **Windows XC-DSC binutils**
(`xc-dsc-as`, `xc-dsc-ld`, … which DO support pic30) to assemble/link, and the
MPLAB X simulator to run. The Linux build only needs to produce `cc1plus`/`elf-g++`.

---

## 6. Complete issue → fix log (symptom-indexed for future versions)

| # | Symptom | Cause | Fix |
|---|---|---|---|
| 1 | `Invalid configuration` / target `${target_alias}` | bad triple auto-recovery | hardcode `--target=pic30-elf` (confirmed by `gcc/config.gcc` `pic30*-*-elf*`) |
| 2 | script aborts silently early | `set -e` + `ls` of a missing file | `pick_tool()` that never returns non-zero |
| 3 | `configure: error: Oops, mp_limb_t doesn't seem to work` | GCC's in-tree GMP fails ABI test under new host | use system GMP/MPFR/MPC (`--with-gmp=/usr`; move in-tree `gmp/mpfr/mpc/isl` aside) |
| 4 | `pic30-c.c: fatal error: resource_info.h: No such file` | pic30 back-end needs Microchip's resource headers | add `-I<c30_resource>/src/c30` + `-DRESOURCE_MISMATCH_OK` to CFLAGS/CXXFLAGS |
| 5 | `cppdefault.c: 'NATIVE_SYSTEM_HEADER_DIR' was not declared` | cross build undefs it; MCHP block uses it | patch 4a |
| 6 | `xc-covtool-comm.c: fatal error: expat.h` | Microchip tool needs Expat | `apt install libexpat1-dev` (Linux) / `pacman -S mingw-w64-x86_64-expat` (MSYS2) |
| 7 | `prefix.c` / `diagnostic-color.c`: errors at `system.h` `abort()` macro | host gcc-15/16 libstdc++ uses abort() in headers | **use gcc-11 host** (root fix); patch 4b is a partial mitigation |
| 8 | `elf-g++: error trying to exec 'cc1plus': No such file` | driver can't locate cc1plus (program-prefix layout) | invoke with `-B<prefix>/libexec/gcc/pic30-elf/<ver>` |
| 9 | dep check fails though `libgmp-dev` installed | Ubuntu multiarch puts `gmp.h` under `/usr/include/x86_64-linux-gnu` | test headers via `echo '#include <gmp.h>' \| gcc -E -` not a fixed path |
| — | **Windows/MSYS2-only** (avoided by building on Linux): | | |
| 10 | `No rule to make target 'all'` in gcc subdir | spaces in build path break GCC's make | build in a space-free dir |
| 11 | `gengtype.exe: …/line-map.h: No such file` | native tools can't open MSYS `/c/...` paths | pass mixed `C:/...` paths (`cygpath -m`) to configure |
| 12 | `NATIVE_SYSTEM_HEADER_DIR` define lost despite being on cmdline | MSYS mangles many quoted `-D` args to native g++ | (was a red herring; real cause was 4a + cross undef) |

---

## 7. Verifying a build (no hardware needed)

1. **Compile the C++ test to assembly** with the new compiler:
   ```bash
   elf-g++ -B<prefix>/libexec/gcc/pic30-elf/<ver> \
     -mcpu=33CK256MP508 -mdfp="<DFP>/xc16" \
     -S -O1 -fno-exceptions -fno-rtti spike/phase1/cpp_test.cpp -o cpp_test.s
   ```
   Confirm the assembly contains a vtable (`_ZTV7Derived`), the virtual method
   (`_ZN7Derived5valueEv`), and a `.ctors` entry referencing `_GLOBAL__sub_I_*`.
2. **Full runtime check (optional):** assemble + link `cpp_test.s` with the Windows
   XC-DSC binutils, then run on the MPLAB X **Simulator** via `mdb` and confirm
   `main_saw == 0xC0DE` and `vcall == 42` (see `spike/phase1/README.md`).

---

## 8. Reproducing on a NEW XC compiler version — checklist

1. ☐ Download the new GPL source drop; note the bundled GCC version (`gcc/BASE-VER`).
2. ☐ Pick a host gcc no more than ~3 versions newer than that (see §1).
3. ☐ Confirm `gcc/config/pic30` exists and `gcc/config.gcc` still matches `pic30*-*-elf*`.
4. ☐ Re-read `src_build.sh` / `build_XC16_*` for any changed flags/defines; update §3.
5. ☐ Run `tools/build/build-on-wsl.sh` (it applies patches 4a, system-gmp, skips binutils).
6. ☐ Work the §6 table for any new symptom.
7. ☐ Verify per §7.
8. ☐ **GPLv3:** publish the modified source you built from (copyleft). See `PLAN.md`.
