# `platform:` is the title's scheme, not the host's

Labels: **verified** = read from the title's bytes or measured in our runs; **open** = not known,
stated as an open question rather than guessed.

## 1. The open is resolved inside the title

- **verified (static reading)** - `0x48C90` walks the list of registered devices and takes the first
  entry whose name matches the request.
- **verified (static reading)** - `0x128BA10` composes the request name;
  `platform:/data/gameconfig.zmt` is one such request.
- So `platform:` is a title-side scheme. xcl does not need to parse it; it has to supply the backing
  storage that the matching device reads.

## 2. One flag byte decides the open

- **verified (static reading)** - the open reports through a single flag byte; when it is clear, the
  assert at `+0x2224517` fires.
- **verified (debugger chain)** - that assert is the second stop in our chain, after `+0x1CE6E13`.

## 3. Our runs never reach that open

- **verified (run records)** - the stop is the pack validator's `int3` at `+0x1CE6E13` in every
  unperturbed run since run 10, before the dispatcher is entered. Run 27's zero-length pack moved it to
  `+0x2237320`, also earlier.
- **verified (debugger chain)** - stepping over `+0x1CE6E13` reaches `+0x2224517`, where the open's
  assert fires. So with the validator bypassed the open still fails: no registered device answered it.
- This is the only evidence we have about the open, and it is all negative. It says nothing about what
  a working `platform:` device would have to do.

## 4. Upstream moves nothing here

- **verified (upstream digest, `ivall/xcl` `66e8e97..956a476`)** - two commits, 15 files.
  `crates/xcl/src/gpu/pack.rs` is byte-identical, and a keyword scan of the patch for mount, VFS,
  pack, `platform:`, XPackage and TLS returns no matching lines.
- The delta is GPU submission, PSO construction and shader-cache persistence: downstream of our stop.
- Recommendation: stay on `66e8e97` for this question. Moving forward would change nothing at
  `+0x1CE6E13` and would cost a re-verified baseline.

## 5. Open questions

- **open** - what provides `platform:` on a console: a folder, a pack, or a virtual device.
- **open** - the smallest legitimate way for an HLE to back it without an RPF filesystem. One device
  entry serving the handful of requested names, and a real filesystem layer, are both untested, and
  nothing above says the title would accept either.
- What would settle it: a run that clears `+0x1CE6E13` and logs the request list the dispatcher walks.

Related: `docs/PACK-KEYS.md` (what the validator expects) and `docs/STOP-1CE6E13.md` (the stop).
