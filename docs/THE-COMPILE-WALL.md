# The compile wall

**Verified**, run 46.

## 1. What the overlay counts

The string is xcl's (`present.rs:401-405`), not the title's. `running`/`done` are `COMPILES` atomics
(`dxc.rs:834`): DXC compiles started versus returned. `queued` is `pso_async::counts().2` — lazy
stand-ins, **not a work queue**. `1 running, 0 done, 57 000 queued` is one compile started, unreturned.

## 2. The measurement

`XCL_PSO_TIMING=1`: four compiles in flight at `+117–136 s` — 289 572, 513 236, 607 276, 1 097 020 B
DXIL; a fifth, 999 764 B, returned at 87.8 s. Every dump read `3 submitted, 3 dequeued, 3 finished`:
workers fed, compiler the cost. All four returned at 93–180 s; presents held 30.0 fps.

## 3. Ruled out

Convoy/`WaitOnAddress` (queue fed), `BIG_COMPILES` serialisation, precompiling — upstream's 51 300
precompiled pipelines left the counter unmoved.

## 4. Implicated

Translation output size: a 9–64 KB console compute shader becomes ~1 MB of DXIL DXC spends minutes
over.

## 5. Upstream

`25c6f7e` / `9a272d4` / `b8b1f49` (head `930e8b4`) did not move the wall: dead at frame 1640,
against 3059 on `47efa2e`.

## Identifiers

`0x6e3fd903765f4376` 1 097 020 B → 179.9 s · `0xe0654264d8ad6489` 607 276 B → 178.7 s ·
`0x130fe330af5688bc` 289 572 B → 173.7 s · `0xa6bbf309e0e2767e` 513 236 B → 93.4 s.

## Reproduce

`xcl-psotime1.exe` SHA-256
`7c2587513ebd3e1c404d58ebae176da971ac0435c07a5fd55b10e10a49860afb`. Env: `XCL_PSO_TIMING=1`,
`XCL_THREAD_DUMP_AT=45,60,150`, `XCL_FRAME_DUMP=60`, `XCL_COHERENT_MB=2048`, `XCL_NETWORK=0`,
`XCL_DSTORAGE_BCPACK_FILL=0`, warm `XCL_DXIL_CACHE`, DXC 1.8.2502.11.
