# Run 48 — patience held for 300 polls, and the backlog was longer than that

**Verified from the run log** unless a line says **inferred**. Offsets are RVAs inside the title
image. This page corrects the run-48 sheet; the stop site is the rule-B abort described in
[STALL-DETECTOR.md](STALL-DETECTOR.md), and the compile numbers are read against
[THE-COMPILE-WALL.md](THE-COMPILE-WALL.md).

## Identity

| | |
|---|---|
| Runtime | `xcl-patience1.exe` — SHA-256 `6022975D8DE049B9CC530FF7C67034399A9C72DB1899776D8F2D198E0FAEA1A4` |
| Build | plain `47efa2e` plus the nine-site tolerance patch; static CRT |
| Patch | nine snapshots of the rule-B threshold, `30 -> 300` polls |
| Dump schedule | `XCL_THREAD_DUMP_AT=120,240,360,480`, `XCL_FRAME_DUMP=60` |
| Harness | `XCL_COHERENT_MB=2048`, `XCL_NETWORK=0`, `XCL_DSTORAGE_BCPACK_FILL=0`, warm `dxil634` |
| Window | last present `+146.773 s`; abort `+454.729 s`; guard `900.586 s`; dump path finished ~`+1 092 s` |
| Exit | `0xc0000005`, write to `0x0`, `+0x23CF683` — rule B, the title's own abort |

The patch line, verbatim:

```
xcl: watchdog tolerance: 9 sites, stall detector now 300 polls (was 30)
```

## What the patch bought

[STALL-DETECTOR.md](STALL-DETECTOR.md) establishes that rule B aborts when the heartbeat at
`[0x71B6338]` has not changed for **N** polls of a 1 Hz thread, and that the abort site is the
deliberate write to address 0 at `+0x23CF683`. Run 48 set N to 300.

| | Value |
|---|---|
| Last present written | `+146.773 s` |
| Abort | `+454.729 s` |
| Gap | **307.96 s** |

307.96 s is 300 polls at 1 Hz plus the poll in flight. Same site, same write, same registers as the
earlier 30-poll runs; only the delay moved, and it moved by the factor the patch chose. **The patch
works, and rule B is confirmed as the stop from timing alone** — no decode was needed this time.
Previous runs aborted 29–42 s after the last present; this one aborted at 308 s.

## The compile backlog, counted correctly

449 log lines carry `compute PSO with the translated console CS`. They are not 449 compiles:

| Kind | Count | Timing |
|---|---|---|
| Real DXC compiles | **239** | median **77 ms**; 10 over 10 s; **6** in 15–23 s |
| Replayed from driver blobs on disk | **210** | single-digit ms, fastest cited **41 ms** (211 716 B DXIL) |

So two corrections. "449 compiles" is log lines, not work. "15–23 s each" describes **6 of 239** —
the heavy tail, 0.34–0.43 MB of translated DXIL.

**Four-way concurrency is not established here.** [THE-COMPILE-WALL.md](THE-COMPILE-WALL.md)
measured four in flight with `XCL_PSO_TIMING=1`; that instrument was off in run 48. The three thread
dumps show 31 `xcl-dxc` workers with at most one inside `dxcompiler.dll` at each snapshot, and the
eight heavy compiles completed about 20 s apart — one per duration, i.e. serial. "Four at a time"
belongs to run 46, not to this run.

The backlog was already visible in the picture rate: **12.5 fps over the final 300 frames**, against
30.0 fps in every earlier window. The frame loop was losing time to compiles well before the
heartbeat froze.

## The budget was ten times too small

The compile tail is the whole story: eight compiles of 15–23.5 s, serial, is roughly **2.5–3 minutes**
of a blocked frame loop — and they were still arriving when the 300th poll elapsed. Raising N again
buys a longer stall, not a shorter compile. **Patience works mechanically and is not sufficient**
(reading of the timing, inferred).

Frame totals do not measure this run:

| Run | Build | Last frame | Fps, last window |
|---|---|---|---|
| 43 | `xcl-culprit2` | **6 860** | 30.0 |
| 48 | `xcl-patience1` | **3 320** | 12.5 |

3 320 is below 6 860, and the tolerance patch is not the cause. Run 48's burst landed around
`+140 s`; run 43 got four minutes of moving art first. **Frame counts across runs track when the
compile burst arrives, not how long the run survived.**

## Corrections to the run sheet

1. **878 s wall** — no log supports it. The guard fired at `900.586 s`; the dump path ran to ~`1 092 s`.
2. **731 s of silence** — it was **307.96 s**. 731 s is not in the log in any form.
3. **449 compiles at 15–23 s** — 239 compiles plus 210 blob replays, 6 of them in that band.
4. **four at a time** — unverified in this run (see above).
5. **480 s thread dump** — never taken; the abort came at `+454.7 s`.
6. **`report.json` and exit code** — both existed and were lost to a later run that reused the same
   output directory 22 s after this one finished. Held in the lab archive, not here.

## Reproduce

1. Build `xcl-patience1.exe` from plain `47efa2e` plus the nine-site `30 -> 300` patch; static CRT.
2. Warm `dxil634`, then set `XCL_FRAME_DUMP=60`, `XCL_THREAD_DUMP_AT=120,240,360,480`, and the usual
   harness variables. Do **not** set `XCL_PSO_TIMING` unless you want the concurrency number too.
3. Expect presents to stop near `+150 s`, ~240 compiles and ~210 blob replays to follow, and the abort
   at `+455 s`.

## What it means

The stop is the title's own, and its timing is now a measured number rather than an inference.
Patience turned a 30 s diagnostic window into a 308 s one and proved the compile backlog is what
sits in the frame loop's way. The next experiment should attack the heavy tail — eight console
compute programs of a third of a megabyte each — not the threshold.
