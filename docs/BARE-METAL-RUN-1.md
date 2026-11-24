# First bare-metal run past the archive gate

**Measured** unless inferred. Counts are log lines unless stated; a shader emits two lines, so
225 `[SHADER]` lines are 96 distinct shaders.

## Setup

A separate Windows install on its own partition, payload staged: 208.6 GiB, total-size delta of zero
against staging. **Size is not integrity, and this run did not establish integrity.** Three small packs
and the executable were SHA-256 verified against our own record; every other file was checked by size
only. A hash check run afterwards found **one of the larger archives torn**: the same size as the
reference, a different SHA-256, and its last several percent reading as zeros — consistent with a copy
that stalled and left the tail unwritten while the size was preallocated. The other 40 staged files
hashed identical to the reference. So one archive in the tree run 1 booted was damaged, and the earlier
"zero delta" wording described bytes on disk, not a verified payload. Vendor display driver staged
offline, installed at first logon (`pnputil` exit 0 after 110 s).
Network adapters disabled, user data volume hidden (see below); 15-minute cap; self-reporting output
with a completion marker.

## Result (measured)

No validation assert: the registration gate cleared. The title built graphics state — 96 distinct
shaders (225 `[SHADER]` log lines, 33 root signatures), 91 compute pipelines,
`CreatePlacedResourceX` at its console virtual addresses — then
`STATUS_ILLEGAL_INSTRUCTION` (0xC000001D) at 7.254 s, at the fault address. Zero presents, zero frame
boundaries. Pipeline setup, not drawing.

## Isolation failure

The non-sandbox disk refused to go offline ("critical disk"); a volume on the same disk was hidden
instead. Hiding removes a drive letter and mount point — not offline, not access control: raw device
access or a remount still reaches the data. A residual.

## What it proves

The archive set is what the gate wants: the two earlier aborts stopped at counter pairs 7/6 and 8/7;
this run does not stop there. It does not prove rendering, menus, or that the instruction fault is
fixed. It does not prove the payload was complete.

## What the torn archive invalidates

- **Anything that needed that archive's contents is unsupported by run 1.** That covers world and
  streaming content, and whatever the title does once it reads that archive: part of the file read as
  zeros, and a zero tail is not the data the title shipped.
- **The earlier results stand.** Everything run 1 measured — the gate clearing, 96 shaders, 91 compute
  pipelines — happened before the title reads that archive. Later runs' menu and movie work also
  precedes it, so the front-end observations remain supported.
- **Read every "zero delta" and every "complete payload" in this record as "all files present, size
  only."** Staging was complete; integrity was not established. The payload becomes verified when it is
  re-copied and hashed whole; until then it is staged, not proven.

## Next test

Current upstream with the instruction-emulation commits: one boot, logging presents, frame boundaries,
stop.
