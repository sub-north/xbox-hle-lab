# How the runtime binds its API without exports

The runtime serves the console runtime DLL's API from a **name → address map**, built at compile
time by a macro and read by name at call time. The runtime binary has no export directory. That is
correct, and this page shows why, how a title reaches the map anyway, and how to tell that it
worked.

Source references are file:line in the GPL-2.0 runtime, paths relative to `crates/xcl/src`; the
API-binding module is `hle/xgameruntime/api.rs`. Line numbers track runtime commit `6ea3c57`, not
the frozen baseline, so they can drift and should be re-checked on any other checkout. The
export-directory figures are measured with a PE reader on three builds of the runtime, not by
searching strings.

## 1. The mechanism — a map, a registration, a fake handle

**The list.** `hle_exports!` (`hle/mod.rs:22-26`) expands to
`vec![(stringify!($name), $name as usize), …]` — the Rust function name *is* the exported name, so
there is no second literal to drift out of sync. `HleModule` stores the result as
`exports: HashMap<&'static str, usize>` (`hle/mod.rs:28-36`), filled in `new()` (`hle/mod.rs:42-44`).
The runtime module's list starts at `hle/xgameruntime/mod.rs:22-30`; its DLL names route there at
`hle/mod.rs:104-107`.

**The registration.** `host_main` calls `hle::init` (`host.rs:33`), which sets a `OnceLock`
(`hle/mod.rs:119-121`) from `build()` (`hle/mod.rs:75-117`); the runtime module is added at
`hle/mod.rs:78`. Registration happens once at startup; in the observed order it precedes the title
being mapped.

**The lookup.** `resolve_hle` (`resolver.rs:106-110`) is the whole answer: `m.exports.get(name)`.
A miss falls through to a cross-module search (`hle/mod.rs:147-151`, for functions the console
hosts elsewhere), then the module's system-DLL fallback list, then an auto-stub
(`resolver.rs:175-179`).

**The handle.** `modules::hle_handle` (`modules.rs:99-115`) commits one 4 KiB page, writes `MZ`,
and caches it per canonical module name; `hle_name_by_handle` (`modules.rs:117-120`) reverses it.
Nothing else is written — no NT headers, no section table, no export directory. The pages exist so
that the value is a readable pointer beginning with `MZ` for callers that sanity-check it; **no image
is mapped into them**, and no lookup ever consults them.

## 2. The call path a title takes

| # | Title calls | Runtime entry | Path |
|---|---|---|---|
| 1 | `LoadLibraryW` | `kernelx.rs:658-660` | `load_library_impl` (`kernelx.rs:640-656`) → `resolver::load_library` (`resolver.rs:279-293`) → `hle_handle` (`modules.rs:99-115`); logged at `kernelx.rs:647`. `LoadLibraryExW` shares it (`kernelx.rs:671-676`) |
| 2 | `GetProcAddress` | `kernelx.rs:689-703` | `resolver::get_proc_address` (`resolver.rs:323-333`) → `resolve_hle` (`resolver.rs:106-110`). A miss returns `NULL` + `ERROR_PROC_NOT_FOUND` (`kernelx.rs:698-701`), deliberately no stub, so optional-API probing works (`resolver.rs:321-322`) |
| 3 | `InitializeApiImplEx2` | `hle/xgameruntime/api.rs:160-166` | `pc_init` (`hle/xgameruntime/api.rs:168-181`) forwards to the PC runtime's own initialiser |
| 4 | `QueryApiImpl(clsid, iid)` | `hle/xgameruntime/api.rs:205-235` | refused classes → `REGDB_E_CLASSNOTREG` (`:211-214`); ours → `S_OK` (`:215-222`); else the PC runtime's object (`:224-229`); else `missing` (`:230-233`) |
| 5 | call through a vtable slot | `hle/xgameruntime/api.rs:341-370` | `call_target` (`:341-345`), shims generated per slot (`:347-370`) |

**Slots are filled by name, once.** The GDK table lists `(function, class id, interface id, slot)`
rows (`hle/xgameruntime/gdk_table.rs:5-6`), extracted from the public PC GDK's thunk library
(`gdk_table.rs:1-3`, `hle/xgameruntime/api.rs:19-24`). `layout()`
(`hle/xgameruntime/api.rs:391-429`) groups interface ids that do not share slots and resolves each
implemented function to a slot *by name* (`:417-423`); `object()`
(`hle/xgameruntime/api.rs:372-387`) writes those pointers into the vtable and fills every other slot
with a logging stub. The title's indirect call is then a plain indirect call: no name resolution
happens at that moment.

Both surfaces exist. The by-name exports in `hle/xgameruntime/mod.rs:22-30` serve titles that import
the functions directly; the thunk protocol above is the path the logged runs take
(`hle/xgameruntime/api.rs:1-5`).

## 3. Why an empty export directory is correct

**Measured** (PE reader, three builds): export directory RVA 0, size 0, no `.edata`, 0 exports,
0 forwarders.

Three reasons this is the right shape:

1. The runtime is a host **executable**, not a DLL. Its `[lib]` declares no `crate-type`
   (`crates/xcl/Cargo.toml:8-10`) and the shipping artifact is the binary (`:12-14`); Rust does not
   emit an export directory for an executable unless asked to.
2. The title never runs under the Windows loader. The runtime maps it and answers its imports itself
   (`resolver.rs:181-226`), so at no point does Windows look up a name in the runtime's image.
3. Resolution is the map, at call time (`resolver.rs:106-110`). An export table would be a second,
   unused copy of the same list.

So **any tool that reports "0 exports" for the runtime binary is reading a table nothing
consumes.** It is not a defect, not a build misconfiguration, and not a reason to add a `.def`. The
useful question is not "what does the image export" but "what is in the map" — section 4.

This does not generalise. A DLL that shipped with the game is a real DLL whose names are read from
its PE export directory; those tables are analysed separately (`CONSOLE-DLL-EXPORTS.md`).

## 4. Verifying that binding happened

Grep the run log, in this order:

| Pattern | Emitted at | Level | Means |
|---|---|---|---|
| `LoadLibrary(xgameruntime.dll) → 0x` | `kernelx.rs:647` | debug | the name was routed to the runtime module and a handle returned. `→ NULL` instead means no handle was produced — no route, or a failed load (`kernelx.rs:651`) |
| `InitializeApiImplEx2(` and `title goes through the GDK thunks` | `hle/xgameruntime/api.rs:161` | info | the title entered the thunk protocol |
| `XGameRuntime: QueryApiImpl ` | `hle/xgameruntime/api.rs:1064` | info | one line per `(clsid, iid)` with a verdict: `XCL`, `PC`, `missing`, or `<name>: unavailable (offline)` |

A healthy binding looks like: the `LoadLibrary` line, then `InitializeApiImplEx2`, then at least one
`QueryApiImpl … → …: XCL`. In a real run this appears as a short run of `QueryApiImpl` lines, each
naming a different API family.

Two reading notes:

- Lines are deduplicated per `(clsid, iid)` by a `SEEN` map
  (`hle/xgameruntime/api.rs:1058-1067`). One line is the whole record for that pair; a repeated
  class does not log again.
- `LoadLibrary` is debug-level. A log level that filters debug removes that line while binding still
  works, so judge on the `QueryApiImpl` lines first and treat the `LoadLibrary` line as
  confirmation.

## 5. The one open gap

**Inferred:** a wide literal for the runtime DLL's name sits in the title's read-only data, beside
other module-name literals.

**Inferred:** the instruction that passes it to `LoadLibraryW`. No RIP-relative `lea` and no
pointer-table entry resolving to that literal was found, so the caller is inferred from where the
string lives, not proven. The literal may be the argument to the call, or data used for something
else entirely (a log, a probe list).

The hit log narrows this but does not close it: it proves *some* caller passed that name, not which
instruction did, and not that the literal was the source.

Either observation closes it:

- **Static** — an x64 `lea` whose RIP-relative target is the literal's address, inside a function
  that also calls `LoadLibraryW`; or a pointer in `.data`/`.rdata` whose value is that address; or a
  delay-import/thunk record naming the runtime DLL.
- **Dynamic** — stop at the runtime's `LoadLibraryW` shim and record the return address and the name
  pointer. A return address inside the title image with an argument equal to the literal's address
  closes it; a return address in another module disproves the inference.

Neither was run. Until one is, treat "the title loads the runtime DLL from that literal" as
inferred.

## 6. Checking your own build

**Source.** The list and the route are independent, and both must be present:

```text
grep -n "hle_exports!" -A40 crates/xcl/src/hle/xgameruntime/mod.rs   # the names
grep -n "XGAMERUNTIME" crates/xcl/src/hle/mod.rs                     # module + route
```

**Log.**

```text
grep -E "LoadLibrary\(xgameruntime|InitializeApiImpl|QueryApiImpl" xcl.log
```

| Result | Verdict |
|---|---|
| `LoadLibrary(xgameruntime.dll) → 0x…`, `InitializeApiImplEx2`, ≥1 `QueryApiImpl … → …: XCL` | healthy — routed, entered, answered |
| `LoadLibrary(xgameruntime.dll) → NULL` | unhealthy — the name is not routed to the module |
| `GetProcAddress(…, …) → NULL (not implemented)` (`resolver.rs:331`) | unhealthy — a name was requested that the map does not hold |
| `QueryApiImpl … → …: missing` (`hle/xgameruntime/api.rs:231`) | unhealthy — the class was asked for and neither the map nor the PC runtime answered |
| no `QueryApiImpl` lines at all | inconclusive — the title never reached the thunk protocol; this says nothing about the map |
| a PE tool says `0 exports` | **neither** — see section 3 |

## Not claimed

- That the GDK table is complete for every GDK version. `hle/xgameruntime/api.rs:21-24` states the
  policy for an interface id the table does not know (the class's main vtable); no independent
  coverage measure is given here.
- That every class the title asks for is implemented. Refusals and `PC` handoffs are by design
  (`hle/xgameruntime/api.rs:7-17`).
- Anything about the title-side call site — section 5 is open, not settled.
