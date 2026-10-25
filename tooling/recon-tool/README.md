# recon-tool

Shared native i386 case runner. Build from this directory with `cargo build -q`;
`.cargo/config.toml` defaults to `i686-unknown-linux-gnu`. Run the resulting binary
directly, rather than `cargo run` for each case. Requires the Rust i686 standard
library and working `gcc -m32`/glibc development files. C++ applications also need
a matching 32-bit C++ toolchain.

```sh
target/i686-unknown-linux-gnu/debug/recon-tool run ../../decomp/PROJECT/BINARY/cases/0000-example
target/i686-unknown-linux-gnu/debug/recon-tool test ../../decomp/PROJECT/BINARY/cases/0000-example
target/i686-unknown-linux-gnu/debug/recon-tool capture pack ~/.ai-tmp/case.json /path/to/case/capture.bin
target/i686-unknown-linux-gnu/debug/recon-tool capture unpack /path/to/case/capture.bin
```

`run`/`test` accept multiple case directories or `case.so` paths. `--capture` overrides
the inferred fixture for a single case. `--report` writes timing/results outside
the ephemeral workspace; keep authoring JSON and reports outside case folders.
`inspect CASE.so` prints descriptors. `registry-export CASE --image-base 65536
--policy policy-data.json --output .build/functions.json` projects the binary's
single registry for the existing graph auditor. `--metadata-output PATH` also
projects finite `policy.graph_metadata` (schema 1, functions, callback tables and
register-import contracts), retaining empty callback tables. Policy data must not duplicate
`target`, `accepted`, or `driver`. Original-compiler builds and audits remain
separate. Native execution never invokes Wine.

Include `include/recon-tool.h` for descriptor ABI **2**. Rebuild the binary and
case modules when this header changes; old descriptor versions are rejected.
`ReconFunction.original_symbol` optionally supplies the original compiler's MAP
name; `native_symbol` remains the ELF name, and `name` the logical identity.
Pending App entries with unknown extents remain in the registry but are omitted
from graph-driver projection. Known implemented/CRT extents remain mandatory.

Metadata annotations cannot override registry names, extents, classifications or
implementation status. Rebuilt address values may be
`{"rebuilt_symbol":"exact MAP symbol","offset":12}`; use
`--rebuilt-image PE --rebuilt-map MAP` to resolve them after PE/MAP base and
timestamp validation. Switch maps can be authored as
`[{"site":address,"targets":[address,...]}]`; register-call maps
`resolved_register_calls` and `direct_import_calls` may likewise use
`[{"site":address,"identity":identity}]`. These become the auditor's ordinary
address-keyed objects. `--ret-checks-output PATH` projects the separate finite
`policy.ret_checks` schema 1/functions/expected_cleanup contract.
Shared make exposes `RECON_METADATA_REBUILT_MAP`,
`RECON_AUDIT_REGISTRY_RET_CHECKS=1` and optional `RECON_AUDIT_RET_CHECKS`.

A binary Makefile defines
its source lists/flags, then includes `recon-tool.mk` once. It builds the
reconstruction, `binary.c`, and uninstrumented support into one PIC `libbinary.so`.
`case.mk` links only case glue/tests against that library. `binary.mk` remains
available separately for small harness fixtures. Support objects retain their
source paths under `.build/support/`, relative to `BINARY_SOURCE_ROOT` (the repo
root by default): `lib/recon/src/trace.c` becomes
`.build/support/lib/recon/src/trace.c.o`. External sources retain their absolute
path under `support/external/`.

```make
RECON_TOOL_DIR := $(abspath ../../../tooling/recon-tool)
BINARY_SOURCES := src/main.c src/options.c build/generated.c
BINARY_SUPPORT_SOURCES := $(addprefix ../../../lib/recon/src/,compat.c crt.c entry.c process.c trace.c win32.c)
NATIVE_CPPFLAGS := -I../../../lib/recon/include -I../../../lib/recon/native -Isrc

# Optional original build: lists can differ from the native build.
ORIGINAL_TOOLCHAIN := 1.0-nt
ORIGINAL_SOURCES := $(BINARY_SOURCES) src/runtime.c
ORIGINAL_BUILD := build
ORIGINAL_OUTPUT := build/example.exe
ORIGINAL_MAP := build/example.map
ORIGINAL_CPPFLAGS := /Isrc /I../../../lib/recon/include
ORIGINAL_CFLAGS := /nologo /c /Od /G3
ORIGINAL_LDFLAGS := /nologo /subsystem:console
ORIGINAL_LDLIBS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/BINMODE.OBJ
ORIGINAL_INCLUDE_DIRS = src ../../../lib/recon/include $(ORIGINAL_TOOLCHAIN_ROOT)/include
include $(RECON_TOOL_DIR)/recon-tool.mk
# Local generated-source rules and target-specific audits follow.
```

`all`, `native` and `native-trace` build the library. `cases` discovers numbered
`cases/NNNN-name/` directories containing `case.c` or `case.cpp` and generates
object and `case.so` targets directly. Cases have no individual Makefiles.
`check`/`replay`/`fixtures` depend on generated `cases/NNNN-name/check` targets;
`unit`/`test-all-cases` depend on corresponding `/unit` targets. Each starts one
runner invocation, so `make -j4 check` runs up to four cases concurrently. Select
one with `make cases/0000-example/check` or its `/unit` target.
These builds never recurse. Each
case library has an order-only dependency on the common library, so an app
change does not relink unchanged cases. Build one case from the binary root with
`make cases/0000-example/.build/case.so`.

Case compilation inherits the binary's native compilers, include paths, defines
and language flags directly, including instrumentation. The application registry
filters case/test glue out of the recorded trace.
`CASE_LDFLAGS` and `CASE_LDLIBS` may supply extra case-library link settings at
the binary root.
Optional `RECON_RUN_FLAGS` and `RECON_TEST_FLAGS` pass runner options. These
native targets never depend on the original build.
`clean` removes `BINARY_BUILD` (default `.build`), `ORIGINAL_BUILD` (default `build`),
and each discovered case's `.build`.

`win32`/`recon-original` build `ORIGINAL_OUTPUT` when `ORIGINAL_SOURCES` is set.
Optional `ORIGINAL_MAP` owns the linker MAP alongside the EXE as a grouped output;
shared rules add `/map:` and relink if either file is missing.
`ORIGINAL_TOOLCHAIN` selects the shared compiler/linker launcher; MSVC defaults
are CL.EXE and LINK.EXE. No binary-specific Wine wrapper is needed.
`ORIGINAL_TOOLCHAIN_ROOT` is the derived toolchain path relative to the binary.
Source flags, libraries and generated-source rules remain binary-specific.
`ORIGINAL_OBJECTS` (also `OBJECTS`) retains source paths and extensions, for example
`src/options.c` → `build/obj/src/options.c.obj`. Sources are binary-root-relative,
without `..` components. `binary.c` and native-only shim files are not added to
this list. `ORIGINAL_COMPILE`/`ORIGINAL_LINK_COMMAND` can replace the MSVC-shaped
recipes for other toolchains; `ORIGINAL_ORDER_ONLY` adds initialization prerequisites.

Native objects use compiler-generated header dependencies. Original builds share
one memoized scan of literal quoted/angle includes, searched through
`ORIGINAL_INCLUDE_DIRS`; it conservatively includes both conditional branches.
Macro-computed includes or generated headers not yet present require explicit
`ORIGINAL_EXTRA_DEPS` or local object prerequisites. Flags in Makefile and the
shared make rules are tracked. Generator rules remain local to the binary.

To enable the common graph audit, set `RECON_AUDIT_PROFILE`, `RECON_IMAGE_BASE`,
and optional `RECON_POLICY` before including `recon-tool.mk`. `RECON_REGISTRY`
defaults to `.build/functions.json`; projection comes from the first discovered
case and the single binary registry. `RECON_AUDIT_FLAGS`,
`RECON_AUDIT_METADATA`, and `RECON_AUDIT_PREREQUISITES` configure target-specific
checks/input generation. `RECON_AUDIT_REGISTRY_METADATA=1` projects declared
function-pointer tables into callback metadata during the same registry export;
no local adapter script or metadata rule is needed. `RECON_AUDIT_REPORT` defaults to
`$(ORIGINAL_BUILD)/graph-audit.json`. `audit` depends on `recon-graph-audit`, which
uses the graph tool's quiet build include. One-time comparisons against original
parser tables or data belong in scratch; routine regression uses case replay and
narrow behavior tests.

`RECON_AUDIT_IDENTIFY_RUNTIME=1` enables the inherited rebuilt-CRT identity rule.
It takes `ORIGINAL_TOOLCHAIN`/`ORIGINAL_OUTPUT` and writes
`RECON_AUDIT_RUNTIME` (default `build/runtime.json`) for the graph audit.
`RECON_RUNTIME_FLAGS` supplies explicit library/object options when needed;
`RECON_RUNTIME_INPUTS` lists their file dependencies (default 1.0-nt LIBC.LIB and
SETARGV.OBJ). Migrated binaries remove their local identification scripts and
unused PE/COFF/archive helpers. Cached original identities stay unchanged.

The executor loads the library, applies fixture environment and cwd, and runs
optional `setup` once. Setup must remain single-threaded and create no children.
Application first-entry tracing begins only for the IO/direct execution, after
setup. A case's explicit `first_entry_seen` RVAs are omitted afresh on each run;
they preserve a captured suffix, not evidence of a complete historical trace.
The current terminal is exit or a named unimplemented App frontier. Implementation
status is bookkeeping, not a claim that the tool has accepted a function.

`test` starts a separate execution. Each forked test starts from the initialized
setup checkpoint and gets a copy of the setup directory. Unforked tests must be
pure. A fork does not isolate inherited open-file offsets or external resources;
tests needing those must arrange them explicitly. Test calls do not enter the
case trace. The supervisor reaps all descendants, including orphaned grandchildren,
before checking outputs and deleting its owned workspace. Timeouts also kill
descendants that moved to another process group. SIGKILL of the supervisor itself
cannot guarantee cleanup.

## capture.bin version 2

Eight-byte `RECONIO\0`, little-endian u32 version, little-endian u64 payload
length, then uncompressed CBOR. No trailing bytes are accepted. The payload's
JSON authoring form uses `text:...` or `hex:...` for byte strings; CBOR stores raw
bytes. Unpack uses hex, preserving arbitrary bytes exactly. Missing/null and empty
byte strings/files remain distinct.

```json
{
  "args": ["example.exe", "input.c"],
  "stdin": "text:",
  "stdout": "text:hello\n",
  "stderr": "hex:",
  "exit_code": 0,
  "input_files": [{"path":"input.c", "data":"text:int main(void){return 0;}\n"}],
  "output_files": [{"path":"output.obj", "sha256":"...64 hexadecimal digits..."}],
  "env": {"EXAMPLE":"value", "UNSET_ME":null},
  "reject_unexpected_files": true,
  "timeout_ms": 30000
}
```

Optional input metadata: `mode` (Unix permission bits),
`modified_unix_seconds`. Output entries may assert `absent:true`, exact `data`,
`sha256`, or explicitly identified nondeterministic `zero_ranges:[[start,end]]`.
Ranges are half-open and zeroed before byte/hash comparison. Unexpected-file
checking allows input files and explicitly listed present outputs. Symlinks and
special output files are rejected. Relative fixture paths cannot contain `..`.

`logical_cwd` may be a stable Windows drive-absolute path. `logical_drive` chooses
its private drive root (for example S); T always maps to this run's private root.
The physical cwd uses the canonical path suffix. Explicit `readonly_dependencies`
contain host absolute `path` and `sha256`, verified before and after execution;
these are inputs, not undisclosed implicit references. Environment variables are
inputs only; output-environment assertions are not supported. Version 2 removes
the former output-environment fields; version 1 captures must be repacked.

Events are ordered. Each has optional `after_ms`, optional `stdout_contains`
(both conditions must hold if present), and an `action` object:
`{"kind":"stdin","data":"text:..."}`, `{"kind":"close_stdin"}`, or
`{"kind":"signal","signal":2}`. Close waits for queued bytes to be written.
Set `stdin_kind:"pipe"` for stdin/close events. Signals go to the executor process group. These are Linux signals, not yet a
claim of arbitrary Win32 console-event equivalence.

Top-level expected outputs describe successful completion. Optional `partial`
has its own stdout/stderr/output_files/reject_unexpected_files and
applies at an expected current frontier. Final expectations are retained intact.

Run `python3 tests/smoke.py` for focused native integration tests.
`python3 tests/toolchain.py` checks launcher profiles and argument/status handling
using a stand-in Wine executable, without starting Windows processes. Ordinary IO
runs require an inferred or explicit capture.bin, even if it only contains `{}`;
DIRECT and unit-only runs may omit the file when self-contained.

## Debugging

`run CASE --gdb` attaches GDB after library loading/setup and before invocation.
`test CASE --test NAME --gdb` selects the actual test process, preserving its
normal fork setting. `--gdb-script /absolute/script.gdb` uses batch GDB (and
implicitly enables debugging); scripts can set named application breakpoints,
continue, inspect state and continue to the normal terminal. Interactive GDB
gets its own foreground terminal group, so Ctrl+C belongs to the debugger.

The target permits ptrace only by its supervisor and that supervisor's descendants.
No global ptrace setting or `PR_SET_PTRACER_ANY` is used. The supervisor retains
workspace ownership and disables the ordinary case deadline in debug mode. GDB
output stays separate from asserted target stdout/stderr. Quitting/detaching ends
the session and terminates any still-running selected invocation; it does not
leave a stopped child behind. As usual, target exit/frontier assertions determine
whether the case passed. Debug timing includes the time spent in GDB.

The shared make rules build the release runner incrementally. `build-quiet.sh` retains
successful Cargo output in scratch and prints diagnostics only when building
fails. It never launches Wine.

## CRT identification

```sh
recon-tool identify-runtime 1.0-nt build/example.exe --output build/runtime.json
recon-tool identify-runtime 1.0-nt toolchains/1.0-nt/bin/C1XX3232.EXE \
  --bindings decomp/vc1.0/c1xx/runtime-compatible.json --output ~/.ai-tmp/original-runtime.json
```

This is read-only PE32/i386 COFF/archive analysis; it never launches Wine.
Identification masks relocation operands for byte matching, then validates REL32
callee identities to reject misleading byte matches. Whole-section matches take
precedence, with per-function fallback where neighboring code differs.
The 1.0-nt default is LIBC.LIB plus SETARGV.OBJ. `--library PATH` and `--object PATH`
replace these inputs explicitly; other toolchains require explicit inputs until
their identification variant is migrated into the shared tool.

`--bindings FILE` adds an already approved compatible-library record only after
checking its target/body/library/member hashes and zero reconstruction credit.
It must match the identified image; rebuilt images normally omit this option.
The identifier supplies CRT names for analysis, never function acceptance or
application coverage credit.

## Original toolchain launcher

```sh
recon-tool original tool 1.0-nt CL.EXE -- /nologo /c test.c
recon-tool original tool 5.10 LINK.EXE -- /nologo /out:test.exe test.obj
```

Only the toolchain name, tool name and original arguments are required. The
repository is inferred from cwd or the recon-tool executable location; `--repo`
can override it. The shared R:/S:/T: Wine prefix is validated, never created or
remapped. The launcher sets toolchain include/library/search paths, clears
inherited compiler option variables and preserves cwd, arguments, streams, exit
status and signals. It adds no ASCII restriction to tool arguments.

Profiles cover `1.0-nt`, `5.10` (native MSVCRT override), and `watcom10.0a`
(Win32 binnt stubs, WATCOM root and the documented hash-verified GS-patched WLINK).
Tool names are case-insensitive and may omit `.EXE`. This command is for explicit
original builds/runs only; native replay never invokes it. `original build-helper`
uses the same shared toolchain setup.

## Explicit original capture

```sh
recon-tool original build-helper --repo /root/msvc2
recon-tool original capture /path/to/case --repo /root/msvc2 \
    --capture ~/.ai-tmp/inputs.bin --output /path/to/case/capture.bin \
    --report ~/.ai-tmp/original-result.json
```

The helper is compiled with the existing VC1.0 original compiler. Its sources and
build outputs live under this tool; no Python program is needed per binary/case.

Wine's executable is resolved absolutely from the host PATH. The Unix launch
retains its host PATH and the fixed shared Wine prefix/configuration. Fixture
`env` entries apply only to the Windows target through an explicit CreateProcess
block, preserving Wine's Windows defaults for unspecified names. Empty values
and removals remain distinct; names are case-insensitive and duplicates fail.
The broker keeps hidden drive-current-directory entries and sorts the child
block by name. The current capture environment transport accepts ASCII only and
bounds both its NUL-delimited overlay and resulting block to 32767 bytes.
Rebuild the shared helper after updating these sources. Native replay is unchanged.
`python3 tests/original-environment.py` checks this separation using a stand-in
Wine plus the actual C merge routine; it never starts a Windows process.
`capture` verifies the original SHA256 and entry against binary_descriptor. The
optional `--program` selects that verified original path explicitly. Existing
output files are not overwritten without `--replace`. Capturing is always an
explicit command, never a side effect of run/test.

`--artifacts-dir NEWDIR` optionally preserves raw generated files in
`NEWDIR/files/<relative-path>` before normalization and workspace cleanup. The
directory must be new, its parent must exist, and it must be outside the capture
workspace/cwd. The manifest and report record raw SHA256, size and exported path;
fixture artifacts remain normalized hashes. Existing export files are never
overwritten. Without this option raw artifacts are not retained.

Ordinary capture executes the untouched original and records exact stdout/stderr,
exit status, changed/new/deleted files (hashes), and application first-entry RVAs.
`--progress` creates a temporary masked copy, restoring classified CRT/import
boundaries and implemented App code/owned text data. It records a named pending
App INT3 and partial observations while preserving the final oracle. It does not
execute that pending body or automatically update function acceptance/case source.
The masker preserves parsed PE import/export/debug loader data, rejecting overlap
with registered App entries or owned extents. Private images stage the supplied
DLL host and declared imports found beside the selected original binary; names
are checked for collisions, and reports record source/staged hashes. `--native-msvcrt`
selects the toolchain-local MSVCRT DLL, stages it beside the private image, and
requires that exact file to load before the target entry. This flag also creates
a private byte-identical image for an ordinary capture. Wine receives the absolute Unix broker executable path directly, avoiding
its start.exe launcher. The broker links at 0x400000: Wine reserves the low DOS
area, and relocating the old 0x10000 default during initial image load caused a
start.exe fallback that could not run with the selected old native MSVCRT. The flag selects `WINEDLLOVERRIDES=msvcrt=n` in that
explicit Wine transport; the broker itself uses static CRT and KERNEL32.
An explicit fixture entry for WINEDLLOVERRIDES conflicts with the flag and is
rejected. Frontier reports retain debugger registers, floating state,
and a 64-byte stack prefix (or the diagnostic read error).
The Wine loader breakpoint is tracked independently of the target entry, since
static DLL attachment precedes it. Only the first-chance main-thread ntdll
process_breakpoint SEH instruction signature is consumed once; other external
breakpoints fail with module/register/stack diagnostics.
Reports include actual order and terminal so the reconstructor can deliberately
update expectations after matching both sides.

For DLLs, `--host PATH.exe` supplies a Win32 host, including an unchanged shipped
EXE that statically imports the target DLL. A dynamic host reads
`RECON_ORIGINAL_DLL`, loads that path, calls exports with the case arguments and
performs normal detach. The broker matches the loaded DLL's file identity, then
uses its actual module base and entry RVA; host code is outside the App trace.
The same host can be reused across cases for that DLL interface.

A bound-import table occupying the spare section-header slot is relocated as a
validated block into the appended helper section; its timestamps and relative
name offsets are retained. Existing section locations and bytes stay unchanged.

For a DIRECT probe at a genuine live EXE marker, compile case-owned C with
`recon-tool original build-direct CASE/direct.c --output CASE/.build/original-direct.dll`
(optionally `--toolchain`, `--include` and `--define`). Shared Make rules expose
that explicit DLL target; `ORIGINAL_DIRECT_FLAGS` passes extra build-command options and `ORIGINAL_DIRECT_DEPS` declares additional case header dependencies. Ordinary native builds/checks never depend on it.
Then capture with `--direct-helper DLL --marker-rva DECIMAL_RVA --progress`.
The original-only branch includes `recon-original-direct.h` and exports
`case_original_at_marker(ReconOriginalProbe*)`. It reads genuine owners using the
actual module base and saved entry ESP/ECX/EDX, invokes helpers through actual
compiler-generated typed calls, optionally emits narrow result bytes, and calls
`recon_original_finish(context, passed)`. A returning callback fails.

This mode patches only private files before launch: it adds a normal helper DLL
import to the masked scratch EXE and configures the marker in the helper's data.
The helper installs a local VEH and calls the callback on the same application
thread, with live owner frames. The broker records the real marker, passes that
first-chance exception to the helper, and reads its completion receipt at a known
helper INT3. Completion terminates the process without target CRT cleanup or
parser continuation. Typed mode rejects legacy remote allocation/write/call RPCs;
existing debugger entry tracing still restores its own breakpoints. Missing VEH,
unexpected threads, bad receipts or callback failure fail the capture. Initial
support is EXE-only and rejects PE files without a free section-header slot or
with unsupported signed/overlapping layouts. Pristine originals stay unchanged.

The older explicit remote-call interface remains available: direct cases export `case_original_direct(ReconOriginal*)` from
direct.c; see `include/recon-original.h`. Shared primitives read/write remote
objects, allocate/release memory, run to a real setup boundary, and call a typed
entry using explicit ECX/EDX/stack words. A call returns real entry/return GPR and
Win32 floating-save bytes; case assertions stay narrow. `emit` saves named result
bytes as input-file records inside capture.bin for native direct assertions.
Direct results do not establish public invocation reachability or full-process
behavior. The caller context is restored after each deliberate helper call.

Explicit captures are serialized in the shared prefix. A requested logical S:/T:
cwd is reserved only if absent and deleted afterwards; existing directories are
never overwritten. Without one, capture uses its unique temporary directory.
Use a stable logical cwd whenever it is observable, as for debug records. R:/S:/T:
are never remapped per case. Original-capture references/reports are saved outside
the owned ephemeral directory before it is removed.

The original backend supports PE32 i386 images and the diagnostic CF entry-stop
mode described below. Ordinary CF progress and the Watcom gateway remain closed
pending their separate catalog/controller qualification. Direct calls support ordinary
MSVC ECX/EDX/stack conventions, not arbitrary register ABIs such as Watcom watcall.

Current limits: original pipe stdin, timed/console events, new target threads,
zero-entry DLLs and repeated target-module reload
are rejected/unsupported. ANSI transport requires ASCII paths, arguments and
explicit environment strings; arbitrary file/stream bytes are preserved. Native
replay supports saved pipe/events.
Arguments are a complete argv vector, defaulting to `[binary_descriptor.name]`.
`stdin_kind` is `file` by default; set `pipe` for stdin/close events, or
`null` for genuine `/dev/null` (native) / `NUL` (original) device input. Null
input requires omitting `stdin`; it cannot contain stdin/close events. Normal IO
completion uses libc exit with application tracing active through atexit callbacks.

The native SIGTRAP observer records the actual PC/cause, verifies INT3, and resolves
its owning App ELF range to the original RVA. It does not infer the frontier from
the last entry trace or claim instruction-offset equivalence. Reports expose
trap_rva/trap_site. Non-App `implementation` fields must be zero; classified
CRT/import boundary availability is independent of App implementation flags.

`python3 tests/original.py` is an explicitly Wine-using harness test. It checks an
authored EXE invocation, a narrow direct object probe, a DLL through a supplied
Win32 host, and masked progress, then runs each saved result natively. It does not
replay historical reconstruction evidence or alter existing cases.

`python3 tests/make.py` checks shared case discovery, parallel/incremental builds,
original-header dependencies, and optional audit wiring without Wine.

## Native live-state direct probes

`binary.mk` links the tool's uninstrumented entry-hook support once into
`libbinary.so`. Include `recon-native.h` in direct glue and call
`recon_native_at_entry(entry, typed_callback, invoke, argument)`. `invoke` runs the
real main/parser; the one-shot callback has exactly the marker's ABI and receives
its live stack/register arguments. After narrow assertions it calls
`recon_native_entry_finish(passed)`, which returns to the shared checkpoint without
resuming parser mutation. Test `at_entry(...) == 1`: zero means missed/failed,
minus one is a configuration error with errno. Returning from the typed callback
fails closed. Only one hook may be active, on a single native thread.

The marker is recorded through the existing application tracer, despite skipping
its entry instrumentation. The original entry byte and previous signal action are
restored; unrelated traps retain their original PC/cause and observer behavior.
This support is host test glue, with no changes to reconstructed functions.
`python3 tests/native-hook.py` exercises actual i386 cdecl/stdcall/fastcall paths.

Native replay supplies the existing shim's `RECON_STATUS_FD` before module load.
For CRT-equipped modules, full 32-bit terminal status requires a valid PID-owned
receipt matching POSIX's low byte; expected signed JSON values compare as DWORD
bit patterns. Normal adapter returns are recorded before libc exit, avoiding an
identical existing receipt. Tiny modules without the receipt API use POSIX status.
A returned DIRECT boolean always means assertion success/failure; genuine
non-returning target exits compare the capture's expected status, including
`0x8000`. A false boolean cannot be made successful by expecting exit one.

Rebuilt images with reviewed historical CRT source overrides may opt into
`--override-registry PROJECT/.build/functions.json --override-map PROJECT/build/tool.map
--override-object PROJECT/build/obj/src/runtime.c.obj`. Only explicit registry
`category: runtime` names qualify. The tool verifies MAP base/timestamp, unique
selected-object ownership, actual compiled code bytes and REL32 destinations.
Qualified addresses resolve exact named public callees (including same-object
public definitions preempted by the linker); static targets remain strict.
Overrides add no runtime records or reconstruction credit. Other shipped-body
checks remain unchanged. Use `RECON_RUNTIME_OVERRIDE_OBJECTS` and
`RECON_RUNTIME_OVERRIDE_MAP` in the binary Makefile; registry defaults to
`RECON_REGISTRY`, and shared rules track all three inputs.

Original-runtime and pending-entry projection: opt in with
`RECON_AUDIT_REGISTRY_RUNTIME=1` (optional `RECON_AUDIT_ORIGINAL_RUNTIME`), or
`registry-export --runtime-output PATH`. Verified CRT/import code and owned data
are emitted as original VAs; classification adds no App reconstruction credit.
All discovered identities, including unknown-extent entries, also appear in
`function_registry` with RVA extents. Unimplemented App driver rows default to
`pending_extent:true`; only a reviewed whole ABI-only tail may set
`policy.function_properties[name].pending_extent:false`.

Runtime identification understands bounded i386 Microsoft short import members
and the older code-free machine-neutral COFF weak-alias members used in VC5
MSVCRT.LIB. Both describe imports, never CRT function bodies, and are skipped by
code matching. Malformed/unsupported members fail. Dynamic import thunks remain
the graph auditor's PE-import catalog responsibility; a pure /MD image can
legitimately produce an empty matched-static-runtime array.

Classified runtime entries provide endpoint names; they do not automatically
become audited CRT implementations. `policy.runtime_audit_names` is a finite
list of previously reviewed CRT/import graph vertices to qualify for
`--include-evidenced-runtime`. Every listed name must have a classified verified
extent. Other classified records retain identity/ranges without graph evidence.
`function_registry` also carries DLL/name/ordinal import identities.

For a pipe that must stay open while the target waits for a signal, set
`hold_stdin_open:true`. It is valid only with `stdin_kind:"pipe"`; explicit
`close_stdin` still closes after queued bytes drain. Otherwise the existing
finite-input automatic EOF behavior is unchanged.

An explicit second audit view can use `registry-export --accepted-scope NAME`.
`policy.accepted_scopes[NAME]` must be a nonempty array of unique, currently
accepted registry names. Missing, stale or unaccepted names fail; the selected
view changes only `accepted`, retaining the complete driver/identity indexes.
The normal export and full graph audit remain unrestricted.

Rust host memory uses a pinned, prefixed jemalloc allocator with arena reuse.
This separates Rust registry/ELF/parser buffers from the application's unchanged
libc malloc/free/realloc heap. Build settings disable DSS (sbrk) and background
threads, use one host arena, and keep 32-bit virtual-memory retention disabled.
The startup guard validates the effective settings before CLI dispatch or any
application load; it fails instead of changing an already initialized heap.
No C malloc symbol replacement, LD_PRELOAD, target zeroing or heap tuning is used.

FFI arguments, pointer arrays, trace labels and descriptor views remain borrowed.
C must not free/realloc Rust-owned storage; Rust must not adopt target/libc-owned
buffers as Vec/Box/CString ownership. Creator-owned release remains required for
any future ownership transfer. jemalloc's ordinary atfork hooks cover inherited
host allocations, while runner forks remain single-threaded and outside signal
handlers; no background allocator threads run. Loader/libc allocations remain,
so this separation does not guarantee values of unwritten target fields.

Focused allocator checks are available as the host_allocator_contract Rust test
(run with --test-threads=1). They cover alignment/reallocation, arena reuse,
libc symbol ownership, and inherited live allocation behavior across fork.

Native execution always installs a process boundary. Without an explicit case
policy every RSP1/RSP2 request is denied, with no fallback to a host program. A
rejected request fails the entire fixture even if the application handles the
rejection. Ordinary host controller forks remain supervised independently.
Unit mode has the same deny boundary but does not consume the IO request budget.


## Capture-owned fixture assets

Optional `assets:[{"path":"captures/response.bin","data":"hex:00ff2a"}]`
records use the same exact byte encoding as input files. They live read-only in a
separate harness directory outside the application's cwd, so complete target file
inventories remain meaningful. Paths must be unique canonical relative paths;
file/directory collisions and symlinks are rejected. After all descendants exit,
the runner checks the complete asset inventory, bytes and read-only modes.

Native fixture glue can include `<recon-fixture.h>` and call
`recon_case_assets()`, `recon_case_workspace_cwd()` and
`recon_case_logical_cwd()`. Borrowed paths are configured before setup; absent
assets/logical cwd return NULL. Assets are host metadata, never a target
environment variable (`RECON_CASE_ASSETS` is removed). The shared make include
links the small uninstrumented support once in libbinary.so. This optional capture
field preserves version 2 compatibility; ordinary `input_files` remain target
inputs.


## Captured process responses

A case may export `const ReconProcessPolicy case_process_policy` from
`<recon-process.h>`. Set the version, exact structure size, expected request count
and typed callback. The callback receives the complete request and a trusted
prepared environment/workspace snapshot; it must check the complete invocation
and relevant input bytes before replaying declared file effects and replying.
Captured streams belong to the response, not callback stdout/stderr. Callback
work is untraced in a separate pristine post-setup broker; its diagnostics stay
outside asserted target streams. Request/response pointers have the borrowed
lifetimes documented in the header. Assets can hold large captured bytes without
adding harness files to the target tree.

The broker supports RSP1 captured status `-1` (errno 1..4095) or 0..255, with exact
stdout/stderr. RSP2 keeps its existing reviewed success/parent-state contract.
Malformed replies, rejection, extra or missing requests fail the fixture. The
supervisor hands over its socket before target execution; the broker closes
inherited client, status and trace endpoints, and finalizes the request count
only at EOF after all target client holders exit. Unit mode does not run this
IO provider or require its expected requests. Intentionally suspended responses
are unsupported. No original executable or Wine is invoked.

For a genuine synchronous same-program child, set the input environment
`RECON_SELF_SPAWN=1` and return `RECON_PROCESS_SELF` only after validating the
complete request and its inputs. SELF requires RSP1 and an otherwise empty,
zero-status response; RSP2 retains its captured-success-only contract. The shared
shim matches the canonical program path and launches the registered public command:

```text
recon-tool native-child /absolute/case.so --workspace /tmp/recon/run-... \
  --cwd /tmp/recon/run-.../case-cwd --trace-fd N --trace-state-fd N -- ARGS...
```

The command uses an existing supervisor-owned workspace and inherited descriptors.
`ARGS` includes the application's argv[0]. Its actual cwd, environment and standard
streams are inherited normally; `--cwd` retains the prepared fixture path accessor.
The launcher forks before loading the target DSO, giving the executor fresh
application/CRT globals, configures the existing fixture paths, reruns setup, and
calls the IO adapter. The shared RST1 bitmap attaches without resetting, so parent
and children record each application first entry only once. Nested policy requests
use the requesting executor's post-setup environment snapshot.

Readable `children/PID/result.json` files outside the application cwd record the
kernel terminal, full DWORD receipt, and any child INT3 owner RVA/site. Results
are included in the final report's `process_requests.self_children` before workspace
cleanup. The broker checks the permitted SELF count against completed results.
Unexpected child signals, setup failures and invalid/missing receipts fail the
whole fixture even if the application parent handles the child failure. The
existing supervisor owns descendants, timeout and cleanup.

The launcher preserves a normal exit byte, including nonzero status 7, and
propagates real terminal signals. The current shim's synchronous parent sees only
the POSIX exit byte; full DWORD values are retained in reports, not propagated as
full DWORD parent statuses. Run `python3 tests/self-child.py` for focused native
self-child controls.

## Post-exit file checks

A case may export `const ReconOutputPolicy case_output_policy` from
`<recon-output.h>`. Its typed `check_outputs` callback runs in a pristine snapshot
forked after setup, with tracing off and private diagnostics. The supervisor sends
GO only after reaping the executor and every other execution descendant; the
checker closes inherited transport endpoints and cannot hold the process broker
open. Callback failure, crash, cancellation or timeout fails the fixture; the
checker is reaped before workspace cleanup. Setup/constructors are not rerun.

`ReconOutputContext` supplies the actual executor PID, full DWORD exit status or
signal, actual cwd/assets paths, raw immutable stdout/stderr paths, and separate
comparison copies. The callback is for files and outputs, never target post-exit
globals. A case-specific dynamic substitution must first validate actual values
and exact identified offsets, then change only comparison copies. Raw streams
are hashed and checked unchanged. Generic frozen stream/file assertions always
run after the callback; a true callback cannot override them. Execution timing stops when the target and all nonchecker descendants are
reaped. The supervised checker phase, dependency/asset validation and generic
assertions contribute to checks_ms.


Optional native APIs and case policies are checked against the actually defined,
visible dynamic exports of the loaded case/application ELF modules before any
loader lookup. Missing exports return absent without dlsym/dlerror: loader error
strings otherwise allocate/free memory in the target's libc heap. A declared
export that cannot resolve remains a hard error. This complements Rust allocator
isolation; it does not initialize unwritten application fields.


Compiled historical CRT override anchors use the unique selected-object MAP
owner, so a same-named shipped library definition remains a separate identity.
Explicit CRT entries in the authoritative function registry may qualify compiled
callee anchors even when their original extent is pending. Exact compiled bytes,
relocations and object ownership remain mandatory. This adds no original runtime
body, availability, reconstruction credit or graph acceptance; newly qualified
callee addresses can also reject previous unvalidated byte-shape aliases.


Native execution infers the repository before changing cwd/loading the target and
defaults `RECON_DRIVE_R` to that root, matching the shared original prefix. An
explicit capture value or removal takes precedence, including unit forks. A
capture's explicit logical R drive can still select its isolated workspace when
no environment override is declared. Standalone fixtures without a discoverable
repository leave this default unset. Wine mappings are never changed.

One-shot native entry observation uses
`recon_native_observe_entry(entry, observer, observer_argument, invoke, argument)`.
The observer makes only signal-safe scalar/pointer writes inside SIGTRAP; the
real prologue, ABI, entry instrumentation and caller then execute normally.
Registers, stack/FP context and errno are restored. It returns one if observed,
zero if missed, or minus one for invalid configuration. It shares the one active
hook with the typed finish hook; calling entry_finish in observer mode fails.

Optional capture `readonly_tree_roots` lists absolute dependency directories.
Before/after execution, the runner verifies their complete regular-file membership
against existing `readonly_dependencies` paths and hashes. Additions, missing files,
changed bytes, symlinks and special files fail. Every root needs declared files;
empty directory membership is outside this file-set contract. This adds an optional
version-2 capture field without changing the C descriptor ABI.


`original build-cf-helper --repo ROOT` builds the private Watcom debug transport.
The first CF frontend mode is explicitly diagnostic:

```sh
recon-tool original capture CASE --repo ROOT --capture INPUT \
  --cf-profile PROFILE --entry-stop --helper CF_BROKER \
  --output UNUSED_ORACLE_PATH --report REPORT
```

This mode currently recognizes only the hash-bound WCC386 fixed loader protocol.
The profile identifies the original binb payload, its base-zero CF header and
relocation metadata, and the separate binnt loader. Both retain their original
sibling paths. Fixture arguments, stdin, cwd and environment remain inputs; the
profile cannot supply executable instructions or admission rules. The shared
Windows environment overlay is applied to the debuggee, and its requested keys
are checked against the actual payload environment before the handoff.

The controller verifies the complete relocated loader code/data and CF image,
then executes six fixed loader instructions and stops at the actual CF entry
INT3. It executes zero CF body instructions. The report includes the observed
allocation, real context/selectors/frame, byte identities, owned teardown, and
backend source/helper hashes; floating state is explicitly absent. It writes no
fixture at `--output`, records no App entries, and rejects `--artifacts-dir`.
Use `--report` to retain success or failure diagnostics across workspace cleanup.
Ordinary CF progress, CF DIRECT and other CF loader geometries remain unavailable
until their separate source/catalog and execution controls have qualified them.
Native replay and the PE capture backend keep their existing paths.


A binary that has several public method names for one physical body can set
`RECON_ALIAS_MAP = symbol-aliases.json` before including `recon-tool.mk`.
The map contains names only:

```json
{"schema":1,"aliases":[{"win32_alias":"?Reset@Plain@@UAEXXZ","win32_target":"?Reset@Lexer@@UAEXXZ","native_alias":"_ZN5Plain5ResetEv","native_target":"_ZN5Lexer5ResetEv"}]}
```

The shared rule generates `.build/aliases/symbol-aliases.obj` (zero COFF sections,
weak externals with SEARCH_ALIAS=3) and `symbol-aliases.rsp` (GNU linker aliases).
It adds the object to the original link and the response to the native link;
changes to the map relink both. The response is a dependency, never an object
input. `recon-tool build-aliases MAP --coff OBJ --native-rsp RSP` also generates
these files explicitly without Wine. Symbols are validated names, with no
arbitrary linker expressions, duplicates, alias chains or cycles. Missing
canonical bodies produce ordinary link errors. VC5 LINK and native i386 vtable
controls qualify this metadata format; the binary still owns the ABI/identity
proof and ordinary graph/vtable checks. Keep one canonical registry function and
body; aliases add no function extents, wrappers or reconstruction credit.

For a fresh original/native pair, pass its original receipt explicitly:

```sh
recon-tool run CASE --capture original.bin --original-report original-report.json --report native-report.json
```

This compares the actual native first-entry order and terminal/frontier against
the selected original receipt, overriding the case descriptor's cached trace and
terminal expectations. `--capture` by itself selects invocation and I/O data;
trace and frontier expectations then come from the descriptor.

New original receipts bind the exact capture SHA-256 and binary/case/execution
identity. A mismatch rejects before the application callback runs. An explicitly
selected older receipt remains usable: the native report records both selected
file hashes and `source_binding_available: false`; no source identity is inferred
or added to that old receipt. The option requires one case and `--capture`.
