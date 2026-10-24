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
single registry for the existing graph auditor. Policy data must not duplicate
`target`, `accepted`, or `driver`. Original-compiler builds and audits remain
separate. Native execution never invokes Wine.

Include `include/recon-tool.h` for the versioned C ABI. A binary Makefile defines
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
ORIGINAL_SOURCES := $(BINARY_SOURCES) src/runtime.c
ORIGINAL_BUILD := build
ORIGINAL_OUTPUT := build/example.exe
ORIGINAL_CC := python3 scripts/wine-tool.py CL.EXE
ORIGINAL_LINK := python3 scripts/wine-tool.py LINK.EXE
ORIGINAL_CPPFLAGS := /Isrc /I../../../lib/recon/include
ORIGINAL_CFLAGS := /nologo /c /Od /G3
ORIGINAL_LDFLAGS := /nologo /subsystem:console
ORIGINAL_LDLIBS := ../../../toolchains/1.0-nt/lib/BINMODE.OBJ
ORIGINAL_INCLUDE_DIRS := src ../../../lib/recon/include ../../../toolchains/1.0-nt/include
ORIGINAL_EXTRA_DEPS := scripts/wine-tool.py
include $(RECON_TOOL_DIR)/recon-tool.mk
# Local generated-source rules and target-specific audits follow.
```

`all`, `native` and `native-trace` build the library. `cases` discovers numbered
`cases/NNNN-name/Makefile` directories and includes their settings to generate
object and `case.so` targets in the parent Makefile. `check` and `unit` depend
directly on those files; binary-root builds never launch case sub-makes. Each
case library has an order-only dependency on the common library. `check`/`replay`/`fixtures` run all discovered cases;
`unit`/`test-all-cases` run their unit tests. Optional `RECON_RUN_FLAGS` and
`RECON_TEST_FLAGS` pass runner options. These targets never depend on the original
build. Standalone case builds update the common library incrementally. Case Makefiles
include `$(RECON_TOOL_DIR)/case.mk`, with a standalone fallback such as
`RECON_TOOL_DIR ?= $(abspath ../../../../../tooling/recon-tool)`. Use `CASE_CC`,
`CASE_CXX`, `CASE_CPPFLAGS`, `CASE_CFLAGS`, `CASE_CXXFLAGS`, `CASE_LDFLAGS` and
`CASE_LDLIBS` for per-case settings. Compilers default to the binary settings
(or GCC standalone); flags are isolated per case. Relative include paths should
use `$(CASE_DIR)` (the case path at the binary root, or `.` standalone), for
example `CASE_CPPFLAGS = -I$(CASE_DIR)/../../src`.
`clean` removes `BINARY_BUILD` (default `.build`), `ORIGINAL_BUILD` (default `build`),
and each discovered case's `.build`.

`win32`/`recon-original` build `ORIGINAL_OUTPUT` when `ORIGINAL_SOURCES` is set.
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
checks/input generation. `RECON_AUDIT_REPORT` defaults to
`$(ORIGINAL_BUILD)/graph-audit.json`. `audit` depends on `recon-graph-audit`, which
uses the graph tool's quiet build include. Append local data/grammar targets or an
`audit` recipe without duplicating the graph invocation.

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

## capture.bin version 1

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
these are inputs, not undisclosed implicit references. Native `output_env`
observations run during normal process exit, after application exit callbacks.
Abrupt termination such as `_exit` bypasses that observation.

Events are ordered. Each has optional `after_ms`, optional `stdout_contains`
(both conditions must hold if present), and an `action` object:
`{"kind":"stdin","data":"text:..."}`, `{"kind":"close_stdin"}`, or
`{"kind":"signal","signal":2}`. Close waits for queued bytes to be written.
Set `stdin_kind:"pipe"` for stdin/close events. Signals go to the executor process group. These are Linux signals, not yet a
claim of arbitrary Win32 console-event equivalence.

Top-level expected outputs describe successful completion. Optional `partial`
has its own stdout/stderr/output_files/output_env/reject_unexpected_files and
applies at an expected current frontier. Final expectations are retained intact.

Run `python3 tests/smoke.py` for focused native integration tests. Ordinary IO
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

## Explicit original capture

```sh
recon-tool original build-helper --repo /root/msvc2
recon-tool original capture /path/to/case --repo /root/msvc2 \
    --capture ~/.ai-tmp/inputs.bin --output /path/to/case/capture.bin \
    --report ~/.ai-tmp/original-result.json
```

The helper is compiled with the existing VC1.0 original compiler. Its sources and
build outputs live under this tool; no Python program is needed per binary/case.
`capture` verifies the original SHA256 and entry against binary_descriptor. The
optional `--program` selects that verified original path explicitly. Existing
output files are not overwritten without `--replace`. Capturing is always an
explicit command, never a side effect of run/test.

Ordinary capture executes the untouched original and records exact stdout/stderr,
exit status, changed/new/deleted files (hashes), and application first-entry RVAs.
`--progress` creates a temporary masked copy, restoring classified CRT/import
boundaries and implemented App code/owned text data. It records a named pending
App INT3 and partial observations while preserving the final oracle. It does not
execute that pending body or automatically update function acceptance/case source.
Reports include actual order and terminal so the reconstructor can deliberately
update expectations after matching both sides.

For DLLs, `--host PATH.exe` supplies typed Win32 invocation glue. The host reads
`RECON_ORIGINAL_DLL`, loads that path, calls exports with the case arguments and
performs normal detach. The broker matches the loaded DLL's file identity, then
uses its actual module base and entry RVA; host code is outside the App trace.
The same host can be reused across cases for that DLL interface.

Direct cases optionally export `case_original_direct(ReconOriginal*)` from
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

The original backend supports PE32 i386 images. CF payloads and the Watcom gateway
remain in their existing separate capture tooling. Direct calls support ordinary
MSVC ECX/EDX/stack conventions, not arbitrary register ABIs such as Watcom watcall.

Current limits: original pipe stdin, timed/console events, generic output_env
observation, new target threads, zero-entry DLLs and repeated target-module reload
are rejected/unsupported. ANSI transport requires ASCII paths, arguments and
explicit environment strings; arbitrary file/stream bytes are preserved. Native
replay supports saved pipe/events and returned-adapter output_env observations.
Arguments are a complete argv vector, defaulting to `[binary_descriptor.name]`.
`stdin_kind` is `file` by default; set `pipe` for stdin/close events. Normal IO
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
