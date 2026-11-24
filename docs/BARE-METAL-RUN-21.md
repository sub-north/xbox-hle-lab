# Run 21: the 300-poll tolerance is spent in full, and the freeze does not lift

**Verified** unless labelled **inferred** or **run record**. Counts, frames and times are from the run's
own log; the exception from its exception record. No shipped file is named or hashed here: the title image
is `<title image>`, the package is `<package>`, the runtime is `xcl`. Offsets, registers and message text
are exact.

This closes the question left open by the previous run, run 20, and by
[STALL-DETECTOR.md](STALL-DETECTOR.md): *if the watchdog that ends the run were given ten times the
tolerance, would present resume?* It was given ten times the tolerance. It did not resume. It waited
**303.79 s** and then executed the same deliberate null write at the same address.

## The change under test

The stall watchdog reloads a poll counter at nine sites in `<title image>`. One constant, `0x1E` (30
polls), was rebuilt as `0x12C` (300 polls); nothing else was patched.

```
02215acc bd 1e 00 00 00   mov ebp,0x1e        ; entry init      file 0x2214ECC
02215c5d 488b05d406fa04   mov rax,[0x71b6338] ; heartbeat
02215c64 bb 1e 00 00 00   mov ebx,0x1e        ; reset value     file 0x2215064
02215c69 4939c7           cmp r15,rax
02215c6c 75d6             jne 0x2215c44       ; moved -> reload
02215c6e ffcd             dec ebp
02215c70 7404             je  0x2215c76 -> 0x23cf620 (abort)
```

Sites: `0x2215ACC/B3A/B62/BFD/C64/CDE/D06/D67/D78`. The probe verifies all nine before it writes and
refuses on any mismatch.

## The run

| | value |
|---|---|
| duration | **700 s** to exit; crash clock **+574.216 s** |
| presents | **2 082** (~7.7/s to the freeze — the normal rate for this title) |
| last dumped present | frame **2 079** at **+270.426 s**, fully black (0 of 518 400 samples) |
| thread dumps | **+281.763 s** (164), **+301.590 s** (164), **+401.570 s** (168) |
| last counter line | `20 037 draws … 8 192 dispatches (4 skipped)` at **+349.914 s** |
| pipelines | 424 translated, **0** failed, **0** fallback, 482 DXIL compiles, 0 failures |
| stop | `0xc0000005` at `+0x23cf683` — write to `0x0` |

Sequence: the first non-black frame is at +24.4 s; present cadence holds ~7.7/s until ~147 s, then
degrades (16 s, 40 s, 24 s gaps) as large compute programs are translated, presenting through every one of
them; then one black frame at +270.4 s, and silence.

## Present never resumes after the last black frame

| final gaps | from → to (+s) | resumed |
|---|---|---|
| 9.78 s | 14.58 → 24.36 | yes |
| 16.36 s | 147.16 → 163.52 | yes |
| **40.25 s** | 163.52 → 203.77 | yes |
| 23.65 s | 203.77 → 227.42 | yes |
| 18.22 s | 243.43 → 261.65 | yes |
| 8.78 s | 261.65 → 270.43 | yes (black) |
| **303.79 s** | **270.43 → —** | **no** (abort at +574.22) |

Two facts from the same table. Every gap earlier in the run — including 40.25 s — resumed, so a stall of
that length is survivable and the title can present again after one. And a black frame is not itself
diagnostic: at **+108.712 s** frame 1 119 was *equally* black (0 of 518 400 samples) and the next present
came **0.6 s later**. What is diagnostic is the gap, not the image.

## The frozen threads are the same threads, in the same frames, 120 s apart

All three dumps show the same three title threads:

```
thread 11988: ntdll+0x166464 < KERNELBASE+0x1427f < <title>+0x56a0d < <title>+0x23cf735 < <title>+0x8126a7 …
thread  5332: ntdll+0x166a64 < ntdll+0xd3194 < KERNELBASE+0x8cca1 < <title>+0x2215b0b < <title>+0x2215eca
thread 39024: ntdll+0x166464 < KERNELBASE+0x1427f < <title>+0x43c3d5d < <title>+0x23cf4f2 < <title>+0x23cf2d3 < <title>+0x23cf104
```

Thread 11988 is parked at `+0x23cf735` in all three dumps and is the thread that later faults. Thread 5332
is parked at `+0x2215b0b` — inside the watchdog function itself, on its lock gate. Between the first and
the third dump the title gained four threads (164 → 168) while losing no others: the process is alive,
scheduling and creating threads, and one of them has not moved in 120 s.

Meanwhile the driver's compiler (`nvgpucomp64`) is resident in every dump (1, 3, 1 threads) and the
runtime translated **296** console compute programs during the freeze, the largest **904 324 B** of DXIL at
**+418.9 s** — 149 s after the last present. Seven programs crossed the 30 s bound and were built in the
background. None of that work released the title.

## What it establishes

**The tolerance is a timer on a deadlock, not a bound on compile latency.** Run 20, at 30 polls, ended
37.78 s after its last present (1.26 s/poll). Run 21, at 300 polls, ended 303.79 s after its last present
(1.01 s/poll). The watchdog is consumed to completion, at roughly one second per poll, and then fires —
which means the poll counter is not observing anything that could recover: the condition it times is
permanent for the life of the process. Ten times the tolerance bought ten times the silence and nothing
else.

The corollary is a **stop**, not a next step: 3 000 polls would buy about fifty more minutes of the same
frozen frame, with the title's own command submission already stopped (last counter line +349.9 s) and its
present thread parked at `+0x23cf735` from +281 s onward. The freeze must be attacked at that park —
what holds the lock the scope is waiting to close — not by extending the clock.

**Second, the onset is now observable.** The title presents a black frame and then never presents again;
the same content at +108.7 s was harmless. Between the two black frames the title's cadence was already
collapsing (40 s, 24 s, 18 s gaps), so the freeze arrives as the end of a degradation, with a black
present as its last visible act — not as a mid-scene snap.

## Honest limits

- **One arm.** Run 21 differs from run 20 only in the tolerance constant; both are the same build family
  and the same bed. That is the strength of the comparison and also its scope: it says nothing about what
  the parked thread waits on.
- **The park is not decoded.** `+0x23cf735` is where the thread sits; the holder of the lock is not
  identified here. The three dumps bracket it, they do not explain it.
- **Poll pacing is inferred.** 303.79/300 = 1.013 s and 37.78/30 = 1.259 s are consistent with ~1 s/poll;
  the poll interval is not read from the counter's own code path in this run.
- **No crash context.** The crash generator produced a zero-byte dump and no crash log for this run, so the
  exception record and the log's last lines are the whole record.

## Provenance

Frames: the shared dump folder is written by every run, and the next run began writing while this one was
being archived — dumps `frame_19`–`frame_1099` were overwritten. Four surviving dumps are included
(source mtimes): `frame_1119` (+108.7 s, black), `frame_1139` (+109.3 s, non-black), `frame_1599`
(+127.6 s), `frame_2079` (+270.4 s, black).

Related: the watchdog's abort mechanism and the "two presents then this fault means a 30 s stall" reading
in [STALL-DETECTOR.md](STALL-DETECTOR.md), the earlier freeze record in
[BVH-WALL-CLEARED.md](BVH-WALL-CLEARED.md), and the run-20 baseline quoted throughout (30 polls, final
freeze 37.78 s, same stop address), which is lab record and not published.

Documentation only. No game data, no title or executable name, no package identity, no shipped-file
hashes, no machine paths, no credentials.
