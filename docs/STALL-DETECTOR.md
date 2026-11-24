# The stop is a heartbeat stall detector, not a GPU watchdog

**Verified in decode** unless a line says **reported**, **inferred** or **measured**. Offsets are
RVAs inside the title image; no shipped file is named or hashed here. This page corrects the label
used in [BARE-METAL-RUN-6.md](BARE-METAL-RUN-6.md) and its pull request, and the timer reading in
[BARE-METAL-RUN-7.md](BARE-METAL-RUN-7.md).

## What was published, and why it is wrong

The run-6 page said, of the stop at `+0x23CF683`:

> The reading that this is the title's **GPU-progress watchdog** is **reported** by #505, not
> decoded here.

That wording is **superseded**. Two things in it are wrong:

- It is not a GPU-progress check. The rule that fired reads only a CPU-side heartbeat.
- It is not the 300 s scope deadline. That is the *other* rule, and no scope was open when this run
  stopped.

The fault is a **deliberate diagnostic abort**, taken by a detector thread when the title's own frame
loop stops advancing. The crash is the symptom; the stall is the fault.

## The predicate thread

One thread polls **once per second** and evaluates two rules.

| Rule | Condition | Reads |
|---|---|---|
| **A** | a load scope is open, and `elapsed >= 300.0f` | scope QPC stamps against the float at `0x6159E20` |
| **B** | **no** scope is open (`[0x71B6334] == 0`) **and** the heartbeat (`[0x71B6338]`) has not changed for **30** consecutive polls | the scope-open count and the heartbeat word |

Both rules end in the same abort site, which is why the two are easy to confuse from the crash alone.

Rule A is a scope deadline: it needs an open scope to have any subject. Rule B is a heartbeat
watchdog: it needs the **opposite** — no scope open — and it measures the frame loop, not a load
scope. Under rule B the timeout is 30 polls at 1 Hz, i.e. about 30 s, not 300 s.

## The register evidence

Run 6's fault reported these two registers (of the ten in the log):

| Register | Value |
|---|---|
| `rbp` | `0x0` |
| `rbx` | `0x0` |

`rbp=0` and `rbx=0` are the count and the accumulated value the predicate carries into the abort call.
That pair can only be produced on the `count == 0` path — the branch taken when no scope is open. The
rule-A path always reaches the abort with a non-zero elapsed count. **So run 6 aborted under rule B.**
(verified: register values in the run log, path in the decode)

## The arithmetic

| Event | Time from boot |
|---|---|
| Last present written | +413.2 s |
| Abort | +442.4 s |
| Difference | **29.2 s** |

29.2 s is 29 completed polls of a 1 Hz thread — rule B's 30-poll threshold, less the poll that was in
flight. A rule-A abort would instead sit at *scope start + 300 s* and would carry a non-zero count in
the registers. Neither holds. (measured gap, verified threshold)

This also retires run 7's arithmetic. Run 7 aborted 67.6 s later, which run 7 read as "scope opened
later"; under rule B it means the **heartbeat froze later** — about `+481 s` — so the fill moved the
stall in wall-clock terms without removing it. (inferred from the run-7 timestamp and rule B's 30 s)

## The confirming experiment

A probe patched the rule-A limit at `0x6159E20` from `300.0f` to `1800.0f`, which makes a rule-A abort
impossible for 30 minutes. The probe then logged the scope state directly:

```
scopes count=0        <- +60 s
scopes count=0        <- +240 s
```

No load scope was ever open, at either sample. Rule A had no subject at any point in that boot, so it
cannot be the rule that fired. (verified: probe log)

## What the title actually does

1. The frame loop stops advancing. Presentation, streaming progress and whatever else the loop drives
   all halt with it.
2. The heartbeat word at `[0x71B6338]` stops changing.
3. Thirty polls later the predicate thread crashes the title on purpose:

```
mov dword [0], 0x4D8BD421
```

That is a write to address 0 to force the access violation at `+0x23CF683` and take the diagnostic
path. Run 6's two minidumps and its crash-context block are the output of that path. **The access
violation is not a bug in the title's logic and not a missing GPU feature check: it is the title's own
dump trigger.** (verified in decode; the dumps as its output is inferred)

## `GPU Loc Sig` is a label, not a signal

`GPU Loc Sig` (`7333478d` in runs 6 and 7) is assembled for the crash report by an inline 13-byte
immediate at `+0x23CF62E`. It is not a string-table entry, it is not read from a GPU object, and it is
not compared against anything in the predicate. It is a location token for the report — useful to
match one stop against another, useless as evidence about GPU progress. (verified in decode)

That the value matches ivall/xcl#505's report exactly is still good evidence that #505 saw **this same
abort site**, because the token is generated at that site. It is not evidence that both labs saw the
same *cause*.

## What this changes in practice

- **Stop chasing the abort.** The 300 s deadline is not what stopped these runs; budget spent on it is
  spent on a branch the title never took.
- **Do not extend the timer.** Patching `300.0f` upward was proposed in run 7; the experiment above
  shows it buys nothing at this stop, because rule B fires first.
- **The stall is the target.** Whatever the frame loop blocks on — a wait, a fence, a leak of a
  per-frame object — is what has to be found.
- **The 30 s window is a gift.** The abort lands about 29 s after the last present, deterministically.
  A probe that samples the frame-loop stack at +5 s, +15 s and +25 s after the last present will catch
  the stall in the act, before the dump erases it.
- **The gap to the last frame is not a fingerprint.** It is 29 s by construction of rule B; run 6's
  "about 40 s" was the same rule, sampled less precisely.

## Leads

1. **`Dispatch -> pso_async::resolve_wait -> p.cv.wait`, with no timeout.** Another lab symbolized the
   same stop as this chain in ivall/xcl#505: a dispatch path waiting on a condition variable that no
   other thread ever signals. That is exactly the shape rule B reports — a blocking wait with no
   deadline stops the frame loop and freezes the heartbeat. (reported; **not** symbolised in our
   build, so it is a lead, not a finding)
2. **Pipeline backlog `[26, 0, 58736]` at the stop.** The pipeline counters are lopsided: one small
   number, a zero, and a very large queue. A backlog that large sitting idle is consistent with a
   producer that stopped and a consumer that never drains. (verified observation; the reading of each
   field is inferred)
3. **One very large DXC compile.** A single shader compile dominates the work in the window before the
   stall, far above the rest of the batch. If the frame loop is serialised behind compile it would
   stall exactly once, at this point, and never recover — which is what rule B sees. (verified
   measurement; the causal link is inferred)

## Honest limits

- **Rule B firing is inferred from two independent facts, not logged directly.** The registers say
  `count == 0` and the 29.2 s gap matches 30 polls at 1 Hz. A log of the actual rule taken would
  close it completely.
- **The confirming experiment proves a negative.** It shows rule A could not have fired in that boot;
  it does not name what the heartbeat waits for. It was one boot on one bed.
- **Two boots** (run 6, run 7) and one decode underpin the timing. A third boot with a stack sample in
  the 30 s window is the cheapest confirmation available.
- **The term is ours.** "Heartbeat stall detector" describes what the decoded code does; #505's
  "GPU-progress watchdog" was a reading of the cause, and stays reported.
