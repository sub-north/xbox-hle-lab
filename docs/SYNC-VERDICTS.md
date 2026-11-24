# Sync verdicts

**Verified** from our trace log.

## Why new instrumentation
No title synchronisation was HLE: `kernelx` exported no sync entry points, so every wait and signal went to host `kernel32` unobserved. 23 wrappers now intercept create/wait/set/reset/fence/close; `XCL_SYNC_TRACE=1` gates the trace.

## How a verdict is produced
`sets_at_arm` snapshots the handle's successful-release count at `wait_begin`. At dump, `signals > sets_at_arm` with no wake gives `verdict=lost-wakeup`; otherwise `verdict=no-release-during-wait`. Zero-timeout polls are logged unconditionally: `WaitForSingleObject(h, 0)` on an auto-reset event can consume a signal invisibly.

## Measured
200 `no-release-during-wait`, 12 `lost-wakeup`, 17 673 polls, from 43 outstanding waits on 40 handles at the dump.

```text
SYNC stuck h=0xdb0 kind=event waiter=38984 waited 16 ms creator=20732 via=CreateEventA signals=8719 sets_at_arm=8719 last_by=20732 last=16 ms before the dump verdict=no-release-during-wait
SYNC stuck h=0x1c84 kind=event waiter=14792 waited 16 ms creator=15384 via=CreateEventA signals=0 sets_at_arm=0 last_by=- last=never verdict=no-release-during-wait
SYNC stuck h=0x2008 kind=sem waiter=38856 waited 37907 ms creator=38096 via=CreateSemaphoreExW signals=1 sets_at_arm=1 last_by=38096 last=37907 ms before the dump verdict=no-release-during-wait
```

## What this corrects
"Never signalled" for most handles was invalid - the earlier trace truncated at 12 s. #505's cross-queue `Wait64BitValueX` defect is unrelated: different API and object, 0 occurrences here. Only the consequence coincides.

## Reproduce
```
XCL_SYNC_TRACE=1
XCL_SYNC_WRAP=1
XCL_SYNC_SLOW_MS=100
XCL_THREAD_DUMP_AT=100,115,125,135
```
Tracing perturbs timing (1839 traced frames vs 1579 untraced), so traced frames are not comparable with untraced ones.

## Not proven
Which party owes the missing release.
