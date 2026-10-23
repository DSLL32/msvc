CVPACK 4.25 (vc1.0, toolchain 1.0-nt)
====================================

Reconstruct readable Win32 C with original function/ABI boundaries. Source is in
`src/`, owned tooling in `scripts/`, ordered cases and evidence in `cases/`.
The PE/COFF analysis reader was reused from LINK; no sibling files are changed.

`make -j4 native check` builds/runs native i386 fixtures. `make -j4 all audit`
also verifies original-compiler compilability. Debug with GDB against
`build/native/CVPACK.EXE`. `/help` is complete: exact 219-byte stdout, exit 1 and
empty stderr. Twelve application functions are accepted. The first orchestrator review passed;
work is released toward the fifty-function review. The independent DLL case stops
at the named `seek_buffered_file` stub. CRT startup has a
separate exact-library and observed-state proof and is counted separately.
`audit` checks complete original/compiler CFGs and callees, RET cleanup, SEH
roots, contextual initial data, library relocations, and 50 rejection checks, including ownership, snapshots, conditional ABI words and
case-specific guarded CRT variants.

`scripts/capture-original.py --case help --output ~/.ai-tmp/...` is an explicit
original-only operation after `make build/debug-run.exe`. It creates an entirely
masked original and unmasks only accepted function/text-data ranges. It is never
a fixture prerequisite. `--unmasked` is explicit reference capture.
`--discover-runtime` restores only reached exact-library/import ranges in the
original debugger, recording their order. It also records instrumented first
entries into accepted/candidate application functions; unknown traps stop it.
`--candidate NAME` validates proposed source before acceptance. Captures keep
configuration, image and helper hashes, registers/stack at a frontier, streams,
artifact hashes and separate setup/execution timing.

The root Wine prefix `~/.ai-tmp/vc1.0-capture/wine` is shared by explicit user
authorization. These scripts never initialize, remap, reset or stop it. Z: paths
and per-run private working directories avoid changes to its persistent state.
Existing canonical Z: references remain fixed; newer captures can use the shared
R: repository and S: scratch mappings without remapping either drive.

Initial cases: `/help` (exit 1, 219 stdout bytes), then the exact independent
`/nologo /cvnew hello-debug.dll` process frozen by LINK. The second case reuses
read-only prepack/packed bytes, not LINK execution. An independent original-only
rerun produced the exact same 71,800-byte packed DLL without normalization.
Partial fixtures compare their frozen frontier; completed ones compare full
streams/artifacts and projected entry traces. `/help` attempts a missing QH.EXE;
its exact invocation and -1/errno2 response are frozen and brokered natively.

`runtime-anchors.json` contains one reviewed identity-only `_freebuf` variant:
the original clears flag 0x8, while shipped LIBC clears 0x408. It resolves exact
caller identities but is never an accepted/unmasked range or coverage gain. The
mask-negative check verifies this even under the entire exact runtime closure.

Root reviewed and passed the scaffold before reconstruction, with graph/ABI/data
checks required before acceptance; those checks are now implemented. Stop for orchestrator review
at 10 and 50 application functions, counting CRT/support separately. After both
initial cases, inspect remaining application code and propose further cases to
the orchestrator before starting them. Shared shim changes belong to root.

The ten-function review and measured build/fixture/audit receipts are in
`cases/reviews/0010/`. Forced full builds: original 6.82 s, native 1.34 s;
audits 0.53 s, rejection checks 1.55 s. Native case totals sum to 0.043 s
(0.116 s including Python runner startup). Function-ten acceptance with fresh
GDB state took 0.240 s natively; explicit masked-original capture took 5.72 s.

The first source review and its tooling fixes passed; the fixes are recorded in
`cases/reviews/0010/fixes/review-fixes.json`. Acceptance now requires matching
current candidate snapshots and target/image hashes. The audit rejects overlapping
application code/text data/runtime ownership; same-range runtime aliases need
explicit identities and evidence. Historical proof snapshots remain unchanged.

The actual `_sopen` CRT variant has separate case-specific support in
`runtime-support.json`, with whole body/CFG/callee proof and per-invocation
mode/name/share/input guards. It is never an exact-LIBC identity or ordinary
unmask range and contributes no application coverage. Explicit original-only C
controllers verify rearming twice and denying a bad second mode/share before body
execution; fixtures only inspect those frozen receipts. See
`cases/runtime-variants/sopen-review.json` for scope and review. The unused optional
permission word retains both raw observations, with strict comparison whenever
the create bit is active. Buffered FileSlot comparisons now include allocated
name bytes and complete tagged page tables; mapped mode remains fail-closed.
