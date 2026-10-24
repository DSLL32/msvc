# Reconstruction harness standardization

Shared Rust `recon-tool` is implemented under `tooling/recon-tool/`: native
execution/tests, GDB debugging, capture pack/unpack, registry projection, and
explicit PE original capture. VC1.0 C1XX has migrated to eight numbered cases.
Original capture has smoke-test coverage; it has not yet driven a real C1XX
function-reconstruction step.

A capture is a saved invocation's inputs and expected results. Original-capture
smoke tests validate the shared harness; they are not reconstruction cases.

The tool is the entry point for native frontier discovery, case replay and
optional unit tests. It also orchestrates explicit original captures under
Wine, including direct-function probes. Normal regression runs use saved
references and never silently execute Wine or refresh the oracle.
An original capture is a reference run saving observable outputs, exit status
and first-entry order for later native comparison.

## Layout and builds

```text
decomp/{project}/{binary}/
    binary.c                    # exports binary_descriptor
    Makefile
    src/                        # reconstructed source
    scripts/                    # project-specific helpers only
    .build/libbinary.so         # shared native reconstruction + binary.c
    cases/
        0000-example/
            case.c              # exports case_descriptor; case.cpp also possible
            case.h              # optional case-specific declarations
            capture.bin         # applicable input/reference data, uncompressed
            direct.c            # direct-function setup, invocation and assertions
            tests.c             # optional, narrow behavior tests
            .build/case.so
```

`scripts/` is for project-specific helpers that do not warrant shared support,
such as generators for that binary's grammar. Common build, capture, replay,
CRT-identification and graph-audit machinery belongs in shared tooling.

After migration, case directories contain only the applicable files shown
above. No nested directories except generated, untracked `.build/` output;
no Markdown, text reports, logs, manifests or evidence trees. Put diagnostic
artifacts in scratch space, outside the case. Keep descriptions in the case
descriptor and fixture data in `capture.bin`. Migration removes superseded material
from the working tree; its existing Git history remains available.
The `cases/` root contains only the numbered case directories. Preserve distinct
invocations and necessary narrow tests from historical material, but do not
turn per-function frontier snapshots or acceptance receipts into new cases.
Do not retain the legacy hierarchy beside the migrated cases or move its proof
packages elsewhere in the binary folder.

Build the native reconstruction once as `.build/libbinary.so`, including
`binary.c` and the required shared recon support. Each case builds an i386
`case.so` containing its descriptor, adapters and tests, dynamically linked
against that library. Do not relink all reconstruction objects into every case.
A separate descriptor-only shared library remains an option if independent
metadata loading proves useful; it is not required for sharing the application.

Use PIC, debug symbols and the existing O0 policy. Default Rust target:
`i686-unknown-linux-gnu`, matching the current native libc and allowing the tool
to load these shared objects. Support C++ case glue where necessary.

Application implementation changes rebuild the shared application library;
case libraries only need rebuilding when their own sources or used interfaces
change. Shared Makefile rules should preserve that incremental behavior.
Keep one copy of the recon shim/tracer state in each executor. Sharing code
does not mean sharing mutable application state between independent cases:
retain per-case process isolation or a defined pristine fork starting point.

Reconstruction source remains faithful Win32 code and must still compile with
the original compiler. The original-toolchain build and graph/ABI checks remain
separate from native case execution. Reuse `tooling/graph-audit/`.

## Binary descriptor

`binary.c` holds the single function registry for the binary, shared by all
cases. It describes the original binary identity, entry point, exports and
discovered functions. Conceptually, each function has:

```text
name
original_entry_rva
original_extents[]              # half-open [start, end) RVA ranges
owned_text_data_extents[]       # e.g. jump tables needed when unmasking
definition:
    Unknown
    Import { dll, symbol: Name(string) | Ordinal(u16) }
    Crt { name }
    App { name, native_symbol, implementation_status }
```

Use fixed-width RVAs for original addresses. A function can have discontiguous
hot/cold extents; its entry is explicit. Record aliases consistently. These
records guide original code masking/unmasking and native entry identification.
Keep extent arrays inline with their function records in `binary.c`, rather
than a separate numbered declaration for every function's code/data extents.
The implementation flag applies only to application reconstruction. Leave it
zero for CRT, imports and unknown entries; their classification determines the
runtime boundary, not an implementation flag. Historical CRT qualifications
remain separate from application reconstruction credit.

Only application functions are reconstruction targets. Keep unknown functions
unclassified until their role is established; an accepted CRT/library entry
does not imply a handwritten reconstruction of its body.

No reconstructed function pointer is required in this descriptor. The tool
resolves native addresses from the application library's ELF symbols and the
symbol mapping, including C++ symbols. This avoids pointer-to-member conversions
and ABI-specific casts.

## Case descriptor

`case.c` exports `case_descriptor`, conceptually:

```text
name, description
first_entry_order[]             # original entry RVAs in encounter order
exec: IO | DIRECT_FUNCTION(callback -> bool)
tests[]: { name, fork, exec: callback -> bool }
```

Define the concrete, versioned C ABI once in a shared header, using explicit
tags, field widths and pointer/count pairs. The notation above is conceptual,
not a Rust ABI exposed to C.

First-entry order records application function encounters, not every repeated
call, return or branch. Filter runtime/import/shim entries. Scope tracing to the
case execution; additional unit-test calls must not pollute its expected order.
Cases should have explicit expectations independent of which prior cases ran.
Existing seeded captures may declare an explicit per-case seen-RVA set. Apply
that same set afresh on every run and compare the remaining first encounters;
do not present such a capture as the complete unseeded entry order.

Distinguish the final oracle from current reconstruction progress. A completed
case checks its expected exit and complete outputs. An unfinished case checks
the expected named INT3 frontier and applicable partial observations. It cannot
be required to produce final outputs before the missing code is reconstructed.

## Inputs and assertions

`capture.bin` is an uncompressed, versioned binary fixture. Applicable fields include
arguments, stdin, expected stdout/stderr, input files and metadata, expected
output paths/hashes, input environment, exit code and synchronized input events such
as Ctrl+C. Distinguish missing values/files from present-but-empty ones. Check
unexpected and deleted outputs where relevant. Normalize only specifically
identified nondeterministic fields, such as established clock fields.

The `recon-tool` implementer owns the wire format. Provide clap subcommands to
construct and deconstruct `capture.bin`, preserving arbitrary bytes and the
distinction between absent and empty values. Authoring files and unpacked
contents belong outside the committed case directory.

Environment variables are inputs only. Captured dependency responses must verify
the request and inputs before replay.

IO execution uses a standard binary adapter: reconstructed main for an EXE,
or attach, typed export invocation and detach for a DLL. Existing DLL export
inputs are principally argument strings, scalars and callbacks.

Direct-function probes are internal unit tests, distinct from public invocation
cases. `direct.c` supplies setup, typed invocation and assertions on both sides,
using common process/debugger facilities. Stateful probes may first run real
input to construct the necessary objects. Their expected results can also be
saved as fixture data. A generated state report is test output, not ordinary
user-visible tool output; direct acceptance does not establish public reach.

Additional `tests.c` tests are optional and as narrow as necessary. `fork` is
true unless the test is pure. Forked tests start from a defined setup state;
fork alone does not isolate filesystem changes or inherited file offsets.

## Native execution and cleanup

For each case, fork a supervisor. The supervisor creates and owns an ephemeral
workspace under `/tmp/recon/`, enables `PR_SET_CHILD_SUBREAPER`, then forks the
executor. Enable subreaping in the supervisor itself: the flag is not inherited
across fork. The executor can fork further for tests; it does not need to be a
subreaper merely for the supervisor to collect orphaned descendants.

Unpack inputs into the workspace and set the executor's cwd there. Keep captured
streams, tracing and harness diagnostics distinct. The supervisor orchestrates
external inputs/timeouts, waits and reaps until `ECHILD` (retrying `EINTR`), then
checks outputs and removes its workspace. On timeout, terminate remaining case
processes and finish reaping before cleanup. Save requested reports or original
references outside the workspace before deleting it.
An application adapter returning normally must take the normal process-exit
path, including registered application exit callbacks and stream cleanup. Keep
case tracing active through those callbacks; `_exit` is for harness helpers,
not a replacement for returning from the application's `main`.

This handles executor exit/crashes. Killing the supervisor itself can leave a
workspace behind; stale-run cleanup is needed for that case.

Keep process forking single-threaded. Setup and test checkpoint ordering are
specified under the implemented native interface below.

## Debugging

Provide `--gdb` for a selected native case or named unit test, with interactive
and batch-command/script support. Stop at the actual executing process after
loading/setup, before invocation; workers should not have to chase supervisor
and test-fork PIDs. Keep normal isolation semantics and supervisor-owned cleanup.
Suspend ordinary case timeouts during explicit debugging. Route debugger output
separately from the target streams used by assertions.

## Paths and Wine

The shared repository Wine prefix has fixed `T:` → `/tmp/recon/` alongside
R:/S:. Do not remap drives per case. Prefer relative inputs. Where
paths are observable, preserve a stable Windows-visible cwd, for example:

```text
visible: T:\vc5.10\c1\0000-example
native:  /tmp/recon/run-<unique>/vc5.10/c1/0000-example
```

The native shims can map T: to the private run root using `RECON_DRIVE_T` and
present the stable cwd using `RECON_CANONICAL_CWD`. Original captures use the
fixed Wine mapping; serialize captures sharing the same canonical directory.
Delete only directories owned by the run.

## Shared implementation and scope

`recon-tool` resolves application-library ELF symbols, including static and
mangled names. Add only a small explicit address/name registration and
enable/reset interface to the shared tracer; reuse its instrumentation
callbacks and shared seen-state. Preserve existing executable/environment
tracing. No Win32 or CRT shim behavior changes are required by this design.
Case/test adapters remain outside the application trace. Keep DLL
adapters, debugger transport, output comparison, timing and build rules shared
rather than recreating Python frameworks for each binary.

Validate a fixed candidate pair once and reuse that validated identity for
promotion. Reserve harness mutation tests for relevant harness changes, and
restore probes for changes affecting restoration. Avoid accumulating old
auxiliary observers, repeated historical state checks and per-function review
documents. Preserve historical evidence in Git history; migrated case folders
must follow the scoped layout above rather than retaining legacy evidence.

Concrete interfaces and supported limits are documented in the tool README and
shared headers. The repository prefix now includes fixed T: → /tmp/recon/.
Reconstruction work remains separately controlled by the orchestrator.

## Implemented native interface

The first implementation lives in `tooling/recon-tool/`; its shared C ABI is
`include/recon-tool.h`. A binary defines source lists/flags then includes
`recon-tool.mk` once. It owns native builds, automatic numbered-case discovery,
`check`/`replay`/`fixtures`, `unit`/`test-all-cases`, and generated-output cleanup.
Every discovered case participates in both execution and unit-test targets.
Numbered directories with `case.c` or `case.cpp` are discovered automatically.
Shared rules generate object/library targets; `check` and `unit` depend on those
targets without recursive make. Cases have no individual Makefiles and inherit
the binary compilation settings, including instrumentation. The application
registry filters case/test glue out of the recorded trace.
The common library is an order-only prerequisite so changes to it do not relink
unchanged cases. Build a single case by naming its `.build/case.so` target from
the binary root.
C-only libraries do not require libstdc++.

Optional original builds select `ORIGINAL_TOOLCHAIN` by name and use a separate
`ORIGINAL_SOURCES` list plus `ORIGINAL_BUILD`, `ORIGINAL_OUTPUT` and compiler/linker
flags. `recon-tool original tool TOOLCHAIN TOOL -- ARGS` owns shared-prefix and
toolchain environment setup; binary-specific Wine wrappers are unnecessary.
Shared rules derive objects with full source paths and track literal include
dependencies in one memoized scan; target-specific generators and special include dependencies stay
local. Optional audit profile/image-base/policy variables enable common registry
projection and the existing Rust graph checker; target-specific build inputs
remain local. `audit` inherits the Rust function-graph/ABI checks from the
shared include; registry export can also generate callback-table metadata.
Shared `identify-runtime` performs read-only library/COFF matching and relocation
checks for CRT names. Migrated binaries must use this command and the inherited
runtime rule, removing local identify-runtime scripts and unused parsing helpers.
Extend the shared implementation for needed variants instead of retaining a
per-binary Python identifier. Cached original identities remain reference data;
this analysis does not reconstruct CRT functions.
C1XX passes `-q` to graph-audit to suppress successful stdout JSON; `--report`
still writes the report, and failures remain visible on stderr.
Keep parser/data generators needed for builds. One-time comparisons against
original parser tables or data belong in scratch work, not a routine audit on
every reconstruction turn. Use case replay and narrow behavior tests for routine
regression; revisit generator verification when the generator/grammar changes.
Concrete variables are documented in the tool README.

The CLI provides `run`, `test`, `inspect`, `registry-export`, `identify-runtime`,
`original tool`, original capture/helper commands, and `capture pack/unpack`.
`registry-export` generates the existing graph auditor's config from `binary.c`,
optionally merging separate non-function policy data. It does not duplicate the
function registry or replace the graph engine. JSON authoring files and generated
reports belong outside committed case directories.

The fixture format is `RECONIO\0`, u32 version 2, u64 payload length (little endian),
then uncompressed CBOR. JSON pack/unpack uses `text:`/`hex:` byte strings and
preserves absent versus empty. See the tool README for its concrete fields.
Top-level outputs describe final completion; optional `partial` assertions apply
at an unfinished frontier. Ordered input events support delayed/output-triggered
stdin, queued close, and Linux process-group signals. Original Win32 console
capture remains separate from these native signal semantics.

Setup occurs after loading/environment/input/cwd initialization and before the
execution/test checkpoint. It must remain single-threaded and create no children.
`run` traces only application execution. `test` starts a separate initialized
executor; forked tests receive separate directory copies. Inherited open-file
offsets and other external resources still need explicit test handling.

The tool resolves native ELF symbols once per module, supplies address/name
entries to the existing shared tracer, and omits each case's explicit seed set.
There is no module ELF resolver or Win32/CRT behavior change in the tracing layer.
Native replay remains Wine-free. Explicit original capture uses the shared
Win32 debugger broker; ordinary tests never invoke it.

Native debug CLI: `run CASE --gdb`, or `test CASE --test NAME --gdb`. Batch
`--gdb-script PATH` implicitly enables debugging. GDB attaches to the selected
invocation after setup, with the supervisor specifically authorized for ptrace.
The supervisor's GDB child inherits that permission; no global ptrace change is
needed. Normal case deadlines are disabled during explicit debugging. Quitting
GDB terminates any remaining selected invocation, then normal descendant reaping
and workspace cleanup finish. Debugger streams are separate from target streams.

Function implementation status applies only to App records. CRT/import/unknown
records leave it zero. Identified runtime/import boundary availability follows
classification and verified extents, independently of App implementation status.

## Current capture and frontier behavior

Each ordinary IO case requires `capture.bin`; even a no-input case can pack `{}`.
DIRECT cases and standalone unit tests may omit it when self-contained. Arguments
include argv[0]; an empty list defaults to `[binary_descriptor.name]` on both sides.
`stdin_kind` explicitly selects regular `file` (default) or `pipe`; stdin delivery
and close events require pipe. Native signal-only events can use either.

Native frontier reports observe SIGTRAP's actual PC and INT3 cause, resolve the
owning ELF function range, and compare its original entry RVA. This checks
function ownership rather than assuming the last first-entry function trapped;
it does not claim matching original/native instruction offsets.

`original build-helper --repo REPO` explicitly builds the common Win32 helper.
`original capture CASE --output capture.bin` saves an untouched original oracle;
`--progress` masks code sections and restores classified CRT/import boundaries
and implemented App code plus owned text data, then stops at the next unknown
App INT3. Final oracles and partial observations remain separate. The tool reports
actual order/terminal but never promotes functions or rewrites case source.

DLL capture supplies `--host HOST.exe`; typed Win32 host glue loads the path in
`RECON_ORIGINAL_DLL` and invokes exports. The broker identifies that exact loaded
file and its actual module base/entry.

The current direct-capture prototype exports `case_original_direct(ReconOriginal*)`
from Linux glue and uses remote memory/register-call operations through the broker.
Only synthetic harness tests use it; no migrated reconstruction case does. It
still differs from the intended flow: compile typed setup/invocation code for
both Win32 and native execution, letting each compiler handle target-call ABIs.
Resolve that difference before introducing a real direct case.

The original backend supports PE32 i386 images; CF payloads/Watcom gateway capture
remain separate. Direct calls use ECX/EDX and stack words for ordinary MSVC ABIs,
not arbitrary register conventions such as Watcom watcall.

Current original capture transport is single-threaded, ANSI/ASCII-path/argument based,
and uses regular-file stdin. It rejects pipe/timed-console events rather than
silently changing them. Native replay can still exercise saved interactive cases.
DLLs without an entry point and repeated target-module reload are unsupported. SIGKILL of a supervisor can leave a folder;
its `.owner` marker and saved workspace path support explicit manual cleanup.
