# Working rules for this project (dspicArduino)

**WRITE DOWN WHAT YOU LEARN — IMMEDIATELY, AS YOU LEARN IT.**
The single most important rule here. Every time you figure out a non-obvious fact
(a register layout, a pin/RPn mapping, a silicon quirk, a build/linker gotcha, a
toolchain path, a working command), append it to **`docs/PROJECT_STATUS.md`** in
the moment — do NOT batch it to the end of the task, and do NOT rely on the next
session re-deriving it. Treat the doc as the project's long-term memory:

- New silicon/peripheral facts → §4 "Technical knowledge".
- Build/flash/test commands and gotchas → §3.
- Task progress (done / in-progress / next) → §6, and bump the header date.
- If a fact is reusable across sessions, also mirror a one-line hook into the
  Claude memory index (`MEMORY.md`) pointing back at the doc section.

Rationale: this is a long, multi-session hardware bring-up. Re-learning register
maps and toolchain quirks from datasheets/headers is the dominant time sink;
capturing each finding once eliminates it. The user has asked for this explicitly.

**Other standing conventions** (full detail in `docs/PROJECT_STATUS.md`):
- `docs/PROJECT_STATUS.md` is the source of truth — read it first when resuming.
- Sync edited `cores/dspic/*` files to the active install dir before compiling
  (see §3). Watch the stale `core.a` cache after editing `.c/.cpp` sources.
- Never redistribute Microchip proprietary files (DFP, ipecmd, datasheet PDFs).
- Hardware self-test is available: flash via PKOB4, read `Serial` on COM6 (§3).
