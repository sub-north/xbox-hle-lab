# Next steps: help clear the current stop

What the lab knows, where it is blocked, and three concrete ways to contribute. Labels: **verified**
= reproduced here with a log; **reported** = someone else's claim, not reproduced; **hypothesis** =
reasoning we have not confirmed. Nothing here needs the title's data, and nothing here is a fix.

## Where the lab is

The title runs unmodified under a console-to-PC HLE runtime (xcl). It clears XMem allocation, the
D3D12 configure call and descriptor/heap creation, XGameRuntime, GameInput, the DirectStorage
factory, and XPackage identity. With the complete, hash-verified payload the early init validator
clears, the run builds graphics state, and it now presents sustained frames of its own UI.

**Run 6 changed the blocker.** With the one archive that had been torn replaced by a byte-identical
copy, the ScriptStore registry assert at `+0x29DEFB1` **did not fire** — zero occurrences in the log,
after three consecutive runs that terminated on it. The run went on to **1 111 presents in 442.7 s**,
active streaming, and — in the title's own crash context — a **populated world**: `Game State: Game
Running`, 22/600 peds, 24/700 cars, 6 910/22 500 buildings, a real camera and player position, one live
script thread. Then it aborted itself at `+0x23CF683` (write to address 0). See
[BARE-METAL-RUN-6.md](BARE-METAL-RUN-6.md). (verified)

**Run 7 is a negative result.** A solid-block fill for refused BCPack destinations works — the
`filled_bytes` counters left zero for the first time — and it **halves throughput** (1 111 → 606
presents over near-identical windows) with the **stop unchanged** at `+0x23CF683`, same registers.
See [BARE-METAL-RUN-7.md](BARE-METAL-RUN-7.md). (measured)

## The blocker

- The stop is `0xC0000005`, a write to `0x0`, at the title's RVA **`+0x23CF683`**, with the stack
  `+0x4E4BA89`, `+0x2215C35`, `+0x71B9A20`, `+0x2215ECA` and `GPU Loc Sig` = `7333478d`.
  (verified)
- **Same stop, same stack, same signature** as ivall/xcl#505, an independent report on the same
  title build. (verified by comparison)
- **The stop is a heartbeat stall detector, not a timer on the load scope.** A 1 Hz predicate thread
  holds two rules: **A**, a load scope open for 300 s (float at RVA `0x6159E20`); **B**, no scope
  open (`[0x71B6334] == 0`) *and* a heartbeat (`[0x71B6338]`) frozen for 30 polls. Run 6 faulted with
  `rbp=0 rbx=0`, which only the no-scope path produces, so **rule B** fired: 29.2 s between the last
  present (+413.2 s) and the abort (+442.4 s), i.e. 30 polls at 1 Hz. A probe that raised the 300 s
  limit to `1800.0f` logged `scopes count=0` at +60 s **and** +240 s — no scope was ever open, so rule
  A had no subject. The title then crashes itself deliberately (`mov dword [0], 0x4D8BD421`) to force
  a diagnostic dump; the access violation is the dump trigger, not a title bug. (verified) Superseded
  wording, kept so the record shows what changed: "The abort is **scope start + 300 s**, not a fixed
  wall clock." See [STALL-DETECTOR.md](STALL-DETECTOR.md).
- **Resolved (inferred, strong):** the registry assert was a consequence of the torn archive, not a
  missing subsystem. With the archive intact the lookup is never reached. The run-5 advice to
  instrument that lookup is withdrawn for this stop. One boot so far; a repeat without the probe is
  still owed.
- **Not a visible render.** The simulation was running in the world — pools populated, player on the
  ground, a script thread live — but every captured frame is still the loading spinner. No frame
  shows the world, and nothing here is interactive play.

## Three named leads

1. **Why the frame loop stalls.** This is the whole remaining question: the detector is a symptom, so
   the target is whatever blocks the loop for 30 s. Three facts bear on it. Another lab reports
   `Dispatch -> pso_async::resolve_wait -> p.cv.wait` with no timeout for this same stop — a blocking
   wait nothing signals is exactly the shape rule B sees. (**reported**, from #505; not symbolized in
   our build.) Our pipeline backlog at the stop is lopsided, `[26, 0, 58736]` (**verified
   observation**, field meanings inferred). And one very large DXC compile dominates the window before
   the stall (**verified measurement**, causal link inferred). What would settle it now: a **stack
   sample 5 to 25 s after the last present** — inside the detector's 30 s window, before the dump
   overwrites the evidence. A `Game State` value at the abort would help too; #505 reported one, our
   parameter list does not carry it. (verified decode + reported chain)
2. **`ID3D12GraphicsCommandList10::#101`.** #505 names this call returning 0 as the suspected trigger.
   It does **not** appear in our unimplemented-call list, so we cannot confirm it from our own log.
   (reported) Our own per-frame unknowns are `#126`, `#142` and `#75` at exactly 1 111 calls each —
   one set per presented frame — and `ID3D12CommandAllocator::#9` at 1 116. They are the obvious
   candidates for a console-specific call whose absence stalls forward progress. Mapping any of
   these four slots to its console operation would be a real contribution. (verified counts)
3. **The BCPack decoder.** Streaming is on the critical path and 512 requests were refused at run 6's
   stop: BC1 348, BC5 107, BC6H 1, BC7 56, plus a DirectStorage request failure. Run 7 tested the
   cheap version of this and it failed: a solid-block fill for refused destinations works (BC1 1.1 MB,
   BC5 7.0 MB, BC7 4.7 MB written) but **halves** the present rate and leaves the stop identical. So
   the refusal path is not the gate; real decoding (or a documented BCPack → PC block-format mapping)
   is still untested and still worth doing. (verified counts, measured negative)

## What is ruled out

Ten tested explanations are in [FALSIFIED.md](FALSIFIED.md), each with the run and log line that
killed it. Read that table before proposing a cause, and do not retest a row unless its "do not
retest unless" condition is now met.

Two rows need an explicit update after run 6:

- **"The registry assert is the gate."** Retired. It reproduces only on a torn or incomplete payload;
  with the archive intact it is not reached. Do not resurrect the lookup-instrumentation plan.
- **"Skipping the break does not advance."** That observation was made on the old payload. With the
  intact archive the break is not present to skip, so it no longer describes the current run.

## Reproduce and compare

Run the same build on an intact payload, capture the first 30 s after the last frame, and send:

- the stop offset and the full register set;
- the title's crash parameters (`GPU Loc Sig`, `Data Version`, `Code Branch`, `Build Type`,
  `Game Environment`);
- the unimplemented-call list with counts.

Same values mean the same stop; different values are a lead. A run on the build **without** the
probe is the cleanest control, because it removes the instrumentation as a variable. (verified
method)

## How to run

```
xcl run <game folder> --
```

with `XCL_NETWORK=0` and a logged-in interactive desktop. SSH lands in session 0, where presentation
and GameInput fail for reasons unrelated to this stop, so a session-0 result is not comparable.
(verified)

## What we will publish

Logs with RVAs and register values, plus the probe output above. No game files, no hashes, no
package identity. (verified)
