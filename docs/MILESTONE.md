# MILESTONE — "a verified Xbox Series runtime result on Windows"

**Current: L1 (verified). Next: L2.** The one artifact that proves it: the title's own state block from a
run that clears `+0x1CE6E13`, showing a `Game State:` later than `System Init`, beside that run's
int3-free log.

**Labels.** *verified* = observed here with an archived artifact; *inferred* = reasoning with its premise
stated; *reported* = a third party's claim. A level passes only if its artifact is archived, listed with
its hash, and produced a second time with the same result (our "the crash moved" rule).

## 1. Rubric

| L | Claim | Pass evidence — the exact artifact a stranger checks | Must not count |
|---|---|---|---|
| **L0** | Title entry reached | The run log's `start`, with the entry point at `+0x<RVA>`, **plus** the title's own debug block (it prints `TitleID`, `Conole Type: Anaconda`, `Frame:`) — the title's code, not xcl's | a module load line (mapping ≠ execution); a debugger breakpoint at entry |
| **L1** | N HLE subsystems bound, M stubs | The run report: ≥8 `hle*` keys in `imports_by_source`, `stubbed_imports` fully enumerated, `called_stubs` exact; the log supplies ≥3 distinct subsystems with a *logged call* (args + return) | binding without a call ("silence is not success"); a stub returning `S_OK` |
| **L2** | Title's own init passes a **named** stage | The title's own `Game State:` advances past `System Init` (alt: `Currently running script threads` ≥1), emitted on both sides of the pass, with the module and RVA that emitted it | any host-side stage (`backend 'real'`, `Device14 created`) — that is xcl; "survived 30 s" |
| **L3** | First command list created **and** closed | New counters `d3d12.command_lists_created/closed` ≥1 **and** a log pair `CreateCommandList(type DIRECT)` → `Close → S_OK`, caller at `+0x…` | the `RSSetDepthBias` capability probe; a list from `present_probe` |
| **L4** | First Present | `d3d12.presents` ≥1 **and** a log line naming swapchain, present index and HWND, plus a desktop screenshot of the window | `CreateSwapChainForHwnd` = `S_OK` alone; `present_probe`'s 55,057 frames |
| **L5** | First non-blank frame | PNG from the title's swapchain + SHA-256 of the PNG + ≥1,024 distinct RGB triples + mean abs diff ≥2.0/255 vs the same run's blank control + one sentence matching the image | clear colour; single-colour readback (already retracted once); synthetic triangle |

Each level also needs the executable hash and the xcl commit recorded in the run archive; no hash, no
run (our race-kit rule).

## 2. Score — run 16, counters from the run-14 report

| L | Status | Evidence |
|---|---|---|
| L0 | **MET** (verified) | `[0.581] start …: entry point +0x56A1EC8`; the run-16 title debug block reads `Game State: System Init`, `Frame: 0`, `Conole Type: Anaconda` |
| L1 | **MET** (verified) | Run-14 report: 10 `hle*` keys = **9 DLLs / 99 imports**; `stubbed_imports` **6** (1 `xmem`, 5 `chad`); `called_stubs: []`; log: **7** `QueryApiImpl` → XAsync, XNetworking, XUser, XPackage, XGame, XSystem, XSystemAnalytics |
| L2 | **NOT MET** (verified negative) | same block reads `Game State: System Init` at the stop; no later state is ever emitted; `Frame: 0` |
| L3 | NOT MET | zero `CreateCommandList` lines; the only `CommandList` hit is a `RSSetDepthBias` capability probe |
| L4 | NOT MET | `presents: 0`, `frame_boundaries: 0` |
| L5 | NOT MET | `presents: 0`, `shaders.unique: 0`, `audio.engines: 0` — no swapchain exists to capture |

Trend (verified): stop RVA `+0x1E1CFE9 → +0x1E1D01B → +0x1D8E505 → +0x2237178` (×3) `→ +0x1CE6E13`
(×2); stubs 30 → 6; `called_stubs` 1 → 0. Run 16 vs run 14 normalise to a 10-line diff, 4 of them the
new `XPackage` answers — the change landed, the stop did not move (our rule: not a fix, not a
regression).

Two gaps found while scoring (verified): **no run-16 report** is archived, so all run-16 counters are
inferred from run 14 (same RVA, identical call path); and **no executable hash** appears anywhere in
our records, which breaks the L0 evidence chain and our own "no hash, no run" rule.

## 3. What can be claimed publicly today

| Can claim (verified, after redaction) | Must not claim |
|---|---|
| xcl 0.1.0 loads a shipped Xbox Series GDK title and reaches its entry point (`+0x56A1EC8`) on Windows, on a real RTX GPU in a GPU-PV VM | "the title runs / boots / works / is playable on PC"; any fps |
| 9 HLE DLL surfaces and 7 GDK API classes answer queries; 6 imports stay stubs and **0** are called | "engine initialised" — the title's own block says `System Init`, `Frame: 0` |
| UMA opened (3 sparse windows, 12 GiB VA + 512 MB coherent) and `ID3D12Device14`/`ID3D12Heap1` created | "shaders execute" (`shaders.unique: 0`); "audio works" (`engines: 0`) |
| First failure reproducible twice at `+0x1CE6E13` with `called_stubs: []`; the run-16 change landed and the stop did not move | that a missing export caused it — a title-internal assert on a derived handle (`rdx=0x173400000001`); an ABI mismatch is *inferred* only (public report, reported) |
| The title's own crash reporter runs (16,431,851 B payload, 30+ threads enumerated) | publish the package identity, host paths, crash-endpoint URL/DSN, or the minidump |

## 4. Next three artifacts, in order (L1 → L2)

| # | Artifact | What it proves |
|---|---|---|
| 1 | A next-gate note: the body containing `+0x1CE6E13`, the branch that raises `int3`, and the precondition named **before** the next run | defines "cleared" so a later stage advance cannot be read as post-hoc |
| 2 | One run with report **and** log pulled host-side, stop RVA ≠ `+0x1CE6E13`, `called_stubs` still `[]` | the gate cleared rather than retried |
| 3 | That run's title debug block showing `Game State:` past `System Init` | **L2** |

## 5. Redistribution independence

| Level | Auditable from artifacts, executable never published | Independently reproducible |
|---|---|---|
| L0–L1 | yes — log, report, executable hash | no — needs the binary |
| L2–L5 | yes — log, report, PNG + hashes | no — needs the binary |

No level requires redistributing the executable, so the public form of this milestone is the audit trail
plus a hash: the report and log ship as redacted extracts (package identity, lab paths and DSN removed)
with the full artifact's SHA-256 stated alongside.
