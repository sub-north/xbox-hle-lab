# Run 32: BC1 requests are decoded, not refused - 2103 of them, 0 failed

**Verified** unless labelled **inferred**. Every count, frame number and byte total below is read from
this run's own logs (`stderr.log`, its timed twin, `report.json`, `exit.txt`). No shipped file is named
or hashed here: the title image is `<title image>`, the package is `<package>`, the runtime is `xcl`.
The instrumentation is ours, and its hash is published.

`BCPACK-BC1.md` closed with the decoder "off by default and not yet wired into the runtime". This run
wires it in. The question was narrow: once BC1 BCPack requests are handed to the decoder instead of
being refused, do they come back as blocks, and does anything of them reach the screen?

## Which run this is

The fastlane session stamped **2026-11-24 04:19:32Z**, binary `xcl-bc1-1.exe`
(sha256 `03472171F4165E583D8A12DA8D2E97E7E1618EB91056B60D000B55369E57AFCE`).

| Variable | Value |
|---|---|
| `XCL_BCPACK_BC1` | `1` - route BC1 BCPack requests to the decoder |
| `XCL_FRAME_DUMP` | `20` - one present frame in twenty |
| `XCL_THREAD_DUMP_AT` | `280,300` - not reached; the run ends at 109 s |
| cache | `dxil634` |

## The counter that moved

The refusal line, which used to lead with BC1, no longer mentions it at all. Final line of the run:

```
BCPack refusals so far: 256 requests: BC5 107 (7077888 bytes), BC6H 1 (1048576 bytes), BC7 148 (13504512 bytes)
```

The run before the integration (`2026-11-24 04:08:57Z`) ended on:

```
BCPack refusals so far: 2048 requests: BC1 1758 (108699648 bytes), BC5 107 (7077888 bytes), BC6H 1 (1048576 bytes), BC7 182 (24756224 bytes)
```

BC1 was the majority of everything refused - 1758 of 2048 requests, 108.7 MB - and then it was nothing.
`report.json` carries the other side of the ledger:
`dstorage.bcpack_decoded.BC1` = `{requests: 2103, decoded_bytes: 148795392, failed: 0}`.

**2103 requests, 148 795 392 bytes, zero failures.** There is no separate
`BCPack BC1 decoded N requests (M bytes), K failed` line in stdout, stderr or `xcl.log`; the
`report.json` counter is the only record, and it is the one quoted here.

**Read the refusal line with care.** It is printed at each doubling, and this run stops at 256
requests. In the pre-integration build the 64- and 128-request lines also carried no BC1 - BC1 first
appears at 512 (`512 requests: BC1 321 ...`). So "no BC1 in the refusal line" at 256 is *consistent
with* decoding, but it is not on its own independent of how far the load got. The evidence that does
not depend on the stage is the counter above: 2103 BC1 requests were decoded and none failed, in a
run whose refusals totalled 256.

## What the screen showed

The run is short and it only ever reaches one scene. It presents 86 times, frames 19 to 1719, then dies
at 108.9 s on an unhandled C++ exception (`0xE06D7363`) whose stack lists `xboxbvhbuilder` - the same
wall as before, unrelated to textures.

- Frame 19 is black: nothing has been drawn yet.
- Frame 699 is the Rockstar splash.
- Frames 1199 and 1699 are the key-art screen, and they are **right**: full colour, no magenta, no
  grey blocks where a fitted table should be, no black holes where a chunk should have decoded.
  Sampled over the frame: 0.0 % magenta pixels, 0.0 % near-black outside the letterbox, mean RGB
  144, 111, 131 - which is the artwork's own sunset palette, not a swapped channel.
- Frame 1579 is the useful one. In the pre-integration run, frame 1579 (04:10:34) is **pure black**,
  mean luma 0. In this run, frame 1579 (04:20:48) has mean luma 126.4. The same frame index carries a
  picture only once BC1 is decoded. **Inferred**, not a controlled A/B: the two runs crash at
  different points, and that frame is not guaranteed to be the same scene state.

Present cadence averages 17.7 fps over the 96 s of dumps, with one 15 s stall between frames 119 and
139. This is a diagnostic build and no performance claim is made.

## Proven

- BC1 BCPack requests are **decoded rather than refused**: 2103 requests, 148 795 392 bytes, 0 failed,
  and no BC1 entry in the refusal counter for the whole run.
- The decoded result reaches the screen: the key art renders in full colour where the previous build
  left that frame black.

## Not proven

- **Visual correctness at scale.** One static scene, one resolution, no gameplay, no camera movement.
  The failure modes a lane or block-mapping bug produces - wrong colours, half-blocks, seams - need
  motion and varied textures to show up. Two good key-art frames do not clear the decoder for the game.
- **The other formats.** BC5, BC7 and BC6H are still refused in the same run (256 requests: BC5 107,
  BC7 148, BC6H 1). The BC7 total is not comparable with the older run's 182 - that line was printed
  at 2048 requests, this one at 256 - so nothing here says BC7 refusals fell.
- **Byte-exactness.** `failed: 0` counts requests, not correctness. Nothing here has been compared
  texel-for-texel against console output.
- **Stability.** The title still ends in an unhandled C++ exception from `xboxbvhbuilder` at 109 s.
