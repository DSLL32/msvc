# VC 5.10 RC

Target: `toolchains/5.10/bin/RC.EXE`, resource compiler 5.00.1472.1.
The source entry is `main` (Win32 MAP `_main`); the historical logical ID
`_rc_main` is retained explicitly, and native target translation units use
`-Dmain=rc_main` to keep the host adapter separate. The naming-only migration
retains the old raw evidence and did not recapture the original.
The released first case, `/help`, is complete. The rich resource case is also complete through a strict native dependency replay. Two application functions account for all 277 identified application
bytes. Reached startup support accounts for 350 of 353 bytes; the unvisited
three-byte math-error hook remains an INT3 stub. Three accepted import thunks
account for another 18 bytes, separately. This earns no RCDLL implementation credit.

RC is a console host with static `RCDLL.dll!RC` and `Handler` imports. Its two
application functions register a console handler, call the six-slot stdcall export,
format mutable diagnostic messages, unregister the handler and exit. The complete
callback body preserves percent escaping, allocation failure and CRLF/LFCR mutation,
including the original retained temporary allocation. The resource-callback slot
is opaque and NULL-only in this case. No resource-callback record ABI is invented.

`make -j4 native` builds the instrumented i386 Linux executable with `-O0 -g`.
`make -j4 fixtures` (or `check`) uses only native executables and frozen references:
status 0, exactly 589 stdout bytes, empty stderr, application entry trace, the six
DLL arguments, four callback results and every mutable post-buffer byte. It also
checks 12 malformed/unknown dependency requests fail before any callback. No
fixture invokes Wine or a child PE helper.

`make -j4 original` verifies a real Win32 EXE with the supplied compiler. Only the
original application translation unit uses `/O2 /Oy-`; startup uses `/Od` and
native builds remain `-O0`. `make audit` is an explicit original-build/static-audit
command, separate from fixtures. The embedded CRT does not byte-match the supplied
library object: startup and its reached helpers are handwritten, ABI-preserving C
with the original SEH, and are counted as support. The shared native layer supplies
reviewed imported CRT behavior, with captured argv and all target environment
strings bound independently of Linux tracing/shim environment controls.

The canonical imported CRT is the shipped 277,776-byte MSVCRT.DLL, SHA-256
`e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210`.
Original debug-event file handles verify the loaded RC, RCDLL and CRT bytes.
`cases/help/reference/` contains the provider-verified master, complete dependency
response and source provenance; no VC1.0 oracle output has been reused. The native
adapter validates its entire response before calling target code, verifies exact
arguments and input content, and fails on unknown dependencies or handler calls.

Fresh promotion uses `scripts/capture-pair.py --candidate FILE NEW_DIRECTORY`, then
`scripts/accept.py DIRECTORY`. Each promotion covers exactly one whole function
or import thunk. The original executable section is masked, including unaccepted
support; only verified IAT/debug/import metadata remains globally. Owner-specific
SEH/newline data is restored with its function. The actual mask is read back and
checked. Native traps require actual named ELF INT3/SIGTRAP/PC proof; import-entry
breakpoints are a distinct typed boundary. Completion requires real exit events,
matching traces and the canonical output. Compiled-source content, actual trial,
PEs, ELF, MAP, auditor and deterministic runtime indexes are pinned. Explicit
`--reuse-original` preserves unchanged original evidence when only native shared
inputs or validators change. Later shared worktree edits do not invalidate frozen
compiled provenance. Fresh latest-shared fixtures rebuild normally, while current
target-owned source checks remain in place.

Rust performs CFG/ABI/extent/reference/import checks. Its optional import check
qualifies exact IAT transfers and import thunks, **not cached register calls**.
The narrow `iat-references.py` residual separately records import data references
and RC's two observed callee-saved caches (ESI: SetConsoleCtrlHandler; EBX: strchr),
the exact IAT loads/call sites, no full/partial-register writes through the POP
restore, and no direct branch entering that lifetime across its load or restore.
Indirect jumps fail this residual. It relies on the recovered callee-saved ABI,
not general dataflow. Twenty reason-checked slot/register/alias/branch mutations
test this residual. Original and rebuilt static call counts can differ
because the compiler rotates a loop. No audit flag is weakened to hide this limit.

`cases/help/evidence/summary.json` records ordered promotions, hashes, split timing
and full scratch-proof locations. Its raw completion records, immutable source
snapshots and audit are retained alongside it. The startup review passed 16
rejection checks, including wrong trap PC/signal; the completed pair has 17
corresponding fail-closed checks, including missing actual completion events.

The original-only `probe-help` requires a new explicit `REFERENCE_DIR`; resource
references are coordinated with RCDLL, and fixtures never regenerate them. `cases/minimal/source` and `resources-rich/source`
retain the source seeds; resources-rich now pins its actual VC5.10 reference.
Historical groundwork timeouts remain recorded as such. All original work uses
the fixed shared-prefix R:/S: mappings without remapping or stopping its server.

## Rich resources

`make -j4 fixtures` also runs `/r /dCASE_FROM_COMMAND=7 /iinclude
/foresources.res resources.rc`. The single shared original capture supplies an
actual RC main entry/export caller, empty streams, no callbacks and the exact
3,672-byte `.res`. The fixture verifies all nine source/assets and conservatively
pins all 428 supplied include files before execution; the in-process boundary
rechecks each case byte. Its native working directory represents the captured
Windows directory for these relative inputs. The complete observed UTF16 process
environment is ASCII and is projected losslessly to the registered narrow vector;
this does not claim a separate snapshot of the original rich-case CRT heap.

The dependency changes argv pointers, including making one slot alias the original
`/fo` argument and another point three bytes into that same storage. Replay applies
those original-storage/offset effects, and a native caller tests the returned
pointer identities. Eighteen controls reject wrong inputs, arguments, environment,
CWD and preexisting artifacts before replay effects. The driver produces the same
resource bytes and RC-only trace; this adds no function or DLL coverage.
`cases/resources-rich/evidence/summary.json` records timings and immutable proof.
The next authorized case is a missing filename containing `%`, to exercise the
already-reconstructed diagnostic escaping branch through a real DLL error.
