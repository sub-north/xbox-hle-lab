# The registration loop aborts on the seventh key

## The loop

`0x292D370` walks 12 dwords at `C+0x68` (count `C+0x64`), stride 4 from the list base, first element
read without pre-increment. Tested `[rsp+0x3c]` (`0x292D452`), appended `[rsp+0x28]` (`0x292D985`),
compared at `0x292D9A8`.

An abort leaves them unequal and the output map empty; key dwords stay (**measured**, `ka1`). It reads
as "no keys found" — a rollback signature, retracted in [`STOP-1CE6E13.md`](STOP-1CE6E13.md).

## Measured (`ka1`, `xcl.log:1372-1386`)

| # | key | `al` |
|---|---|---|
| 1 | `d521d1ea` (unnamed) | 1 |
| 2 | `ac9e7e35` common | 1 |
| 3 | `20ef298e` movies | 1 |
| 4 | `938b83b2` shaders | 1 |
| 5 | `0a802776` anim | 1 |
| 6 | `5eb5e655` data | 1 |
| 7 | `275b8a30` levels | **0** |

`[rsp+0x3c]=7`, `[rsp+0x28]=6`, `equal=false`. Abort is `list[6] = 0x275b8a30` = `levels` (PE read;
0/1-based agree). All 12 key ids are now dumped, `d521d1ea` … `e7a8cdf1` (two runs, `crc5`/`crc6`); the
earlier "only 8 of 12" limit is closed, and only the slot order of indices 8–11 remains unmeasured.

**Causation open.** Placeholders left the assert unmoved ([`PACK-KEYS.md`](PACK-KEYS.md)); a stub
archive is **notes-only** linked to the child-open returning 0. That the stub is *why* `al=0` is a
**hypothesis under test**, not a finding.

**Prediction (inferred).** A real archive for that key should let both counters reach 12 and
registration complete; the next stop is the observation to watch.

The gate is per-archive, measured across two runs with one variable changed: [`ARCHIVE-GATE.md`](ARCHIVE-GATE.md).
