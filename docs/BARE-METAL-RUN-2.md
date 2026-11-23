# Second bare-metal run: sustained presentation, then a title-side breakpoint

**Measured** unless labelled **user-observed** or **inferred**. Counts are from `report.json`;
timings are from `xcl.log`. No image was captured, so every statement about the screen is a human
observation, not an artefact.

## Setup

Bed unchanged from run 1: the separate Windows install on its own partition, full payload, vendor
driver staged offline, self-reporting harness with a completion marker, 15-minute cap. Two things
changed:

- **Runtime**: current upstream `main`, static CRT. Identified from artefacts, not from intent —
  the harness record names `xcl-main-7a033c8.exe` with SHA-256 `D5EEC4B2…`, and `report.json`
  carries a `dxc` section (DXcompiler 1.8.2502.11, commit `23992152`) that the older build cannot
  emit. The reported build and the artefact's own feature set agree.
- **No emulation workaround for the previous fault**: this build is expected to handle it.

Network adapters were disabled (adapters up: 0) and the firewall rules applied. Isolation did **not**
hold — see below.

## Result (measured)

| | Run 1 | Run 2 |
|---|---|---|
| Runtime | older build | upstream `main`, static CRT (`xcl-main-7a033c8.exe`, SHA-256 `D5EEC4B2…`) |
| Stop | `0xC000001D` illegal instruction | `0x80000003` breakpoint, title-side |
| Time to stop | 7.254 s | **171.460 s** (crash reporter runs to 172.054 s) |
| Presents | 0 | **2 766** |
| Frame boundaries | 0 | **2 766** |
| Shaders | — | **19 209 unique**, 274 766 submitted |
| Wall clock | — | 22:44:27 → 22:47:21 |

Shader submissions by stage, summing to the total exactly: ps 134 398 · vs 112 038 · as 25 479 ·
cs 2 742 · rootsig 109 = **274 766**. RDNA2 instructions decoded: 37 858 990.

Presentation rate: 2 766 frames over 171.46 s = **16.1 fps average**; the log's own 300-frame windows
range 9.7–30.0 fps. The 40 refused mesh-pipeline translations in the report are recorded here with
them; fallback and failed pipelines are both 0.

## Operator observation (user-observed)

The operator reports **seeing the menu on screen** during this run. That is a human observation of the
display output. It is not in the metrics table because no artefact in this repository contains the
image, and it is not counted, only reported.

## What the numbers prove

The archive gate is cleared again, on a newer runtime, and the run goes far past it. Run 1 stopped at
7.254 s with zero presents; this run built 19 209 unique shaders and presented 2 766 frames on real
hardware before the title stopped itself at 171.460 s.

Presents and frame boundaries are equal — 2 766 and 2 766. Every present the runtime counted was
matched by one frame boundary, so the title held a frame loop for ~171 s rather than sitting on one
image. **Inferred** from that equality; the counters are measured.

## What they do not prove

- **No image exists.** The runtime can dump a frame (`last_frame_dump`); it is null. Nothing here
  shows what was on the display.
- **A visible menu is user-observed, not measured** — and it is not interactive gameplay. No input
  was driven, no menu state was changed, no scene is described. Nothing here is a 3D or gameplay claim.
- **The stop is undecoded.** It is a title-side breakpoint at 171 s, not the archive gate and not
  run 1's illegal instruction. Why the title raised it is unknown.
- **Nothing about streaming, stability or performance.** One boot, 171 s, ~16 fps through a
  translation layer.

## Isolation failure

The non-sandbox disk again refused to go offline — `Operation not supported on a critical disk` —
and the run's own summary is exact:

```
isolation summary: 0 volume(s) hidden; offlined disks: 0
```

So this run was **not** isolated from the main disk. The network was down (adapters up: 0) and the
firewall rules were applied; disk isolation was not achieved. That is a residual, stated plainly, and
it is unchanged from run 1.

## Next observation

Capture a frame dump on the next boot. A dumped frame turns the operator's observation into an
artefact: viewable, hashable, and comparable against the next run. Secondary: extend the stop report so
the title-side breakpoint at 171 s carries enough context to say what raised it.
