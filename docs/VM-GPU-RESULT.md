# GPU-capable isolated VM — result (2026-11-21)

**Host:** Windows 11 Home 26H2 `26300.9974`, RTX 4070 Ti SUPER, driver `32.0.16.1742`
**VM:** App Sandbox v0.1.13, Windows 11 guest `26300.9974`, 8 GB / 8 cores, **network none**,
GPU-PV assigned, snapshot `gpu-ok`

## GPU probe (guest)

```
[adapter 0] NVIDIA GeForce RTX 4070 Ti SUPER
  vendor=0x10de device=0x2705 flags=0x0 vram=16063 MB
  software=no
  feature_level=12_2 wave_lanes=32..32
  raytracing=1.2(12) mesh=1.0(10) binding_tier=3
```

Three identical paravirtualized adapters are exposed (0, 1, 2), plus one
`Microsoft Basic Render Driver`. All three report the RTX name, vendor/device, 16 GB VRAM,
feature level 12_2, **wave 32..32**, **DXR 1.2**, hardware (`software=no`).

**This is real hardware-class D3D12 inside the VM — not WARP.** The duplicate adapters are a known
GPU-PV behaviour (multiple partitions / duplicate LUIDs, see App Sandbox #110/#129); xcl must not
assume a single adapter or select by vendor only.

## Unified memory (xcl's requirement)

`uma_probe` mirrors xcl's `uma.rs` path:

```
VirtualAlloc2(4GiB aligned): 000001AE00000000
OpenExistingHeapFromAddress: 0x00000000  heap=...
heap size=2147483648 alignment=4194304 flags=0x421
```

**Pass**, with an important detail: the region must be allocated **4 GiB-aligned** (VirtualAlloc2
`MEM_ADDRESS_REQUIREMENTS`). Aligning *inside* a 2 GB block leaves less than 2 GB and the call fails
with `E_INVALIDARG` — that was our first probe's bug, not a GPU-PV limitation.

## xcl test title inside the VM (2026-11-21)

**Scope:** every count, result and "game frame" in this section belongs to xcl's own synthetic test
title (`xcl_testtitle.exe` + `testgame_mw.dll`). It is not the retail title, which has not been run.

`xcl.exe` (lab build, **static CRT**) + `xcl_testtitle.exe` + `testgame_mw.dll` were copied into
`C:\lab\e2e\game` and run over SSH (`network none`):

```
xcl_testtitle: done, failures: 1     exit 1
[PASS] 29   [FAIL] 1
```

| Check | Result |
|---|---|
| UMA | **4095 MB at 0x400000000, opened as GPU heaps** (`uma_error: ""`) |
| D3D12 backend | `real` — NVIDIA GeForce RTX 4070 Ti SUPER (16063 MiB) |
| Console-only IIDs | 3 unknown (same class as host: expected) |
| Pipelines | `failed_pipelines: 0`, `fallback_pipelines: 2` (console-format shaders, by design) |
| Loader / VFS / TLS / XMem / XAudio2 / Opus | all PASS |
| GameInput | **FAIL in session 0 only** — `GameInputCreate 0xd000003a`; passes on the interactive desktop, no install needed |
| PresentX | swapchain `0x887A0022` because SSH runs without an interactive desktop; offscreen render+readback checks passed |

Two build/deploy bugs found on the way, both ours, not the runtime's:

- A Rust MSVC debug build needs `VCRUNTIME140.dll`; putting it **inside the game folder** makes xcl
  classify it as a game DLL and load it manually — the title then dies silently (exit 101). Fixed by
  building with `-C target-feature=+crt-static`; never place a system runtime DLL in the game folder.
- The App Sandbox SSH agent splits commands on `&&` into separate shells, so `set VAR=... && app`
  never passes the variable. Use a `.bat`, or one command per session.

**GPU-PV stability note:** two `LiveKernelEvent 193` (dxgkrnl live dump) reports were logged at
**21:53 and 21:56**, i.e. at GPU assignment/driver staging, before any D3D12 workload ran. No
recurrence after the reboot, across repeated `uma_probe` runs and the full test-title run. Treat
GPU assignment as the risky moment, not steady-state rendering.

## Interactive desktop: presentation verified (2026-11-21)

The SSH session is session 0 and has no desktop; `CreateSwapChainForHwnd` fails there with
`0x887A0022`. Anything that presents must run in the logged-on session, so both tests were started
through a scheduled task with an **Interactive** logon principal.

**`present_probe`** (new, `tools/gpu_probe/present_probe.cpp`) — minimal Win32 + D3D12 flip swapchain:

```
CreateSwapChainForHwnd hr=0x00000000
frames presented=55057 last_present=0x00000000   (sync interval 0, 10 s)
present_probe: ok
```

Screenshot `archive/reports/vm-present.png` shows the opaque blue frame in the window.

**xcl test title, interactive session:**

```
xcl_testtitle: done, failures: 0      EXIT=0
present: swap chain 640x360 DXGI_FORMAT(28) - the game image will appear in the XCL window
present: first game frame shown (640x360)
report: "presents": 1, "failed_pipelines": 0, exit_code 0
```

The "game frame" above is the test title's 640x360 image — xcl's synthetic title, not the retail one.

Screenshot `archive/reports/vm-title-06.png` shows the `XCL - XCL.TestTitle` window. Its contents
are present but composite **translucently** in the capture (the game frame's alpha reaches the
desktop capture path; the window itself has no layered style). Not yet determined whether this is
capture-only or also on a physical monitor — a fidelity item, not a presentation failure.

**GameInput was a session artefact, not a missing runtime.** `GameInputCreate 0xd000003a` appears in
session 0 and disappears in the interactive session — the same test-title run is 30/30 (synthetic
`xcl_testtitle`, not the retail title). **Do not install the
GameInput redistributable**; there is nothing to fix.

**Snapshot gotcha:** taking a snapshot and restarting reverted the guest to the pre-`e2e` disk even
when starting with the new snapshot's index, so `C:\lab\e2e` and the run's report were gone. The
re-run from that clean boot reproduced `done, failures: 0 / EXIT=0`, so the result is repeatable, but
the guest disk must be treated as **ephemeral**: keep reports, logs and screenshots on the host and
re-push the payload after any snapshot/start. `archive/reports/vm/` is the durable record.

Artifacts: `archive/reports/vm/report-desktop.json`, `vm/xcl-desktop.log`.

## Caveats

- DXR 1.2 is what the adapter reports; no DXR workload has been executed in the guest yet.
- The guest uses Microsoft's paravirtualized driver; **do not install an NVIDIA driver in it**.
- Vulkan in GPU-PV is reported as tearing/stuttering (#228); DX12 is the stable path.
- Guest and host are on the same build; no guest update was needed. Never service the guest with
  GPU-PV active (#180).

## Next

1. Run xcl's test title **inside this VM** (loader, imports, XMem, VFS, D3D12.X, PresentX) with
   `gpu_probe`-equivalent checks.
2. Then the first title-startup attempt for an authorised executable.

Artifacts: `tools/gpu_probe/gpu_probe_mt.exe`, `tools/gpu_probe/uma_probe.cpp`,
`tools/appsandbox/*` (create/snapshot/push/exec helpers).
