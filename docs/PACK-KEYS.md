# The 12 pack-name hashes the early validator expects

Labels: **verified** = measured in our runs or read directly from the title's bytes; **reported** =
a third party's claim; **hypothesis** = reasoning, not confirmed.

## What was measured

- **verified (crash-time probe)** — the settings object the validator receives is reached from
  `Stage+0x2b0`. Its `+0x60` header holds `{tag = 12, count = 12, list}`, and the list yields twelve
  `u32` pack-name hashes.
- The values are **Jenkins one-at-a-time** (`one_at_a_time`) hashes of the game's pack names, computed
  over the lower-case name without an extension.

## The twelve ids

| # | Hash | Name |
|---|---|---|
| 1 | `0xac9e7e35` | `common` |
| 2 | `0x5eb5e655` | `data` |
| 3 | `0x938b83b2` | `shaders` |
| 4 | `0xa26a72dd` | `textures` |
| 5 | `0xe7a8cdf1` | `script` |
| 6 | `0xb80c6ae8` | `audio` |
| 7 | `0x20ef298e` | `movies` |
| 8 | `0x0e5d52e0` | `packs` |
| 9 | `0x275b8a30` | `levels` |
| 10 | `0x0a802776` | `anim` |
| 11 | `0x2eb2ee6e` | `navmeshes` |
| 12 | `0xd521d1ea` | **not matched** — do not guess it |

Eleven of the twelve were recovered by hashing the names above and matching against the probe's
values. The twelfth, `0xd521d1ea`, is in the list but has no name yet; it is left unnamed rather than
assigned a plausible-looking candidate.

## Why it matters

- **verified (static reading)** — the validator checks that every name in the list exists in a runtime
  map, keyed by these ids.
- **verified (probe)** — the twelve keys are looked up against the descriptor's own map at
  `[Stage+0x2b0]+0x10`, which holds **4 groups, 32 entries (measured)** with a live base pointer; the
  control run shows 14 of its 16 slots occupied. The map is populated, not empty.
- **verified (probe)** — the keys are **present** in that map: `joaat("common") = 0xac9e7e35` is among
  its entries. The earlier "the twelve keys miss" reading is refuted; there is no lookup miss to
  explain.
- **what remains** — the install gate plus rollback, not a lookup miss. The pack has to be registered
  as a device before this check runs; a key that is present is then rejected by the interface check
  and the state rolls back. The per-key control-byte compare at `0x292D465` reads the other map
  (`[[Stage+0x2a8]+0x20]`), which is empty.

## What was tested

- **verified** — staging valid 64-byte empty RPF9 placeholders named `anim.rpf`, `levels.rpf`,
  `navmeshes.rpf` and `audio.rpf` did **not** change the assert. The stop stayed at `+0x1CE6E13`.
- So mere file presence is not enough. The title has to register each pack as a **device** before this
  check runs; a file on disk does not create that registration.

## What would help

- Knowledge of how a RAGE title registers pack devices before its settings validation: which call
  creates the registration, and where the map is seeded.
- Or a way to make that registration happen earlier in startup, ahead of the validator, so the twelve
  lookups can succeed and the assert can be observed to clear on its own terms.

A note on the one unused id: if a later pack name hashes to `0xd521d1ea`, that is a real finding and
worth recording; a name that only *seems* likely is not.
