# Run log

Every guest run so far, including the ones that taught us nothing. The value is in the closed doors:
this table is meant to stop someone (including us) from spending a boot on a question that is already
answered or was never actually asked.

A row is one boot with one runtime build and one change under test: the commit it ran, what differed
from the row above, the first failure that stopped it, the log that backs the claim, and what the run
settled. "First failure RVA" is where the title stopped, not a diagnosis. A run that reproduces the
previous failure without changing the reason is a **null test** — it carries no new information about
the environment, and repeating it is a wasted boot.

## Runs

| Run | Runtime commit | Change under test | First failure RVA | Evidence | Verdict |
|---|---|---|---|---|---|
| 1 | `217ac85` (baseline) | none — first boot | `+0x1e1cfe9` (`int3`) | `d3d12_xs!D3D12ConfigureVirtualMemoryX` auto-stubbed to `E_NOTIMPL`; trace log | verified — runtime reaches title code; contract missing |
| 2 | `217ac85` + patch | `D3D12ConfigureVirtualMemoryX` implemented | `+0x1e1d01b` (`int3`) | `xmem!XMemVirtualAlloc` returned NULL; trace log | verified — advanced; next contract exposed |
| 3 | `217ac85` + both patches | `XMemVirtualAlloc` implemented | no RVA — allocation returned NULL | `XMemVirtualAlloc(0x49400000, type=0x20003000, attrs=0x183, prot=0x22004)` → NULL | verified — large commit fails; naive mapping insufficient |
| 4 | `484dd57` (upstream) + patch | rebase onto upstream | `+0x1d8e505` | `rax=0x80004002` (`E_NOINTERFACE`), 8 stubs, no `unknown_iids`, no `called_stubs` | verified — rebase moved the stop earlier; regression |
| 5 | run 4 + diagnostic | refused-IID logging on every `QueryInterface` | `+0x1d8e505` (unchanged) | crash unmoved; log shows no refusal from `XGameRuntime` or GameInput | verified — no progress; refusal is silent inside DirectStorage, both prior suspects cleared |
| 6 | run 5 + 2 console factory IIDs | accept console DirectStorage factory IIDs | `+0x2237178` (`int3`) | `rax=0x0`, `called_stubs=[]` | verified — `E_NOINTERFACE` resolved; new stop is an in-title assertion |
| 7 | run 6 | dump folder at `D:\LocalDumps` (guest) | `+0x2237178` (unchanged) | `mkdir` failed silently: `D:` was the Windows install ISO (`DriveType 5`, read-only), not writable media | **NULL TEST** — hypothesis never tested; no evidence about dump collection either way |
| 8 | run 6 | move the CD volume to another letter via CIM | `+0x2237178` (unchanged) | volume still at `D:` after the call; `mkdir` still failed | **NULL TEST** — hypothesis never tested |
| 9 | run 6 | `diskpart`/`mountvol` + `subst` | — (no boot) | script stalled; the game was never started | **NULL — no run**; settle nothing, cite nothing |
| 10 | run 6 | `<game>\LocalDumps` (xcl maps `D:` to `{game}`) | `+0x2237178` (unchanged) | directory listed and confirmed present before the boot; same crash, same `rax=0x0` | run record — hypothesis falsified (no surviving artifact) |
| 11 | `9add721` upstream ("Retail game bring-up") + `XCL_NETWORK=0` | network off; rebase onto current upstream | `+0x2237178` (unchanged) | stubs 8 → 6; `hle-cross 14` and `xcl-netoff 6` build markers present; `rax=0x0` | verified — neither the rebase nor network-off moves the stop |
| 12 | run 11 + GameInput device-info and callback-return logging | log `GetDeviceInfo` result and the callback's return | `+0x2237178` (unchanged) | log: `GetDeviceInfo` (size `0x3f0`, family 1, caps `0x8`, input `0x104000e`) → "device callback returned" → crash | verified — the assertion runs **after** the callback returns, not inside it |
| 13 | run 12 + `XCL_CRASH_PROBE addr:len,tls0` | read the key and registry count at crash time | `+0x2237178` (unchanged) | probe: registry count **1** (not 0), the single entry is all zeros, and the key buffer holds **`appdata0.rpf`** | verified - the assert fires because a file-device lookup for `appdata0.rpf` finds an empty entry |
| 14 | run 13 + `appdata0.rpf` staged in the game folder | give the title the data file it asks for | **`+0x1CE6E13`** (moved) | sequence now ends `QueryApiImpl XPackage -> XCL`, then int3 with `rax=0`, `rdx=0x173400000001`, `r8=r9=0xffffffff00000000`; `called_stubs` empty | verified - the crash-reporter assert is cleared; a new stop is reached |
| 15 | run 14 + XPackage entry logging | log `IsPackagedProcess`, `GetCurrentProcessPackageIdentifier`, `Mount`, `GetMountPathSize`/`GetMountPath`, `CloseMountHandle`, `EnumeratePackages` | `+0x1CE6E13` (unchanged) | logging only; the trace it produced is the one quoted in run 16 | verified — adding the logging did not move the stop |
| 16 | run 15 + `package_identifier()` returns the manifest Identity Name only | test whether the identifier itself gates the stop | `+0x1CE6E13` (unchanged) | order: `IsPackagedProcess` → `GetCurrentProcessPackageIdentifier` (buffer 33) → `IsPackagedProcess` → `GetCurrentProcessPackageIdentifier` → `int3`; the 30-char name is accepted (`-> 0x00000000`) | verified — the change landed and the stop did not move; identifier length is **not** the gate |

Runs 1–6, 11–14 and 16 are labelled verified because each has a trace log with the stopping RVA and the
call that preceded it. Run 15 added logging only and changed no behaviour, so its row rests on the run 16
trace. Runs 7 and 8 are null tests because the environment change did not take effect;
run 9 never booted. In none of the three does the matching RVA say anything about the change under
test. Run 10 is the first row that actually tested the dump directory, and it settled it; its log did
not survive, so the row stands on the run record alone. Run 16 is not
a null test on the same grounds: the log shows the new 30-char answer being accepted, so the change
landed and the unmoved RVA is a real falsification.

## The stop at `+0x1CE6E13`

- The `int3` sits in a 41-byte function, `0x1CE6DF0`–`0x1CE6E19`.
- Its predicate lives in `0x292D370` and tests `[[Stage+0x2a8]+0x78]` for null.
- The nearby string is `"The method Stage::displayStateSet() is not implemented"` — an
  unimplemented-method trap, not the crash reporter that runs 6–13 hit.
- The stack is a clean init chain, no faulted frame: `0x2240EAB` → `0x2236CF6` → `0x293EF38` →
  `0x1CE6DF0` (the raw stack words between them also hold the vtable pointers `0x667D140` and
  `0x53EC0F0`).
- `rdx = 0x173400000001` is not an error code: its high 32 bits equal the crashing thread id.
- At this point **no window has been created and no D3D12 command list exists**.

## What we learned from the null tests

- **A silent `mkdir` is not a small error.** On read-only media the directory call fails quietly and
  the run proceeds exactly as if the change had worked. The crash then looks like evidence against the
  hypothesis when nothing was ever changed. The failure has to be checked, not assumed away.
- **Verify the drive type before concluding the environment is wrong.** `D:` was not a data volume; it
  was install media. Three rows (7–9) were spent on "dumps do not help here" when the truth was "the
  path did not exist and could not be created". The cheapest possible check was skipped.
- **"Same crash" and "same crash for the same reason" are different claims.** Identical RVAs only
  mean the run walked the same road. If the change under test never landed, the matching RVA says
  nothing about the change — it is the previous run's result repeated, and repeating a result is not
  corroboration.

## How the diagnosis moved

1. **`E_NOINTERFACE` at `+0x1d8e505`** — the refused IID was a DirectStorage factory IID. Accepting the
   console IIDs fixed it (run 6).
2. **Assertion at `+0x2237178`** — first blamed on a missing dump directory. Runs 7–10 killed that:
   two no-ops, one run that never booted, then a directory confirmed present with the crash unmoved.
3. **Static analysis of the `int3`** — it is `assert(count != 0)` on a registered-handler vector. The
   vector is empty because a factory lookup at `0x56EB10` returned `0`.
4. **The lookup and the TLS slot** — it reads a TLS slot-0 context. The dump shows slot 0 correctly
   held by the title, so TLS is not the fault. Run 13's probe measured what the lookup keys on: the
   registry count was **1**, the single entry was all zeros, and the key buffer read `appdata0.rpf`.
   Staging that file (run 14) cleared the assert.
5. **Trap at `+0x1CE6E13`** — an unimplemented-method trap reached from a clean init chain, decoded
   statically and confirmed by the stack. Runs 15–16 logged and then shortened the XPackage identity
   answer; the stop did not move, and the length is excluded. No window, no command list.

## Method for the next environment test

Before any boot that depends on a path or a volume, on the guest:

1. **Confirm the target path exists** — list it and read back the attributes (the
   `GetFileAttributes`-equivalent: attributes, volume label, filesystem, free bytes).
2. **Confirm it is writable** — create a 1-byte probe in the exact target directory, read it back,
   then delete it. A path that only lists is not yet proven writable.
3. **Confirm the volume is what it claims** — print the mapping (`mountvol` / `Get-Volume`), so a
   drive letter is tied to a real data volume and not to install media.
4. **Save that output as evidence** in the run log, before the game starts.
5. **Then run once.** If the probe failed, stop — the run would be another null test.

One change per boot, and the RVA gets recorded even when it matches the row above.
