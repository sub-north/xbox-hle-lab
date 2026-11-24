# Plain upstream `47efa2e` — the recipe and the measured ladder

Private lab record. If you want the best frame count this title has produced under `xcl`, build
upstream `47efa2e` and change **nothing**. This page is the recipe, the four runs behind it, and what
upstream has already fixed for us. All figures are from the runs' own timed logs and `report.json`,
and every run used the same binary bytes.

## The recipe

| | |
|---|---|
| Source | upstream `ivall/xcl` at `47efa2e` — *"VFS: reject removal of non-empty virtual directories"* |
| Local patches | **none** |
| Performance env vars | **none** — no `XCL_PSO_WAIT_MS`, `XCL_WAVE64_SPIN`, `XCL_BCPACK_BC1`, `XCL_CROSSQ_WAIT`, `XCL_DXC_ARGS` |
| Cache | warm `XCL_DXIL_CACHE` directory (the same one across all four runs) |
| Harness only | `XCL_DATA_ROOT`, `XCL_DXIL_CACHE`, `XCL_DXC_DIR`, `XCL_COHERENT_MB=2048`, `XCL_NETWORK=0`, `XCL_DSTORAGE_BCPACK_FILL=0`, `XCL_FRAME_DUMP` |
| Runtime | `xcl-plain-47efa2e.exe`, 7 315 456 B — SHA-256 `B267FF0C54BD17C3CC96CB427AE772F5B249BF6A4FEB7876E5ACF762A0EEBC01` |

`47efa2e` already contains `02bfd1d` (low-VA link), `8cad517` (BCPack), and `8a5643f` + `285560f`
(`Wait64BitValueX`). Nothing of ours is compiled in.

## The measured ladder

| config | last frame | presents | rate | present stops | title traps |
|---|---|---|---|---|---|
| **plain, no env** | **3 059** | **3 075** | **21.5 /s** | `+151.0 s` | `+183.9 s` |
| `XCL_PSO_WAIT_MS=150` | 1 559 | 1 578 | 18.9 /s | `+87.6 s` | `+119.9 s` |
| `XCL_WAVE64_SPIN=1` | 1 599 | 1 618 | 18.8 /s | `+89.8 s` | `+121.7 s` |
| plain, repeat | 2 679 | 2 682 | 21.0 /s | `+132.4 s` | `+162.6 s` |

Rate is last dumped frame ÷ (last present − first present), so it excludes load: 3 059 / 142.2 s,
1 559 / 82.5 s, 1 599 / 85.2 s, 2 679 / 127.3 s. Both knob runs are **~12 % slower per second and
~60 s shorter**, and neither reaches content the plain run does not.

## Why plain wins

1. **`XCL_WAVE64_SPIN` bypasses the translated-shader cache.** The flag is in the deny list of
   `cache_allowed` (`pso.rs:448-451`), and a denial returns from `console_to_dxil_in` before the
   in-memory `ShaderCache` is ever consulted (`pso.rs`). Every shader that is asked for
   again is re-translated instead of looked up. The on-disk DXIL cache still hits, which is why the
   run looks healthy in `dxil_cache` while losing frames. (Mechanism verified in source; the frame
   cost is inferred.)
2. **Bounding PSO waits trades frames for nothing.** `XCL_PSO_WAIT_MS=150` cuts the run from
   3 059 to 1 559 frames, dies 63 s earlier, and shows the same front-end art. A pipeline that is
   still compiling does not become visible earlier because the waiter gave up; it only stops the
   presenter from waiting on it. On this title the unbounded wait is the faster path.
3. **Nothing else was competing.** Both knob runs differ from the plain run by exactly one
   environment variable, on the same host, cache and launcher.

## What upstream fixed for us

- **The 2^40 destination wall — `02bfd1d`.** Seven lines in `crates/xcl/build.rs` add
  `/HIGHENTROPYVA:NO` for MSVC (and `--disable-high-entropy-va` for GNU). With high-entropy ASLR off
  the runtime's reservations land at `0x7FFF0000` instead of ~2.1 TiB, so the console builder's
  2^40 check never fires. Verified here: the built exe's PE optional header reads
  `DllCharacteristics=0x8140` (HEVA clear), and the run logs **0** `XDXR` errors and no
  `AccelerationStructure` line. The wall that killed runs 13–17 is absent from a stock build.
- **BCPack — `8cad517`.** The merged decoder unpacks BC1, BC3, BC4, BC5, BC6H and BC7 by default
  (BC2 is refused), single-stage and zlib→BCPack alike. There is no gate and no flag. Our log's
  first decode line is `BCPack BC1: first request unpacked (39541 to 65536 bytes)`; the plain run
  finished with BC1 2 236 requests / 156.3 MB, BC7 202, BC5 108, BC6H 1, **0 failed**, and
  `bcpack_refused = {}`.
- **`Wait64BitValueX` — `8a5643f` + `285560f`.** Bridged to GPU fences and on by default, with a
  mid-list split opt-out (`XCL_SPLIT_MIDLIST_WAIT=0`) we do not need. The plain run logged exactly
  one wait, and it was bridged:
  `Wait64BitValueX at … >= 0xc07 bridged to GPU queue.Wait (wait-before-signal)` — **0 timeouts**.

## What of ours is now redundant

| Ours | Why it can go |
|---|---|
| Placement ceiling (sub-2^40 `VirtualAlloc2` / KernelBase fix) | `02bfd1d` does it in the linker, below every allocation. The code path we patched is not reached on `main` (`kernelx`/`uma` never call `VirtualAlloc2`). |
| Vendored BC1 decoder behind `XCL_BCPACK_BC1` | Upstream decodes BC1 itself, and the variable is unread at `47efa2e`. The flag is dead weight and must not be set. |
| Cross-queue wait conversion (`XCL_CROSSQ_WAIT`) | Redundant against the merged bridge. It also never engaged in the run that carried it (0 conversions), so it was never proven. |

**One patch is still worth carrying:** `XCL_DXC_ARGS` is inert upstream — it is read only by
`tests.rs`, and `compile_cached_in` takes no extra arguments (**#559**, unchanged at `47efa2e`). A
patch that threads the arguments into the cache key remains the only local change that still buys
something. Re-apply it over `242a6ea`'s cache key rather than rebasing the old tree.

## What still stops it

Presentation stops on its own, then the title's **own deliberate trap** fires ~30–33 s later:
`0xc0000005` at `game_scarlett_master.exe+0x23cf683`, a `mov dword [0], 0x4d8bd421`. That is the
stall detector's abort (rule B: no open scope and a heartbeat unchanged for 30 one-second polls),
not a host fault and not a device loss. The exact stop time moves between runs — `+151.0 s` and
`+132.4 s` for the two plain runs — but the halt-to-trap gap is stable at 30–33 s. The repeat run's
last dumped frame was fully black (0 of 518 400 samples non-dark), so presentation had already
stopped before the counter did.

**The simulation is loaded behind the art.** The trap's own crash context, captured on the bounded
wait run at frame 1 074, reports `Game State: Game Running` with **8 521 / 22 500 buildings, 84 / 600
peds (60 real, 24 dummy)** and **232 / 700 cars (183 superdummy)**. The repeat run's context at frame
2 349 reports 8 521 buildings, 79 peds, 245 cars. Nothing in any frame dump is world geometry: the
pools are full while the picture is still the account picker and the intro splash art.

## The loop budget, for context

Every loop the translator emits carries a fixed budget: `static uint xcl_budget = 1u << 16`
(`emit.rs:5538`), injected as `if (xcl_budget == 0u) break; xcl_budget--;` (`mod.rs:591`) at the head
of each emitted loop (`mod.rs:945`) and of the dispatcher (`mod.rs:1050`). It is **65 536
iterations per lane per invocation, shared by all loops, never refilled**, and a trip is a silent
`break` — no clamp, no log, execution continues past the loop with the registers it held.

So the budget produces wrong output, not a hang. A run that stops presenting *while frames are still
advancing* is long-loop or badly-emulated work, not the trip count; a run that stops presenting
completely is more likely **barrier desync** — with `XCL_X_SPIN=0`, one half of a wave64 pair that
left a loop or exited leaves its partner in `bar.sync` forever — than a loop that ran too long.

## Reproduce

1. Check out upstream `47efa2e`; build release with the MSVC target. Confirm the exe: 7 315 456 B,
   `DllCharacteristics=0x8140`, and string `XCL_WAVE64_SPIN` present (the flag is off unless set).
2. Point `XCL_DXIL_CACHE` at a warm directory (blobs are keyed on DXIL bytes, not on the runtime
   build, so a rebuild does not invalidate them).
3. Set only the harness variables above. Do **not** set `XCL_PSO_WAIT_MS`, `XCL_WAVE64_SPIN`,
   `XCL_BCPACK_BC1`, `XCL_CROSSQ_WAIT`, `XCL_DXC_ARGS`, `XCL_DSTORAGE_BCPACK_FILL`.
4. Expect ~150 s of front-end art at ~21.5 presents/s, then present silence, then the trap at
   `+0x23cf683` ~30 s later. The process stays alive after the trap; reclaim it.

## What it means

A stock build beats every patched build we have measured on frames and rate, so **judge new work
against the plain numbers, not against our patched runs** — and before porting a patch, check whether
`47efa2e` already does the job. Progress past this point has to come from the stall itself: the
heartbeat rule that fires at `+0x2215A60` and the sync chain behind it, not from the GPU translation
layer.
