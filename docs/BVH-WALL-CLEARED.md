# The 40-bit BVH address wall is cleared; the heartbeat freeze is what remains

**Verified** unless labelled **measured**, **inferred** or **reported**. Counts, frames and times are from
the run's log and its own dump set; the placement and the exception from the run's own records. No shipped
file is named or hashed here: the title image is `<title image>`, the runtime is `xcl`. Offsets, addresses
and message text are exact.

This supersedes the wall in [BARE-METAL-RUN-13-15.md](BARE-METAL-RUN-13-15.md). That document ended with
one allocation above 2^40 and a placement search still to be written. This one closes it, and the run that
closes it reaches the freeze the earlier pair never could.

## The wall

The console BVH builder refuses any acceleration-structure destination with bit 40 set. Its guard is

```
cmp r8,0x10000000000
jae <throw>
```

a compare of the destination against 2^40 followed by a throw: the title converts it to `0x80070057`
(`E_INVALIDARG`), which surfaces as an unhandled `0xe06d7363` in the console builder — not in the
runtime's presentation or compute path. It fired in three separate builds at **~263–283 s** of progress
(runs 13, 15 and 16), from two upstream lineages, at the same check. That is a deterministic wall, not a
race. (verified from the log, the exception record and the decode)

The runtime's only `XDXR` log site is its AS-build path, so the failing build is also the first observable
one. Runs 13 and 15 therefore say nothing about the freeze: they never reached it.

## Three faults, and they had to be fixed together

Each of the three was sufficient on its own to keep the destination above 2^40. Fixing any one and
rerunning would have produced the same `XDXR`.

### (a) `VirtualAlloc2` was never called

The placement search asked `kernel32` for `VirtualAlloc2`. This host exports 1,698 `kernel32` names and
`VirtualAlloc2` is not among them — only `VirtualAlloc`, `VirtualAllocEx` and `VirtualAllocExNuma` are.
The lookup returned `None`, the search never ran, and the old fixed-hint path did its thing. `KernelBase.dll`
*does* export it, and a probe placed 24 GiB through it at `0xf400000000`. The fix resolves `kernel32` first
and then `KernelBase`. (verified)

### (b) The address window's end is inclusive

`HighestEndingAddress = 0xFA00000000` is refused with `ERROR_INVALID_PARAMETER`: an inclusive end must
finish a 64 KiB page, and `0xFA00000000` is the exclusive end of the 24 GiB view below it. `CEIL-1` works.
Two "unrelated" faults — a missing export and a rejected window — were stacked on the same call path, so
the first fix alone changed nothing observable. (verified)

### (c) The hint walk stepped by `size`

The old ceiling search tried eight base addresses, each exactly `size` below the last. `0xf976900000`,
`0xf8ef200000` … `0xf5b4800000`: **all eight land inside the 24 GiB block just placed at `0xf400000000`**
(8 × `0x89700000` = `0x44B800000` < `0x600000000`, verified arithmetic). It then fell back to any address,
which is how the destination ended up past 2^40 in run 15. Eight fixed hints are not a search.

The replacement is a real one: a `[0x10000, CEIL-1]` window with `MEM_TOP_DOWN`, retried with the ceiling
lowered by 1 GiB to the floor; and, where `VirtualAlloc2` is absent, a 4 GiB-stride descending scan of 128
attempts. Either way the answer is a placement decision, not a guess. (verified code)

### Plus two things that must not be quiet

- **UMA slot cap.** The unified-memory slot search (`for_runtime_with`, `with_windows`) now skips any
  16 GiB slot whose end would land above 2^40, logging once. The layout itself is untouched — 4 GiB
  windows, `(a>>32)&3` — and a 16 GiB slot 4 still wins when free, so a run that already satisfies the cap
  behaves exactly as before. Nothing changes when nothing needs to.
- **No silent fallback.** Oversize requests are refused. The fallback, if it is ever reached, logs size,
  address and whether its GPU view is below 2^40, plus one WARN. Run 15's failure was invisible for a whole
  run because that line did not exist; run 18 has **no** `falling back to any address` line at all.

One path is deliberately **not** capped: the driver's GPU virtual address for a resource
(`res_gpu_va_x`). That address is where the GPU reads the resource, so rebasing it to satisfy a CPU-side
check would break GPU access. It warns once with size and address instead.

## Result — run 18

Same bed as runs 13 and 15, one build further on.

```
mem: write-watched reservations end below 0xfa00000000 (2^40 - 24 GiB), so their 24 GiB GPU views stay below 0x10000000000
kernelx: reservation 0xee00000000 (0x600000000 bytes, no CPU access) is the GPU view of 0xf400000000
```

| | value |
|---|---|
| `XDXR` count | **0** |
| AS destinations | four, first at **`0xed8b010000`** — below 2^40 |
| last present | frame **1919** at +262.873 s |
| duration | **302 s** |
| stop | heartbeat abort, `0xc0000005` |

The 2.3 GB (`0x89700000`) reservation that fell back in run 15 now has a home: the patch's own kernelx test
places it at `0xf376900000`, ending exactly at the 24 GiB view's base, in one attempt:

```
mem: placed 0x600000000 write-watched bytes at 0xf400000000 (end 0xfa00000000 <= 0xfa00000000, attempt 1)
mem: placed 0x89700000 write-watched bytes at 0xf376900000 (end 0xf400000000 <= 0xfa00000000, attempt 1)
```

Run 18 does not log addresses for both, so tying the second line to the AS buffers is **inferred** from size
and ordering, as in run 15. What is verified is that four AS destinations were created inside the range and
the builder refused none.

Then the run stops the way the whole exercise was meant to make it stop:

```
unhandled exception 0xc0000005 in <title image>+0x23cf683 (write at address 0x0)
rax=0x0 rbx=0x0
```

`rax = rbx = 0` is the `count == 0` path, so the watchdog's rule **B** fired — no scope open and the
heartbeat unchanged for 30 polls ([STALL-DETECTOR.md](STALL-DETECTOR.md)). This is the old freeze, now
unobstructed: the wall is gone, the frame loop still stops, and 30 s later the title dumps itself.

## World state at the stop

The simulation was live and populated when the frame loop went quiet.

| | |
|---|---|
| `Game State` | `Game Running` |
| buildings | 8 464 / 22 500 |
| peds | 93 / 600 (56 real, 37 dummy) |
| cars | 232 / 700 (189 superdummy) |
| streaming requests | 685 |
| TileHeap | 3 617 MB used / 6 236 MB total |
| objects | 6 052 / 27 500 |
| script threads | 5 |

## Build identity

Two of our artefacts, both static CRT:

| build | SHA-256 | ran |
|---|---|---|
| run 18 | `26A7C67D2BDE5718F5078C1B16BA873CFAB29AF3FB0B429E7F7DB8AC2C2DF19E` | yes |
| superseding | `6525F2E54E8C5659D7D76EC199763CE81E0F2EED6EEF0F6BDC34C9CFB39E5111` | no |

The second was first written to the same output filename as the first. That is a provenance trap: the
filename in an old log no longer implies the bytes the log describes. The newer build is now
`xcl-pr533-sub40c.exe`, and runs are identified by hash, not by name. `cargo test -p xcl --lib`: 644
passed, 0 failed, 21 ignored.

## Next — the freeze

With the wall out of the way the remaining stop has two live candidates:

1. **Skipped compute results.** Run 18 skips work the earlier builds did not: `2750 dispatches (64 skipped)`
   at +283 s, because the 4 s compute bound expired. A skipped dispatch can leave a consumer waiting on a
   result that will never arrive. This mechanism did not exist before PR #533, which is exactly why it is
   worth eliminating first — `XCL_COMPUTE_PSO_WAIT_MS` at 30 s, or unbounded, on this build.
2. **The render thread blocking outside the runtime's instrumentation.** The freeze has always looked like
   a thread parked somewhere xcl does not observe; precompiling the console shaders offline
   (`xcl shaders ... --precompile`) removes the skip path entirely and, if the freeze survives it, leaves
   this as the remaining explanation. A thread dump at last-present + 20 s is the measurement that
   separates them.

## Honest limits

- **One arm, no control.** Run 18 differs from run 15 by all three fixes at once. That is why they are
  documented as a set: nothing here attributes the result to one of them.
- **"AS memory" is still an inference.** The placement is proven for a size and an ordering, not for a
  purpose.
- **The freeze is measured, not explained.** Nothing in this run identifies which thread stopped or why.
  The skipped-dispatch count is a correlation with a plausible mechanism, not a cause.
- **No crash dump.** The exception record and the log's last lines are the whole record, as before.

## Provenance

Runtime patches are published against the upstream GPL-2.0 project, not as a fork. Related: the builder
itself in [GHIDRA-BVH.md](GHIDRA-BVH.md), the abort mechanism in [STALL-DETECTOR.md](STALL-DETECTOR.md),
and the preceding negative results in [BARE-METAL-RUN-13-15.md](BARE-METAL-RUN-13-15.md).
