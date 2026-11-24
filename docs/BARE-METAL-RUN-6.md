# Sixth bare-metal run: the registry assert is gone, and a populated world was running at the abort

**Measured** unless labelled **verified**, **inferred** or **reported**. Counts are from the run's
`report.json`; timings, register values and crash telemetry from `xcl.log`; frame content from the
BMPs the run itself wrote. No shipped file is named or hashed here. In quoted log lines the module
token and lab paths are redacted; offsets and values are exact.

## Setup

Bed unchanged from run 5: the separate Windows install on its own partition, the payload staged
read-only under the lab account, network adapters down, the vendor driver staged offline, the
self-reporting harness with a completion marker, and the 15-minute cap. The coherent window was
**2048 MB**, as in run 5.

Two things changed against runs 1 to 5:

- **The one archive run 1 later found torn was replaced.** The replacement is byte-identical to the
  source copy, at the same byte size; both hashes are held in the lab record and are not published.
  The earlier staged tree was size-checked only, which is exactly how the tear survived staging
  (see [BARE-METAL-RUN-1.md](BARE-METAL-RUN-1.md) and the run-1 correction).
- **The probe build.** `xcl-probe3.exe`, SHA-256 `AC47FF6B…` (full value in the lab record), static
  CRT. It adds a vectored-exception logger at `+0x29DEFB1` and `+0x1330A44` that records the
  registry state and then returns `CONTINUE_SEARCH`. It can therefore observe the trap; it cannot
  swallow it, and no probe line appears in this run's log.

## Result

| Run | Presents | Stop | ScriptStore registry assert reached |
|---|---|---|---|
| 1 | 0 | `0xC000001D`, illegal instruction | no — stopped earlier, zero presents |
| 2 | 2 766 | `0x80000003`, title-side breakpoint, site undecoded | not recorded |
| 3 | 1 188 | `0x80000003` at `+0x29DEFB1` | yes |
| 4 | 1 097 | `0x80000003` at `+0x29DEFB1` | yes |
| 5 | 923 | `0x80000003` at `+0x29DEFB1` | yes |
| 6 | **1 111** | `0xC0000005` at `+0x23CF683`, write to address 0 | **no** |

Run 2 used a different runtime and its breakpoint site was never decoded; it is listed for the
present count, not as a comparable assert run. Runs 3, 4 and 5 are the identical stop: same code,
same offset, same register set across three boots.

Run 6 presented 1 111 frames with 1 111 frame boundaries, in **442.7 s of presentation** (the whole
boot lasted 450.3 s), = **2.51 presents/s**. The string `+0x29DEFB1` does not occur in the run log
at all. The run did not stop at the assert: it ran more than twice as long as any of the three runs
that ended there, and ended somewhere else.

## What this supersedes

**Run 5's conclusion that the registry assert was the gate is superseded.** The run-5 record ended
by proposing to instrument the lookup: log the key, the bucket count, and the searched bucket. That
plan is now moot for this stop, because the lookup is never reached when the payload is intact. The
assert was a **consequence of the torn archive, not a missing subsystem** — **inferred**, strongly:
the archive repair is the substantive difference between runs 1 to 5 and run 6, the probe only logs,
and the assert vanished entirely rather than firing with different register values.

This is one boot, and it carries a second variable (the probe binary). A repeat on the un-probed
build is required before the claim is treated as settled. It is published now because the negative
is unambiguous in this run's log and because run 5's page currently points the search at the wrong
subsystem.

## Where it stopped

Verbatim from the run's own output, module token redacted:

```
CRASH: unhandled exception 0xc0000005 in <title image>+0x23cf683 (write at address 0x0)
rax=0x0 rbx=0x0 rcx=0x70024c348 rdx=0x1 rsi=0x4f96bffb38 rdi=0xcc51740ab rbp=0x0
r8=0x7074d8c40 r9=0x7074d8e20 r10=0x7074d8ad8 r11=0x7074d8c40 r12=0xc97fa1e75
r13=0x146845c28 r14=0x7ffaf4dbb220 r15=0xcb3911ff5 rsp=0x4f96bffac0;
on the stack: <title image>+0x4e4ba89, <title image>+0x2215c35,
              <title image>+0x71b9a20, <title image>+0x2215eca
```

The title's own crash handler emitted these parameters before the exit:

```
parameter `Build Type`, value: `vb`
parameter `Code Branch`, value: `sub_story`
parameter `Data Version`, value: `930.048.020-vb_sub_story`
parameter `GPU Loc Sig`, value: `7333478d`
parameter `Game Environment`, value: `prod`
```

The title also wrote two minidumps, **25.5 MB and 27.1 MB**. Both upload attempts to the publisher's
telemetry endpoint failed with `WinHttpSendRequest error: 12007` (host not found), so **nothing was
transmitted**; the fast-lane firewall now blocks outbound traffic for every runtime path. The endpoint
is not reproduced here.

Last captured frame, at 3840×2160, sampled every 4th pixel:

```
present: frame 1109 (3840x2160, format 28) written ... - 518400 of 518400 samples not black
present: frame 1109 raw texels (most common): 0xff2c2020×518349 0xffd6d0d3×3
             0xff2a1f1f×3 0xffd6d3d3×2 0xff4c4a47×1 0xff302525×1
```

**The last frame is not black.** It is an almost uniform dark red/brown field (`0xff2c2020`, 99.9%
of samples) with a handful of near-white texels (`0xffd6d0d3`) and darker neighbours — the shape of a
dark screen carrying a small bright element, not a clear colour and not a blank present. Earlier
captures in the same run are not all like this: frame 1009 at 346 s was fully black. The last
capture describes the stop, not the whole run.

Streaming was live at the stop, not idle:

```
DirectStorage: BCPack refusals so far: 512 requests: BC1 348 (6287360 bytes),
  BC5 107 (7077888 bytes), BC6H 1 (1048576 bytes), BC7 56 (3932160 bytes)
D3D12.X: a texture at 0x1d1ae5c0000 lies in a GPU view of the title's CPU memory:
  kept in step with 0x1cbae5c0000 (32768 so far)
```

The unimplemented-call list is dominated by three command-list methods at **1 111 calls each** —
`ID3D12GraphicsCommandList10::#126`, `#142` and `#75` — i.e. one per presented frame, alongside
1 116 calls to `ID3D12CommandAllocator::#9`.

## The title's own crash context

The run folder also holds the debug block the title wrote for its crash handler
(`crashcontext.log`, 2.5 KB). Excerpted verbatim:

```
Game Running Time: 0h:6m:43s
Game State: Game Running [T:5071937]
Frame: 1494
Streaming Requests: 3651
TileHeap: used (1671Mb) - total (6236Mb)
Camera Position: <-1782.299927, -1914.123901, 6.035758>
Position: <-1782.699951, -1913.000000, 5.500000>
Peds: 22/600 (real: 9, dummy: 13)
Cars: 24/700 (real: 0(0 traffic), dummy: 0, superdummy: 15)
 - BUILDING: 6910/22500
 - OBJECT: 3979/27500
 - INSTANCE_LIST: 17239/45000
 - GRASS_INSTANCE_LIST: 2272/9000
 - LIGHT: 370/8000
Currently running script threads: 1
   - (2) 0x5b5eeffe
```

The same block was captured at 00:36 in a run on the **torn** archive, which lets the two runs be
compared field by field. Both report `Game State: Game Running`; only the repaired run has content:

| Field | 00:36 run (torn archive) | Run 6 (repaired) |
|---|---|---|
| Streaming Requests | 14 | 3 651 |
| TileHeap used | 527 MB | 1 671 MB |
| Camera Position | 262144, 262144, 21600 (null/placeholder) | -1782.30, -1914.12, 6.04 |
| Player Position | 0, 0, 0 | -1782.70, -1913.00, 5.50 |
| Peds | 0/0 | 22/600 |
| Cars | 0/0 | 24/700 |
| BUILDING pool | 0/0 | 6 910/22 500 |
| Frame at abort | 985 | 1 494 |

The 00:36 block's memory counters are all zero and its positions sit on the 2^18 placeholder —
the signature of a running state with nothing behind it. Run 6 has 3 651 streaming requests, a
1.7 GB tile heap, a player at ground level, 22 peds and 24 cars, and one script thread. The
inference is plain: **the archive repair is what turned an empty running state into a streamed
world.** (inferred; the field values themselves are the title's own, verified in both logs)

`Frame` is the title's own counter (1 494), not the runtime's present count (1 111); the two count
different things and should not be compared as equals.

## What the screen was

The last captures remain the dark loading stage with a single small bright element — a loading
spinner. So the accurate statement is: **the simulation was running in the world, with populated
pools, a real camera and player position and a live script thread, while the displayed frame was
still the spinner.** The title was simulating a world it had not yet drawn. Do not read this as
visible world rendering, and not as interactive play.

## The #505 match

This is the **same stop, same stack, same signature** as ivall/xcl#505, an independent report on the
same title build: same faulting offset, same four stack offsets, same `GPU Loc Sig` value. That
match is **verified** by comparing the two records.

**Correction (this page, and this run's pull request, first called the stop a "GPU-progress
watchdog").** That label is **superseded**: the decode shows a **heartbeat stall detector** — a 1 Hz
thread that aborts the title when its frame loop stops advancing — and this run's registers show it
fired on the heartbeat rule, with no load scope open. See
[STALL-DETECTOR.md](STALL-DETECTOR.md) for the two rules, the `rbp=0 rbx=0` evidence, the 29.2 s
arithmetic and the confirming experiment. The superseded sentence, kept so the record shows what
changed:

> The reading that this is the title's **GPU-progress watchdog** is **reported** by #505, not decoded
> here.

So: the abort arrives about 29 s after the last present, which is the detector's built-in
30-poll window, not a property of GPU progress. #505 names
`ID3D12GraphicsCommandList10::#101` returning 0 as the suspected trigger; `#101` does **not** appear
in this run's unimplemented list, so we cannot confirm that mechanism from our own log. What #505's
match does establish is the same abort **site** — same offset, same stack, same diagnostic token —
not the same cause.

## Honest limits

- **The screen is not the world.** The title was simulating a populated world at the abort — 22/600
  peds, 24/700 cars, 6 910/22 500 buildings, a real camera and player position — but no captured frame
  shows that world. The screen is the loading spinner throughout. This is not a visible-render claim
  and not a claim of interactive play.
- **One boot.** The assert's absence needs a repeat, and ideally a boot on the build without the
  probe, to remove the instrumentation as a variable.
- **Rate is not a performance result.** 2.51 presents/s against run 5's ~4.7 is a single-boot number
  under active streaming, PSO translation and a 2048 MB coherent window. It is not a measurement of
  the build's cost.
- **The watchdog name was borrowed, and it was wrong.** Our decode says the title **aborted itself on
  purpose** at this offset, from a heartbeat stall detector, to force a diagnostic dump; this run's
  registers place it on the no-scope rule, 29.2 s after the last present. #505's "GPU-progress"
  reading of *why* the loop stalled stays reported. See [STALL-DETECTOR.md](STALL-DETECTOR.md).
- **The telemetry is the title's, not ours.** `GPU Loc Sig`, `Data Version` and the branch strings are
  values the title's handler emitted; we publish them as observations, not as an identification of
  the build.
