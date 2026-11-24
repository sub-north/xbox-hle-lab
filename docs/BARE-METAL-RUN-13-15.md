# Runs 13 and 15: key art at 8–19 presents/s, then a deterministic BVH address wall

**Verified** unless labelled **measured**, **inferred** or **reported**. Counts are from each run's log;
frames from the run's own dump set (every tenth present, converted to PNG); the address and the stop
from the run log and the exception record. No shipped file is named or hashed here: the title image is
`<title image>`, the runtime is `xcl`. Offsets, addresses and message text are exact.

Run numbers here are the bare-metal series that [BARE-METAL-RUN-7.md](BARE-METAL-RUN-7.md) belongs to.
They are unrelated to the staging series in [RUNS-15-30.md](RUNS-15-30.md), whose run 15 is a different
boot under a different build.

Both runs are **negative results on the freeze**. Neither reached it: each died on an acceleration-structure
address check first. Nothing here bears on the long-stall question.

## Run 13 — bounded compute wait, new upstream build

The runtime is the upstream PR #533 build at `f849ecc` (parent `7ebf1b8`), static CRT, everything else
unchanged from the previous bed: separate Windows install, payload staged read-only, network adapters
down, 2 048 MB coherent window, `XCL_FRAME_DUMP=10`, 15-minute cap.

| | value |
|---|---|
| duration | 437 s |
| last present | frame 2169 at **+258.749 s** |
| presents/s | **~8.6** over the run |
| draws at the last counter line | 19 685, **0 skipped**, 0 with PC pipelines |
| dispatches at the last counter line | 2 048, **0 skipped** |
| coherent window | 1 851 / 2 048 MB, sparse 7 445 MB, 205 blocks / 205 allocators |
| stop | `XDXR` at +281.7 s, `0xe06d7363` at **+282.051 s** |

Zero skipped draws and zero skipped dispatches matter here: the run is fast while doing all the work it
was asked to do, not fast by dropping it. (measured)

### Frame sequence

| frames | +s | what is on screen |
|---|---|---|
| 9–199 | 6.8–23.5 | black |
| 209–739 | 24.1–41.7 | account picker |
| 749–779 | 43.3–48.0 | five-star loading row |
| 789 | 49.6 | single star flare |
| 799–1059 | 51.2–87.9 | key-art montage, ~0.33 s cuts |
| 1069–1189 | 89.2–105.6 | publisher logo card |
| 1209–2169 | 106.6–258.7 | character key art: fade in, hold, fade out |
| 2169 | 258.7 | black (last present) |

Every row is read from the image. Two absences are as important as the rows:

- **No legal screen.** The candidate windows for one — the first lit frames, the loader, the logo card —
  show background, picker, loader and logo only. No copyright or legal-text frame appears in 217 dumps.
- **No world frame, at any point.** Every surviving frame is letterboxed artwork, a flat cut or black:
  no sky, terrain, city or HUD. The title reached its front-end carousel and sat there for 4.5 minutes
  without drawing gameplay. (verified from frames; the interpretation is **inferred**)

### The frame loop still stalls first

The speed-up is not a cure. Log gaps in the final stretch show the loop dropping to a crawl before the
crash: **frames 2029–2089 run at 0.35–1.7 fps**, an 80 s stretch in which the run was alive but barely
presenting. Then 23 s pass with no dump at all, and the BVH error arrives. (measured)

## Run 15 — the same, plus a sub-2^40 reservation ceiling

Same build lineage with one patch: write-watched GPU reservations are constrained to end below 2^40 so
their views stay inside the 40-bit address space the check demands.

| | run 13 (no ceiling) | run 15 (ceiling) |
|---|---|---|
| duration | 437 s | 283 s |
| last present | 2169 | 1759 |
| stop | `0xe06d7363`, `XDXR … more than 40b` | **identical** |

The ceiling **worked where it was applied**:

```
mem: write-watched reservations end below 0xfa00000000 (2^40 - 24 GiB), so their 24 GiB GPU views stay below 0x10000000000
kernelx: reservation 0xee00000000 (0x600000000 bytes, no CPU access) is the GPU view of 0xf400000000
```

It **failed where it mattered**:

```
mem: nothing below 0xfa00000000 for 0x89700000 write-watched bytes; falling back to any address
```

Read the two lines together. The 24 GiB reservation landed below `0xfa00000000` and its GPU view at
`0xf400000000` is genuinely under 2^40. But a **2.3 GB (`0x89700000`)** write-watched reservation found no
room below the ceiling and **silently fell back to any address** — above 2^40. That allocation is very
likely the acceleration-structure memory, so the destination the check rejects was still rejected, and
the run stopped at the same point in its progress. (verified lines; the identification of the fallback
allocation as the AS memory is **inferred** from size and timing)

The lesson is not about the arithmetic, which is right. It is that eight fixed hint bases are not a
search, and that a fallback which logs one line and proceeds hides the failure for a whole run. The next
step is a real placement search (progressively lowered ceiling under `VirtualAlloc2`, or a bounded
descending scan) and a loud failure instead of a silent fallback.

## The wall, named precisely

```
D3D12 XDXR Error: DestAccelerationStructureData can not have more than 40b of address set
```

then a C++ exception and **unhandled exception `0xe06d7363`**, stack `xboxbvhbuilder.dll+0xd2538` — the
console BVH builder, not the runtime's presentation or compute path. The builder's own guard is

```
cmp r8,0x10000000000
jae <throw>
```

a compare of the destination against 2^40 followed by a throw. The runtime's only `XDXR` log site is its
AS-build path, so the first observable build is the failing one: the run logs AS *descriptors* marked
"writing empty BVH descriptor" at +135.7 s and +259.7 s, and no successful build line at any point.
(verified from the log and the exception record; "first observable build" is **inferred**)

Two builds from two lineages — `7ebf1b8` and the PR #533 head, before and after the ceiling patch —
stop at the same check at **~283 s of progress**. That is a deterministic wall, not a race, and it is
why neither run tests the freeze.

## The attribution correction

The first reading of run 13 was that the bounded compute wait (`XCL_COMPUTE_PSO_WAIT_MS`, default
4 000 ms) bought the speed-up: a timeout is a stand-in, a stand-in removes a wait, and run 13 is much
faster than its predecessors. That reading is **wrong**, and three independent facts refute it:

1. **The timeouts all fire after the last present.** All six bounded-wait events occur at t ≥ 265.9 s.
   The last present is frame 2169 at 258.7 s. At most a 24 s ceiling of effect survives, against ~774 s
   of progress won. The events cannot have caused frames that preceded them.
2. **The rate gap exists in a window with zero such events.** Over frames 9→599 — a stretch containing
   no bounded-wait event at all — run 13 runs at **19.53 presents/s** against run 12's **2.11**: a
   **9.3×** gap. Equivalently, the first timeout's present index is ≥ 2169.
3. **A different lever appears only on the newer lineage.** `title textures: N uploads left out` is
   logged by run 13 and by none of runs 6–12 — 4 096 skipped uploads by t = 136 s, a per-frame cost the
   older builds did not carry in that form.

So the honest statement is: **run 13 is faster, and the bounded compute wait is not why.** The remaining
candidates are the texture-upload path and the PR #533 build as a whole. Neither is claimed. The control
that would separate them — the same build with `XCL_COMPUTE_PSO_WAIT_MS=0` — has not produced a valid
run: the first attempt collided with a live launch after ~5 s and was discarded. (verified facts;
attribution **open**)

The stall also survives: 0.35–1.7 fps over frames 2029–2089 is the old frame-loop halt in a new place,
and it is the reason this pair of runs says nothing about the freeze.

## What this rules out for the wall

- **`XCL_RAYTRACING` cannot suppress the build.** The knob only rewrites the reported tier:
  `config.rs:244-246` parses it, and the sole consumer, `real.rs:2551-2552`, is reached only from
  `adjust_raytracing_options5` after a successful `CheckFeatureSupport(D3D12_OPTIONS5)`. Both runs
  contain **zero** `OPTIONS5 → RaytracingTier` lines, so the title never queries the tier and the
  override has nothing to act on. `CreateStateObject` is not gated by it either, and no HLE build entry
  exists: builds arrive through `XboxBvhBuilder.dll` and console command-list slots 146/148. Setting the
  tier to 0 therefore predicts the same `XDXR` and the same `0xe06d7363`. (verified by decode and by
  absence in both logs)
- **Skipped work.** Not the source of the speed: 0 skipped draws, 0 skipped dispatches. (measured)
- **Presentation.** Not the stop: the last present succeeds, teardown to black succeeds, and the throw is
  in the BVH builder. (verified)

- **Ruled out for the wall, not for the freeze.** The ceiling patch moved the reservations it covered and
  changed nothing else. What remains is the destination address of one allocation.

## Honest limits

- **Two arms, not a sweep.** Run 15 differs from run 13 by the ceiling patch *and* by whatever a different
  boot does on its own. The shared stop point is the agreement that carries weight; the present counts are
  not comparable as a performance result.
- **The frame-set provenance is patched by replay.** The shared dump folder was overwritten while run 13
  was archived; frames 9–1759 were re-dumped by run 15, and their histograms match run 13's exactly. Frames
  archived after 2169 belong to an earlier run and were ignored. The sequence above is taken from the
  run-13-time conversions, never from the overwrite.
- **"Acceleration-structure memory" is an inference.** The fallback line carries a size, not a purpose.
- **No crash log exists for either run.** The exception record and the log's last lines are the whole
  record; a 0-byte dump was produced at 02:00:58 during run 13.
- **One clock.** "~283 s of progress" is wall clock from launch on both arms, not a frame count.

## Next

A placement search that cannot silently escape the ceiling, and a loud failure when it would. Then the
same bed rerun: if the destination lands below 2^40, the build proceeds and the run either reaches the
freeze — which finally tests it — or stops somewhere new. The attribution control (`XCL_COMPUTE_PSO_WAIT_MS=0`)
stays outstanding and should be run on the same binary as the winning placement arm, so one boot answers
both questions.
