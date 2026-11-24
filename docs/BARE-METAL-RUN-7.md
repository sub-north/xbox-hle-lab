# Seventh bare-metal run: solid-block fill works, and halves the frame rate

**Measured** unless labelled **verified**, **inferred** or **reported**. Counts are from the run's
`report.json`; the register set, duration and crash line from its logs; the world state from the
title's own crash context. No shipped file is named or hashed here, and the title image is called
`<title image>`. Offsets and values are exact.

This is a **negative result**. It is published because the counter it was built to move did move,
and the thing we hoped would follow — a different stop — did not.

## Setup

Bed unchanged from run 6: separate Windows install, payload staged read-only under the lab account,
network adapters down, vendor driver staged offline, 2 048 MB coherent window, capture every 10 s,
15-minute cap (the run ended itself at 510.1 s). Same runtime build as run 6, static CRT.

One variable: `XCL_DSTORAGE_BCPACK_FILL=1`. With it set, a DirectStorage BCPack request that the
runtime cannot decode writes a **solid block** into the destination instead of leaving it untouched
and failing the request. Unset — run 6 — the destination is not written and `filled_bytes` stays 0.

## Result

| | run 6 (fill off) | run 7 (fill on) |
|---|---|---|
| presents | 1 111 | **606** |
| duration | 442.4 s | 510.0 s |
| presents/s | 2.51 | **1.19** |
| stop | `+0x23CF683` | `+0x23CF683` |
| TileHeap used | 1 671 MB | 2 014 MB |
| BUILDING pool | 6 910 | 7 297 |
| Peds | 22 | **2** |
| Cars | 24 | 36 |
| Streaming requests | 3 651 | 2 438 |

Presents/s is quoted on each run's own window (run 6: 442.4 s of a 450.3 s boot; run 7: 510.0 s of a
510.1 s boot). On a common whole-boot base the two are 2.47 and 1.19 — the halving is not an artefact
of the denominator. (measured)

## What the counters prove

`filled_bytes` left zero for the first time, per format, against the destination bytes the requests
asked for:

| Format | Refused requests | Destination bytes | Filled bytes |
|---|---|---|---|
| BC1 | 712 | 25 440 256 | **1 110 016** |
| BC5 | 107 | 7 077 888 | **6 971 392** |
| BC7 | 125 | 12 320 768 | **4 710 400** |
| BC6H | 1 | 1 048 576 | 0 |

Run 6 on the same build reported 0 filled bytes in every row. The BC5 row is effectively complete
(98.5% of destination); BC1 and BC7 are partial. BC6H stays at 0 because that one request is refused
one stage earlier, at the unsupported mode — the fill path is never reached for it.

**No `solid block` log line is emitted.** The counters are the only evidence that the path fires;
the run log contains no other trace of it. That is a gap of its own: the next change to this path
should log each fill, because right now a silent regression and a working fill look identical in the
log. (verified by counter, inferred as to which bytes were written)

So the mechanism is confirmed: **refused BCPack destinations can be given solid BCn blocks, and the
counters report it.** The payload is still not real texture data — a solid block is a placeholder,
not a decoded texel, and nothing in this run shows a refused texture appearing correctly on screen.

## What it costs

Frame rate **halved** (2.51 → 1.19 presents/s) while the run did **fewer** streaming requests
(3 651 → 2 438) and presented 505 fewer frames. TileHeap used rose by 343 MB and the BUILDING pool
grew by 387 entries, so the fill arm was carrying more resident geometry, not less work. The world
state also moved in a direction we cannot explain: peds fell from 22 to 2 while cars rose from 24 to
36. The honest reading is that the fill path is expensive and that the run's streaming schedule
changed with it; nothing here separates the cost of writing solid blocks from the cost of holding
more tiles. (measured; the causal split is not established)

The display at the stop is the same dark loading field as run 6 — last capture frame 599, all
518 400 sampled pixels not black, dominant texel `0xff2c2020`. No visible-render claim either way.

## The stop did not move

Same offset, same fault, same stack, and the same three registers that characterise the abort:

```
CRASH: unhandled exception 0xc0000005 in <title image>+0x23cf683 (write at address 0x0)
rax=0x0 rcx=0x70024c348 rdx=0x1
      ... on the stack: <title image>+0x4e4ba89, <title image>+0x2215c35,
                        <title image>+0x71b9a20, <title image>+0x2215eca
```

`rsi`, `r12`, `r15` and `rsp` differ from run 6, as expected for a different boot; `rax`, `rcx` and
`rdx` are identical, which is what makes this the same stop rather than a similar one. (verified)

## The mechanism correction this run forced

Run 7 aborted 67.6 s later than run 6, which invited the reading that the fill bought 68 s of
progress. It did not. **The abort time is not a fixed wall clock. It is scope start + 300 s.**

- Run 6 aborted at +442.4 s, so its timed scope opened at about +142 s.
- Run 7 aborted at +510.0 s, so its timed scope opened at about +210 s.

The 300 s figure is the limit in a **CPU QPC scope timer**: a per-thread stack of open scopes, each
stamped with `QueryPerformanceCounter`, compared against a float limit of `300.0f` at RVA
`0x6159E20`; any scope left open for 300 s fires the abort at `+0x23CF683`. There is no GPU signal
in that predicate. (mechanism verified in the decode; the two scope-start times are inferred as
abort minus 300 s, since the scope table is not logged)

Two consequences:

1. **Run 6's "the abort arrives about 40 s after the last captured frame" is a property of when that
   scope opened, not a constant.** Do not use the gap to the last frame as a fingerprint of the stop.
2. **Calling it a GPU watchdog is a category error at the timer level.** What expires is a CPU-side
   scope deadline; whether the scope is *waiting on* GPU progress is a separate, still-open question.
   The run-6 page's "GPU-progress watchdog" label is ivall/xcl#505's reading of the cause and stays
   **reported**, not decoded here.

The practical consequence for the next run is that extending this timer buys load time without
touching the stall: patching the limit at `0x6159E20` from `300.0f` to `1800.0f` gives the load 30
minutes, and if the title still parks at the same scope nothing was fixed — only the deadline moved.

## What this rules out

- **"Refused BCPack destinations are why the run cannot get past the stop."** They now receive solid
  blocks, countably, and the stop is byte-identical. The refusal path is not the gate. (verified)
- **"Filling refused textures will improve throughput."** It more than halves it on this bed.
  (measured, one boot per arm)
- **"The 300 s abort is a GPU watchdog."** It is a CPU QPC scope deadline. (verified in decode)

It does **not** rule out BCPack decoding as a lead: real decoding is a different code path from
solid-block fill, and this run says nothing about what a decoded BC1/BC5/BC7 block does to the stop.

## Honest limits

- **One boot per arm.** Two variables moved: the flag, and the streaming schedule that met it. The
  throughput drop is real for this run; its split between fill cost and tile pressure is not.
- **Solid blocks are not textures.** `filled_bytes` measures placeholder writes. It is evidence the
  path fires, not evidence that a texture is correct, and not evidence the screen should change.
- **The scope start times are arithmetic.** They are the abort instants minus a decoded 300 s limit.
  No log line records when the scope opened; logging the scope table is the control that would
  settle it.
- **The ped/car inversion is unexplained.** 22 peds → 2 and 24 cars → 36 across the two arms is
  reported here as observed, with no mechanism attached.

## Next

Run 8: turn the fill flag **off**, patch the `300.0f` limit at RVA `0x6159E20` to `1800.0f`, and give
the load 30 minutes. If the abort moves to scope start + 1800 s with the same registers, the timer is
confirmed as the only thing the last two runs measured; if the title uses the time to reach a
different stop, that is the first new information since run 5's archive repair. (planned)
