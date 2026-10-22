Visual C++ 1.0 for NT: LINK.EXE reconstruction

Target: toolchains/1.0-nt/bin/LINK.EXE. Build with its original compiler,
linker, headers and libraries through Wine. Program logic is readable period C;
all 70 formerly lifted implementations and their generators were removed. Only
INT3 statements in pending named stubs remain. No assembly ABI exception exists.
Data is contextual and typed; no original section snapshots are retained.

C sources and headers live in src/; executable Python tooling lives in scripts/.
Run make and script commands from this binary root. Metadata, cases and build/
retain their existing roles. analysis/ is non-executable evidence only.

Eighteen cases are complete:
  /? (exit 3, exact 884-byte stdout, empty stderr)
  minimal-c, hello-win32, hello-dll
  library-create, hello-debug-dll
  library-list, library-extract, library-remove, library-list-trimmed
  library-dump-library, library-dump-object, library-dump-dll
  library-edit, library-dump-edited
  omf-library, omf-link, cpp-undecorate

The two latest assigned families use immutable original-backed fixtures
link-omf and link-undecorate (86a7f87). The valid 32-bit OMF input covers segment,
public/external, explicit-allocation COMDAT, LEDATA and FIXUPP records. Both
archive conversion and direct linking match; the runner checks external 20 +
ordinary data 7 + COMDAT 15 = 42 and exits 0. Converted COFF preserves the original
symbol records, including its duplicate COMDAT definition/undefined records.
C++ archive dump/undecoration matches all 5511 stdout bytes, empty stderr and
exit 0 without normalization. Its original-toolchain source covers constructors,
destructors, inheritance, operators, qualifiers, overloads, arrays, references,
member pointers and function pointers. Templates were rejected by the original
compiler and are not claimed. Unreached demangler branches remain named traps.

Current verification: clean original-toolchain make -j2, full 640 historical
frontier replay, all 18 completed-case regressions, 492 direct/indirect/tail-call
checks, 483 callback checks, 391 contextual-data initializer checks, 646 original
extent instruction-boundary checks and 38 ECX-member return-cleanup checks pass.
cases/omf-undecorate-queue.json records case provenance and verification hashes.
SOURCE-NOTES.txt retains historical checkpoints and recovered implementation
choices; later completed checkpoints supersede earlier active-frontier notes.

The accepted .text extent union is 193671/281634 bytes=68.7669%, up from
160229/281634=56.8926% at 528 frontiers and 110787/281634=39.3372% at 400.
This includes accepted inline tables/alignment, not dynamic branch coverage.
cases/coverage-after-omf-undecorate.json records the union. Separate partial
body accounting is reproducible with scripts/measure-components.py and the analysis-only
Ghidra inventory: 127920 accepted known target-body bytes, 26904 known CRT-body
bytes and 192 import-thunk bytes. Another 38655 accepted extent bytes are outside
that old body inventory. All one-byte inventory entries are excluded. The
unclassified remainder includes uninventoried bodies, inline data and padding;
no exact padding total or target-wide body percentage is claimed.

The expanded earlier cases cover C++ archive/member lifecycle and COMDATs,
full COFF/CodeView/FPO debug DLL linking, native CVPACK, resources, DEF/order
parsing, map/verbose output, archive/COFF/PE dumps and PE stack/heap editing.
Generated console programs exit 0. GUI executables and DLLs pass load/call,
300 painted pixels, WM_CLOSE, exit 0 and DLL unload where applicable. Streams,
status and artifacts match the frozen fixture and untouched-original oracles.

Normalization remains limited to structurally proved clocks and the independently
validated PE checksum derivation where an earlier fixture requires it. No path,
padding, output-order or arbitrary-byte normalization is used. OMF fixtures
normalize only proved archive/COFF/PE clock fields. Undecoration is raw-exact.
The editor's 71800-byte edited.dll is raw-exact, including stack 0x180000/0x2000
and heap 0x140000/0x2000. Native /release sets checksum 1 then emits LNK4058:
checksum calculation is unsupported on this OS. No success behavior is invented.

The debug DLL uses default mapped IO. Archive lifecycle/OMF/undecoration use
LINK32_DB=27, the buffered backend. Under Wine original mapped archive creation
otherwise leaves its 2 MB allocation and subsequent listing reports corruption.
Inputs are not trimmed or patched. Extracted alpha is raw-exact and the trimmed
archive omits beta. Native DEF limitations remain: NONAME and forwarder fixture
inputs exercise parsing, not effective ordinal-only/forwarder output.

Build/check help:
    make -j2 check
Check a fixture (the harness supplies its original environment):
    python3 scripts/compare.py --case cpp-undecorate --require-exit
    python3 scripts/compare.py --case omf-link --require-exit
Replay every recorded frontier and finish with undecoration:
    python3 scripts/compare.py --replay --case cpp-undecorate --require-exit
Measure accepted extents and partial native-body components:
    python3 scripts/measure-coverage.py
    python3 scripts/measure-components.py

The harness masks the entire original code section and restores only accepted
function extents and encountered inline data. Unaccepted reconstruction functions
and its entry are masked too. Each pair runs concurrently and must stop at the
same logical function. Per-run Wine namespaces expose the matching masked tool
copy at its canonical original DOS path; other toolchain files resolve to
untouched originals. argv[0], recursive verbose text and self-spawned LINK
children remain exact. The debugger follows the full child tree with per-process
image bases and loader breaks. Original CVPACK remains an external dependency.
Focused reconstruction acceptance retains fresh paired execution. Ordinary
historical segments now use continuous replay; specialized histories retain
the existing runner. Original toolchain files are never modified.

Runtime members come from relocation-target-verified original LIBC objects.
Older CRT differences, including text-mode ftell/fseek, are readable C in
src/runtime.c. The native C parser rejects __thiscall; demangler methods instead
use __fastcall(ECX this,ignored EDX,native stack arguments). This preserves
native ABI and individual graph vertices without wrappers or assembly. Recorded
symbols and scripts/audit-abi.py verify native callee return cleanup.

Both assigned families are finished; no further cases are queued. Analysis-only
cases/remaining-after-omf-undecorate.json ranks possible public-input follow-ups:
i386 -dump /disasm (~3707 uncovered known body bytes/12 functions) is practical;
valid Macintosh/m68k COFF linking (~16534 bytes/57 functions in a named-root
closure) is a higher-effort alternative requiring original-backed input first.
Remaining OMF closure 9800 bytes includes internal -g conversion code not selected
by the ordinary LINK wrapper (-o/-co), so that total is not promised for public
OMF debug input. Remaining Mac/ROM/bind/rebase/disassembler/diagnostic/dormant CRT
paths and unclassified bytes mean 99% whole-binary coverage is not established.
Shared fixtures remain immutable. Scratch/evidence/private Wine prefixes live
under ~/.ai-tmp/vc1.0-link. New cases require the user's next selection.

Fast historical replay

The default historical command now walks ordinary segments continuously:
    make replay
    python3 scripts/compare.py --replay --case cpp-undecorate --require-exit
The latter retains the old final comparator for the requested case. Ordinary
segments still validate final streams, artifacts and generated programs using
the existing fixture rules and an independent untouched-original run. This
checks first encounters, not repeated calls/branches or live-state equivalence.

Select one segment, batch specialized histories or explicitly restart every
historical frontier:
    python3 scripts/replay-frontiers.py --case minimal-c
    make replay-specialized
    make replay-fresh
    python3 scripts/compare.py --fresh-replay --case cpp-undecorate --require-exit

replay-cases.json declares the initial migration: 510 frontiers in 12 ordinary
acceptance groups, plus four zero-step completed-case checks. DLL import-library
self-spawn and full-debug/CVPACK retain 130 frontiers in the existing specialized
DEBUG_PROCESS runner. Batch those histories at meaningful checkpoints and case
completion, or sooner when a change directly affects them. An explicitly selected
specialized case uses that runner. Unknown fast cases are rejected. This policy
does not defer focused fresh paired acceptance of any newly reached function.
GUI painting and DLL load/call/unload remain post-link checks.

Independent walkers use the shared tooling/replay helper. LINK retains its
canonical per-side DOS namespaces and complete prepared launch environment,
including inherited-environment semantics, LIB and buffered-I/O settings.
Registry terminals are case-specific: help exits 3; other completed cases exit 0.
Each segment restores its prior accepted prefix, then only its own introduced
frontiers. Shared text ranges, import thunks and indexed CRT entry aliases are
included. The whole source/build/index/fixture batch and individual plans are
hashed; changes during replay fail. Preferred bases and root-only process/thread
execution are enforced; unsupported tree/thread modes stay on specialized paths.

For a divergence, preserve the receipt directory and use its failure.json, native
events/context, actual logical site and rerun commands. For example:
    python3 scripts/replay-frontiers.py --case minimal-c --fresh-frontier 232
    python3 scripts/replay-frontiers.py --case minimal-c --stop-at __inc
The continuous rerun leaves that terminal entry masked; it does not accept or
reconstruct it. Restart cleanly after any failure or rebuild. No acceptance
metadata is changed by replay.

cases/fast-replay-validation.json records the frozen build, complete fresh/fast
sequence agreement, final oracles, LINK-specific stop/fallback tests and measured
per-case wall times. Setup, fast pair, final output oracles and fresh history/
terminal calls are separated where measured. Unsplit overhead and unmeasured
clean-build/startup time are labeled. Other workers were active during timing;
prototype speedups are not assumed for this adapter.

Native Linux output checks

The original-toolchain Win32 build remains the default. An additional i386 ELF
build compiles the same reconstruction sources with the shared lib/recon shims:
    make -j4 native
    make -j4 native-check
    python3 scripts/check-native.py --case cpp-undecorate

Clang, 32-bit libc development files and a working -m32 compiler are required.
The native flags are -std=gnu89 -fms-extensions -fasm-blocks -O0 -g; the Microsoft
extensions express the existing calling conventions and pending INT3 stubs.
RECON_NATIVE_TEST enables the header mappings. Every reconstruction C/header
includes recon.h last; the header is empty in the canonical build. Native object
files live in build/native and the final link compiles lib/recon/src/*.c directly
alongside those objects. No shim archive or alternate reconstruction is used.

scripts/check-native.py launches only the native ELF. It verifies captured input
and output hashes before launch, compares exit and streams exactly, and compares
artifacts using the fixtures' existing structural timestamp rules. Missing
captured references fail. Toolchain libraries and the editor message catalog are
read-only inputs with previously captured hashes. It never invokes Wine, an
original executable, reference-refresh tooling, generated PE programs, or the GUI
and DLL-load helpers. The native clock runs normally; only the timestamp fields
already declared by the immutable fixture may differ. Pacific timezone settings
match the captured NT CRT rendering, including daylight saving.

The ordinary native set comprises help, archive create/list/extract/remove,
trimmed listing, four dump phases, editor, OMF library/conversion link, C++
undecoration and the minimal C link. Each phase starts independently with captured
inputs. Fresh private working directories and timing receipts are written under
~/.ai-tmp/vc1.0-link/native-check-*. Receipts separate setup, native execution and
output checks; build timing is labeled unmeasured unless separately recorded.
Produced executable bytes are checked but never executed by this runner.

This output checker does not implement native historical first-entry tracing or
replace the canonical ABI/graph and paired-frontier audits. Windows SEH fault
paths, console-signal delivery and process-tree tests remain outside its scope.
DLL import-library self-spawn, full-debug external CVPACK, GUI paint and DLL
load/call/unload cases are not registered as native checks. The shim process
boundary rejects PE dependencies instead of automatically running Wine.

cases/native-validation.json records the completed native adoption: all fifteen
registered phases passed with exact captured streams/status and artifacts under
the existing timestamp rules. It records per-case setup/execution/oracle times,
three earlier diagnostic fixture batches, measured build receipts and explicitly
unmeasured diagnostic/build work. Raw receipts and logs remain in the recorded
scratch directories. The original-toolchain parallel compile/link and all static
call-target, callback, contextual-data, ABI and extent-boundary audits passed.
The accepted 640 frontiers and 193671/281634-byte extent metric are unchanged.
