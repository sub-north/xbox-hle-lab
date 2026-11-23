# The stop at `+0x1CE6E13`

## Corrections (2026-11-23)

- **Retracted: "the map is empty, so every key misses."** The empty 16 × `0x80` control run belongs
  to `[[rsi+0x2a8]+0x20]`, one of two maps. The settings descriptor at `[rsi+0x2b0]` carries its own
  map at `+0x10`, and that one is populated (50 entries). "Every key misses" is now **hypothesis**.
- **Corrected: the `0x2a8`/`0x2b0` mix-up.** `0x70` sizes the descriptor at `[rsi+0x2b0]`, not the
  0x80-byte instance at `[rsi+0x2a8]`; `0x6195668` is the outer object's vtable, `0x6301540` the
  instance's. The `0x80` size is real (its allocation), not a probe read length.

Evidence levels used here: **verified** = measured in our runs, read directly from the title's bytes,
or present in archived run output; **reported** = a third party's claim that we have not reproduced;
**notes-only** = recorded in our lab notes, with no archived log attached; **hypothesis** = reasoning,
stated as such and not confirmed.

## What the stop is

- **verified** — the stop is an `int3` (exception `0x80000003`) at the title's RVA `+0x1CE6E13`.
- **verified** — the function that contains it spans `0x1CE6DF0`–`0x1CE6E19`, 41 bytes.
- **verified** — its predicate lives in `0x292D370` and contains a scan over `0x110`-byte records.
  That the trap fires because the scan *matches 0* is **hypothesis**, not proven.
- **verified (static census, 2026-11-23)** — no string reference was found in the scanned ranges of
  the crashing function and its predicate. The nearby
  `"The method Stage::displayStateSet() is not implemented"` belongs to a warning thunk at
  `0x4D74DB3` and lies outside those ranges. The earlier association was proximity-only and is
  withdrawn: it is **not** evidence for this stop, and no name it suggests is used here
  (**hypothesis withdrawn**).

## Call chain

- **verified (crash stack scan)** — no faulted frame. The return addresses read from the stack are
  `0x2240EAB` → `0x2236CF6` → `0x293EF38`; `0x293EF38` is the return address of the call at
  `0x293EF33`, whose target is `0x5A3F200` — a call target, not a stack entry. That call is the route
  in to `0x1CE6DF0`. The scan also contains `0x53EC0F0`, which is code in `.text`, not a vtable.

## Measured state at the crash

- **verified (crash probe)** — `rsi` points at an object with vtable `0x6195668` and no RTTI; the
  object's name is unknown. It was previously called the `Stage` object on the strength of the
  withdrawn string association, and is not named here.
- **verified (PE, 2026-11-23)** — `[rsi+0x2a8]` is a 0x80-byte instance (allocated with
  `mov ecx,0x80` at `0x5B2AF04`) whose vtable RVA is `0x6301540`. Call it **A**. A holds a
  `0x290`-stride map at `[A+0x20]`.
- **verified (archived run-26 probe output, line 1373)** — the control bytes of that map at
  `[A+0x20]` begin with sixteen `0x80` bytes, the empty marker: this block is empty.
- **verified (PE and probe)** — `[rsi+0x2b0]` is a 0x70-byte descriptor, **C**, freed with
  `mov edx,0x70` at `0x5A3F2DC`. Its `+0x60` holds `{tag = 12, count = 12, list}`: the twelve
  pack-name hashes.
- **verified (probe)** — C's own flat map at `C+0x10`/`+0x14`/`+0x18` is **populated**: 4 groups,
  50 entries, with a base pointer. It is not the empty block.
- **inferred, not measured** — C and `[[rsi+0x2a8]+0x78]` are the same block: both carry the
  `{12, 12, list}` header at `+0x60`, and the predicate null-tests `[rsi+0x2a8]+0x78`
  (`0x292D3A7`). No run has printed both pointers.
- **verified (static sweep)** — `0x694A1F8` is neither read nor written anywhere in `.text` and is
  zero in the file. It is benign and **not** the gate; it is no longer described as a fill source.
- **verified** — `0x7693C44` also reads `0` at the crash; no role is claimed for it.
- **not proven** — which values the predicate tests. That the scan "matches 0" is not shown to
  follow from the empty block at `[A+0x20]`, from C's 50 entries, or from the per-key compare at
  `0x292D465`. The scan's arms are unproven.

So what is established is an **empty map at `[A+0x20]`** beside a **populated map in C**. Which map
the twelve keys are judged against, and why a key present in C can still be rejected (interface
check, the outer object's vtable call, or the found-vs-installed count), is **hypothesis**.

## Falsified hypotheses

- **verified** — *missing dump directory*: runs 7–9 were **null tests**. The `mkdir` ran against a
  read-only CD volume and failed; the drive letter never moved, so those runs tested nothing. Only
  run 10 actually created the `LocalDumps` directory in the game folder, and it changed nothing.
- **verified** — *missing data archives*: `common.rpf` was present and the result was unchanged;
  `data.rpf` was staged but the title never queried it. Neither moved the stop.
- **verified (archived run-26 log)** — *further staging*: `movies.rpf` present (query returns `0x20`)
  left the stop unchanged, and staging the script, shaders, textures and packs archives changed
  nothing either. Staging content is ruled out as the cause.
  (The earlier crash-reporter assert at `+0x2237178` had already been cleared by staging
  `appdata0.rpf` in run 14.)

## Skipping the assert is not a fix

- **notes-only** (no archived log) — `XCL_SKIP_INT3=1` steps over the break. The title then faults
  with a **write to address 0** inside `vcruntime140.dll` (`0xc0000005`); stack `+0x5B29847` →
  `+0x2240EFC` → `+0x2213025`.
- The assert therefore guards a real precondition that is used later; stepping over it is a
  diagnostic, not a fix.

## Upstream build `66e8e97`

- **notes-only** (no archived log) — xcl `66e8e97` was built and run; our title still stops at the
  **same** RVA, `+0x1CE6E13`. Runs 21 and 22 are recorded in the lab notes only.
- **reported (AUTHOR-REPORTED)** — upstream's notes for the same title claim 45 s of execution and a
  first frame at 3.6 s. That result does **not** reproduce on our title here; it is most likely a
  different, smaller retail title.

## Not claimed

- No window, no D3D12 command list and no frame exist at this stop.
- That the scan *matches 0* is unproven; the empty block at `[A+0x20]` and C's populated map are
  measured.
- Nothing here follows from the `displayStateSet` string; see the withdrawn hypothesis above.
