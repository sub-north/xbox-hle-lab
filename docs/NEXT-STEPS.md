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
- The abort is **scope start + 300 s**, not a fixed wall clock. A per-thread stack of QPC scopes is
  compared against `300.0f` at RVA `0x6159E20`; any scope open 300 s fires the trap at `+0x23CF683`,
  with no GPU signal in the predicate. Run 6 aborted at +442 s (scope ~+142 s), run 7 at +510 s
  (scope ~+210 s), which is why the two gaps to the last frame differ. The timer is **verified** in
  the decode; which scope stalls, and therefore whether it waits on GPU progress, is still open. The
  GPU-progress reading is **#505's**. (verified mechanism, inferred scope identity)
- **Resolved (inferred, strong):** the registry assert was a consequence of the torn archive, not a
  missing subsystem. With the archive intact the lookup is never reached. The run-5 advice to
  instrument that lookup is withdrawn for this stop. One boot so far; a repeat without the probe is
  still owed.
- **Not a visible render.** The simulation was running in the world — pools populated, player on the
  ground, a script thread live — but every captured frame is still the loading spinner. No frame
  shows the world, and nothing here is interactive play.

## Three named leads

1. **Which scope stalls, and for what.** The 300 s deadline is a CPU QPC scope timer (#505's
   "GPU-progress watchdog" is the reading of *why* the scope stays open, not what the timer is). What
   would settle it: a log of the scope table (RVA `0x6844A20`: `+4` open count, `+8` QPC stamps) at
   the abort, or any HLE path that services a periodic progress query for a submitted command list. A
   `Game State` value at the abort would help too — #505 reported one, our parameter list does not
   carry it. (verified decode + hypothesis)
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
