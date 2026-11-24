# Fifth bare-metal run: the intro movie and the UI render, the stop is unchanged

**Measured** unless labelled **user-observed** or **inferred**. Counts are from the run's
`report.json`, timings and register values from `xcl.log`, capture geometry and per-frame statistics
from the 92 BMPs the run itself wrote. No shipped file is named or hashed here; the runtime hash is
ours.

## Setup

Bed unchanged from runs 1–4: the separate Windows install on its own partition, the full payload, the
vendor driver staged offline, the self-reporting harness with a completion marker, and the 15-minute
cap. Network adapters were down (adapters up: 0) and the firewall rules applied; disk isolation again
was not achieved (`0 volume(s) hidden; offlined disks: 0`), the residual earlier runs recorded too.

One binary, identified from its own artefact rather than from intent: the harness records
`xcl-head-85bdf21.exe`, SHA-256 `0C9F6571…`, with harness script `4B02F6F9…` (full values in the lab
record). Captures were taken every **10th** present at 960×540 this time, against every 30th in runs
3 and 4. Nothing else about the bed changed.

## What changed

The build carries two upstream changes that runs 3 and 4 did not have, each with a named mechanism:

- **`9bd7635` — GPU views read the CPU view.** The descriptor a shader samples and the descriptor the
  CPU writes were not the same view, so a surface the title filled on the CPU could be sampled as
  whatever the GPU-side view still held. It is the change aimed at the intro movie, which the title
  decodes and uploads itself.
- **`e93dc9c` — write-watch on GPU-visible memory.** CPU writes into memory the GPU can see are now
  tracked so the copy that reaches the GPU is the latest one. The mechanism is aimed at the glyph
  atlas, which is filled on the CPU after the surface exists.

Both are in the build; neither was toggled separately. That matters for the same reason it mattered in
run 4: a positive frame result cannot be attributed to one of the two.

## Result

| | Run 4 | Run 5 |
|---|---|---|
| Presents / frame boundaries | 1 097 / 1 097 | **923 / 924** |
| Time to stop | 187.518 s | **197.525 s** |
| Unique / submitted shaders | 19 209 / 274 766 | 19 209 / 274 766 |
| Fallback / failed / translated pipelines | 0 / 0 / 20 | 0 / 0 / 20 |
| Stop | `0x80000003`, title-side breakpoint | same code, **same title-side offset** |
| Registers at stop | `rax` `0xffc00000`, `rdx` `0x76`, `rbx` `0xa4`, `rcx` `0x5b5eeffe` | identical |

Presentation rate: 923 presents over 197.525 s = **4.67 fps**. It was not flat across the run — the
presents 199–499 window ran at about 4.0 fps and the last 300 presents at 6.3 fps, so the average
carries the expensive middle.

**One boundary more than presents** (924 against 923) is the only pair here that is not equal; run 4
had 1 097 of each. It is recorded as measured and not explained.

## Frame evidence

92 captures, stride 10, presents 9–919. All comparisons are against run 3's 39 captures at the same
960×540 geometry.

| Presents | What is on screen | Distinct colours | Edge density | Run-3 anchor |
|---|---|---|---|---|
| 9–59 | flat black | 1 | 0 | — |
| 69–159 | account picker, hint bar | 210–220 (`32,32,44` over 99.5% of pixels); 8 417 at present 129 | low | — |
| 169–439 | splash art, then the intro movie | 46 960–223 244 | — | 1 colour, no motion |
| 449–569 | held logo plate (509–569 at 385–400, 499–569 byte-identical) | 385–14 872 | — | — |
| 579–769 | navigation page, legal notices, legal terms | 6 292–6 618 on the text pages; 154–1 864 on the transitions between them | 1.98–9.76% | 1 165–1 352; 1.12–2.57% |
| 779–919 | flat black to the stop | 1 | 0 | text page sat here |

- **Zero flat-green frames in 92.** Run 3 had 15 in 39. The green that dominated run 3's capture set
  is simply absent from this one.
- **The movie window is real motion, not fill.** Mean absolute pixel delta over three 100-present
  windows from present 199: **65.6 / 63.5 / 52.2** of 255, with 150 369–203 266 distinct colours per
  frame in the moving part. Run 3's corresponding frames held **one colour and a delta of 0**.
- **The text pages carry 2.6–5.3× run 3's colour count and 1.4–8.7× its edge density.** Text-shaped
  content is what that ratio means at this sampling: at 960×540 a 4× decimation of a 3840×2160 frame,
  glyph edges are partly a sampling artefact, so the ratio is a floor on the difference, not a measure
  of sharpness.

## Honest limits

**The formal verdict is CONFOUNDED.** At 4.67 fps this run is below the 4.8 fps floor set for the
comparison, and two things could have cost the frames: the write-watch mechanism, which charges every
CPU write into GPU-visible memory, and the capture itself, which is readback at three times run 3–4's
density. This run cannot separate them, so the fps number is not a measurement of the build's cost and
the frame result is not a clean differential.

**The criteria document was written after this run.** It is not pre-registration, and it should not be
quoted as such: its gates were chosen with the captures already on disk. It is published as a reading
order for the evidence, and nothing in this page depends on a gate passing.

**21 of the 92 captures hold a single colour, and it is black every time** — presents 9–59 and 779–919,
with the second block running unbroken to the stop. So "no green" is a statement about the captured
windows and about the 71 frames that carry content, not about every frame the run presented. The text
page that run 3 sampled at 860 and 950 is inside that black tail here, which is why the second text
comparison comes out at 0.00×: an absence, not a regression.

**39 of the 131 files in the run folder are not from this run.** The capture directory is not cleared
between boots, so run 4's stride-30 set (presents 20–1070) and three of run 3's survived alongside this
run's 92. This run's genuine count is **92**. The strays were confirmed by hash against the earlier
run folders — sampled files match byte for byte — so exclude them by content, not by file index:
matching indices across two different strides is a coincidence, not provenance.

## Next step: probe the registry lookup

The stop is a conditional trap taken because a table lookup missed: the searched bucket was empty, the
lookup returned its `0x3FFFFF` sentinel, and the trap fires on that value. The register set is identical
across runs 3, 4 and 5 — same key, same bucket index, same chain terminator — so the miss is
**deterministic** (**inferred** from that identity, not observed twice in one boot); it is not a race or
a timing artefact. Three boots on three builds have now walked to the same hole.

The next boot should instrument the lookup rather than the renderer. At the trap, log:

1. the **key** the lookup was handed — it is read from a field of a live object, not a constant, so it
   is only knowable at runtime;
2. the table's **bucket count, entry count and generation**;
3. the **searched bucket index and its head**, to confirm the bucket is empty rather than the chain
   broken.

That separates two very different faults. If the table holds **no entries at all**, the host never
populated a registry the title expects to exist — an emulation gap, and the missing calls can be found
by asking what registers entries. If the table holds **entries but not this key**, the title asked for
something it has not yet registered, and the interesting question becomes which path was supposed to
register it.

Two housekeeping items belong in the same boot: **clear the capture directory before starting**, so
the folder is one run's evidence, and **capture at stride 30** on the same binary. If the rate returns
to run 3–4's ~6.4 fps, the cost above was readback; if it stays near 4.7, the cost is the write-watch,
and that becomes a real measurement instead of a confound.
