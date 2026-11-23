# Next steps: help clear the current stop

What the lab knows, where it is blocked, and three concrete ways to contribute. Labels: **verified**
= reproduced here with a log; **reported** = someone else's claim, not reproduced; **hypothesis** =
reasoning we have not confirmed. Nothing here needs the title's data, and nothing here is a fix.

## Where the lab is

The title runs unmodified under a console-to-PC HLE runtime (xcl). It clears XMem allocation, the
D3D12 configure call and descriptor/heap creation, XGameRuntime, GameInput, the DirectStorage
factory, and XPackage identity. It then stops inside **its own early init validator**, before the
first engine frame. (verified)

## The blocker

- An `int3` at the title's RVA **`+0x1CE6E13`**; the predicate it guards sits at **`+0x292D370`**.
  (verified)
- The predicate scans records against a `flat_hash_map` at `[[rsi+0x2a8]+0x20]` — A is the 0x80-byte
  instance (vtable `0x6301540`) held at `[rsi+0x2a8]`. All 16 of that map's control bytes read `0x80`,
  the empty marker. (verified)
- The twelve keys live in the descriptor at `[rsi+0x2b0]` (0x70 bytes): `{tag = 12, count = 12, list}`
  at `+0x60`, and its own map at `+0x10` is populated with 50 entries in 4 groups. (verified)
- Our reading, not yet proven: a key present in the descriptor's map is still rejected by one of three
  gates — the interface check, the `Stage` vtable call, or the found-vs-installed count — and that is
  why the scan stops. "Keys miss" is **hypothesis**, not measured. (hypothesis)
- Skipping the break does not advance: the title faults with `0xc0000005`, a write to address 0
  inside `vcruntime140`. The check therefore guards a real precondition used later; it is not a
  stale assert to jump. (recorded; no archived log)
- No runtime assert-disable flag is available: the image carries 572 compiled-in `int3` assert
  sites, and this stop's guard is the map scan, not a settings byte. (verified by census)

## What is ruled out

Ten tested explanations are in [FALSIFIED.md](FALSIFIED.md), each with the run and log line that
killed it. Read that table before proposing a cause, and do not retest a row unless its "do not
retest unless" condition is now met.

## Three ways to help

1. **Describe a fresh per-title pack.** The title reads a per-title pack (`appdata0.rpf`-like)
   during this init; the only copy here is the shipped 8,880-byte one. On a console that has run the
   title once, list the pack's entries and say whether a pack of that size is seed data or user
   state. Entries, not the file. Contents matter: zeroing the pack moves the fault to `+0x2237320`,
   and a structurally valid count-0 RPF9 lands on the same fault, so presence is not enough.
   (verified)
2. **Document an HLE that populates a device/root map.** Point us at a runtime, loader stage,
   storage-root lookup, or static/TLS initializer that seeds a map of device or root names before a
   settings validator runs. A commit, a function name, or a note is enough. (hypothesis)
3. **Reproduce and compare the probe.** Run the same build with `XCL_CRASH_PROBE=keys,stage,stage2`
   (probe patch `0004`) and send the output: the map's 16 control bytes, the probed globals, and the
   stage reads. Same values mean the same stop; different values are a lead. (verified method)

## How to run

```
xcl run <game folder> --
```

with `XCL_NETWORK=0`, the probe patch applied, and a logged-in interactive desktop. SSH lands in
session 0, where presentation and GameInput fail for reasons unrelated to this stop, so a session-0
result is not comparable. (verified)

## What we will publish

Logs with RVAs and register values, plus the probe output above. No game files, no hashes, no
package identity. (verified)
