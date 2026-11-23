# Bare-metal run of an untrusted title

A method for running an untrusted game executable **once**, directly on hardware, when a VM already
reproduces the failure. The goal is not iteration — it is a single cross-check that answers one
question: *is the remaining failure a property of the title, or of the virtualisation path?*

Two binaries are involved and both are treated as untrusted: **the title** (the game image and the
programs it loads) and **the runtime** (the compatibility layer that hosts it). Nothing below
requires a forensic hypervisor, a hardware TPM story, or a second machine. It requires one spare
partition, one boot entry, and the discipline to make every layer checkable.

Steps below are procedures we ran (**verified** as procedure); statements about *why* they work are
labelled **reported** or **inferred** where they are not directly observed, following
`CONTRIBUTING.md`.

---

## 1. Why bare metal at all

A paravirtual GPU is not a physical one. The guest sees a device whose enumeration, power
transitions and memory accounting are owned by a host partition; the guest driver stack is the same
vendor code, but it runs against a descendent adapter. Feature level, wave width, reserved-video-memory
and uniform-memory aliasing are all reported *through* that layer. A failure reproduced under it is
consistent with **either** the title **or** the virtualisation path, and no amount of guest-side logging
separates the two.

Four concrete reasons to leave the VM for one run:

1. **Device parity.** A paravirtual device can report a capability set that no physical adapter of the
   same model reports, and vice versa. Where the title's early init depends on a capability query, the
   answer is the experiment.
2. **Driver and session parity.** A remote shell lands in **session 0**, where desktop presentation and
   input APIs fail for environmental reasons. That masks the difference between "the title cannot
   present" and "there is no desktop to present into". Booting the hardware with an interactive logon
   removes the ambiguity without a scheduled-task workaround.
3. **Host policy.** Anything the title reads that reflects host configuration — adapter LUIDs, monitor
   topology, memory budget — can differ on bare metal even when the driver version is identical.
4. **The stop must be shown to move.** If a stop is identical on hardware, it is not caused by the
   virtualisation layer. That is a result worth one reboot.

And the reasons to stay in the VM, which is why this is one run and not a workflow: snapshots,
seconds-long rollback, no partition surgery, no boot risk, and the ability to retry. Bare metal is for
the question the VM cannot answer about itself.

---

## 2. Carving a test partition without touching data

The rule is absolute: **the only bytes written are to free space and to the new volume.** No
`diskpart clean`, no `Initialize-Disk`, no reformat of an existing partition, no shrinking the system
volume. If a step would move or restructure existing data, it is the wrong step.

Pick the volume with the most free space that is not the system volume. Then let the OS tell you what
it can do, rather than computing the arithmetic:

```powershell
$p      = Get-Partition -DriveLetter <main>
$min    = (Get-PartitionSupportedSize -DriveLetter <main>).SizeMin
$target = $p.Size - 120GB
"min {0} GB, target {1} GB" -f [math]::Round($min/1GB), [math]::Round($target/1GB)
if ($target -lt $min) { throw 'target below supported minimum' }
Resize-Partition -DriveLetter <main> -Size $target
```

Three points that are easy to get wrong:

- **The supported-minimum check is not optional.** A volume's minimum is set by immovable files
  (the MFT zone, the pagefile, shadow copies), not by free space. Asking `/SizeMin` is the only
  reliable answer; a shrink that ignores it either fails or, worse, succeeds partially and leaves a
  volume that cannot be extended back to where it started.
- **Create with `-UseMaximumSize`, not a computed size.** A 120 GiB request becomes "whatever is
  actually free", which is what you want.
- **Label the new volume with a unique label and read it back by label.** Drive letters are assigned
  by the guest, and a test OS routinely renumbers them. Anything in the harness that assumes a letter
  is a latent false failure.

Record the pre-change layout — partition numbers, sizes, offsets — before touching anything. The
removal in §8 is then *derived* from a record rather than reconstructed from memory.

Rollback of the shrink itself is one call, and it must also use the API:

```powershell
Resize-Partition -DiskNumber <n> -PartitionNumber <m> `
  -Size ((Get-PartitionSupportedSize -DiskNumber <n> -PartitionNumber <m>).SizeMax)
```

`SizeMax` is what reclaims everything the new partition occupied. Hand-computed sizes leave
fragments behind, and repeated hand-computed shrinks are how a volume shrinks permanently.

---

## 3. Applying the image with DISM, not interactive Setup

Interactive Setup is a decision tree: firmware checks, an account gate that increasingly prefers an
online identity, a region and keyboard prompt, and an OOBE that may or may not have a network to
satisfy it. On a machine with no network — which is the whole point — that tree is a hang waiting for
a human. It is also unverifiable: two runs of the same wizard are not the same run.

DISM applies bytes and stops:

```
dism /Get-ImageInfo /ImageFile:<image>\sources\install.esd
dism /Apply-Image  /ImageFile:<image>\sources\install.esd /Index:<n> /ApplyDir:<test>:\
mountvol <esp>: /S
bcdboot <test>:\Windows /s <esp>: /f UEFI
```

- **Choose the index deliberately.** Index 1 is not reliably the edition you want; enumerate first.
  The edition determines which optional components exist, and a missing in-box module is a failed
  isolation layer later.
- **Apply to an already formatted, empty volume.** DISM does not partition, and it will happily apply
  over a non-empty directory if asked.
- **`bcdboot` creates one entry with a locale-default description.** Rename both entries explicitly
  (§4); a boot menu that says "Windows Boot Manager" twice is how an operator picks the wrong one at
  2 a.m.
- **The answer file is the automation.** `unattend.xml` placed in `\Windows\Panther\` on the offline
  image is the only way to get a local administrator, autologon, and a first-logon hook without a
  human. Two facts about it are worth stating plainly: the file is readable in cleartext by ordinary
  users on that volume, and Windows does not delete it after setup. Keep `LogonCount` at **1** — a
  second autologon buys nothing (the hook fires on the first logon only) and re-arms the plaintext
  credential — and have the script delete the file as its last action.
- **Assume the harness may never run.** Every failure before the script's first line yields *no
  evidence at all*: a missing volume letter, a read-only mount, a blocked OOBE. So the first lines of
  the script create the report and write a header, the body is wrapped so an exception still writes a
  line, and the final line is a completion marker. A report without the marker means a truncated run,
  which is a usable signal; no report at all means nothing ran, which is a different signal.

---

## 4. Boot isolation: one shot, named, default untouched

The property that makes a failed test non-stranding is that **the test entry can be selected at most
once**, and the next boot goes back to the main OS on its own — even after a hang, a bugcheck, or a
power cut.

```
bcdedit /set '{main}' description "Main system"
bcdedit /set '{test}' description "Test: bare metal"
bcdedit /default '{main}'
bcdedit /timeout 10
bcdedit /bootsequence '{test}'          # consumed at the next boot, then gone
bcdedit /enum {bootmgr}                 # verify bootsequence AND default in one read
```

- **`bootsequence`, never `default`.** Setting the test entry as default leaves a machine that boots
  the wrong OS after any power event, and requires a second successful boot to fix. The one-shot value
  is self-clearing and idempotent in the failure direction.
- **Name both entries.** You want the menu to be readable by someone who has not read this document.
- **Verify the arm before rebooting**: both values present in one `enum`. A missing sequence value
  means nothing will run; a wrong default means the test may run twice.
- **Memorize the recovery command before rebooting.** BCD lives on the main disk's EFI system
  partition, so from a recovery console the volume must be mounted first and every command pointed at
  the store explicitly: delete the sequence value, set the default, delete the entry. Three ordered
  lines, no `/store` confusion under pressure.
- **Keep a human card.** The operator is not the author. What to click, how long "normal" is, what
  each failure looks like, and one rule that outranks all others: *photograph the screen before
  power-cycling*. A stop code and a clock are evidence; a rebooted machine is not.

---

## 5. Runtime isolation, in layers

Each layer is applied in order, **checked**, and logged. A layer that fails is a named degradation or
an abort — never a silent pass. The single most dangerous pattern in a harness like this is a `catch`
that logs and continues: it produces a report that *reads* like a clean isolated run.

1. **Network adapters down.** Enumerate and disable every adapter, then **assert** that none still
   reports `Up`. If one is, abort. Disabling at the adapter level takes IPv4, IPv6, DHCP, DNS, NCSI and
   SMB with it, and writes only to the test OS's own configuration.
2. **Per-program outbound block rules.** Path-exact block rules for the runtime and for the title,
   created before the process starts. This is defence in depth, not the primary control: a rule
   matches one image path, so a helper process, a re-executed child, or a copy under a different name
   is not covered. Say so in the report instead of implying sandboxing.
3. **All non-test disks offline.** Before the untrusted process starts, set every disk except the
   hosting disk offline, then re-enumerate and log the state of every disk. Two caveats to record
   rather than hide: a volume that shares the test disk stays online, and an offline request can be
   refused if something on that disk is in use — log the refusal with the disk number and treat the
   run as degraded.
4. **One bounded run.** `WaitForExit(n)` on the runtime, then kill the **process tree**, not the
   parent. The runtime's child holds the title; killing the parent alone leaves the title running and
   yields no exit code, which is exactly the case where a report says `exit: ` and nothing else.
5. **Writes confined to the test volume.** Every path in the harness derives from the script's own
   location. No literal drive letters, no assumption about where the staging directory landed. Output
   — report, stdout, stderr, runtime log, crash data — all live under that root.
6. **Operator discipline.** No browser, no sign-in, no second run, no manual intervention while the
   process is live. Any of these invalidates the run, and none of them is visible in the logs
   afterwards.

What these layers do **not** close, and which belong in the report as accepted residuals: telemetry
and error reporting run in services, not in the blocked image path; DNS and DHCP happen in a shared
service host before any rule applies; loopback is local by definition. Physical disconnection remains
the only real guarantee — but see §9 for why it cannot be claimed here.

---

## 6. Offline driver staging, and why the run can be invalid

An image applied with DISM contains only in-box drivers. For the display adapter that means the
**Basic Display Adapter** and its software renderer: `d3d12.dll` is present and loads, and hardware
device creation still fails. That failure looks exactly like a title bug, and is not one.

Stage the vendor driver into the *offline image*, not into a running guest:

```
dism /Image:<test>:\ /Add-Driver /Driver:<test>:\drivers\<vendor>.inf /Recurse
```

- The INF and its payload come from the **existing** install's driver store on the main OS; copy them
  to the test volume first. Never fetch a driver during the run — there is no network, and a downloaded
  driver would be a second variable.
- **Verify staging, do not assume it.** The package directory must appear in the image's own driver
  store, and the kernel-mode binary must appear under `\Windows\System32\drivers`. A package that is
  staged but whose `.sys` is missing is a package the loader will not use.
- **Gate the run on the adapter name.** The script reads the video controller at start-up. If it
  reports a Basic Display Adapter, the run is **invalid for any GPU question**: write the report, stop,
  and do not interpret anything downstream. A WARP-only run answering a device question is worse than
  no run, because it looks like data.
- **UAC is part of this layer.** Firewall rules and disk-offline calls need elevation. Either disable
  the elevation gate in the offline system hive, or relaunch elevated and log the relaunch. A script
  that silently runs unelevated has no isolation layers at all — only log lines that say they failed.

Finally, prefer a **single self-reporting script** over a sequence of manual steps, and let it write
each line as it happens rather than at the end. A crash mid-run should still leave a report that says
which layer it reached.

---

## 7. Evidence that survives the reboot

The minimum is small and it is not negotiable:

- **Hash before, on the main OS.** Title and runtime, recorded with the runtime's build identity
  (branch and commit it was built from) and the image version. No hash, no run.
- **Hash again inside the test OS**, at the top of the script, and write both values into the report.
  Equal values across the two sides prove the guest executed the exact bytes the main OS measured. A
  mismatch means the staging copy is not the artifact under test, and every downstream conclusion is
  void regardless of how interesting the log is.
- **A self-locating report.** Paths derived from the script, not from an assumed letter, so the report
  can be read afterwards from the main OS by volume label even if the guest assigned a different
  letter than the one used during staging.
- **A fact header before any action**: OS caption, adapter name, elevation state, script root, disk
  numbers, timestamps. These are the lines that make a failed run diagnosable rather than mysterious.
- **Re-hash after the run** on the main OS, and keep the before/after files beside the report.

Classification after the fact, from the artifact set alone:

| Observation | Reading |
|---|---|
| No report file | Nothing ran. Boot or setup failed — not the title. |
| Report present, no completion marker | Truncated run; the last line names the layer reached. |
| Marker present, empty stdout after minutes | Early death inside the runtime. |
| Non-zero exit before any title output | A guardrail fired; this is not a boot. |
| Long silence, no crash, no output | Suspect a host-protocol wait with no timeout. Photograph the screen and the clock before touching power. |
| Any window, frame or command list | The milestone. Screenshot, timestamp, log tail. |

Keep stderr even though it is usually empty. When a process dies before writing anything else, it is
the only thing that explains the death.

---

## 8. Removal, in an order that cannot strand the machine

The sequence is chosen so that a partial removal always leaves a bootable machine.

1. **Delete the one-time sequence value first.** A stale `bootsequence` is the only thing that can
   point the next boot at a volume you are about to delete.
2. **Set the default explicitly** to the main entry. Do not rely on it being unchanged.
3. **Delete the test boot entry.** This also removes it from the display order.
4. **Delete the test partition.** The OS will refuse while a pagefile or memory dump lives on it —
   reboot and retry rather than forcing.
5. **Extend the original volume to its supported maximum**, from the API, not arithmetic.
6. **Verify**, in one read each: no sequence value and correct default; no test volume; no leftover
   firewall rules on the main OS (rules created inside the test OS die with it); the disk layout
   matches the record taken in §2.

Two habits make this routine instead of risky: keep the removal steps in the same file as the
creation steps, in the *same* order each time, and rehearse the whole cycle once on a machine you do
not care about before using it on one you do.

---

## 9. Honest limits

- **The disk cannot be physically removed.** The boot loader lives on the main disk's EFI partition,
  and arming the boot is a write to it. Every other disk can be offlined; that one cannot. Anyone who
  claims the untrusted process ran with no main disk attached is either wrong or using different
  hardware.
- **Program-path firewall rules are one image deep.** A helper, a child, or a copy is not covered.
  With adapters down the rules are mostly redundant; with adapters up they are mostly decoration.
- **Telemetry is not fully suppressed.** Service-host traffic, time sync, and error reporting are not
  addressed by any layer here; they are bounded by the network layer when it works and by the
  write-confinement rule always.
- **A driver injected offline is untested in that configuration.** It is the vendor's signed package,
  but it has not been through its own installer's verification.
- **One run, no snapshot.** Bare metal is a cross-check, not an environment. If it disagrees with the
  VM, **the disagreement is the result** — do not average the two, and do not treat the more
  convenient answer as the true one.
- **The autologon credential is cleartext** on the test volume between staging and removal, readable
  by any user on that volume and by the host while it is mounted. Treat it as disposable and delete
  the answer file as the script's last action.
- **A hang is not a result until it is classified.** The most likely unattended failure is a protocol
  wait with no timeout: the process stays alive, writes nothing, and consumes the whole window. That
  is a finding about the host boundary, not about the GPU, and it must be reported as such.
