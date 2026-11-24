# Pipeline demand

**Verified from the run logs and the runtime source**, unless a line says **inferred**. This page
corrects a reading that has circulated — including in this lab's own notes — that the overlay's
`queued` count is a backlog of pending work. It is not. The demand is also smaller and more
repetitive than the count suggests, and the reason a run does not finish the load is arithmetic, not
patience.

## 1. What the numbers mean

The overlay is the runtime's, not the title's: `compiling shaders: {running} running, {done} done,
{queued} queued`, pushed through `SetWindowTextW` (`present.rs:401-411`).

| Field | Source | Meaning |
|---|---|---|
| `running` | `COMPILES[0]` (`dxc.rs:834`) | DXC compiles started and not yet returned |
| `done` | `COMPILES[1]` (`dxc.rs:834`) | DXC compiles that have **returned** — success or failure, before any PSO exists |
| `queued` | `pso_async::counts().2` (`pso_async.rs:479`) | `PENDING` entries not yet marked done: **lazy stand-ins** handed to the title in place of a pipeline that is still being built |

The stand-ins let the title keep submitting work against a pipeline object before that pipeline
exists; each is resolved when its real object arrives. They are not jobs waiting for a worker, they
are not drained in order, and their count is not remaining work.

So the reading

```
0 running, 34 done, 57 560 queued
```

means: **the compiler is idle, 34 compiles have returned, and 57 560 pipeline objects are still
represented by stand-ins.** It does not mean 57 560 jobs are pending. `done` is cumulative within the
process, so 34 is where a run stood when the reading was taken and it keeps climbing from there. The
arithmetic rules the backlog reading out on its own: at the measured tail rate of 3 compiles per minute
(§3), 57 560 compiles would take **13 days**, and the run aborts 455 s in.

## 2. The measured demand

Fresh compiles per run, counted from the per-PSO lines rather than from log lines: **221, 211, 239**
over three consecutive runs. Flat — and the last one is the highest. Meanwhile only **16–31** driver
blobs persist per run (the counter prints at powers of two, so `16` means at least 16 and fewer than
32).

That combination is the finding: **each run redoes the same ~200 compiles, and the cache frontier does
not advance.** A cache that banked 200 compiles per run would show fresh counts falling toward zero
within a few runs. They do not fall.

Why the frontier stalls (inferred): of the ~239 compiles a run returns, only **16–31** are banked. A
blob is stored only when the compile passed the gates — at least 250 ms inside the driver, at least
16 KiB of DXIL, an adopted PSO, and a blob actually returned — and the longest compiles, the ones worth
keeping, are still arriving when the run is aborted. The work is paid for and not kept.

## 3. The serialisation

With `XCL_PSO_TIMING=1`, the maximum number of compiles in flight per 60 s window:

| Window (s) | In-flight max |
|---|---|
| 0–60 | 12 |
| 60–120 | 1 |
| 120–180 | 7 |
| 180–240 | 1 |
| 240–300 | 1 |
| 300–360 | 1 |
| 360–420 | 1 |
| 420–455 | 1 |

Concurrency exists during startup and nowhere else. **From 180 s onward the maximum is 1**: one
blocking compile at a time, each costing about **20 s** (≈16.5 s inside DXC plus ≈3.5 s of translation
and queueing). That is **≈3 compiles per minute**.

Two hundred compiles at 3/min is **≈67 minutes**. The tolerance patch in the measured run allowed
300 s — five minutes, about 15 compiles — and the run aborted at 455 s. **Raising the stall tolerance
moves the abort; it does not move the frontier.**

## 4. The rate curve

Compile-PSO lines per 60 s window, split into fresh compiles and driver-blob replays:

| Window (s) | All/min | Fresh | Blob replay |
|---|---|---|---|
| 0–60 | 290 | 147 | 143 |
| 60–120 | 2 | 2 | 0 |
| 120–180 | 137 | 72 | 65 |
| 180–240 | 9 | 8 | 1 |
| 240–300 | 5 | 4 | 1 |
| 300–360 | 2 | 2 | 0 |
| 360–420 | 3 | 3 | 0 |
| 420–455 | 1 | 1 | 0 |

Fresh totals 239, replays 210 — the 449 per-PSO lines of the run. The busy windows run **290 → 137 → 9
→ 5 → 2 → 1** per minute: it falls and never rises. The
60–120 s lull is requests stopping while the title still renders, not the work finishing. Almost all
blob replay is over by 180 s (143 + 65 of 210); from 300 s on, every request is a fresh compile of
15–23 s.

## 5. Therefore

The levers with the right magnitude are two, and neither is patience.

**(a) Persist more compiled blobs.** A replay is single-digit milliseconds against ~20 s for a
blocking compile. Replace 200 × 20 s with 200 × 5 ms and the same load costs **about one second
instead of 67 minutes**. This is the only lever that changes the order of magnitude. It needs the run
to reach the heavy compiles and bank them, which is what §2 says is not happening.

**(b) Let more compiles run at once.** At one in flight, workers are irrelevant. Sixteen in flight at
~20 s each puts 200 compiles in **≈250 s**, inside a window the run already survives. Thread count
alone does not buy this: the runtime's admission gate is pagefile-based (6 GB reserve plus ~1 GB per
running compile), so stock admits roughly four regardless of how many threads exist.

**The arithmetic of patience.** 200 compiles at 3/min is 67 minutes. One hour buys about 180 — still
short. A 300 s tolerance buys 15. There is no threshold that patience can cross, and no taper to
outlast, because the curve only falls (§4). Waiting longer makes the abort later, not the load
shorter.

## 6. Reproduce

**Environment.** Warm DXIL cache (`XCL_DXIL_CACHE`), `XCL_COHERENT_MB=2048`, `XCL_NETWORK=0`,
`XCL_DSTORAGE_BCPACK_FILL=0`, `XCL_FRAME_DUMP=60`. Add `XCL_PSO_TIMING=1` for the concurrency numbers;
add `XCL_THREAD_DUMP_AT=120,240,360,480` to snapshot the worker pools.

**The two log lines.** Per compile:

```
D3D12.X: compute PSO with the translated console CS (<N> B DXIL, <M> ms)
```

`M` is the compile cost; a line ending `…, the driver's blob of an earlier build` is a replay, not a
compile. Per stored blob, printed at power-of-two thresholds:

```
D3D12.X: <N> driver blobs kept on disk for the next run (this one: <ms>, <KB>)
```

Count fresh lines and replay lines per 60 s window, and the kept-blob line gives the persist count.

**Caveat.** In-flight counts exist only with `XCL_PSO_TIMING=1`, which emits `PSO timing: <N>
submitted, <M> dequeued, <K> finished` and `<N> compile(s) in flight`. With the instrument off there is
no concurrency record at all — a log that lacks it is not evidence of serial execution. The
serialisation in §3 is from a run with the instrument on; frame totals on other runs track *when* the
compile burst arrives, not how long the run survived.

**Cross-check.** Three consecutive runs, the same build family, warm cache: 221 / 211 / 239 fresh
compiles and 16–31 blobs kept. If a build makes the fresh count fall, the cache is advancing; if it
stays near 200, only the clock moved.
