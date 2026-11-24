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

## The blocker

- The stop is `0xC0000005`, a write to `0x0`, at the title's RVA **`+0x23CF683`**, with the stack
  `+0x4E4BA89`, `+0x2215C35`, `+0x71B9A20`, `+0x2215ECA` and `GPU Loc Sig` = `7333478d`.
  (verified)
- **Same stop, same stack, same signature** as ivall/xcl#505, an independent report on the same
  title build. (verified by comparison)
- The abort arrives about 40 s after the last captured frame, while streaming is live and no
  logged call has returned an error. That is consistent with a progress watchdog whose expected
  counter never advances. The watchdog reading is **#505's**, not decoded here. (reported)
- **Resolved (inferred, strong):** the registry assert was a consequence of the torn archive, not a
  missing subsystem. With the archive intact the lookup is never reached. The run-5 advice to
  instrument that lookup is withdrawn for this stop. One boot so far; a repeat without the probe is
  still owed.
- **Not a visible render.** The simulation was running in the world — pools populated, player on the
  ground, a script thread live — but every captured frame is still the loading spinner. No frame
  shows the world, and nothing here is interactive play.

## Three named leads

1. **The GPU-progress watchdog.** Something in the title polls for progress and gives up ~30 s into
   a stage that is not advancing. What would settle it: the name or address of the counter, or any
   HLE path that services a periodic progress query for a submitted command list. A `Game State`
   value at the abort would be ideal — #505 reported one, our parameter list does not carry it, so
   we cannot yet say which stage the title thought it was in. (hypothesis + reported)
2. **`ID3D12GraphicsCommandList10::#101`.** #505 names this call returning 0 as the suspected trigger.
   It does **not** appear in our unimplemented-call list, so we cannot confirm it from our own log.
   (reported) Our own per-frame unknowns are `#126`, `#142` and `#75` at exactly 1 111 calls each —
   one set per presented frame — and `ID3D12CommandAllocator::#9` at 1 116. They are the obvious
   candidates for a console-specific call whose absence stalls forward progress. Mapping any of
   these four slots to its console operation would be a real contribution. (verified counts)
3. **The BCPack decoder.** Streaming is on the critical path and 512 requests were refused at the
   stop: BC1 348, BC5 107, BC6H 1, BC7 56, plus a DirectStorage request failure. Until those blocks
   decode, a whole class of textures never arrives, and a stage could be waiting on them. A
   CPU-side BC1/BC5/BC6H/BC7 path, or a documented mapping from BCPack to the PC block formats, moves
   this without needing the title. (verified counts)

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
