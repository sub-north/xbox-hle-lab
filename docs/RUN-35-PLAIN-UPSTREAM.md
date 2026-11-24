# Run 35 — a stock upstream build runs 3 075 frames at 30 FPS with no local patches

Private lab record. The point of run 35 was to answer one question: **how far does a plain upstream
`xcl` build get, with nothing of ours in it?** The answer sets the baseline every patch has to beat.

## Identity

| Field | Value |
|---|---|
| Runtime | `xcl-plain-47efa2e.exe`, 7 315 456 B — SHA-256 `B267FF0C54BD17C3CC96CB427AE772F5B249BF6A4FEB7876E5ACF762A0EEBC01` |
| Patches | none — upstream only |
| Title | `RockstarGamesInc.28872443D20A7` — `game_scarlett_master.exe` |
| Env | `XCL_FRAME_DUMP=10`, `XCL_DSTORAGE_BCPACK_FILL=0`, `XCL_COHERENT_MB=2048`, `XCL_NETWORK=0`, `XCL_DXIL_CACHE=…\cache\dxil634`, `XCL_DXC_ARGS=-O3`, `XCL_COMPUTE_PSO_WAIT_MS=30000` |
| Window | `13:18:39.376Z` → exception `13:21:43.326Z` = **183.95 s** (process reaped at 273.4 s) |
| Exit | `-1`; no kill needed |
| Host | TRIXY, `labsim` interactive |

## Metrics

| Metric | Value |
|---|---|
| Presents | **3 075** |
| Last presented frame | **3 059** at `+132.68 s`, 3840×2160, 518 384 / 518 400 non-dark |
| Present rate | **30.0 fps** in six of nine 300-frame windows; 27.7 (frame 1 800), 28.7 (2 400), **12.0** in the last one |
| Gaps > 10 s | **1** — 17.7 s at frames 119 → 139 (the level-load stall every run has). Plus **33.3 s terminal silence** before the fault |
| Stop line | `xcl: unhandled exception 0xc0000005 in game_scarlett_master.exe+0x23cf683 (write at address 0x0)` @ `+183.951 s` |
| `XDXR` | **0** — no `D3D12 XDXR Error`, no `AccelerationStructure` line |
| BCPack refusals | **0** — `bcpack_refused = {}` |
| BCPack unpacked | BC1 2 236 / 156.3 MB, BC7 202 / 30.0 MB, BC5 108 / 7.1 MB, BC6H 1 / 1.0 MB, all `failed: 0`; BC3/BC4 never requested |
| Pipelines | 922 translated, 33 528 draws + 8 192 dispatches, 0 skipped; DXIL cache 1 304 hits / 346 compiles / 0 failures |
| Mesh shaders | 40 PSOs refused: console MS sitting in the amplification-slot position, untranslated |
| Sync | **1** `Wait64BitValueX`, **bridged** to `GPU queue.Wait`; **0** timeouts |
| Crash context | absent — no guest `*.crash.log` in the window, 0-byte dmp; Game State / Peds / BUILDING unknown |
| On screen | account picker → intro splash art; no world geometry |

## Comparison

| Run | Build | Last frame | Rate | Content |
|---|---|---|---|---|
| **35** | **plain 47efa2e** | **3 059** | **30.0 peak / 21.5 in-span** | intro art |
| 24 | patched sub40d | 2 519 | 30 max, 8.6 final | intro art |
| 16 | patched | 1 919 | — | **8 521 buildings, 85 peds** |
| 30 | patched sync2 | 1 839 | 18.4 /s | intro art |
| 33 | patched crossq | 1 899 | 18.3 /s | intro art |

**Frames: better.** +540 over run 24 (+21 %), +1 220 / +1 160 over runs 30 / 33 (+66 % / +61 %).
**Rate: same at peak, better at the end** — run 24 had decayed to 8.6 fps, run 35 held 27.7–30.0 until
its last 300 frames. **Wall time: better** — 184 s for 3 059 frames vs 312 s for 2 519.
**Content: not comparable** — run 16's world-loaded state (8 521 buildings / 85 peds) was never
reached here; run 35 shows the account picker and intro art only.

## What it means

Two of the walls the lab has been fighting are invisible to the plain build:

- **XDXR / acceleration-structure wall: absent** (0 lines). It killed runs 13–17, not this one.
- **2 s guest-memory wait timeouts: absent.** Run 33 logged three; run 35's only `Wait64BitValueX`
  was bridged to the GPU queue and never expired.

So neither the frame rate nor the BCPack paths need our patches. The death is a guest null write at
`+0x23cf683` — **the same fault site as run 30**, after 33 s of present silence during two very large
compute-PSO compiles (605 504 B / 41 855 ms and 545 700 B / 2 428 ms).

**Verdict:** a stock upstream build reaches the account picker and the intro splash art, presents
3 075 4K frames at a locked 30 FPS, decodes real BCPack textures and dies on a null write — and on
frame count and rate it is the **best run recorded so far**. Any future patch should be judged
against these numbers, not run 24's.

Archive: `gta6-lab/archive/reports/baremetal/20261124-131839-run35-plain47efa2e/`.
