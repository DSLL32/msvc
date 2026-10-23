CVPACK 4.25 (vc1.0, toolchain 1.0-nt)
====================================

Reconstruct readable Win32 C with original function/ABI boundaries. Source is in
`src/`, owned tooling in `scripts/`, ordered cases and evidence in `cases/`.
The PE/COFF analysis reader was reused from LINK; no sibling files are changed.

`make -j4 native check` builds/runs native i386 fixtures. `make -j4 all audit`
also verifies original-compiler compilability. Debug with GDB against
`build/native/CVPACK.EXE`. The first accepted application function is main; the
current `/help` frontier is the named `parse_options` stub. CRT startup has a
separate exact-library and observed-state proof and is counted separately.
`audit` checks complete original/compiler CFGs and callees, RET cleanup, SEH
roots, contextual initial data, library relocations, and twelve negative probes.

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

Initial cases: `/help` (exit 1, 219 stdout bytes), then the exact independent
`/nologo /cvnew hello-debug.dll` process frozen by LINK. The second case reuses
read-only prepack/packed bytes, not LINK execution. Current partial fixtures
compare their frozen frontier; complete-output and projected entry-trace checks
will replace that terminal assertion when each case completes.

Root reviewed and passed the scaffold before reconstruction, with graph/ABI/data
checks required before acceptance; those checks are now implemented. Stop for orchestrator review
at 10 and 50 application functions, counting CRT/support separately. After both
initial cases, inspect remaining application code and propose further cases to
the orchestrator before starting them. Shared shim changes belong to root.
