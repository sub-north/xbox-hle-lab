# LAB PLAYBOOK — running and verifying Xbox Series code

Purpose: how this lab executes, measures and reports on Xbox Series compatibility work. It is a
method document, not a runtime. Runtime base is **xcl `lab/nvidia-wave32-v2 @ 217ac85`** unless a
test explicitly says otherwise.

## 1. Licence gate (verified 2026-11-23)

Only these can contribute **code** to a GPL-2.0 xcl tree:

| Project | Licence | Verdict |
|---|---|---|
| californiaAlmonds/OpenScarlett | **MIT** | copy-safe |
| R0Y00/Xbox2Windows | MIT | copy-safe |
| Ahmet-Dedeler/rage9-re, bcpack-rs | **MIT OR Apache-2.0** (dual) | copy-safe **as MIT** — take the MIT branch |
| Dark3581/AnyBox | **GPL-2.0** | compatible |
| brcly/TuxBox | **no licence declared** (no `LICENSE`, no `license` key in `Cargo.toml`, README silent — rechecked 2026-11-23; earlier "Apache-2.0" entry is stale) | **no** |
| ItsNotRexed/Leonida | GPL-3.0 | no — different version; GPLv3 code cannot be combined into a GPL-2.0-only work |
| drycheetah84/TomBox | none | no |
| rpatty-dev/PercyStar | proprietary / non-commercial | no |
| rpatty-dev/PercyScarlett | **GPL-2.0-only** (since `dd6ec590`, 2026-11-23T21:08Z) | **compatible — copyleft** (see below) |
| Freminet/ScarlettX, jadeowaowa/scarlett-layer | none / Apache-2.0 | no — same Apache-2.0 barrier |

Precisely: **Apache-2.0 is incompatible with GPL-2.0** but **compatible with GPL-3.0**. Its patent
and indemnity terms add conditions GPLv2 §6 forbids, so Apache-2.0 code cannot enter a GPL-2.0 tree;
under GPLv3 §7 the same terms are permitted. A project licensed **MIT OR Apache-2.0** (rage9-re,
bcpack-rs) is usable here because the MIT branch is GPLv2-compatible — dual licensing is what makes it
copy-safe, not the Apache option.

Everything from a "no" row is **behaviour to reimplement**, never pasted code.

**PercyScarlett, reclassified 2026-11-23.** Commit `dd6ec590` (an upstream commit announcing retail
progress; relicense to GPL-2.0-only) moves it from *idea-only, no licence* to **GPL-2.0-only**. Code may
now be taken, but GPL-2.0 is copyleft: anything derived from it — and any binary we ship containing it —
must be GPL-2.0 as a whole. That is already our position (`patches/xcl` is GPL-2.0), so there is no new
licence conflict; there is a **new obligation to track provenance** per hunk. Its evidence status does not
change: it is still **external-report only** (no artifact, no log, no capture in the lab), so its claims
remain unverified here. Note the same commit states it was written *after* xcl (GPL-2.0), so much of its
content is downstream of our own baseline rather than independent work — take only what is genuinely new.

## 2. Verification levels

| Level | Pass condition | Does not prove |
|---|---|---|
| 1 Imports resolve | loader maps the image, imports bind | anything executes |
| 2 Execution | an export runs with controlled inputs and returns | values are correct |
| 3 Correctness | output matches an independent reference (PC `d3d12.dll`, CPU reference, pixel diff) | title-scale behaviour |
| 4 Console fidelity | pre-mask console state captured (e.g. flags `0x03000000`) | game-specific semantics |

Rules:
- Level 1 alone is never reported as progress.
- A null-backend success (`S_OK` + zero blob) is a **fail**; run on the real GPU.
- Capture pre-mask values before any sanitising path.
- Every level-2/3 claim needs a log, capture or measurement; commit messages are not evidence.

## 3. Failure taxonomy — check in this order

1. **Imports / stubs** — `report.json` `called_stubs`, `unknown_iids`.
2. **Silent MQ waits** — `gamecoreclient` / `DisplayMode 0x00010007` / `0x00020007` /
   `GameCoreProxyApp 0x0064003f` wait with **no timeout**. Symptom: no crash, no progress.
3. **UMA pressure** — per-draw 64 KiB allocations exhaust the 2 GB window; look for skipped draws.
   Instrument with `XCL_MEM_ACCESSORS` (inferred — not verified on this tree).
4. **Root signatures** — missing `RTS0`, flags masked, console params in `s4..s15`, descriptor tables.
5. **Wave64 on NVIDIA** — mask truncation, cross-half ops, implicit LDS lockstep. Refusal is correct;
   silent wrong values are not.
6. **Command-list slots** — e.g. #148 `AllocateUploadMemory`, `SetDescriptorHeaps` at console slot 32.
7. **Shader compilation** — DXC missing (`dxcompiler.dll`), OOM from huge programs, driver rejects.
8. **Presentation** — session 0 has no desktop; `CreateSwapChainForHwnd` fails there by design.

## 4. Measurement rules

- Per-run log line: `vad/trace` style timings, draw counts, skipped draws, pipeline counts.
- Report P50 and P95; a single best run is not a measurement.
- Negative controls: prove the test fails when it should.
- Keep hashes of every artifact (binary, capture, log).

## 5. Consolidation plan

**Base:** xcl. **Code donors:** OpenScarlett (MIT) for the DXIL emitter, wave64→wave32, tiling/DCC,
BCPack and UMA model; Xbox2Windows (MIT) for root-signature evidence and import fixtures; rage9-re /
bcpack-rs for PSIN and BC1.

**Ideas to reimplement:** TuxBox root-signature synthesis, offline identity (`net_guard`), DirectStorage
queue, GameInput, PSO disk cache. PercyScarlett's GDK boot surface. Leonida's verification rigs.
ScarlettX's DXIL/DXBC chunk stripper.

**Open gaps:** `gamecoreclient` MQ host protocol; BC7 decode (claimed, unproven); slot #148; root
signatures for shaders that ship without RTS0.

## 6. Pitfalls

- Commit-message milestones ("playable", "title screen") without captures.
- AI-fabricated format docs — one project's README admits it.
- Star inflation: eight repos appeared within hours; judge by tests.
- Null-backend and WARP results presented as hardware.
- Rebasing the frozen baseline before the first executable run.
