# Authored debugger transport proof

Exactly four reviewed authored no-CRT child controls passed in 3.010s. No
original CF application ran and no application function was accepted.

The source is tracked at `../../win32-broker.c`, `../../broker.py` and
`../../tests/win32-gate-child.c`. `plan.json` binds the actual source and generated
helper/child identities; `profile.json` contains the explicitly reviewed tiny
child instructions, not a Python CFG discovery. The historical `probe.py` keeps
its original scratch paths and must not become a fixture or silent recapture.

The PUSH proof retains complete before/ARM/DB contexts: flags246→346→246,
actual ESP31ff54→31ff50 and pushed EBX3fff1000. The guarded unknown CALL stops
at410000 with the real return410032 on its stack, the target CC intact and
marker0. A fresh known-leaf run follows the same caller path, writes marker1
and executes the genuine RET to410032/SP31ff3c. Wrong expected prebyte90 is
rejected against actual53 before any child application instruction; its expected
broker exit is92. Each child has an actual EXIT_PROCESS event and signaled
handle, followed by natural broker termination. Target and launcher streams are
retained as binary bytes.

Individual large receipts/event logs are gzip-compressed without changing their
decoded bytes. `evidence-index.json` binds both stored and decoded hashes.
The parent's cached independent review binds the decoded successful receipt.
Generated EXEs/objects and private full host-environment captures stay in their
referenced scratch locations; no toolchain or binary archive is bundled.

`failed-runtime1/` preserves the first command publication race (helper90,
events0, no debug child, controller12s timeout). `failed-runtime2/` preserves the
next fail-closed pre-entry UNLOAD event (helper92, zero child steps, actual owned
exit). No identity is retrospectively assigned to that old unloaded module.
The successful run records its own matched LOAD/UNLOAD base10000000.

`builds/` retains the failed unsupported START link, corrected no-CRT link,
entry-field correction and subsequent broker build receipts. The separate child
copy changes only AddressOfEntryPoint (one differing byte); its code is unchanged.
`source-deltas/` retains the two exact broker corrections. Reversing the unload
patch and then startup-race patch from current tracked source recovers the two
earlier source hashes. Earlier runtime runners differ from the current historical
runner only by their explicit WORK-path literal; their plans retain exact hashes.

This proves only the listed entry/PUSH/ARM/DB/CALL/frontier/RET/prebyte controls.
Actual indirect, callback/thread/exception closure, missed trap/flush negatives
and CF allocation/post-relocation snapshots/corruptions remain unproved. There
is no general startup/CRT/OS-service execution waiver.
