# First bare-metal run past the archive gate

**Measured** unless inferred. Counts are log lines unless stated; a shader emits two lines, so
225 `[SHADER]` lines are 96 distinct shaders.

## Setup

A separate Windows install on its own partition, full payload present: 208.6 GiB, zero delta against
staging. Vendor display driver staged offline, installed at first logon (`pnputil` exit 0 after 110 s).
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
fixed.

## Next test

Current upstream with the instruction-emulation commits: one boot, logging presents, frame boundaries,
stop.
