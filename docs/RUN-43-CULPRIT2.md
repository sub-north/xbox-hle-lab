# Run 43 — 6 860 frames of real art, and an instrument that never saw the GPU

Private lab record. `xcl-culprit2.exe` was built to answer one question: *which compute dispatch
never retires on this host?* It rendered the best picture this title has produced under `xcl` — and
its answering mechanism did not work at all. Both halves are recorded here, because the second
half decides how much of the first can be trusted.

## Identity

| | |
|---|---|
| Runtime | `xcl-culprit2.exe`, 7 390 208 B — SHA-256 `B3545AD9D4C809A86388728E7A6775E6E959B69989D56456C809AC9C6DE340A0` |
| Build | upstream `47efa2e` plus the GPU-culprit patch series; static CRT, release |
| Instrument on | `XCL_GPU_CULPRIT=1` — a 1 024-entry marker ring (`seq, phase, hash, gx, gy, gz`) in unified memory |
| Dump schedule | `XCL_THREAD_DUMP_AT=60,150,240,330,420`, `XCL_FRAME_DUMP=60` |
| Harness | `XCL_COHERENT_MB=2048`, `XCL_NETWORK=0`, `XCL_DSTORAGE_BCPACK_FILL=0`, warm `XCL_DXIL_CACHE` |
| Window | 306.58 s total; last present at `+264.28 s`; 42 s of present silence before the fault |
| Exit | `0xc0000005`, title's own trap at `+0x23cf683` (write to `0x0`); no kill needed |

## Metrics

| Metric | Value |
|---|---|
| Presents | **6 887** |
| Last presented frame | **6 860**, 3840×2160, format 28, 518 082 / 518 400 non-dark |
| Rate | **30.0 fps** in 19 of 21 measurement windows; 29.1 / 29.9 / 29.3 / 30.0 in the last four |
| Frame dumps (in window) | **115** — 3 black, **112 non-black** |
| Distinct images | **25** of 115: 3 identical black, **89 identical stills** (frames 200–5 480), then **22 distinct frames** (5 540–6 860) |
| Pipelines / shaders | 1 131 translated pipelines, 19 251 unique shaders, 274 823 submissions, **0** XDXR errors |
| Console counters | **19** cumulative lines; last one `82 893 draws with translated pipelines (0 skipped, 0 with PC pipelines), 8 192 dispatches (0 skipped)` |
| Waits | 8 `Wait64BitValueX`, all bridged to the GPU queue, 0 timeouts |
| Stop | `unhandled exception 0xc0000005 … +0x23cf683 (write at address 0x0)`, stack `+0x4e4ba89, +0x2215c35, +0x71b9a20, +0x2215eca` |
| Crash context | present, mtime inside the window: `Game State: Game Running`, `Frame: 6295`, BUILDING 8 521/22 500, PED 86/600, VEHICLE 237/700, Player Health 130/130 |

The art, honestly described: black for the first three dumps (frames 20–140), one **byte-identical
still** for 89 consecutive dumps (frames 200–5 480, dominant texel 516 897 counts — a flat
near-solid image), then a genuine picture from frame 5 540 onward, where the dominant colour falls to
~900 counts and every dump differs from the last. The last 22 dumps are the run's real output;
the previous 89 are a frozen splash.

## Frame counts: a number is only as good as what it counted

| Run | Build | Last frame | What was on screen | Reading |
|---|---|---|---|---|
| **43** | `xcl-culprit2` | **6 860** | changing art for the last 22 dumps | **best run recorded** |
| 35 | plain `47efa2e` | 3 059 | one still splash only | −3 801 frames vs run 43 |
| 40 | `xcl-culprit1` | 9 200 | **every one of 154 dumps fully black** | **meaningless** |

Run 43 beats the previous best (run 35) by **+3 801 frames, +124 %**, and unlike run 35 its picture
moves.

**Run 40's 9 200 must not be read as better than 6 860.** That run's log contains no draw or
dispatch accounting at all — the console draw path never reported a single draw — and all 154 of
its dumps read `0 of 518 400 samples not black`. It cleared and presented blank frames at ~26/s
until it hit the same trap. A counter that ticks on empty clears measures how long the title waited
before dying, not how much it rendered. Run 40 is a null result with a big number attached.

## The instrument failed

Every dump, including the one taken while the title was at a locked 30 fps and dumping 99.9 %
non-black frames, reports the same thing:

```
GPU culprit: trigger=XCL_THREAD_DUMP_AT: 0 of the last 1024 recorded dispatches got a GPU marker
GPU culprit: no unfinished dispatch among the markers (none of them reached the GPU, or unified
             memory is not host-coherent here)
```

Fired at `+60.08 s`, `+150.06 s` and `+240.10 s` on schedule; again at `+300.34 s`
(`unhandled exception`) and `+301.45 s` (`NtTerminateProcess`). Every entry in the newest-8 list
reads `unreached`. **Not one of 1 024 slots was ever written back by the GPU.**

The 60 s dump is the proof: at that moment the run was healthy — 30.0 fps in the window, frames
pouring out, 3 black dumps followed by 89 clean ones. A title that was not completing dispatches
could not be presenting like that. So the fault is in the marker path, not in the title.

**Therefore: no dispatch conclusion may be drawn from this run — not one.** "0 of 1 024 marked" is
not evidence that 1 024 dispatches failed to complete. It is evidence that the marks never came back
to the host. Any statement of the form "the GPU wedges on dispatch N" from this run is unsupported.

What the ring *did* record is host-side and real: the sequence counter reached **14 464** by the
crash, so dispatches were being recorded continuously at a healthy rate to the end. The ring is an
issuance log, not a completion log, on this host.

Fix direction, not attempted: the instrument needs a coherent mapping it does not have. Either read
the ring through a copy plus fence (a readback buffer the host reads after a completed queue
signal), or debug the `WriteBufferImmediate` slot-66 path. Until one of those works,
`XCL_GPU_CULPRIT` should be treated as **unvalidated** and its output as absent.

## What the ring did capture, and what it cannot mean

The ring records the console compute-shader hash and the dispatch group count for every dispatch it
issues. During the steady state (60–240 s) the newest-8 window shows a repeating **three-dispatch
cycle**, all 32-wide groups, over 5×5×33 groups elsewhere; by the crash the mix had widened to
include a 40×23×1 and a 1×1×1. The full hashes are withheld here by posting policy (they are
derived from shipped shader content and are identification material); they are held in the lab
record with the run archive.

What this pattern supports: three compute pipelines, 32-thread-wide, were submitted in rotation for
the whole steady state — consistent with the wave-of-32 halving the runtime warns about, and worth
following once a working completion probe exists.

What it does **not** support: that any of those dispatches finished, stalled, or was the culprit.
Issuance order is not completion order, and this run never observed completion.

## Corrections to the run sheet

1. `112 non-black / 3 black` holds, but "real rendered art" overstates it — 89 of the 112 are the
   same still image. Real distinct art is the **last 22 dumps**.
2. The dump schedule fired at **60, 150 and 240 s only**. 330 s and 420 s never arrived: the run
   ended at 306.58 s. The final two dumps are crash-triggered, not scheduled.
3. Run 40's 9 200 is not a comparable result (see above).
4. Everything else on the run sheet checked out against the log and `report.json`: binary hash
   prefix `b3545ad9`, last frame 6 860, 19 console counter lines, `0xc0000005` at `+0x23cf683`,
   warm `dxil634` cache, four shader hashes.

## Reproduce

1. Build `xcl-culprit2.exe` from the culprit patch series over `47efa2e`; confirm 7 390 208 B and
   no `VCRUNTIME140`/`MSVCP140` imports.
2. Warm the DXIL cache directory, then set only: `XCL_GPU_CULPRIT=1`, `XCL_FRAME_DUMP=60`,
   `XCL_THREAD_DUMP_AT=60,150,240`, plus the usual harness variables.
3. Expect ~100 s of a frozen still, then changing art from ~`+200 s`, then the trap at
   `+0x23cf683` around `+300 s`.
4. Expect every culprit dump to read `0 of the last 1024 … got a GPU marker`. That is the current
   state of the instrument, not a finding about the title. Do not report it as one.

## What it means

**Rendering is ahead of instrumentation.** The frame counter, the pipeline count and the dumps all
show a title that draws real, changing 4K art at 30 fps with the simulation loaded behind it —
8 521 buildings, 86 peds, 237 cars — for nearly four and a half minutes. The probe written to
explain the stop explains nothing on this host, and its headline number must not enter the record as
a result. Judge future work against 6 860 frames with moving content; before the next culprit run,
repair the marker path or replace it.
