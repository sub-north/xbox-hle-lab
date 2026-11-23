# Falsified hypotheses

Rejected explanations for the first-run stop (`+0x1CE6E13`), with the evidence that killed each one.
Append only; never delete a row, because a deleted row gets retested.

Labels: **falsified** = tested and the prediction failed; **null test** = the precondition never held,
so the run proves nothing. Log references are by run and line; the package identity, mount letters and
machine paths are elided.

| # | Hypothesis | Test (run) | Evidence | Verdict | Do not retest unless |
|---|---|---|---|---|---|
| 1 | Missing crash-dump directory caused the assert | `mkdir` the directory (runs 7–8); `diskpart`/`mountvol` + `subst` (run 9); create it in the game folder (run 10) | Runs 7–8: `mkdir` on read-only install media failed silently, so nothing was ever created — null. Run 9 never booted (script stalled). Run 10 listed the `LocalDumps` directory as present **before** the boot and the crash was unchanged. | null test (7–8), no run (9), **falsified** (10) | a guest directory is created **and** `Test-Path` is printed in the same run |
| 2 | Moving the CD drive letter (diskpart, subst, `Set-CimInstance`) would fix it | reassign the optical letter (runs 7–8); run 9 stalled before launch | The volume stayed on its original letter (0 bytes free, install ISO) and the call returned nothing; xcl already maps the console drive to the game folder, so the path resolves inside it. | null test | never — read the emulator mount table instead of moving guest letters |
| 3 | Missing `movies.rpf` caused the stop | stage 8.8 GB `movies.rpf` (run 24) | Run record only — no artifact survives: `GetFileAttributesW` on the console `movies.rpf` returned `0x20` (present) and the crash was still `+0x1CE6E13` immediately after. | **falsified** (run record) | the failing predicate is shown to read an asset handle rather than a settings map |
| 4 | More game data (script, shaders, textures, packs) would clear it | stage `script`/`shaders`/`textures`/`packs` archives plus three more (run 26) | Archived run-26 log, line 1367, and that run's report agree byte-for-byte: `+0x1CE6E13`, `called_stubs: []`, after 5.1 GB more data. | **falsified** | a probe shows the stop consuming an asset, not a runtime map |
| 5 | The XPackage identifier length was the gate | return the 30-char identity in a 33-byte buffer (run 16) | Archived run-16 log, line 1300: `GetCurrentProcessPackageIdentifier -> '<id>' (30 chars, buffer 33) -> 0x00000000`; line 1303 still `+0x1CE6E13`. | **falsified** | a caller is logged rejecting the *contents* of the id, not its length |
| 6 | `XPackageIsPackagedProcess = true` was the gate | force the answer to false (run 17) | Run 17 returned false and the stop did not move; runs 16 and 26 returned true (archived run-16 log line 1299, run-26 log line 1361) with the same stop — the bool is not the gate in either direction. | **falsified** | the caller's branch on the bool is logged |
| 7 | Skipping the `int3` is a fix | `XCL_SKIP_INT3=1` vectored handler, title images only (run 22) | The title stepped over the break and faulted with `0xc0000005` — a **write to address 0** inside `vcruntime140.dll`, stack `+0x5B29847` → `+0x2240EFC` → `+0x2213025`. | **falsified** | never — the assert guards a real precondition used later |
| 8 | The global at `[game+0x694A1F8]` is the cause | full xref sweep + value probe (run 26) | Sweep: **0 readers, 0 writers**; the slot is 0 in the image, and the probe read `[game+0x694A1F8] = 0` as expected. A slot nothing reads or writes is not a gate. | **falsified** (benign) | a writer appears in a re-run of the sweep |
| 9 | A stale per-title data root caused it | fresh `XCL_DATA_ROOT` (run 28) | Fresh root, same assert at the same RVA. The root's *name* was the only variable changed. | **falsified** | the fresh root is shown to differ in **content** (list both trees) |
| 10 | TLS slot 0 was wrong | read the TLS index and the registry count at the stop (run 16) | Archived run-16 log, line 1251: the title's executable owns slot 0 while the runtime's own TLS is in slot 17; line 1305: `probe reg: count=1` — the title owns slot 0 and the count is 1, not 0. | **falsified** | the log shows the title in a slot other than 0 |

## Still open

1. **What populates the `flat_hash_map` whose 16 control bytes are all `0x80`?** The map at
   `[[rsi+0x2a8]+0x78]+0x20` is empty (16 × `0x80`) at the stop, and its owner is a 0x70-byte object
   with vtable `+0x6195668` and no RTTI. Candidates: a TLS/static initializer that never runs under
   manual mapping, or a GDK storage-root COM lookup the host does not provide. Needed: the writer that
   seeds the map, or the initializer that should have run before this step.
2. **Are `appdata0.rpf` contents inconsistent with this build?** Staging an **empty** file changes the
   fault to `0xc0000005` at `+0x2237320`, so the consumer reads the file rather than only its handle.
   That is a different failure, not a fix: it says the file is opened and its bytes are used. Needed: a
   size comparison against the shipped asset and a read of the consumer at `+0x2237320`.

**Artifact gaps (do not read as disproof).** The run archive holds no log or report for runs 10, 17,
22, 24, 25, 27 and 30, and the archived run table stops at run 14, so each of those rests on the run
record alone. The run-16 report is also missing, so run-16 counters come from run 14 (same RVA, same
call path).

## What these tests do not prove

None of these rows shows that the title cannot progress. Each one removes an explanation: an
environment change that left the stop where it was says the hypothesis was not the gate, not that a
gate does not exist. The stop itself is untouched by every test above, so the remaining work is to
identify what the predicate reads, not to re-run the same staging. Rows marked **run record** rest on
the operator's record, not on a surviving log.
