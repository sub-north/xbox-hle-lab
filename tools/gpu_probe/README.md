# gpu_probe — PC-side D3D12 oracles

Small single-file programs used to separate "the GPU works" from "the VM can reach the GPU",
and to get an independent PC reference for console data. Sources only; build them yourself.

| File | Builds | Answers |
|---|---|---|
| `main.cpp` | `gpu_probe.exe` | adapter list, VRAM, feature level, wave lane range, DXR tier, mesh-shader tier, binding tier |
| `descriptor_heap_probe.cpp` | `descriptor_heap_probe.exe` | shader-visible CBV/SRV/UAV heap limits on hardware and WARP, with a CPU-only control |
| `uma_probe.cpp` | `uma_probe.exe` | can a 4 GiB-aligned 2 GB region be opened as a D3D12 heap (`OpenExistingHeapFromAddress`) — the unified-memory path a console runtime needs |
| `present_probe.cpp` | `present_probe.exe` | minimal Win32 + flip-model swapchain: clear and present for N seconds, printing every HRESULT |
| `rootsig_check.cpp` | `rootsig_check.exe` | deserialize a serialized root signature blob and print its normalised structure |

## Build

Use an **x64 Native Tools** command prompt (MSVC 2019/2022). `/MT` (static CRT) is deliberate: a
statically linked probe runs in a guest that has no VC++ redistributable, and a runtime DLL dropped
next to a game folder can be misclassified as a game DLL and break the loader.

```bat
cl /nologo /O2 /MT /EHsc main.cpp           /link d3d12.lib dxgi.lib user32.lib
cl /nologo /O2 /MT /EHsc /std:c++17 descriptor_heap_probe.cpp /link d3d12.lib dxgi.lib
cl /nologo /O2 /MT /EHsc uma_probe.cpp      /link d3d12.lib dxgi.lib user32.lib
cl /nologo /O2 /MT /EHsc present_probe.cpp  /link d3d12.lib dxgi.lib user32.lib
cl /nologo /O2 /MT /EHsc rootsig_check.cpp  /link d3d12.lib user32.lib
```

`gpu_probe_mt.exe` in older notes is simply the `/MT` build of `main.cpp`.

## Use

```bat
gpu_probe.exe                 :: run on the host and in the guest, compare the two
gpu_probe.exe --serial COM1   :: also write the report to a serial port (how results left the isolated VM)
descriptor_heap_probe.exe     :: allocate and release each test heap before trying the next count
present_probe.exe 12 0        :: 12 seconds, sync interval 0
rootsig_check.exe blob.bin    :: dump params, descriptor ranges, flags, static samplers
```

## Interpretation notes (learned the hard way)

- **Binding tier and heap capacity are separate.** The heap probe reports the runtime's
  `OPTIONS19.MaxViewDescriptorHeapSize` when available, plus allocation HRESULTs. Compare
  the shader-visible requests with the CPU-only control and check the device-removed reason.

- **Duplicate adapters are normal under GPU partitioning.** Three identical paravirtualised adapters
  with the same name and LUID class were exposed in our VM. Do not assume a single adapter, and do not
  select by vendor ID alone.
- **`present_probe` needs an interactive desktop.** Over SSH (session 0) `CreateSwapChainForHwnd`
  returns `0x887A0022` by design. Run it from a logged-on session, e.g. a scheduled task with an
  `Interactive` logon principal.
- **UMA needs 4 GiB alignment.** Aligning *inside* a larger block leaves less than the requested size
  and the call fails with `E_INVALIDARG`. `uma_probe` falls back to overallocate-and-align and shows
  both outcomes.
- **`rootsig_check` prints a normalised structure, not bytes.** Compare flags, parameters, descriptor
  ranges, register spaces and static samplers against a console blob; a byte match is not required and
  not expected.
- A `S_OK` from a null/software backend is not evidence. Check `software=no` and the adapter name.
