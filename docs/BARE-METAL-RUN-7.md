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
progress. It did not. **The abort is not a fixed wall clock, and it is not a GPU check.**

**Superseded — this page's first reading of it.** The text below was published in this page's original
version and is withdrawn; it is kept so the record shows what changed:

> The abort time is not a fixed wall clock. It is **scope start + 300 s**. Run 6 aborted at +442.4 s,
> so its timed scope opened at about +142 s; run 7 aborted at +510.0 s, so its timed scope opened at
> about +210 s.

Both derivations are wrong. A probe that raised the float at `0x6159E20` from `300.0f` to `1800.0f`
logged `scopes count=0` at +60 s **and** +240 s: no load scope was ever open, so the 300 s rule had no
subject and could not have fired in these runs. The stop is a **heartbeat stall detector** — a 1 Hz
thread that crashes the title on purpose (`mov dword [0], 0x4D8BD421`) when nothing has advanced for 30
polls. The full mechanism, the `rbp=0 rbx=0` register evidence and the arithmetic are in
[STALL-DETECTOR.md](STALL-DETECTOR.md).

Corrected reading of this run: rule B fires about 29 s after the heartbeat stops, so run 7's frame loop
stalled at about **+481 s** against run 6's about **+413 s**. The fill moved the stall later in
wall-clock terms; it did not remove it. (inferred: abort timestamp minus the verified 30-poll window)

Two consequences:

1. **The gap to the last frame is not a fingerprint.** It is about 29 s by construction of rule B, and
   run 6's "about 40 s" was the same rule sampled less precisely. Do not compare the two as if they
   measured different phenomena.
2. **Calling it a GPU watchdog is a category error.** Nothing about GPU progress is read by the rule
   that fires; the detector watches a heartbeat, and #505's "GPU-progress" label for *why* the loop
   stalled stays **reported**, not decoded here.

The practical consequence for the next run changed too: extending the 300 s limit buys nothing at this
stop, because rule B fires 30 s after the stall regardless. The load time has to be won from the stall
itself, not from the deadline.

## What this rules out

- **"Refused BCPack destinations are why the run cannot get past the stop."** They now receive solid
  blocks, countably, and the stop is byte-identical. The refusal path is not the gate. (verified)
- **"Filling refused textures will improve throughput."** It more than halves it on this bed.
  (measured, one boot per arm)
- **"The 300 s abort is a GPU watchdog."** It is neither a GPU check nor, in these runs, the 300 s
  rule: it is a heartbeat stall detector firing its 30-poll rule. See
  [STALL-DETECTOR.md](STALL-DETECTOR.md). (verified in decode)

It does **not** rule out BCPack decoding as a lead: real decoding is a different code path from
solid-block fill, and this run says nothing about what a decoded BC1/BC5/BC7 block does to the stop.

## Honest limits

- **One boot per arm.** Two variables moved: the flag, and the streaming schedule that met it. The
  throughput drop is real for this run; its split between fill cost and tile pressure is not.
- **Solid blocks are not textures.** `filled_bytes` measures placeholder writes. It is evidence the
  path fires, not evidence that a texture is correct, and not evidence the screen should change.
- **The stall times are arithmetic.** They are the abort instants minus a decoded 30-poll window at
  1 Hz. No log line records when the heartbeat froze; a stack sample inside that window is the control
  that would settle it.
- **The ped/car inversion is unexplained.** 22 peds → 2 and 24 cars → 36 across the two arms is
  reported here as observed, with no mechanism attached.

## Next

Run 8: **do not re-patch the timer.** The `300.0f` → `1800.0f` experiment has already been run as a
probe, and it returned `scopes count=0` at +60 s and +240 s: the 300 s rule is not the active one, so
raising it cannot help. Arm the run to **sample the frame-loop stack at +5, +15 and +25 s after the
last present**, inside the detector's 30 s window and before its deliberate dump. What that sample
names — a condition variable, a fence, a compile — is the first new information since run 5's archive
repair. See [STALL-DETECTOR.md](STALL-DETECTOR.md). (planned)
