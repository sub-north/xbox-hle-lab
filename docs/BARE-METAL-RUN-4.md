# Fourth bare-metal run: a null result on the glyph patch

**Measured** unless labelled **user-observed** or **inferred**. Counts come from the run's
`report.json`, timings and register values from `xcl.log`, capture geometry from the BMP headers.
Nothing here names or hashes a shipped file.

## Setup: one binary, two mechanisms

Runs 3 and 4 used the same bed: the separate Windows install on its own partition, the payload staged as
in run 1 — size-checked only, with one archive later found torn (see run 1) — the vendor driver staged
offline, the self-reporting harness with a completion marker, and a 15-minute cap. One thing changed.

- **Run 3** ran the base build — the same runtime as run 2, upstream `main` at `7a033c8`, applied
  and built by hand, no unmerged work.
- **Run 4** ran that base plus exactly one unmerged upstream change (PR #269, head `8111cf9`),
  built the same way. Same machine, driver, payload, harness and cap; no workaround added, no input
  driven.

That change carries **two** mechanisms at once: a rework of upload ordering, and a shared-page
placement subsystem for 64 KiB pages. The binary differs by one variable; the change does not. Even
a positive result could not have said which mechanism produced it — and neither mechanism was
toggled separately. That is the main reason this run cannot answer its own question.

Two limits were fixed before the boot and matter after it: captures are taken every 30th present and
are 960×540 (a 4× decimation of a 3840×2160 frame), and run 3's `report.json` was not retained. Run 3
therefore has presents and time but no shader or pipeline totals.

## Result

| | Run 3 (base `7a033c8`) | Run 4 (base + `8111cf9`) |
|---|---|---|
| Presents / frame boundaries | 1 188 / 1 188 | 1 097 / 1 097 |
| Time to stop | 184.886 s | 187.518 s |
| Unique / submitted shaders | not retained | 19 209 / 274 766 |
| Fallback / failed / translated pipelines | not retained | 0 / 0 / 20 |
| Stop | `0x80000003`, title-side breakpoint | same code, same title-side offset |
| Registers at stop | `rax` `0xffc00000`, `rdx` `0x76`, `rbx` `0xa4`, `rcx` `0x5b5eeffe` | identical |
| Captures retained | 39 | 36 (the loop ended at 1 097 presents) |

Run 4's shader and pipeline totals are the same as run 2's on the base build, so the patch added no
pipeline work of its own. Presents fell 7.7% and time rose 1.4%; both sit inside the spread of a
single binary, which produced 2 766 presents in 171.5 s on one boot of the base and 1 188 presents in
184.9 s on the next. Neither is a throughput or stability observation.

The frame metrics pre-registered for this pair (`RUN4-CRITERIA`, frozen before boot) came out as
follows:

| Metric | Run-3 anchor | Confirms if | Run 4 | Ratio |
|---|---|---|---|---|
| M1 distinct colours, exact 1:1 | 239 (mis-derived, see below) | ≥ 478 (2.0×) | 5 329 | **0.96×** |
| M2 RMS σ (BT.601) | 41.5 | ≥ 62 (1.5×) | 40.2 | **0.97×** |
| M3 mean &#124;ΔLx&#124; | 5.49 | ≥ 8.2 (1.5×) | 6.28 | **1.14×** |
| Blind read, 5 UI strings | 5/5 legible (**user-observed**) | ≥ 3/5 | legible, but the anchor already was | — |

## Why the result is null

**The confirm clause failed on all three metrics.** 0.96×, 0.97× and 1.14× against thresholds of
2.0×, 1.5× and 1.5×. Nothing moved beyond the spread of the bed.

**The refute clause could not be met either.** It required 0/5 strings legible *and* zero
`tiled resources on 64 KiB pages` lines. The page read in run 4 was already read as 5/5 in the run-3
anchor (**user-observed**, one reader), and the same placement line appears once in each run, with
identical tile milestones in both logs. A check whose two branches both need something this run did
not supply tells us nothing; this is a null, not a refutation of the patch.

**The frame index is not a time anchor.** Captures are every 30th present, and the runs presented at
different rates — 1 188 against 1 097 over a similar duration. Index 950 is therefore a different
moment in each run, and the prescribed index-matched pair compared two *different* pages. M3's 1.14×
is a text-heavier page, not sharper glyphs.

**The content-matched pair is byte-identical.** The page that run 3 captured at frame 950 appears in
run 4 at frame 920, and the two captures match under SHA-256. So "the text was fine in run 4" is not
a differential observation: the same page was fine before the patch. No artefact in this run shows a
page that was illegible without the patch and legible with it.

**One metric was mis-derived, and that is recorded rather than hidden.** M1's anchor of 239 distinct
colours is not reproducible: an exact 1:1 count on the anchor capture returns 5 532, and no
quantisation tried yields 239. Its 2.0× threshold therefore came from a different operator than the
one applied to run 4, so the 0.96× is a same-operator ratio with an invalid baseline. M1 was the one
pre-registered metric that could not be applied as written.

**The fault is untouched.** Same exception, same title-side offset, same register values, 39 and 36
captures of the same loop, ending 2.6 s apart. The patch changed neither the stop nor the pipeline
work. It also did not change what could be measured about glyphs.

**Limits of the artefact.** Captures are a 4× decimation, so glyph edges the metrics tried to
resolve are partly a sampling artefact. The runtime's own final frame sample (frame 1 070) is
entirely dark — 0 of 518 400 sampled pixels non-dark — so the end of the run is not a text
observation either.

## What would actually test the text defect

The defect being chased is partial letters: glyph caches the CPU fills arriving at the GPU partially
or stale. That needs a build carrying the **targeted atlas fix** — the CPU-fill / dirty-tracking
path — not the placement subsystem that arrived alongside it. Concretely:

1. **One mechanism per binary.** Build the atlas-side change alone, or run `8111cf9` with its new
   pages path switched off, so placement is the only variable. `RUN4-CRITERIA` already names this
   toggle; it was not used.
2. **A baseline with a spread, not a single boot.** Run the same binary twice (A/B/A) and measure the
   between-boot spread of the chosen metric first. Run 2 and run 3 on the same build differed by
   1 578 presents; a single-boot comparison cannot resolve small differences.
3. **Dense captures of one text-heavy page.** Capture at every present, or at least 5 per second,
   for the whole window the page is displayed — at full resolution, no decimation. Identify and pair
   pages by content (pixels or OCR), never by frame index.
4. **A metric with one stable operator.** Count partial glyphs (components narrower than a complete
   glyph on the same line) or per-string OCR confidence, computed by one script over both matched
   captures, and report the ratio against the measured baseline spread.
5. **Legibility as a recorded reading.** Two readers, the same five strings, per-string answers,
   blind to build. Publish the anchor's readings from a run on the *same* build, so a legible page is
   only evidence when the comparison build was illegible on that page.
6. **Keep the confound checks.** The stop, the placement log lines and the tile milestones stay in
   the report — but a check only counts if it can come out differently.

## How to report a null result

- **State the metric, the baseline and the tolerance before the run**, then publish the measured
  ratio even when it is ~1. "No change" is a result; an unstated threshold is not.
- **Name the decisive artefact.** Here it is two byte-identical captures of the same page. A
  remembered screen is not an artefact and cannot carry a claim in either direction.
- **Say which of the two happened**: the predicted effect was smaller than the threshold, or the
  test could not see the effect at all. Run 4 was the second, with a saturated legibility anchor and
  an index that is not a time anchor.
- **Never promote a non-metric into the result.** What the operator read is **user-observed**; it
  belongs in the record, labelled, not in the verdict.
- **Do not round a null up.** No part of this run shows the patch fixed the text, and no part shows
  it did not.
