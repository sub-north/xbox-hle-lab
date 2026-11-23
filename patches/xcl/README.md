# xcl patches — first-exe HLE coverage

Patches that add missing console API surface to xcl's HLE layer, so the Xbox
Series title's startup path can resolve the entry points its middleware requires.
All were generated with `git format-patch` on branch `lab/first-exe`.

This directory is **GPL-2.0**; see `COPYING` for the full licence text.

## Contents

| Patch | What | Applies to | Status |
|---|---|---|---|
| `baseline-only/0001-d3d12x-implement-D3D12ConfigureVirtualMemoryX-consol.patch` | `d3d12x`: adds `D3D12ConfigureVirtualMemoryX` and its `VirtualMemoryConfiguration` struct; validates `flags`, the 33-page page-table limit, zeroes both outputs, returns NT `0x10000000` (originally commit `68aa5e3`) | `217ac85` baseline; pre-`9add721` main | **SUPERSEDED** (baseline-only) |
| `baseline-only/0002-kernelx-implement-XMemVirtualAlloc-console-virtual-m.patch` | `kernelx`: adds `XMemVirtualAlloc(address, size, alloc_type, attributes, protect)` mapped onto `VirtualAlloc` (originally commit `6ea3c57`) | `217ac85` baseline only | **SUPERSEDED** (baseline-only) |
| `0003-dstorage-log-refused-QueryInterfaces-and-factory-calls.patch` | `dstorage`: logs the refused `QueryInterface` IID, the factory's requested IID and result, and `CreateQueue`'s source/capacity/priority/IID. Logging only, no behaviour change (`+33/-1`) | `9add721` (current upstream head) | **CURRENT** |
| `0004-eh-crash-time-memory-probe.patch` | `eh`: crash-time memory probe for diagnosing title asserts — `XCL_CRASH_PROBE` dumps `addr:len` windows as hex + ASCII at an unhandled exception, plus `tls0`/`reg` tokens for the slot-0 TLS chain and the records behind it. `ReadProcessMemory`-based, so a bad address cannot fault the handler; logging only (`+141`) | `9add721`+ | **CURRENT** |

`baseline-only/` holds the two patches that are no longer wanted on current
upstream: they exist only to reproduce the frozen `217ac85` baseline.

## Why `0001` and `0002` are no longer needed

- Upstream `9add721` ships its own `D3D12ConfigureVirtualMemoryX` (accepts the
  descriptor and returns `S_OK`, never fails) and its own `XMemVirtualAlloc`, so
  applying either patch on that head defines an already-present function twice.
- `0001` still applies to `217ac85` and to old `main`, but `git am -3` on
  `9add721` conflicts, and a naive resolution leaves two definitions of the same
  symbol — no conflict marker, so it only fails later at `cargo build`.
- Nothing else is missing at that head: the DirectStorage IIDs the title asks
  for are now upstream as well, so `0003` needs to add only logging.

### 0001 — `D3D12ConfigureVirtualMemoryX` (baseline reproduction)

The struct is `VirtualMemoryConfiguration` (`flags`, `page_table_pages`,
`frame_buffer_preserve_bytes`, `reserved`, two `u64` outputs); the function
validates `flags & 1` and the 33-page (132 MB) page-table limit, zeroes both
outputs, and returns the NT success code `0x10000000` — not `S_OK`, because the
console ABI returns an NTSTATUS here. Null/oversized input returns
`E_INVALIDARG`.

**Evidence — reported, not verified.** The signature and the 33-page limit come
from GDK console documentation. Not yet run when this patch was written; runs 1–6 exercised the
virtual-memory path. This is defensive startup coverage. Upstream's version takes
`*const c_void` and always accepts, which is the behaviour to prefer now.

### 0002 — `XMemVirtualAlloc` (baseline reproduction)

Console alloc types pass through; a zero type becomes `MEM_RESERVE|MEM_COMMIT`;
console protection values (`0x01..0x80`) fall back to `PAGE_READWRITE` while PC
`PAGE_*` values above `0x1000` pass through. Every call is logged.

**Evidence — verified from the shipped artifacts.** The dump's middleware
imports a separate module named `xmem.dll`: `Bink2Scarlett.dll` imports
`XMemAlloc`/`XMemFree`/`XMemVirtualQuery`/`XMemGetWorkingSetStatistics`, and
`XboxBvhBuilder.dll` and `amd_fidelityfx_upscaler_xbox.dll` import `xmem.dll` as
a module (lab report `CONSOLE-DLL-EXPORTS.md`, not shipped in this directory).
Console `xgameruntime.dll` exports `XMemVirtualAlloc`, but xcl's HLE table did
not, so any redirect through the shim would have failed resolution before a
frame was attempted.

Upstream's `XMemVirtualAlloc` now has the same five-argument ABI with more
behaviour than this patch — it decodes `XALLOC_ATTRIBUTES`, ORs in
`MEM_GRAPHICS` for graphics memory types and handles the `page_size == 2` case.

## How to apply

### Current upstream `9add721` — `0003` and `0004`

```sh
git clone https://github.com/ivall/xcl
cd xcl
git checkout 9add721
git am -3 /path/to/patches/xcl/0003-*.patch \
       /path/to/patches/xcl/0004-*.patch
cargo build --release
```

Verified 23 Nov 2026 in a throwaway clone at `9add721`: `git am -3` exited `0`,
applied `+33/-1` and `+141/-0` with no fuzz and no conflict markers, and left a
clean tree. The patched tree builds with
`RUSTFLAGS=-C target-feature=+crt-static cargo build --release -p xcl --bin xcl`
(exit `0`) and `cargo test -p xcl --lib -- eh::tests` passes 4/4.

### Using `0004`

```sh
XCL_CRASH_PROBE=0xADDR:LEN,tls0,reg xcl.exe ...
```

Entries are comma-separated: `addr:len` (hex, `0x` optional, length capped at
256 bytes), or `tls0` / `reg`. Unknown entries are skipped; with the variable
unset the probe does nothing. Output goes to the crash log as one
`probe <addr> (<n>): <hex> |<ascii>|` line per window.

`git am -3` is the intended invocation throughout: the surrounding `d3d12x`,
`dstorage` and `eh` code has moved since the patches were cut, so a plain
`git am` is not enough. Three-way apply is also why the duplicate-definition
failure above is silent — `git am -3` happily auto-merges a second copy of a
function that already exists.

### Baseline `217ac85` — both superseded patches, in order

```sh
git clone https://github.com/ivall/xcl
cd xcl
git checkout 217ac85
git am /path/to/patches/xcl/baseline-only/0001-*.patch \
       /path/to/patches/xcl/baseline-only/0002-*.patch
cargo build --release
```

Order matters: `0001` then `0002`. Verified 23 Nov 2026: both apply cleanly and
the round-trip tree is byte-identical to the originating `lab/first-exe`
(`6ea3c57`) tree — `f8030d96`. Kept only for reproducing the frozen baseline
`lab/nvidia-wave32-v2`.

## Upstream context

- Upstream: `ivall/xcl`, head `9add721`.
- Baseline `217ac85` is the frozen lab baseline `lab/nvidia-wave32-v2` and
  remains pinned there; it is not rebased before the first executable run.
- `0003` and `0004` are written to be upstream-sendable in intent: logging only,
  no lab-specific harness code, no host paths, no game data.

## Provenance and hygiene

- Author on all commits: `lab <lab@local>` (no personal identity embedded).
- No absolute host paths, credentials, tokens, or game data in any patch; only
  Rust source under `crates/xcl/src/`.
- Patch trailer tool version: `2.55.0.windows.5`.

## License

**This directory is GPL-2.0.** The full text is in `COPYING` (GNU General Public
License, Version 2, June 1991). See `NOTICE`. The patches are derivative work of
xcl, which is GPL-2.0, so they carry the same licence.

The enclosing publication repository is MIT for the original code under
`tools/`, and its markdown under `docs/` is CC BY 4.0; neither applies to
anything in this directory.
