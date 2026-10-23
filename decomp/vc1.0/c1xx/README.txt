C1XX3232.EXE reconstruction (project vc1.0, toolchain 1.0-nt)
==========================================================

PARTIAL binary reconstruction: 855 accepted frontiers, including 762 frontend
functions and seven older CRT functions reconstructed as source. cpp_basic is
complete. virtual_diamond currently stops at _normalize_initializer_expression@4,
original VA 0x47dc0. All unimplemented callees remain meaningful named INT3 stubs.
The approved copy/assignment, stack-lifetime/destruction and new/delete/operator
cases follow virtual_diamond; cases/reconstruction-queue.json records the queue.

Accepted original extents cover 233719 / 476418 code-section bytes (49.057550%).
The union includes function bodies, shared text data, exact matched CRT and import
thunks, clipped to section VirtualSize (backed raw size only if zero). This is
not branch coverage. Whole functions include statically recovered unvisited paths.

Native build and regressions
----------------------------
Run from this binary directory:

    make -j4
    make -j4 check
    make -j4 native-check
    make -j4 native-rejections

Source is Win32 C under src/, scripts are under scripts/. Native builds use
Clang i386 -O0 -g; shared lib/recon source files are linked directly. Original
BINMODE.OBJ behavior is retained with RECON_DEFAULT_FMODE=0x8000. Application
instrumentation excludes target runtime.c and shared CRT/import/shim entries.

check/replay run only native cached fixtures. They check first-entry order,
terminal status, raw stdout/stderr, complete file sets and all four frontend
intermediates. Unknown definitions, modified references and external processes
fail. No fixture invokes Wine or refreshes an oracle. cpp_basic expects 553
application first entries and exit 0. virtual_diamond seeds that accepted prefix,
then expects 209 new application entries plus its named terminal trap.

cases/original-entry-references.json binds fresh original first-entry plans,
raw events and capture receipts. Each debugger first-entry trap is checked,
restored, instruction-cache flushed and EIP rewound. Missing/out-of-order entries,
unexpected exceptions and early exit reject the capture. functions.json retains
the ordered 855-entry acceptance provenance and full original extents; the cache
also classifies exact CRT/import entries. The projection cache is rebuilt only
explicitly after verified metadata changes:

    python3 scripts/native-traces.py --write

Original compiler and capture operations
----------------------------------------
Explicit operations, separate from fixtures:

    make -j4 win32
    make -j4 audit
    python3 scripts/capture-original.py --case virtual_diamond --entry-plan
    python3 scripts/capture-native.py --case virtual_diamond
    python3 scripts/coverage.py

Original builds and captures use ~/.ai-tmp/vc1.0-capture/wine. Fixed R: maps to
/root/msvc2 and S: to ~/.ai-tmp/msvc2. No tool creates/remaps drives or stops the
shared server. Cases use S:\vc1.0\c1xx\<case>; own explicit captures serialize
cwd access with ~/.ai-tmp/msvc2/vc1.0/c1xx/capture.lock. Inputs remain relative.
Native runs map each process's S: to its isolated host namespace while exposing
that same canonical cwd. The actual CL child argv0 and sole MSC_CMD_FLAGS block
are preserved; ordinary argv is not substituted for the frontend transport.

scripts/capture-shared-fixtures.py explicitly captures all five approved sources
from original CL, independent C1XX/C2/LINK phases and the final silent exit-0
programs. It writes a fresh staging directory and never runs from a fixture.
The frontend streams compare raw. Only independently demonstrated COFF and PE
TimeDateStamp DWORDs may differ in downstream comparisons; each differing offset
is recorded. No path or pointer normalization occurs.

Fresh function acceptance additionally compares four pending FILE buffers,
fourteen checkpoint state words, raw streams, full visible files and the exact
canonical original-toolchain call graph/ABI. capture-native.py records GDB state;
advance-frontier.py requires a fresh original entry-plan capture and persists its
new reference only after the matching full oracle. The runtime-chain counterpart
also requires exact library relocation/callee/global identity and explicit
original/native argument/return evidence for newly encountered CRT semantics.

Shared-prefix migration and retained analysis
--------------------------------------------
All five phase fixtures were freshly recaptured, including an owned
../fixtures/c1xx-cpp-basic copy. The shared ../fixtures/cpp-basic input is untouched.
cases/shared-prefix-validation.json records measured build/capture/native phases,
exact comparisons, rejection checks, affected fixtures and private-prefix retirement.
The private transport histories and alternate oracle bundles have been removed;
their original bytes remain in Git at 0812fa9 and were never relabelled.

Independent ABI/layout/branch analysis remains under evidence/. In particular,
evidence/runtime-analysis retains unchanged historical raw CRT observations with
explicit original path/hash provenance, separate from active frontend references.
These observations establish close, flush, exit, realloc and overlapping-copy
semantics; all required offline corruption checks remain enforced. Named raw
historical logs are read-only analysis, never executable transport configuration.
evidence/function-analysis and recovered-implementation-notes.txt retain contextual
function findings. historical-analysis-index.json binds retained bytes to their
original paths. historical-replay-timings.json retains old measurements without an
alternate runner/oracle dependency. Current migration timings are separate.
