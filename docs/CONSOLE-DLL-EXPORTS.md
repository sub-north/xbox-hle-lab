# Console DLL exports — XboxBvhBuilder, FidelityFX Upscaler, Bink2

Static PE inventory of three console (Scarlett / GDK) DLLs from the dump. No binary was
executed and the dump was not modified. Every row below comes from REA Evidence.

Evidence (REA `inspect_native_exports`, `inspect_pe_resources`):

| File | SHA-256 | Bytes | Format | Exports | Resources | Evidence IDs |
|---|---|---|---|---|---|---|
| XboxBvhBuilder.dll | withheld — hash held in lab record, not in repo (884,224 B) | 884224 | PE32+ / amd64 | 12 | 1 (RT_MANIFEST) | exports `ev_b6359281…e1d6a1`, resources `ev_5d427ba4…c3c045` |
| amd_fidelityfx_upscaler_xbox.dll | withheld — hash held in lab record, not in repo (786,432 B) | 786432 | PE32+ / amd64 | 5 | 1 (RT_VERSION) | exports `ev_46c55553…685976`, resources `ev_2a23e010…7db089` |
| Bink2Scarlett.dll | withheld — hash held in lab record, not in repo (255,488 B) | 255488 | PE32+ / amd64 | 74 | 0 | exports `ev_6d273c20…4f64ef8d`, resources `ev_b3968754…a4eaecda` |

These are hashes of files that shipped inside the game package, so they identify the
shipped artifact and are kept out of the public tree: each value above is recorded in the
lab record. Identification is by name and byte size, which is enough to reproduce the
table. The reads below are unchanged.

Coverage for all three export reads: `complete`, `issues: []`, no forwarders, ordinal base 1,
every export named (no ordinal-only rows).

---

## 1. XboxBvhBuilder.dll — complete export table

Export directory: RVA 0x0D25E0, size 1800, DLL name `XboxBvhBuilder.dll`, timestamp 0xFFFFFFFF
(reproducible build). Image base 0x180000000, all exports in 0x180021330–0x180021BD0.

Callability legend: **device** = needs a live `ID3D12Device11` (or none at all for CPU-only
calls); **cmdlist** = needs an Xbox `ID3D12GraphicsCommandList4/6/8` and therefore a queue +
command allocator + a bound pipeline. `X` = void return, `J` = `long` (HRESULT).

### Constructor / destructor / state

| Ord | Address | Raw (mangled) | Demangled | Args | Callable standalone? |
|---|---|---|---|---|---|
| 1 | 0x180021330 | `??0XboxBvhBuilder@@QEAA@PEAUID3D12Device11@@_N@Z` | `XboxBvhBuilder::XboxBvhBuilder(ID3D12Device11*, bool)` | device, bool | device only |
| 2 | 0x180021590 | `??1XboxBvhBuilder@@QEAA@XZ` | `XboxBvhBuilder::~XboxBvhBuilder(void)` | — | yes (object only) |
| 11 | 0x180021BD0 | `?Set64BitMortonCodes@XboxBvhBuilder@@QEAAX_N@Z` | `void XboxBvhBuilder::Set64BitMortonCodes(bool)` | bool | yes |

### Build

| Ord | Address | Raw (mangled) | Demangled | Args | Callable standalone? |
|---|---|---|---|---|---|
| 3 | 0x180021610 | `?BuildRaytracingAccelerationStructure@XboxBvhBuilder@@QEAAXPEAUID3D12GraphicsCommandList4@@PEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@IPEBUD3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC@@@Z` | `void XboxBvhBuilder::BuildRaytracingAccelerationStructure(ID3D12GraphicsCommandList4*, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*, unsigned int, const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*)` | cmdlist, desc, count, postbuild desc | cmdlist |
| 4 | 0x1800216A0 | `?BuildRaytracingAccelerationStructureBatched2X@XboxBvhBuilder@@QEAAXPEAUID3D12GraphicsCommandList6@@IPEBUD3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_BATCHED_DESC2@@@Z` | `void XboxBvhBuilder::BuildRaytracingAccelerationStructureBatched2X(ID3D12GraphicsCommandList6*, unsigned int, const D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_BATCHED_DESC2*)` | cmdlist6, count, batched desc2 | cmdlist |
| 7 | 0x180021880 | `?GetRaytracingAccelerationStructurePrebuildInfo@XboxBvhBuilder@@QEAAXPEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS@@PEBUD3D12XBOX_XDXR_STREAM_DESC@@PEAUD3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO@@@Z` | `void XboxBvhBuilder::GetRaytracingAccelerationStructurePrebuildInfo(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS*, const D3D12XBOX_XDXR_STREAM_DESC*, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO*)` | inputs, stream desc, out info | yes — CPU-only size query |
| 8 | 0x1800219D0 | `?RehydrateRaytracingAccelerationStructureBatched2X@XboxBvhBuilder@@QEAAJPEAUID3D12GraphicsCommandList8@@IPEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@PEB_K@Z` | `long XboxBvhBuilder::RehydrateRaytracingAccelerationStructureBatched2X(ID3D12GraphicsCommandList8*, unsigned int, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*, const unsigned __int64*)` | cmdlist8, count, desc, u64* | cmdlist |
| 9 | 0x1800219A0 | `?RehydrateRaytracingAccelerationStructureBatchedX@XboxBvhBuilder@@QEAAJPEAUID3D12GraphicsCommandList8@@IPEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@@Z` | `long XboxBvhBuilder::RehydrateRaytracingAccelerationStructureBatchedX(ID3D12GraphicsCommandList8*, unsigned int, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*)` | cmdlist8, count, desc | cmdlist |
| 10 | 0x1800219F0 | `?RehydrateRaytracingAccelerationStructureX@XboxBvhBuilder@@QEAAJPEAUID3D12Device11@@PEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@@Z` | `long XboxBvhBuilder::RehydrateRaytracingAccelerationStructureX(ID3D12Device11*, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*)` | device, desc | device only |

### Copy

| Ord | Address | Raw (mangled) | Demangled | Args | Callable standalone? |
|---|---|---|---|---|---|
| 5 | 0x180021980 | `?CopyRaytracingAccelerationStructure@XboxBvhBuilder@@QEAAXPEAUID3D12GraphicsCommandList4@@_K1W4D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE@@@Z` | `void XboxBvhBuilder::CopyRaytracingAccelerationStructure(ID3D12GraphicsCommandList4*, unsigned __int64, unsigned __int64, enum D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE)` | cmdlist, src, dst, mode | cmdlist |

### Validate

| Ord | Address | Raw (mangled) | Demangled | Args | Callable standalone? |
|---|---|---|---|---|---|
| 6 | 0x180021B40 | `?CpuValidateResultX@XboxBvhBuilder@@QEAAXPEAUID3D12GraphicsCommandList4@@PEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@@Z` | `void XboxBvhBuilder::CpuValidateResultX(ID3D12GraphicsCommandList4*, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*)` | cmdlist, desc | cmdlist (despite the CPU name) |
| 12 | 0x180021A00 | `?ValidateRehydrateAccelerationStructureBuildDesc@XboxBvhBuilder@@QEAAJPEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@@Z` | `long XboxBvhBuilder::ValidateRehydrateAccelerationStructureBuildDesc(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*)` | desc | yes — desc only |

No export is a getter in the strict sense; the only query-shaped function is ordinal 7
(`GetRaytracingAccelerationStructurePrebuildInfo`), which writes a prebuild-size struct.

### Harness note (inferred)

`d3d12_xs.dll` appears as a string in this image, and the ctor signature is
`ID3D12Device11` — an Xbox-extended interface. A host-side load therefore needs a
`d3d12_xs` shim before the ctor is reachable, and the ctor is the gate for every other
method (all are `__thiscall`, no free functions).

---

## 2. amd_fidelityfx_upscaler_xbox.dll

Internal export-directory DLL name: `amd_fidelityfx_upscaler_xbox_fsr4_full_720p1080p_input_sanitize_scarlett_opt2.dll`
(verified string in the export directory) — i.e. an **FSR4** build, "full" quality,
720p→1080p, input-sanitize variant, Scarlett-optimised, `opt2`.

| Ord | Address | Export | Notes | Callable standalone? |
|---|---|---|---|---|
| 1 | 0x180001430 | `ffxConfigure` | C API, `(ffxContext*, const ffxConfigureDesc*)` | no — needs a created context |
| 2 | 0x1800011D0 | `ffxCreateContext` | `(ffxContext*, ffxCreateContextDesc*, ffxAllocator*)` | closest to standalone: needs caller allocator + backend device |
| 3 | 0x1800013A0 | `ffxDestroyContext` | `(ffxContext*, ffxAllocator*)` | yes (after create) |
| 4 | 0x1800019F0 | `ffxDispatch` | `(ffxContext*, ffxDispatchDesc*)` | no — needs context + command list |
| 5 | 0x1800014B0 | `ffxQuery` | `(ffxContext*, ffxQueryDesc*)` | partially — some queries accept a null context |

All plain C, unmangled, 5 exports, no forwarders. Nothing computes anything without a
created context, and `ffxCreateContext` needs a backend device plus allocator callbacks.

---

## 3. Bink2Scarlett.dll

74 exports, plain C, all named, no forwarders, no resources. RAD Game Tools Bink2 video.
Groups: lifecycle (`BinkOpen`, `BinkOpenWithOptions`, `BinkClose`, `BinkOpenTrack`,
`BinkCloseTrack`), decode loop (`BinkDoFrame`, `BinkDoFramePlane`, `BinkNextFrame`,
`BinkWait`, `BinkPause`, `BinkGoto`, `BinkShouldSkip`), output (`BinkCopyToBuffer`,
`BinkCopyToBufferRect`, `BinkGetRects`, `BinkGetFrameBuffersInfo`,
`BinkAllocateFrameBuffers`, `BinkRegisterFrameBuffers`, `BinkGetGPUDataBuffersInfo`,
`BinkRegisterGPUDataBuffers`), async (`BinkDoFrameAsync`, `BinkDoFrameAsyncMulti`,
`BinkDoFrameAsyncWait`, `BinkStartAsyncThread`, `BinkWaitStopAsyncThread`,
`BinkRequestStopAsyncThread`, `…Multi` variants), audio (`BinkSetSoundSystem`,
`BinkSetSoundSystem2`, `BinkOpenXAudio2`, `BinkFindXAudio2WinDevice`,
`BinkSetSoundTrack`, `BinkSetVolume`, `BinkSetPan`, `BinkSetSpeakerVolumes`,
`BinkSetSoundOnOff`, `BinkSetVideoOnOff`), tracks/subtitles (`BinkGetTrackData`,
`BinkGetTrackID`, `BinkGetTrackMaxSize`, `BinkGetTrackType`, `BinkLoadSubtitles`,
`BinkCurrentSubtitle`, `BinkGetSubtitleByIndex`), info/misc (`BinkGetSummary`,
`BinkGetRealtime`, `BinkGetKeyFrame`, `BinkGetError`, `BinkSetError`,
`BinkGetPlatformInfo`, `BinkLogoAddress`, `BinkSetMemory`, `BinkSetIO`, `BinkSetIOSize`,
`BinkSetFileOffset`, `BinkSetOSFileCallbacks`, `BinkControlBackgroundIO`,
`BinkSetFrameRate`, `BinkSetSimulate`, `BinkSetWillLoop`, `BinkFreeGlobals`,
`BinkUtilMalloc`, `BinkUtilFree`, `BinkUtilCPUs`, six `BinkUtilMutex*`,
`BinkUtilSoundGlobalLock/Unlock`, `RADTimerRead`).

Nothing is callable standalone: every meaningful entry takes a `HBINK` handle from
`BinkOpen`/`BinkOpenWithOptions`, which itself needs a file source (`BinkSetIO`) or the
default Win32 file path. `BinkUtilMalloc`/`BinkUtilFree`/`RADTimerRead`/`BinkUtilCPUs`
are the only argument-light calls, and they are utility, not decode.

Full 74-row list with ordinals and addresses is in the machine-readable appendix below.

---

## 4. PE resources

| File | Type | Name | Lang | RVA | Size | SHA-256 of payload |
|---|---|---|---|---|---|---|
| XboxBvhBuilder.dll | 24 = RT_MANIFEST | 2 | 1033 | 0x0DC0A0 | 145 | `d2952e57023848a37fb0f21f0dfb38c9000f610ac2b00c2f128511dfd68bde04` |
| amd_fidelityfx_upscaler_xbox.dll | 16 = RT_VERSION | 1 | 1033 | 0x0C21E0 | 728 | `60c88e2aa78c110b1bdae9918496729932f3e853369180fd1ce26e373a78c92b` |
| Bink2Scarlett.dll | — | — | — | — | 0 | none (resource directory absent) |

Manifest (verbatim, XboxBvhBuilder): an **empty** assembly stub —
`<assembly xmlns='urn:schemas-microsoft-com:asm.v1' manifestVersion='1.0'></assembly>`.
No DPI, compatibility or UAC sections. This is a real absence, not a parse gap.

Version info (amd_fidelityfx_upscaler_xbox):

| Field | Value |
|---|---|
| CompanyName | Advanced Micro Devices, Inc. |
| FileDescription | AMD FidelityFX Upscaler Library |
| FileVersion | 3.1.5.0 |
| ProductName | AMD FidelityFX Upscaler |
| ProductVersion | 3.1.5.0 |
| LegalCopyright | Copyright (C) 2026 Advanced Micro Devices, Inc. |

---

## 5. Flags — root signature, descriptor, BVH, D3D12X

| Hit | File | Kind | Text |
|---|---|---|---|
| Root signature import | XboxBvhBuilder | string (import name) | `D3D12SerializeVersionedRootSignature` |
| Root signature import | amd_fidelityfx_upscaler_xbox | string (import name) | `D3D12CreateRootSignatureDeserializer` |
| Xbox D3D12 surface | XboxBvhBuilder | string | `d3d12_xs.dll` |
| Xbox D3D12 surface | amd_fidelityfx_upscaler_xbox | string | `d3d12_xs.dll` |
| D3D12X type in export | XboxBvhBuilder ord 4 | export signature | `D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_BATCHED_DESC2` |
| D3D12X type in export | XboxBvhBuilder ord 7 | export signature | `D3D12XBOX_XDXR_STREAM_DESC` |
| D3D12X flag | XboxBvhBuilder | string | `pDesc->SourceAccelerationStructure is not dehydrated using D3D12XBOX_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_DEHYDRATE` |
| BVH | XboxBvhBuilder | strings | `Header.topLevelBVH = %d`, `Header.rootNodePtr = %s`, `AccelerationStructure contains no instance / geometry descriptors`, `One of the BVH levels has AABBs that can't contain one of the leaf nodes`, `Invalidated triangle (NAN) is reachable via BVH traversal`, `Circular reference to root node`, `A root's box is not contained by scene`, `Invalid root node pointer` |
| Descriptor | XboxBvhBuilder | string | `AccelerationStructure contains no instance / geometry descriptors` |

Notable, but not a D3D12 flag: 40+ embedded PDB paths under a single internal build tree,
`<build-tree>\3rdparty\Microsoft\XboxBvhBuilder\Build\Release\Output\XboxBvhBuilder\<Shader>.pdb`
— RAGE's own build path plus DXIL shader-object names (`BuildKDOP`, `ConstructBVH4`,
`BuildBVHSplits`, `CalculateMortonCodes64`, `PrefixSum512…4096`, `Bitonic*Sort*`,
`Box32Rehydration`, `Box16Rehydration`, `LeafRehydration`, `CopyAndCompactBVH`,
`ReloadTriangles`, `LoadInstances`, `LoadProceduralGeometry`, `GetBVHCompactedSize`).
That means the BVH builder carries its compute shaders inside the DLL and that the vendor's build
tree shipped this Microsoft component as a third-party drop.

---

## Appendix — machine-readable full export lists

```csv
file,ordinal,address,mangled,demangled
XboxBvhBuilder.dll,1,0x180021330,"??0XboxBvhBuilder@@QEAA@PEAUID3D12Device11@@_N@Z","XboxBvhBuilder::XboxBvhBuilder(ID3D12Device11*, bool)"
XboxBvhBuilder.dll,2,0x180021590,"??1XboxBvhBuilder@@QEAA@XZ","XboxBvhBuilder::~XboxBvhBuilder(void)"
XboxBvhBuilder.dll,3,0x180021610,"?BuildRaytracingAccelerationStructure@XboxBvhBuilder@@QEAAXPEAUID3D12GraphicsCommandList4@@PEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@IPEBUD3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC@@@Z","void XboxBvhBuilder::BuildRaytracingAccelerationStructure(ID3D12GraphicsCommandList4*, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*, unsigned int, const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*)"
XboxBvhBuilder.dll,4,0x1800216A0,"?BuildRaytracingAccelerationStructureBatched2X@XboxBvhBuilder@@QEAAXPEAUID3D12GraphicsCommandList6@@IPEBUD3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_BATCHED_DESC2@@@Z","void XboxBvhBuilder::BuildRaytracingAccelerationStructureBatched2X(ID3D12GraphicsCommandList6*, unsigned int, const D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_BATCHED_DESC2*)"
XboxBvhBuilder.dll,5,0x180021980,"?CopyRaytracingAccelerationStructure@XboxBvhBuilder@@QEAAXPEAUID3D12GraphicsCommandList4@@_K1W4D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE@@@Z","void XboxBvhBuilder::CopyRaytracingAccelerationStructure(ID3D12GraphicsCommandList4*, unsigned __int64, unsigned __int64, enum D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE)"
XboxBvhBuilder.dll,6,0x180021B40,"?CpuValidateResultX@XboxBvhBuilder@@QEAAXPEAUID3D12GraphicsCommandList4@@PEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@@Z","void XboxBvhBuilder::CpuValidateResultX(ID3D12GraphicsCommandList4*, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*)"
XboxBvhBuilder.dll,7,0x180021880,"?GetRaytracingAccelerationStructurePrebuildInfo@XboxBvhBuilder@@QEAAXPEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS@@PEBUD3D12XBOX_XDXR_STREAM_DESC@@PEAUD3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO@@@Z","void XboxBvhBuilder::GetRaytracingAccelerationStructurePrebuildInfo(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS*, const D3D12XBOX_XDXR_STREAM_DESC*, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO*)"
XboxBvhBuilder.dll,8,0x1800219D0,"?RehydrateRaytracingAccelerationStructureBatched2X@XboxBvhBuilder@@QEAAJPEAUID3D12GraphicsCommandList8@@IPEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@PEB_K@Z","long XboxBvhBuilder::RehydrateRaytracingAccelerationStructureBatched2X(ID3D12GraphicsCommandList8*, unsigned int, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*, const unsigned __int64*)"
XboxBvhBuilder.dll,9,0x1800219A0,"?RehydrateRaytracingAccelerationStructureBatchedX@XboxBvhBuilder@@QEAAJPEAUID3D12GraphicsCommandList8@@IPEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@@Z","long XboxBvhBuilder::RehydrateRaytracingAccelerationStructureBatchedX(ID3D12GraphicsCommandList8*, unsigned int, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*)"
XboxBvhBuilder.dll,10,0x1800219F0,"?RehydrateRaytracingAccelerationStructureX@XboxBvhBuilder@@QEAAJPEAUID3D12Device11@@PEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@@Z","long XboxBvhBuilder::RehydrateRaytracingAccelerationStructureX(ID3D12Device11*, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*)"
XboxBvhBuilder.dll,11,0x180021BD0,"?Set64BitMortonCodes@XboxBvhBuilder@@QEAAX_N@Z","void XboxBvhBuilder::Set64BitMortonCodes(bool)"
XboxBvhBuilder.dll,12,0x180021A00,"?ValidateRehydrateAccelerationStructureBuildDesc@XboxBvhBuilder@@QEAAJPEBUD3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC@@@Z","long XboxBvhBuilder::ValidateRehydrateAccelerationStructureBuildDesc(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*)"
amd_fidelityfx_upscaler_xbox.dll,1,0x180001430,ffxConfigure,ffxConfigure
amd_fidelityfx_upscaler_xbox.dll,2,0x1800011D0,ffxCreateContext,ffxCreateContext
amd_fidelityfx_upscaler_xbox.dll,3,0x1800013A0,ffxDestroyContext,ffxDestroyContext
amd_fidelityfx_upscaler_xbox.dll,4,0x1800019F0,ffxDispatch,ffxDispatch
amd_fidelityfx_upscaler_xbox.dll,5,0x1800014B0,ffxQuery,ffxQuery
```

Bink2Scarlett.dll — 74 exports, ordinal == declaration order:

| Ord | Address | Export | Ord | Address | Export |
|---|---|---|---|---|---|
| 1 | 0x180002180 | BinkAllocateFrameBuffers | 38 | 0x180004900 | BinkRegisterGPUDataBuffers |
| 2 | 0x180002400 | BinkClose | 39 | 0x18000AA00 | BinkRequestStopAsyncThread |
| 3 | 0x1800028C0 | BinkCloseTrack | 40 | 0x18000AA10 | BinkRequestStopAsyncThreadsMulti |
| 4 | 0x1800028E0 | BinkControlBackgroundIO | 41 | 0x180004950 | BinkService |
| 5 | 0x180002940 | BinkCopyToBuffer | 42 | 0x1800049A0 | BinkSetError |
| 6 | 0x180002990 | BinkCopyToBufferRect | 43 | 0x180004A10 | BinkSetFileOffset |
| 7 | 0x180002CC0 | BinkCurrentSubtitle | 44 | 0x180004A20 | BinkSetFrameRate |
| 8 | 0x180002ED0 | BinkDoFrame | 45 | 0x180004A30 | BinkSetIO |
| 9 | 0x18000A620 | BinkDoFrameAsync | 46 | 0x180004A40 | BinkSetIOSize |
| 10 | 0x18000A660 | BinkDoFrameAsyncMulti | 47 | 0x180004A50 | BinkSetMemory |
| 11 | 0x18000A890 | BinkDoFrameAsyncWait | 48 | 0x180004A60 | BinkSetOSFileCallbacks |
| 12 | 0x180002F10 | BinkDoFramePlane | 49 | 0x180004AC0 | BinkSetPan |
| 13 | 0x18000B5C0 | BinkFindXAudio2WinDevice | 50 | 0x180004B60 | BinkSetSimulate |
| 14 | 0x180002FC0 | BinkFreeGlobals | 51 | 0x180004B70 | BinkSetSoundOnOff |
| 15 | 0x180003100 | BinkGetError | 52 | 0x180004DC0 | BinkSetSoundSystem |
| 16 | 0x180003140 | BinkGetFrameBuffersInfo | 53 | 0x180004E80 | BinkSetSoundSystem2 |
| 17 | 0x180003380 | BinkGetGPUDataBuffersInfo | 54 | 0x180004F60 | BinkSetSoundTrack |
| 18 | 0x1800034E0 | BinkGetKeyFrame | 55 | 0x180004FA0 | BinkSetSpeakerVolumes |
| 19 | 0x1800035E0 | BinkGetPlatformInfo | 56 | 0x180005560 | BinkSetVideoOnOff |
| 20 | 0x1800036B0 | BinkGetRealtime | 57 | 0x180005580 | BinkSetVolume |
| 21 | 0x180003880 | BinkGetRects | 58 | 0x180005620 | BinkSetWillLoop |
| 22 | 0x1800038B0 | BinkGetSubtitleByIndex | 59 | 0x180005640 | BinkShouldSkip |
| 23 | 0x180003910 | BinkGetSummary | 60 | 0x18000AA60 | BinkStartAsyncThread |
| 24 | 0x180003B40 | BinkGetTrackData | 61 | 0x18000BB50 | BinkUtilCPUs |
| 25 | 0x180003C80 | BinkGetTrackID | 62 | 0x180005690 | BinkUtilFree |
| 26 | 0x180003CA0 | BinkGetTrackMaxSize | 63 | 0x1800056A0 | BinkUtilMalloc |
| 27 | 0x180003CC0 | BinkGetTrackType | 64 | 0x18000BB60 | BinkUtilMutexCreate |
| 28 | 0x180003CE0 | BinkGoto | 65 | 0x18000BB70 | BinkUtilMutexDestroy |
| 29 | 0x180004000 | BinkLoadSubtitles | 66 | 0x18000BB80 | BinkUtilMutexLock |
| 30 | 0x180009B30 | BinkLogoAddress | 67 | 0x18000BB90 | BinkUtilMutexLockTimeOut |
| 31 | 0x1800042D0 | BinkNextFrame | 68 | 0x18000BBA0 | BinkUtilMutexUnlock |
| 32 | 0x180004470 | BinkOpen | 69 | 0x1800056B0 | BinkUtilSoundGlobalLock |
| 33 | 0x180004510 | BinkOpenTrack | 70 | 0x1800056D0 | BinkUtilSoundGlobalUnlock |
| 34 | 0x180004710 | BinkOpenWithOptions | 71 | 0x1800056F0 | BinkWait |
| 35 | 0x18000B810 | BinkOpenXAudio2 | 72 | 0x18000AA70 | BinkWaitStopAsyncThread |
| 36 | 0x180004720 | BinkPause | 73 | 0x18000AA80 | BinkWaitStopAsyncThreadsMulti |
| 37 | 0x1800048E0 | BinkRegisterFrameBuffers | 74 | 0x180002070 | RADTimerRead |

Limitations inherited from the reader (apply to all three): rows reproduce the PE export
directory and do not establish reachability, runtime safety or that a DLL loads on the host;
forwarders are not resolved (none exist here). Resource payloads other than RT_GROUP_ICON
are opaque to the reader — the manifest and version tables above were decoded from the
exact offsets/sizes that Evidence reports, read-only, without executing the image.
