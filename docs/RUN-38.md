# Run 38: package manifests, and the stall watchdog

Labels: **verified** = measured in our runs or read from the title's bytes; **reported** = a
third party's claim; **run record** = operator-recorded, not verified here; **hypothesis** =
reasoning, not confirmed.

## Run 38 — staging the package manifests

- **run record** — `appxmanifest.xml`, `microsoft_gameconfig.bin` and the `layout_*_xs.xml`
  files were staged into the game folder, and their presence was confirmed before boot.
- **run record** — one boot was made; the first failure did not move. It remains the `int3`
  at `+0x1CE6E13`.
- **run record** — `platform:/data/gameconfig` is never reached, because the pack validator
  fires first.
- The run-id-to-RVA mapping for run 38 rests on the guest console capture `run38.txt`, which
  is reported to contain the `+0x1ce6e13` line. That capture is not in this archive.

## The archived log

- **verified** — the archived run-38 log is 187,777 bytes, SHA-256
  `f0826f10c3784e3ee550f8adb09edb22cdd8266c057107e3f4fa196bb4e97cb9`.
- **verified** — it contains **no `unhandled exception` line**. It ends cleanly at 2.488 s in
  `XGameRuntime` / `XPackage`. It is therefore **not** the crash record for run 38.
- **verified** — `report38.json` (4,620 B) records `crash: null`; it does not contain a crash.
- **open question** — the archived log may be from a later or interrupted boot rather than
  run 38. Until that is settled, run 38's crash evidence is the console capture and the run
  record only.

## What that rules out

- **verified** — package manifests are not the "prepared storage" the pack table needs.
  Manifests on disk did not change the stop, so they do not seed the map the validator reads.

## The fault at `+0x2215C35` is a watchdog, not a gameplay crash

- **verified (static reading)** — the containing function is a detached stall watchdog:
  `QueryPerformanceCounter` sampled around `Sleep(1000)` (`0x3e8`), 30 tries (`0x1e`),
  with thresholds of 2.0 s and 300.0 s.
- **verified (static reading)** — on expiry it executes `mov dword ptr [0], 0` and then
  `int3`. The fault is deliberate, not corrupted execution.
- **verified** — consequence: a run that reports two presents and then this fault has
  **stalled for 30 s**. Those presents are the last frames before the stall; they are not
  sustained execution, and nothing about later gameplay follows from them.

## The debugger chain

- **verified** — passing stops 1–5 in the debugger returns to the same `+0x5B29847` fault.
  Skipping asserts is a dead end, not a route forward.

## Current blocker

- **verified (static census)** — the pack table has `0x110`-byte slots and no insert for it
  anywhere in the image. Only the copy source is unknown.
- **hypothesis** — seeding the table from the correct copy source would let the twelve
  name lookups succeed. This has not been tested.

Full log: archived at docs-level reference (path is lab-local, so name it "the archived
run-38 log"), 187,777 bytes, SHA-256 f0826f10c3784e3ee550f8adb09edb22cdd8266c057107e3f4fa196bb4e97cb9,
crash line: absent. The log ends at 2.488 s with no exception line; run 38's crash record is `run38.txt`.
