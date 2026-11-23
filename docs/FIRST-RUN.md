# First run of a retail Scarlett title under xcl

Status: **reached the title's own code and advanced through five distinct stops** — not playable, no
frame. The crash-reporter assertion that blocked runs 6–13 was cleared by staging the data file the
title asks for (`appdata0.rpf`); the title then stopped in an unimplemented-method trap at `+0x1CE6E13`,
before any window existed and before any D3D12 command list was created — and stopped there until the
complete payload cleared it ([BARE-METAL-RUN-1.md](BARE-METAL-RUN-1.md)).
Evidence level: our own runs, in a GPU-PV VM, with logs. The title itself is not distributed here.

## Environment

| | |
|---|---|
| Runtime | xcl `lab/nvidia-wave32-v2 @ 217ac85`; runs 4–6 use upstream `484dd57` plus small patches; runs 11–13 use upstream `9add721` ("Retail game bring-up") with `XCL_NETWORK=0` |
| VM | Windows 11 guest, GPU-PV, **network off**, interactive session |
| GPU | NVIDIA RTX 4070 Ti SUPER, 16 GB, wave 32..32 |
| Config | `log_level=trace`, `XCL_D3D12_TRACE=1`, `XCL_CONSOLE_LISTS=1`, `XCL_FORCE_WAVE32=1` |
| Memory | UMA 4095 MB opened as GPU heaps |

## Progression (sixteen runs)

| Run | Runtime | First failure | Detail |
|---|---|---|---|
| 1 | `217ac85` | `d3d12_xs!D3D12ConfigureVirtualMemoryX` | auto-stub returned `E_NOTIMPL`; title raised `0x80000003` at `+0x1e1cfe9` |
| 2 | `217ac85` + `D3D12ConfigureVirtualMemoryX` | `xmem!XMemVirtualAlloc` | stub returned 0; `0x80000003` at `+0x1e1d01b` |
| 3 | `217ac85` + both patches | large commit allocation | `XMemVirtualAlloc(0x49400000, type=0x20003000, attrs=0x183, prot=0x22004)` returned NULL |
| 4 | `484dd57` + `D3D12ConfigureVirtualMemoryX` | **`E_NOINTERFACE`** | 8 stubs left (from 28); crash at `+0x1d8e505`, `rax=0x80004002`, no `unknown_iids`, no `called_stubs` |
| 5 | `484dd57` + patch + refused-IID diagnostic | **`E_NOINTERFACE`** (still) | same `+0x1d8e505`, `rax=0x80004002`; the new diagnostic cleared both prior suspects |
| 6 | run 5 + two console DirectStorage factory IIDs | **`0x80000003`** assertion inside the title | `+0x2237178`, `rax=0x0`, `rdx=0x66`, `r8=0x7EFEFEFEFEFEFEFF`, right after the GameInput device callback |
| 7 | unchanged | dump folder at `D:\LocalDumps` | `mkdir` failed silently — `D:` was read-only install media (`DriveType 5`); **null test** |
| 8 | unchanged | CD letter moved via CIM | volume stayed at `D:`; **null test** |
| 9 | unchanged | `diskpart`/`mountvol` + `subst` | script stalled; **no boot** — nothing settled |
| 10 | unchanged | `<game>\LocalDumps` (xcl maps `D:` to `{game}`) | directory confirmed present; same crash — dump-directory hypothesis **falsified** |
| 11 | `9add721` upstream + `XCL_NETWORK=0` | network off on current upstream | stubs 8 → 6 (`hle-cross 14`, `xcl-netoff 6` markers); same `+0x2237178`, `rax=0x0` |
| 12 | run 11 + GameInput device-info and callback-return logging | — | `GetDeviceInfo` (size `0x3f0`, family 1, caps `0x8`, input `0x104000e`) → "device callback returned" → crash; the assert is **after** the callback |
| 13 | run 12 + `XCL_CRASH_PROBE addr:len,tls0` | crash-time memory probe | registry count **1**, single entry all zeros, key buffer reads **`appdata0.rpf`** |
| 14 | run 13 + `appdata0.rpf` staged | give the title the file it asks for | **`+0x1CE6E13`** — reporter assert cleared; new stop follows `QueryApiImpl XPackage`, `rax=0`, `rdx=0x173400000001`, `r8=r9=0xffffffff00000000` |
| 15 | run 14 + XPackage entry logging | — | logging only; the trace it produced is the one quoted in run 16, at the unchanged RVA |
| 16 | run 15 + `package_identifier()` returns the manifest Identity Name only | test whether the identifier gates the stop | `+0x1CE6E13` (unchanged) | log: `(30 chars, buffer 33) -> 0x00000000` — the name is accepted; **identifier length is not the gate** |

The `appdata0.rpf` detail is the useful one for anyone else at this stage: the title builds the
name in an obfuscated decoder, looks it up in a file-device registry, and asserts if the entry is
empty. The registry is populated from the file being present in the game folder, not from any
console-only capability. An 8,880-byte file was the whole difference between runs 6-13 and run 14.

Run 4 import resolution: `acphal` 14, `xcrdapi` 1, `xg_xs` 1, `minuserx` 2, `gameinput` 1,
`kernelx` 54 (+13 cross-HLE), `d3d12_xs` 4, `xaudio2_9` 1 — **8 stubs**.

Run 6 import resolution is **unchanged**: the same counts, the same **8 stubs**. `report.json` for run 6
shows `called_stubs = []` — the eight stubbed imports (`XMemIsToolsMemAvailable`,
`XMemMapPhysicalPagesScatter`, `XDspActivate`, `XDspConnectWithMaximumStreamLimit`, and four more) were
resolved but never called.

Run 11 rebases onto upstream `9add721` with `XCL_NETWORK=0`: the stub count falls **8 → 6**, with the
`hle-cross 14` / `xcl-netoff 6` markers present. The crash does not move — neither does its reason.

### Run 5 — the refusal is not where we guessed
- Added a diagnostic that logs every refused `QueryInterface` IID.
- Crash did not move: `+0x1d8e505`, `rax=0x80004002` (`E_NOINTERFACE`).
- Log contains **no** refusal from `XGameRuntime` `QueryApiImpl`, **no** GameInput `QueryInterface`
  refusal line, and no stub call at that point — both prior suspects are excluded.

### Run 6 — factory accepted, next assertion
- Runtime now accepts two console DirectStorage factory IIDs at `DStorageGetFactory` / `factory_qi`:
  `62959d11-d45f-4884-a0a3-f7862ea243ae` and `3c02cb0c-c050-4195-99a1-a1ce94ba33c5`, plus a warning
  whenever any DirectStorage `QueryInterface` is refused.
- The `E_NOINTERFACE` crash **disappeared** and the title advanced past it.
- New stop: `0x80000003` in the title at `+0x2237178`, `rax=0x0`, `rdx=0x66`,
  `r8=0x7EFEFEFEFEFEFEFF`, immediately after
  `GameInput: device callback for kinds 0x40000 (enumeration 2)`.
- A vtable at RVA `0x53EC0F0` (image base `0x140000000`) appears twice on the crash stack.

### Runs 7–10 — the dump directory, falsified
- The first explanation offered was that the title asserted because it had nowhere to write a dump.
- Runs 7 and 8 never changed anything: `D:` in the guest was the Windows install ISO (`DriveType 5`,
  read-only), so `mkdir` failed silently; the CIM call to reletter the volume did not move it. Run 9's
  script stalled and the game never started.
- Run 10 tested it properly: `<game>\LocalDumps` — the guest `D:` the runtime maps to `{game}` —
  listed and confirmed present before the boot. Same crash, same `rax=0x0`. **Hypothesis falsified.**

### Run 12 — the assertion is after the callback
- Added `GetDeviceInfo` result logging and a line on the device callback's return.
- Log order: `GetDeviceInfo` (size `0x3f0`, family 1, caps `0x8`, input `0x104000e`), then
  "device callback returned", then the `int3`. The callback completes; the assert follows it.

### Run 13 — an empty registry entry, not the key
- `XCL_CRASH_PROBE addr:len,tls0` reads the key and the registry count at crash time.
- Probe result: registry count **1** (not 0), the single entry is all zeros, and the key buffer holds
  `appdata0.rpf`. The assert fires because a file-device lookup for that name finds an empty entry.

### Runs 15–16 — the identity answer is not the gate
- Run 15 logged every XPackage entry point the title touches early: `IsPackagedProcess`,
  `GetCurrentProcessPackageIdentifier`, `Mount`, `GetMountPathSize`/`GetMountPath`,
  `CloseMountHandle`, `EnumeratePackages`.
- In run 16's log the sequence is `IsPackagedProcess` → `GetCurrentProcessPackageIdentifier` (buffer 33) →
  `IsPackagedProcess` → `GetCurrentProcessPackageIdentifier` → `int3`, at the unchanged RVA.
- Run 16 made `package_identifier()` return the manifest Identity Name only instead of the synthesized
  family name. The log reads `(30 chars, buffer 33) -> 0x00000000`.
- The stop did not move: still `+0x1CE6E13`. **The identifier length was not the gate.**

### The stop at `+0x1CE6E13` — decoded
- A 41-byte function, `0x1CE6DF0`–`0x1CE6E19`; the `int3` is at `+0x1CE6E13`.
- The predicate at `0x292D370` tests `[[Stage+0x2a8]+0x78]` for null.
- Nearby string: `"The method Stage::displayStateSet() is not implemented"`.
- Stack is a clean init chain: `0x2240EAB` → `0x2236CF6` → `0x293EF38` → `0x1CE6DF0`; the raw
  stack words between them also hold the vtable pointers `0x667D140` and `0x53EC0F0`.
- `rdx`'s high 32 bits equal the crashing thread id, so the value is not an error code.
- No window has been created and no D3D12 command list exists at this point.

## Contracts recovered

### `D3D12ConfigureVirtualMemoryX`
- Signature: `HRESULT fn(VirtualMemoryConfiguration*)`, struct `{u32 flags; u32 page_table_pages;
  u32 frame_buffer_preserve_bytes; u32 reserved; u64 output0; u64 output1}` (32 B).
- Validate `flags & ~1 == 0`; `page_table_pages <= 33`; zero both outputs; register with `xmem`.
- Success is `HRESULT_FROM_NT(STATUS_SUCCESS) = 0x10000000`, **not** `S_OK`.
- Called once, 0 ms after entry.

### `XMemVirtualAlloc(address, size, type, attributes, protect)`
Observed calls:

| size | type | attrs | protect | result |
|---|---|---|---|---|
| `0x600000000` (24 GB) | `0x20402000` | `0x21` | `0x4` | reserved OK |
| `0x600000000` | `0x20402000` | `0x3` | `0x801` | reserved OK |
| `0x49400000` (1.2 GB) | `0x20003000` | `0x183` | `0x22004` | **NULL** with naive mapping |

`0x20402000` is a 64 KiB-page reservation (not 2 MiB). The protection word carries console flag bits;
masking it to a valid `PAGE_*` value is required.

### Physical pages
Downstream expectation: `XMemAllocatePhysicalPages` must return **real, tracked** page identifiers,
not success with fake IDs. Upstream xcl already implements allocate/map/free over tracked 64 KiB pages.

## What is missing now

1. **The trap at `+0x1CE6E13`** — the current stop. It is an unimplemented-method trap, not the crash
   reporter: a 41-byte function, predicate in `0x292D370` testing `[[Stage+0x2a8]+0x78]` for null, with
   the string `"The method Stage::displayStateSet() is not implemented"` next to it. It is reached from
   a clean init chain, after the XPackage identity is answered successfully, with no window and no
   command list. The value of `[[Stage+0x2a8]+0x78]` at crash time has not been measured.
2. **The earlier assertion at `+0x2237178`** — cleared by staging `appdata0.rpf` (run 14). Static
   analysis reads it as `assert(count != 0)` on a registered-handler vector that was empty because a
   factory lookup at `0x56EB10` returned `0`; run 13's probe tied that emptiness to an all-zero entry
   for `appdata0.rpf`, and TLS slot 0 was correctly held by the title throughout.
3. **No DXC in the guest** — `dxcompiler.dll` + `dxil.dll` absent; shaders cannot compile, so nothing
   would draw even if startup completed.
4. **No system `xgameruntime.dll`** — xcl's own parts work; task queues/networking/dialogs fall back.

### Open leads (not yet verified)

- **VERIFIED by run 12** (was inferred) — the crash follows the GameInput device callback with no
  intervening HLE call, and the callback's return is logged before the assert. The title asserted on a
  value it derived itself.
- **REPORTED, deprioritised** — a third-party project documents the console returning 8-byte descriptor
  handles in `RAX` where the PC path uses the hidden return pointer in `RDX`. Run 12's logging gives
  that call path no place to hide: the failing site counts a handler vector, not a handle. Keep the
  lead for later stages only.
- **REPORTED** — the same third-party chain reaches its first `CreateDescriptorHeap` and asserts there
  on handle identity; that is a later stage than ours.

## Reproduction

Requires the original title, a legal copy, and a Windows VM with GPU-PV. This repository ships no game
content and no executable. The value here is the failure taxonomy and the contracts, not a boot.

## Labels

- Verified: import resolution counts (runs 4 and 6 identical), crash RVAs, `rax` codes, `rax`/`rdx`/`r8`
  at the run 6 assertion, `called_stubs = []` for run 6, the vtable RVA `0x53EC0F0` on the run 6 stack,
  the three `XMemVirtualAlloc` argument triples, the `D3D12ConfigureVirtualMemoryX` contract (source +
  reference implementations); `<game>\LocalDumps` present before run 10; the stub count 8 → 6 in run 11;
  the run 12 log order (`GetDeviceInfo` → callback return → `int3`); the run 13 probe values (count 1,
  single all-zero entry, `appdata0.rpf`); the run 14 move to `+0x1CE6E13`; the run 16 log order and its
  33-byte buffer with the 30-char identity accepted (`0x00000000`) at the unmoved RVA; the `+0x1CE6E13` decode — function bounds `0x1CE6DF0`–`0x1CE6E19`, predicate
  `0x292D370` testing `[[Stage+0x2a8]+0x78]` for null, the `Stage::displayStateSet()` string, the
  the frame chain `0x2240EAB` → `0x2236CF6` → `0x293EF38` → `0x1CE6DF0`, `rdx`'s high half being the
  thread id; no window and no command list at the stop.
- Reported: upstream claims about later stages (physical pages, DirectStorage factory, main code), and
  the third-party descriptor-handle convention in "Open leads".
- Pending: the value of `[[Stage+0x2a8]+0x78]` at the `+0x1CE6E13` stop — not measured. Nothing else in
  runs 15–16 is open.
- Inferred: that the `int3` at `+0x2237178` is `assert(count != 0)` on a registered-handler vector and
  that the vector is empty because `0x56EB10` returned `0` — static reading of code, already superseded
  at runtime by the run 13 probe.
- Not claimed: a frame, a menu, or playability.

Status: **the title has not rendered a frame.**
