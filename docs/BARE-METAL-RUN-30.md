# Run 30: 43 sync waits are outstanding when the watchdog aborts at `+0x23CF683`

**Verified** unless labelled **inferred**. Every count, frame number, handle value and offset below is
read from this run's own logs (`stderr.log`, its timed twin, `report.json`, the exception record and the
crash context). No shipped file is named or hashed here: the title image is `<title image>`, the package
is `<package>`, the runtime is `xcl`. The instrumentation is ours, and its hash is published.

This run asked one question that the earlier runs could not answer: *at the moment the title stops
presenting and the watchdog fires, what are its threads actually waiting on?* The previous records could
only show stacks. This build logs every kernel synchronisation operation it serves, so it can name the
handle, the kind, the semaphore count, who created it, and how long the waiter had been parked.

**This is a diagnostic build.** It traces every `CreateEvent`/`CreateSemaphore`/wait/set, which costs
time and inflates the sync bookkeeping. Nothing in this record is a performance result: the durations,
the present cadence and the stall lengths below must not be compared with an uninstrumented run of either
build.

## Which run this is

The fastlane session stamped **2026-11-24 04:04:42Z**. `RUNS-15-30.md` also has a row numbered 30 — that
row is a different session (serving a valid count-0 RPF9 from the overlay), and this document does not
change it. The numbering here is the fastlane session number used by the operator that dispatched this
run.

## The change under test

| Variable | Value |
|---|---|
| `XCL_SYNC_TRACE` | `1` — log every sync create/wait/set, and the stuck table at the dump |
| `XCL_SYNC_SLOW_MS` | `100` — a wait longer than this becomes a `SYNC slow` line |
| `XCL_FRAME_DUMP` | `20` — every twentieth present is written out |
| `XCL_THREAD_DUMP_AT` | `280,300,400` — **never reached**; the run ended at +149 s |
| `XCL_DXIL_CACHE` | warm cache directory, `dxil634` |

Build: `xcl-sync2.exe`, 6 793 728 B, SHA-256
`F272294FB7951D958A24B07DA3A6606480DC8D6D1FE2168431B5A65192544E8A`. Launcher `run-fastlane-fixed.ps1`
(`fastlane-guard-1`).

## The run

| | value |
|---|---|
| duration | **149.2 s** wall; exception at **+145.874 s** |
| last presented frame | **1 839** at **+111.547 s** — fully covered (518 400 of 518 400 samples non-black) |
| presents, measured | 92 dumped frames, every twentieth, frames 19 → 1 839 |
| present rate | **18.4 /s** average over the dumped span; the first 100 frames are cheap clears at ~28 /s |
| first non-black frame | **159** at **+34.955 s**; frames 19–139 were 0 of 518 400 |
| gaps over 10 s | **two**: **17.06 s** (frames 119 → 139) and **10.36 s** (1 779 → 1 799) |
| terminal silence | **34.3 s** with no presented frame before the abort |
| draws / dispatches | 17 770 draws, 8 192 dispatches, **0 skipped** |
| audio | 16 851 Opus packets, 16 137 024 frames, 0 errors — still decoding at the abort |
| stop | `0xc0000005` at `<title image>+0x23CF683` — write to address `0x0`; exit `0xC0000005` |

Two facts about the sequence. Presents were healthy — 18.4/s with two stalls that both resumed — and the
image was complete, not a black clear, on every dumped frame from 159 onward. The title did not fade out:
presents stopped at +111.5 s while the audio and shader threads kept working (16 851 audio packets, and a
903 000 B DXIL compute pipeline compiling for 18 089 ms as the last line before the exception). Then the
null write.

## What the sync layer was holding

At the dump the trace reports its own total: `sync trace: 43 wait(s) outstanding at the dump`. Those 43
waits sit on **40 distinct handles** — three handles are waited on by two threads each. Ordered by how long
the waiter had been parked:

| # | handle | kind | `signals=` | `waited` | creator tid | created via |
|---|---|---|---|---|---|---|
| 1 | `0xEB0` | semaphore | 6 | **144 047 ms** | 28296 | `CreateSemaphoreExW` |
| 2 | `0xF78` | semaphore | 193 | 134 844 ms | 36572 | `CreateSemaphoreExW` |
| 3 | `0x1FD8` | semaphore | 1 | 134 109 ms | 42364 | `CreateSemaphoreExW` |

Three more waits tie at 134 109 ms: `0xA28` (event, `signals=0`, creator 28296), `0x1F24` (event,
`signals=0`, creator 42364) and `0x206C` (semaphore, `signals=0`, creator 34436). Below them,
`0x1DD0` (semaphore, `signals=1`, creator 11180) at 81 719 ms and `0x15E0` (event, `signals=88`,
creator 1512) at 57 859 ms.

The shape matters more than the individual handles:

- **22 waits are on auto-reset events, 21 on semaphores.** Almost all of the long ones sit at
  `signals=0` or `1`: narrow, single-wake objects, not broadcast gates.
- **One thread created 22 of the 43 waited-on handles.** Creator tid **28296** owns more than half of
  them; 42364 owns 6, 14240 owns 5, 36572 and 23216 own 3 each. This is a fan-in on one thread's object
  set, not a wide deadlock.
- **The waits outlast the silence.** 134–144 s of waiting against 34.3 s of no presents means these
  threads were already parked long before the screen stopped updating, and they did not wake when the
  presents stopped.
- **The aborting thread was itself waiting.** Thread 24440 — the one that took the exception — has a
  recorded wait on `0xD98` (semaphore, `signals=1`, creator 28296) **1 000 ms** before the write.

## What this establishes

The stop is the same null write at the same offset as the earlier tolerance run, and the exception record
shows the same aborting thread family, so this run does not discover a new failure mode; what it adds is
the state of the synchronisation layer at the instant of the stop. 43 waits were outstanding across 40
kernel handles, three of them for 134–144 s, all but a handful on narrow semaphores and auto-reset events
that one title thread (28296) created — and the thread that performed the write had itself been parked on
one of that thread's semaphores a second earlier. That is the difference between "the renderer stopped
presenting" and "a set of threads were blocked on objects owned by 28296, and the thread that aborts was
waiting too": the abort is downstream of the block, not the block itself.

The bound on that conclusion is the trace cap. `SYNC trace truncated (20000 lines)` is stamped at
**+11.875 s**, 134 s before the abort, so the 43 waits are only the ones the recorder still had room to
track: a handle first touched after +11.9 s is invisible here, and the true outstanding count can only be
larger. A future run should either raise the cap or filter the trace to waits exceeding
`XCL_SYNC_SLOW_MS` so that the ledger covers the whole session. Even untruncated, this evidence is
descriptive: it says which handles the threads were on, not which `SetEvent` never arrived.

Because the build traces every sync call, the timings are not a measurement of the title's cadence and
must not be quoted as one. What is measurable and reproducible from this record is the **count** and the
**identity** of the blocked handles, and the timestamps relative to the stop.
