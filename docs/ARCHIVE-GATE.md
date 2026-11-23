# The archive gate is per-archive

**Measured (`ka1`, `lv1`).** Two runs of the same binary in the same environment differ in exactly
one variable: whether the real archive for the seventh key was present. The registration loop
stopped at a different key in each run — and at the same instruction in both.

## The loop

The loop tests every key in list order and appends only the keys it accepts. A key whose archive
cannot be opened returns `al=0`; the walk ends there and the rollback counters stay unequal. Slot
order and key names come from the verified list; every `al` value here is measured.

## Two runs, one variable

Same runtime binary, same build, same environment, one run each, no retry. `ka1` had a 64-byte
placeholder archive for every slot. `lv1` was identical except that the real archive for key 7 was
staged in place over its placeholder.

| | `ka1` | `lv1` |
|---|---|---|
| keys 1–6 | `al=1` | `al=1` |
| key 7 `levels` | **`al=0` — stop** | **`al=1`** |
| key 8 `navmeshes` | not reached | **`al=0` — stop** |
| counters tested / appended | 7 / 6 | 8 / 7 |
| stop RVA | `+0x1CE6E13` | `+0x1CE6E13` |
| stack | five frames | identical five frames |
| exit | `0x80000003` | `0x80000003` (`-2147483645`) |

Slot 8 still held a 64-byte placeholder in `lv1`. Clearing key 7 moved the failure one slot down
the list and changed nothing else.

## Why this is a gate, not a fault at key 7

- **measured (`ka1`, `lv1`)** — the accepted prefix is contiguous. Keys 1–6 are accepted in both
  runs, and the first rejection is always the first slot whose archive cannot be opened.
- **measured (`ka1`, `lv1`)** — the counters follow the walk, not the failure: `7/6` and `8/7`,
  each one higher in the run with one more openable archive.
- **measured (`ka1`, `lv1`)** — after the single variable changed, the stop is byte-identical in
  RVA, identical in stack and identical in exit status. The stop therefore belongs to no single
  archive.

## What this predicts

- **inferred** — registration can only succeed once every archive in the key list is present and
  openable. A partial set produces the same stop at whichever slot is still a placeholder.
- **inferred** — clearing one key buys nothing observable: the abort relocates to the next
  unopenable slot, so only a complete set can move the counters to their full value.
- **inferred, and the next observation** — whether the stop at `+0x1CE6E13` clears once every
  archive is present. This is one decisive test: complete archive set, same binary, same
  environment, one run. If the stop survives a complete set, archive presence is not the gate and
  the prediction above is falsified.

## Cost (measured)

Staging is the expensive half of that test. The single archive staged for `lv1` took 22.7 min to
copy through the guest channel, the guest disk is ephemeral across restarts, and each restart
repeats the copy. A complete set is therefore a staging-budget decision before it is a technical
one.

## Not claimed

- Why an open fails is not measured; the 64-byte placeholder is notes-only linked to `al=0`.
- That a complete set clears the stop is **inferred**, not a result. Nothing here shows what the
  title does after the counters agree.
- No per-archive behaviour beyond accepted or rejected is claimed: no bounds, no ordering effect
  between accepted keys.
