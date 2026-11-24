# What stops the load

**verified** = our log or decode; **reported** = another lab's claim. Offsets are RVAs.

## The symptom

With the complete payload the simulation runs in the world: the title's crash context reports
`Game State: Game Running`, a real camera and player, peds, vehicles, one script thread,
6 910 of 22 500 building slots, and the runtime counts 1 111 presents over 442.4 s of presentation.
The screen does not follow: the last frame is dark with a spinner. 29.2 s after the last present the
process dies writing to `0x0` at `+0x23CF683`. (verified)

## The abort mechanism

The write is not a fault. One thread polls at 1 Hz, two rules: **A**, a scope open for
`elapsed >= 300.0f` (`0x6159E20`); **B**, no scope open (`[0x71B6334] == 0`) *and* the heartbeat
(`[0x71B6338]`) unchanged for 30 polls. Both reach `+0x23CF620`: `mov dword ptr [0], 0x4D8BD421`,
a deliberate null write for the dump. `rbp=0`, `rbx=0` occur only on the `count == 0` path, so
**B** fired; 29.2 s is 30 polls at 1 Hz. (verified)

## Ruled out

| Candidate | Evidence |
|---|---|
| Coherent window | Ceiling 2 048 MB hit by +34 s, flat to 20 647 draws. (verified) |
| BCPack refusals | Complete; 705/3 651; fill-on halved throughput and 7 297 building slots; same stop. (verified) |
| Streaming backlog | No cap; 2 400–3 800 varies; refusals arriving at +418 s. (verified) |
| Command-list slot 101 | Ignored: `eax` never tested, no fence. (verified flow) |
| Device loss | `0xC0000005`; no `0x887A0005`, no device-removed line. (verified) |
| 300 s scope timer | Rule A needs an open scope; ledger empty twice. (verified) |

## What remains

The frame loop stops. Two candidates:

1. **Compile completion never signalled** — `resolve_wait`'s condition-variable wait has no timeout;
   dispatch can block behind it. (reported)
2. **Commit-headroom admission** — a compile is admitted only if system commit ≥ 6 GB plus 1 GB per
   running compile; `cores-1` workers put that near 21 GB on 16 cores. Below it work serialises
   unlogged in 50 ms retries. (verified code)

## Experiments in flight and proposed

- **Started, no result archived:** run 10, a cold pipeline cache with `-O1`. If the freeze moves,
  compile cost is the cause. (started)
- **Built, not run:** `XCL_PSO_PRIORITY=1` builds an awaited pipeline on the waiting thread and logs
  off-queue and duplicate builds. The binary exists; no boot has used it. (built)
- **Proposed:** compile-held-ms logged at the memory gate's 50 ms retry, the `XCL_DXC_THREADS`
  sweep, and a detour logging the caller of `0xB5B6B0`. (proposed)

## How to help

- **Two counters.** *Off-queue/duplicate builds*: compiles on a caller thread versus the pool, and
  duplicate builds of one pipeline. *Compile-held milliseconds*: ms held by the gate with avail
  commit, need commit. Held ≥ the freeze with `avail < need` and zero dispatches names the gate.
- **`XCL_DXC_THREADS` at 1, 2, 4.** Fewer workers lower `need`; if the gate is the cause the freeze
  shortens. An unsignalled wait will not.
- **Our dump cannot say.** Post-crash, 60 pool threads sit in one untimed wait with **zero
  `dxcompiler` frames** — parked, not sampled at +413 s. (verified) A sample 5–25 s after the last
  present is worth more.
