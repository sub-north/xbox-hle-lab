# Posting policy

Nothing is pushed here until it passes this gate. The repository is **public**: every file is
readable by strangers, search engines and rights holders, so write for that audience.

## Allowed

- Our original code: probes, VM automation, watcher, build scripts.
- Patches against upstream GPL-2.0 projects, as patches (not as a vendored fork).
- Method documents: verification levels, failure taxonomy, measurement rules, race kits.
- Evidence summaries that describe *observations*: "call X returned E_NOTIMPL at RVA Y".
- Hashes of our own artefacts (binaries we built, captures we produced).
- Names of public projects and public commits.

## Not allowed

- Game binaries, game assets, extracted textures/shaders/audio, keys, decryption material.
- The leaked package, its torrent/magnet, or any mirror link.
- Hashes of the leaked package or of the game executable.
- Instructions for obtaining the game or bypassing DRM/anti-tamper.
- Decompiled or disassembled output of third-party binaries: conclusions and RVAs only.
- Credentials, tokens, VM passwords, local absolute paths, or machine names.
- Third-party code without a compatible licence (TomBox has none; Leonida GPL-3; PercyStar
  non-commercial; ScarlettX none).
- Claims without evidence. If it is "reported", say so; if it is ours, attach the log or hash.

## Identifiers: hashes vs offsets

Identifiers are judged by kind, not together. Applied in PR #10.

- **Full hashes of files that shipped in the product are not published.** They are identification
  material: a value someone can match against a file they already hold. Three such hashes were
  withheld in PR #10; each is recorded in the lab record, with name and byte size published instead.
- **Code offsets and RVAs are published.** An offset inside a large image is meaningless without the
  binary, so it identifies nothing. Publishing addresses is already this repository's convention —
  the console-DLL export tables — and removing them would destroy these records' usefulness as
  records.
- **Hashes of our own artefacts are published.** Run logs, reports, captured blobs and extracted
  resource payloads are evidence we produced; their hashes prove provenance and stay in the tree.

One-line test for a future contributor: *would this value let someone obtain, verify or identify a
shipped file?* If yes, withhold it and say where it is held. If it only locates code inside a file we
do not publish, it is fine.

Counts at the time of this decision (2026-11-23), so the call is auditable: 51 lines across `docs/`
carry the stop offset `+0x1CE6E13` (52 occurrences; one comparison table repeats it), 0 other
game-executable offsets, 2 extracted-resource hashes published, 3 shipped-file hashes withheld.

## Verification gate before any commit

1. `git status` — inspect every file.
2. Scan for secrets: `password`, `token`, `gho_`, `ssh-`, `PRIVATE KEY`, `C:\Users\`, `/home/`.
3. Scan for binaries: `*.exe *.dll *.zip *.bin *.rpf *.ttpf`.
4. Scan for game references that would imply redistribution: torrent, magnet, scene release names.
5. Re-read the diff, not the summary.
6. Run `git diff --stat` and confirm the change set matches the intent.

A commit that fails any check is not pushed. Fix, re-verify, then commit.
