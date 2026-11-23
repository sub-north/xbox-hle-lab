# GHIDRA-BVH — `XboxBvhBuilder.dll` root-signature / XMem path

Static, read-only analysis of a shipped Xbox Series binary: never executed, dump never modified (only read), Ghidra 12.1.4 PUBLIC `analyzeHeadless`, x86:LE:64. No decompiler output is reproduced here — see the closing note.
Labels: **[observed]** = read directly from bytes/decompiler/listing (our *verified* level); **[inferred]** = reasoning over observations (our *reported* level).
## 0. Artifact
| Field | Value |
|---|---|
| Path | `<LAB>/dump/<vendor-package>/XboxBvhBuilder.dll` |
| Size / MD5 | 884,224 bytes / MD5 withheld — held in lab record, not in repo |
| SHA-256 | withheld — held in lab record, not in repo (884,224 B); recomputed, matches REA |
| Image base | `0x180000000` **[observed]** |
| Sections | `.text 0x180001000–0x180023FFF` (139 KB), `.rdata 0x180024000–0x1800D39FF` (700 KB), `.data`, `.pdata`, `.xbld`, `.rsrc`, `.reloc` |
| Functions / exports / toolchain | 464 functions, 12 exports (entry RVAs in §4); `.xbld` records GXDK `April 2025 GXDK Update 5`, product build `10.0.26100.4125` **[observed]** |
Middleware BVH/XDXR library: `.rdata` holds 52 root-signature blobs, 52 DXBC shaders and the pipeline-name strings; only 139 KB is code.
## 1. Which functions call the two IAT slots?
| Import | IAT slot | Caller | Call site(s) |
|---|---|---|---|
| `d3d12_xs.dll!D3D12SerializeVersionedRootSignature` | `0x180024250` | `FUN_18000c2d0` (`0x18000C2D0`) | `0x18000C32F` |
| `xmem.dll!XMemVirtualQuery` | `0x180024260` | `FUN_180019BB0` (`0x180019BB0`) | `0x180019C69`, `0x180019CCB`, `0x180019D9D`, `0x180019DF9` |
Exactly **one** caller each: the serializer has a single call site in the whole image, the memory query one wrapper with four call sites. No thunks were generated — both are indirect calls through the IAT pointer (`02_import_xrefs.txt`) **[observed]**.
## 2. The serializer caller
### 2.1 `FUN_18000c2d0` — "create root signature from desc" **[observed]**
Copies a 48-byte versioned desc from argument 2 into a stack temporary, ORs `0x03000000` into the `Flags` field at `+0x28`, calls `D3D12SerializeVersionedRootSignature` with a null error blob, and on failure emits a fixed diagnostic through `OutputDebugStringW` naming the failed API; on success it reads blob `GetBufferSize`/`GetBufferPointer` (vtable `+0x20`/`+0x18`), calls `CreateRootSignature` (device vtable `+0x80`, `NodeMask = 1`) and then the `SetName`-style call (vtable `+0x30`).
`0x1800CAAE8` holds `IID_ID3D12RootSignature`, GUID `c54a6b66-72df-4ee8-8be5-a946a1429214` **[observed bytes; IID identified from public headers]**.

| # | Register | Meaning |
|---|---|---|
| 1 | RCX | device object (vtable `+0x80` = `CreateRootSignature`) |
| 2 | RDX | `const D3D12_VERSIONED_ROOT_SIGNATURE_DESC*` (48 bytes, copied) |
| 3 | R8 | `ID3D12RootSignature**` out |
| 4 | R9 | `LPCWSTR` debug name (vtable `+0x30`); meanings **[inferred from the call sequence]** |
### 2.2 Desc layout recovered from the constructor **[observed]**
`XboxBvhBuilder::XboxBvhBuilder` (`0x180021330`) builds the desc on the stack and passes it at `0x18002150A`; each field is one immediate store just before the call (`03_callsites.txt`). Records are 0x20 bytes: `Type@+0`, `Flags@+4` (**never written**), union `@+8`, `Visibility@+0x18`.

| Offset | Field | Value in ctor |
|---|---|---|
| `+0x00` | `Version` | `2` (`D3D_ROOT_SIGNATURE_VERSION_1_1`) |
| `+0x08` | `NumParameters` | `3` |
| `+0x10` | `pParameters` | `RSP+0xA0` |
| `+0x18` / `+0x20` | `NumStaticSamplers` / `pStaticSamplers` | `0` / `NULL` |
| `+0x28` | `Flags` | `0` → `0x03000000` (OR at `0x18000c30e`) |
Parameters: `p0` = UAV root descriptor `u0` space 0, `p1` = UAV root descriptor `u1` space 0, `p2` = 16 dwords of 32-bit constants at `b0`, all visibility ALL. **Descriptor ranges: none. Static samplers: none** — no `D3D12_DESCRIPTOR_RANGE` construction exists in the image, and `NumStaticSamplers`/`pStaticSamplers` are `0`/`NULL` in all 16 desc initialisers.
**Flags** are `0` everywhere and forced to `0x03000000` (two console bits, `0x01000000|0x02000000`, absent from public `D3D12_ROOT_SIGNATURE_FLAGS`); every RTS0 blob (§5) carries the same value **[observed value; console-bit meaning inferred]**. Quirk **[observed, consequence inferred]**: `Version = 2` makes the runtime read `D3D12_ROOT_PARAMETER1.Flags` at `+4`, but nothing stores there — it works only if those stack bytes happen to be zero.
### 2.3 The whole family — 16 descs, 10 functions **[observed]**
`FUN_18000c2d0` is called 16 times; each site passes its own desc and PSO name:

| Desc builder (RVA) | Desc name (`LPCWSTR`, RVA) | Params | Call site |
|---|---|---|---|
| `XboxBvhBuilder::XboxBvhBuilder` `0x180021330` | `XDXR CopyShaders` (`0x1800254C8`) | 3 | `0x18002150A` |
| `FUN_1800144A0` | `XDXR BVHDecompressor` (`0x1800CBB78`) | 4 | `0x18001458A` |
| `FUN_18000D850` | `XDXR MortonCodesCalculator` (`0x1800CD768`) | 6 | `0x18000D998` |
| `FUN_18000D850` | `XDXR LoadInstancesPass` (`0x1800CD410`) | 4 | `0x18000DAF7` |
| `FUN_18000D850` | `XDXR SplitInstancesPass` (`0x1800CDB68`) | 5 | `0x18000DBFD` |
| `FUN_18000D850` | `XDXR ConstructHierarchyPass` (`0x1800CD388`) | 4 | `0x18000DD18` |
| `FUN_18000D850` | `XDXR TreeletReorder` (`0x1800CDB98`) | 7 | `0x18000DEA7` |
| `FUN_18000D850` | `XDXR ApexPointMapComputer` (`0x1800CD108`) | 3 | `0x18000DF88` |
| `FUN_18001A6B0` | `XDXR BitonicSort` (`0x1800CD1B0`) | 4 | `0x18001A810` |
| `FUN_18001B310` | `XDXR BVH4Builder` (`0x1800CD2F8`) | 9 | `0x18001B499` |
| `FUN_18001C160` | `XDXR LoadPrimitivesPass` (`0x1800CD498`) | 9 | `0x18001C2F9` |
| `FUN_18001D400` | `XDXR PostBuildInfoQuery` (`0x1800CD810`) | 4 | `0x18001D4B2` |
| `FUN_18001DA20` | `XDXR RadixSortCountAndAlloc` (`0x1800CD878`) | 7 | `0x18001DD25` |
| `FUN_18001DA20` | `XDXR RadixSortPrefixSum` (`0x1800CD8B0`) | 3 | `0x18001DDED` |
| `FUN_18001F530` | `XDXR RearrangeElementsPass` (`0x1800CDAD8`) | 8 | `0x18001F680` |
| `FUN_180020270` | `XDXR UtilityShaders` (`0x1800CDBE8`) | 3 | `0x1800203B2` |
Parameter types are only `1` (32-bit constants), `3` (SRV/CBV root descriptor) and `4` (UAV root descriptor); no `0` (descriptor table) was observed. Worked example, `FUN_1800144A0` **[observed]**: 19 dwords of constants, UAV `u0`, UAV `u1`, SRV `t0` — `NumParameters = 4`, name pointer `0x1800CBB78`.
Each desc is followed by `FUN_18000C470` = `CreateComputePipelineState` (device vtable `+0x1F0`) with `{pRootSignature, CS={bytecode,len}, NodeMask=0, CachedPSO=null, Flags=0}`; the 12 named DXBC blobs are the compute shaders. This table also serves as Appendix A.
### 2.4 The `XMemVirtualQuery` caller — `FUN_180019BB0` **[observed]**
Pure validator for `ValidateRehydrateAccelerationStructureBuildDesc` (`0x180021A00`): four pointers checked against a 64-byte `MEMORY_BASIC_INFORMATION`-shaped buffer — non-zero result, memory state `& 3`, protection bit `0x1000`, two console bits `0x600000000` (state at MBI `+0x20`, protection at `+0x30`) **[inferred from bit use]**.

| Site | Object checked | Contract on failure |
|---|---|---|
| `0x180019C69` | `pDesc->SourceAccelerationStructureData` | non-null, CPU-readable memory |
| `0x180019CCB` | `pDesc->DestAccelerationStructureData` | GPU memory |
| `0x180019D9D` | `pGeoDesc->Triangles.IndexBuffer` | GPU-readable memory |
| `0x180019DF9` | `pGeoDesc->Triangles.VertexBuffer` | GPU-readable memory |
It also enforces the rehydration contract before querying: source must carry `D3D12XBOX_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_DEHYDRATE` (`0x1000000`), and `Inputs.Flags` must not include `PERFORM_UPDATE` (`0x20` at `pDesc+0xC`).
## 3. Which exports reach each path?
### 3.1 Root-signature path (serializer)
Only the **constructor** reaches it; no `Build*` and no `CpuValidate*` calls `FUN_18000c2d0`, they use the objects it created. Path: ctor `0x180021330` → `0x18002150A` `FUN_18000C2D0`; → `0x18002139C` `FUN_18000D850` (sites `0x18000D998`, `0x18000DAF7`, `0x18000DBFD`, `0x18000DD18`, `0x18000DEA7`, `0x18000DF88`); → `0x1800213D7` `FUN_1800144A0` (`0x18001458A` plus 4× `FUN_18000C470`); → `0x180021563` `FUN_18001D400` (`0x18001D4B2`).
The other desc builders (`FUN_18001A6B0`, `FUN_18001B310`, `FUN_18001C160`, `FUN_18001DA20`, `FUN_18001F530`, `FUN_180020270`) list `FUN_18000D850@0x18000D850` as their only caller **[observed]**, so one `XboxBvhBuilder(device, false)` creates **16 root signatures** and 43 `CreateComputePipelineState` call sites from the embedded DXBC library — the expensive part of the constructor.
### 3.2 `XMemVirtualQuery` path
`ValidateRehydrateAccelerationStructureBuildDesc` (`0x180021A00`) → `FUN_180019BB0` (`0x180019BB0`) → four `XMemVirtualQuery` calls. `CpuValidateResultX` (`0x180021B40`) reaches neither slot: it calls `FUN_1800090F0` (CPU-side BVH check) and frees a std::string.
## 4. Calling convention / arguments for a harness
All 12 methods are `__thiscall` on x64 (`this` in RCX, then RDX, R8, R9, stack after); types come from the **mangled export names**, not guesses **[observed]**. RVAs and names only.

| RVA | Name | Arguments (types) |
|---|---|---|
| `0x180021330` | `XboxBvhBuilder::XboxBvhBuilder` | `this` (caller-allocated, 0x18 bytes used), `ID3D12Device11*`, `bool skipCore` |
| `0x180021610` | `XboxBvhBuilder::BuildRaytracingAccelerationStructure` | `this`, `ID3D12GraphicsCommandList4*`, `const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*`, `UINT NumDescs`, stack `const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*` |
| `0x1800216A0` | `XboxBvhBuilder::BuildRaytracingAccelerationStructureBatched2X` | `this`, `ID3D12GraphicsCommandList6*`, `UINT`, `const D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_BATCHED_DESC2*` |
| `0x180021980` | `XboxBvhBuilder::CopyRaytracingAccelerationStructure` | `this`, `ID3D12GraphicsCommandList4*`, `UINT64 src`, `UINT64 dst`, `COPY_MODE` |
The constructor is mandatory first and `skipCore` must be FALSE: TRUE skips `FUN_18000D850` and leaves `this->[0]` null. `BuildRaytracingAccelerationStructure` copies the 0x30-byte desc and tail-calls the batched path with `NumDescs = 1`; the cheapest smoke test is `CopyRaytracingAccelerationStructure`, which does work only when `mode == 1` (`CLONE`).
Prerequisites **[inferred]**: the device must be a GDK `ID3D12Device11`; `CreateRootSignature` at vtable `+0x80` and `CreateComputePipelineState` at `+0x1F0` do not match the public PC `ID3D12Device` layout, and both the `0x03000000` flags and the XDXR DXBC are console-only, so a stock PC `d3d12.dll` is not a valid target; use a guest harness under xcl.
## 5. Embedded root-signature blobs (`RTS0`)
52 blobs, all in `.rdata`, `0x180025814`–`0x1800C9794`, 7,152 bytes total **[observed]**. Header, offsets from the magic **[observed, decode inferred from consistent sizes]**: `+0x00` magic `RTS0`; `+0x04` body size; `+0x08` version = 2 (root signature 1.1); `+0x0C` `NumParameters`; `+0x10` offset to the parameter array (always `0x18`, relative to `+0x08`); `+0x14` `NumStaticSamplers` = 0; `+0x18` offset to static samplers (= body size, i.e. empty); `+0x1C` `Flags` = `0x03000000`; `+0x20` parameters, 12 bytes each `{Type, 0, offset-to-payload}`, then payloads, 12 bytes each `{reg, space, count/0}`.

| Size | Count | Params | Types seen |
|---|---|---|---|
| 96 | 22 | 3 | `4,4,1` or `3,4,1` (matches the ctor's CopyShaders signature) |
| 120 | 14 | 4 | `1,4,4,3`, `1,1,4,4` or `4,4,4,1` |
| 144 / 168 | 1 / 2 | 5 / 6 | `4,4,4,4,1` / `1,4,4,4,4,4` |
| 192 / 216 | 4 / 2 | 7 / 8 | seven UAVs then constants |
| 240 | 7 | 9 | eight or five UAVs, then `3,3,3` or constants |
The identity `size = 0x18 + 24 * NumParameters` holds for all 52. Across them: 161 UAV, 58 constant-buffer and 27 SRV/CBV parameters, **no descriptor tables**, no samplers, every blob with the same `0x03000000` flags.
**These blobs are dead data in this image**: no code reference and no pointer to any of them exists (0 hits for 8-byte VAs and for 4-byte RVAs). They sit interleaved with DXBC shaders and pipeline-name strings, i.e. a compiled shader-library artefact **[inferred]**. Not found: `RTS1`, `ROOT`, `SFI0`; `DXBC` appears 52× (the shader blobs), `D3D12` only in error strings.
## 6. Method, limits, environment
* Ghidra 12.1.4 PUBLIC, `analyzeHeadless`, auto-analysis, `x86:LE:64:default`, image base `0x180000000`; scripts `CollectBvh.java` (companion to this report) and `DumpAsm.java`. Raw outputs in `%TEMP%\opencode\bvh-ghidra\out\` (`01_exports.txt` … `07_all_functions.txt`, all 464 functions) stay local and are not published.
* Struct field meanings (`D3D12_ROOT_PARAMETER_TYPE`, MBI bits) come from public headers; only the binary's own layout was read. Not verified: runtime behaviour (not executed), GDK-specific flag semantics, whether `0x03000000` survives the serializer on the console.

**Environment repairs (host, not the dump)** — the installed Ghidra was pruned: 18 files listed in `Module.manifest` were missing and 51 module directories had no `Module.manifest` at all (`ModuleUtilities.isModuleDirectory` = "`Module.manifest` exists", verified in bytecode), so every launcher aborted with a missing-module error. Repaired additively: created empty `Module.manifest` in 2 Framework, 14 Features and 37 Processors/GPL module dirs; restored `commons-lang3-3.20.0`, `commons-codec-1.18.0`, `gson-2.13.2`, `jgrapht-core-1.5.1`, `jgrapht-io-1.5.1`, `antlr-3.5.2`, `slf4j-api-1.7.25`, `slf4j-nop-1.7.25`, `log4j-slf4j-impl-2.12.1` from Maven Central into the paths the manifests expect. Neither change touches the target binary; the `SevenZipRecognizer`/`PkzipRecognizer` log errors are residual install damage and do not affect PE analysis.
### Reproduction
Run `analyzeHeadless.bat` with JDK 21, project `BVH5`, importing `XboxBvhBuilder.dll` with processor `x86:LE:64:default`, `-scriptPath <scripts>` and the post-script `CollectBvh.java <out>`; the export, xref and callsite tables above come out of that run. Filter the log through `Select-String` — the `SLF4J: multiple bindings` banner goes to stderr and makes PowerShell report exit code 1 even on success.
## Appendix — the 52 RTS0 blob RVAs
In file order: `0x180025814`, `0x180026954`, `0x180029484`, `0x18002C2DC`, `0x180035834`, `0x1800371A4`, `0x180038F34`, `0x18003ABFC`, `0x18003BCFC`, `0x18003D244`, `0x18003F2E4`, `0x18004150C`, `0x180042964`, `0x1800448C4`, `0x180046A24`, `0x18004C7F4`, `0x180054D74`, `0x18005BE5C`, `0x18005DEFC`, `0x18005FD6C`, `0x180072064`, `0x180073C34`, `0x18007EE3C`, `0x18008102C`, `0x18008B324`, `0x18008E724`, `0x180091AAC`, `0x180092F2C`, `0x180093F34`, `0x180095F94`, `0x180099E74`, `0x18009B544`, `0x18009CB94`, `0x18009E1E4`, `0x18009F644`, `0x1800A0A14`, `0x1800A1D94`, `0x1800A30F4`, `0x1800A48C4`, `0x1800A6544`, `0x1800AEE44`, `0x1800B3BF4`, `0x1800B4CCC`, `0x1800B5CF4`, `0x1800B6F4C`, `0x1800B83DC`, `0x1800B93B4`, `0x1800BA504`, `0x1800C1A34`, `0x1800C6374`, `0x1800C78F4`, `0x1800C9794`.

## What is deliberately not included
Decompiled and disassembled output is **not republished** here. This document carries only conclusions, RVAs, symbol names, offsets, counts and the SHA-256 of the DLL — a hash identifies a file, it is not a reproduction. The instruction listings, pseudocode and string dumps behind these conclusions stay in our local working directory; readers should reproduce them locally, from their own licensed copy of the DLL, using the recipe in §6. We cannot grant rights to the vendor's code, and our CC BY 4.0 covers only the prose above.
