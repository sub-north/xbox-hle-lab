# xbox-hle-lab

Tools and methods for **Xbox Series compatibility testing**: GPU-PV VMs, capture probes,
root-signature oracles, source watching, and the lab playbook.

Nothing here contains game binaries, game assets, decryption keys, or leaked packages. XCL changes are
distributed as patches against the upstream GPL-2.0 project, not as a fork.

## Status

The title now runs at **30 FPS** by the runtime's own present-rate counter: **30.7 presents/s**
instantaneous and **17.8 presents/s** averaged over a full run, against a previous best of 8.4/s and
2.5/s on most runs. The run is upstream main `63cdc25` plus our sub-2^40 placement fix alone — no
compute-wait bound, no tolerance patch. The 40-bit BVH address wall is cleared in that same run
(`XDXR` count 0, four acceleration structures accepted, the first at `0xed8b010000`), and the GPU
driver's compiled pipeline blobs now persist and replay: `N compute pipelines made from the driver's
blob of an earlier run (kept on disk)`, with per-PSO lines reading like `… (86776 B DXIL, 5 ms, the
driver's blob of an earlier build)` where the same pipeline cost 681 ms to build cold.

What is still missing is the picture. The screen shows front-end key art and menus, never a world
render, and a run still ends when the title's own stall detector fires: `0xc0000005` at
`+0x23cf683` after a ~40 s stall. The last run presented 1 639 frames in 133 s. See
[`docs/WHAT-STOPS-THE-LOAD.md`](docs/WHAT-STOPS-THE-LOAD.md) for the mechanism and what is ruled out,
and [`docs/STALL-DETECTOR.md`](docs/STALL-DETECTOR.md) for the two rules behind the abort.

Three changes produced this run, and only three. **Payload integrity**: the payload is staged and
checked before the title starts, so these figures describe the title's own behaviour. **Address
placement**: the runtime now decides where its large write-watched reservations go, keeping their GPU
views below 2^40, instead of walking eight fixed hints into the block it had just placed
([`docs/BVH-WALL-CLEARED.md`](docs/BVH-WALL-CLEARED.md)). **Driver blob cache**: pipeline blobs
written by an earlier run are found and reused — 5 ms replay against 681 ms cold, 29 → 56 entries
across two runs — so a run spends its time presenting rather than rebuilding pipelines.

Cleared batches are published by a deterministic runner and merged automatically when the pull
request head is exactly the cleared commit; external pull requests are reviewed by hand.

## Contents

| Path | What |
|---|---|
| `tools/gpu_probe/` | device/UMA/present probes, root-signature deserializer oracle |
| `tools/vm/` | App Sandbox VM automation (create, GPU assign, exec, file push) |
| `tools/watch/` | source registry + head/delta watcher |
| `docs/` | lab playbook, race kit, verification reports |
| `patches/xcl/` | GPL-2.0 patches for ivall/xcl (d3d12_xs VM config, xmem virtual alloc) |

## Quickstart

### 1. Build the probes

`tools/gpu_probe/*.cpp` are single-file programs. From an **x64 Native Tools** prompt (MSVC
2019/2022), in `tools/gpu_probe`:

```bat
cl /nologo /O2 /MT /EHsc main.cpp           /link d3d12.lib dxgi.lib user32.lib
cl /nologo /O2 /MT /EHsc uma_probe.cpp      /link d3d12.lib dxgi.lib user32.lib
cl /nologo /O2 /MT /EHsc present_probe.cpp  /link d3d12.lib dxgi.lib user32.lib
cl /nologo /O2 /MT /EHsc rootsig_check.cpp  /link d3d12.lib user32.lib
```

`/MT` (static CRT) is deliberate: a VC++ runtime DLL dropped beside a game folder can be
misclassified as a game DLL and break the loader. Then:

```bat
gpu_probe.exe                 :: adapters, VRAM, feature level, wave lanes, tiers
uma_probe.exe                 :: 4 GiB-aligned 2 GB region as a D3D12 heap
present_probe.exe 12 0        :: 12 s clear/present, sync interval 0
rootsig_check.exe blob.bin    :: deserialize a root signature blob
```

### 2. Point the VM scripts at a lab root

Set `LAB`; nothing else needs editing:

```powershell
$env:LAB = "C:\lab"
```

Expected layout under `LAB`: `tools\appsandbox\extracted\headless-api` (App Sandbox `asb` SDK),
`iso\Windows.iso` (guest installer), and `vm\appsandbox-vm.json` (generated connection record —
git-ignored, never commit). With `LAB` unset the scripts resolve relative to the current directory.

```powershell
python tools\vm\create_vm.py                   # create guest, wait up to 40 min for it online
python tools\vm\assign_gpu.py                  # snapshot, stop, gpuMode=1, boot
python tools\vm\vm_exec.py "cmd /c ver" 60     # one command over SSH (session 0)
python tools\vm\push_file_cmd.py build\gpu_probe.exe C:/lab/gpu_probe.exe
```

### 3. Watch upstream heads

```powershell
powershell -File tools\watch\sweep.ps1
```

Needs `git` on `PATH` and network; ~20–40 s; spawns no agents. Writes
`notes\watch\state.json` and `notes\watch\latest.json`, and appends
`notes\watch\deltas.md`. These generated files are git-ignored.

## Principles

1. Evidence before claims: every result carries a hash, a log, or a measurement.
2. No host execution of untrusted console binaries; VMs only.
3. Console flags are captured before sanitising; null-backend results are rejected as evidence.
4. No game data in this repository, ever.

## Contributing

Read [`CONTRIBUTING.md`](CONTRIBUTING.md) before opening a pull request. Bug reports and first-run
reports have templates in [`.github/ISSUE_TEMPLATE/`](.github/ISSUE_TEMPLATE).

## Licence

Licences apply per scope; each covers only its own paths.

| Scope | Licence | File |
|---|---|---|
| `tools/**` — original code | MIT | `LICENSE` |
| `docs/**` — documentation | CC BY 4.0 | `docs/LICENSE` |
| `patches/xcl/**` — xcl patches | GPL-2.0 | `patches/xcl/COPYING` |

The root MIT licence does not relicense the GPL-2.0 patches, and the documentation licence covers
prose, not code.
