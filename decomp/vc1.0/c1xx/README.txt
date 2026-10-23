C1XX3232.EXE reconstruction (project vc1.0, toolchain 1.0-nt)
==========================================================

PARTIAL binary reconstruction: 993 accepted frontiers, including 899 frontend
functions and seven older CRT functions reconstructed as source.
Complete cases: cpp_basic, virtual_diamond, copy_assignment, stack_lifetime.
Active case new_delete stops at _make_class_delete_call@12, original VA 0x38390.
All unimplemented callees remain meaningful named INT3 stubs.
The user selected preservation of scope entry's original Win32 uninitialized
token bug, with an explicit native-only INT3 when cached parser lookahead would
read that caller-derived stack byte. That malformed-input path is unsupported
natively and is not claimed equivalent. evidence/implicit-scope-lookahead records
the instruction, grammar repair counterexample, last writer and original probe.
Multiplication overflow checking retains the original Win32 uninitialized
threshold for signed opposite-positive-status operands with unsupported widths.
That exact native path traps; malformed/internal widths are not claimed equivalent.
Normal integer widths1/2/4 and the actual lifetime invocation have exact original/
native return evidence under evidence/multiply-boundary. native-rejections checks
all three documented guards without running Wine.
Include-directory search preserves the original Win32 unwritten last-character
pointer when a nonnull directory is empty. Valid public -I "" can succeed in the
original using a stale caller strchr argument; native execution explicitly traps
on that exact branch. It is a third coverage limitation, not a malformed-only
input or a universal stack invariant. evidence/empty-include-directory retains
the successful original probe, public producer and exact observed last writer.
The copy/assignment and stack-lifetime cases are complete; the approved
new/delete/operator case is active. cases/reconstruction-queue.json records it.

Accepted original extents cover 257272 / 476418 code-section bytes (54.001318%).
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
fail. No fixture invokes Wine or refreshes an oracle. cpp_basic expects 553 application first entries and exit 0.
virtual_diamond expects 225 application first entries and exit 0.
copy_assignment expects 61 application first entries and exit 0.
stack_lifetime expects 31 application first entries and exit 0.

cases/original-entry-references.json binds fresh original first-entry plans,
raw events and capture receipts. Each debugger first-entry trap is checked,
restored, instruction-cache flushed and EIP rewound. Missing/out-of-order entries,
unexpected exceptions and early exit reject the capture. functions.json retains
the ordered 993-entry acceptance provenance and full original extents; the cache
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

Shared host graph audit
-----------------------
make audit builds the shared tooling/graph-audit host binary once through its
quiet Make include, then invokes it directly. Cargo build errors and the tool's
own diagnostics remain visible. Ordinary native fixtures never build or invoke
the auditor, Cargo or Wine. The audit command uses the c1xx profile with CFG
discovery plus --check-ret, --check-references, --check-callbacks,
--check-ownership and --check-extents. build/graph-audit.json records the full
canonical result. There is no private instruction decoder or legacy fallback.

The two contextual hash/equality callback tables are described by
scripts/graph_metadata.py. Unreconstructed callbacks may remain named traps;
the auditor compares their ordered non-null logical identities. audit-data.py
additionally verifies every exact callback slot and null sentinel, option/state
aliases/defaults, encountered data, parser and lookahead policies. The separate
parser/lookahead audits and reached-CRT identity/behavior proofs are retained.
Candidate acceptance proposes only its complete recovered original extent to
the same shared engine before changing accepted metadata, and saves its full
JSON result, flags and tool hash with the frontier evidence.

    make -j4 graph-audit-tool
    make -j4 audit graph-negative-check

graph-negative-check uses disposable host-side PE/config copies to reject a
changed call edge, RET cleanup, truncated instruction extent, reordered callback
and moved callback sentinel. It also checks the candidate adapter and freezes
the canonical inputs throughout. build/graph-negative-validation.json records
the commands, original diagnostics and elapsed checks. These tests never launch
either Windows executable or modify a canonical file.

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

Explicit build provenance
-------------------------
scripts/build-frontier.py LABEL records native and original-toolchain builds
separately. Each phase archives the encountered shared C/header sources under
evidence/shared-build-inputs, keyed by the manifest hash, and verifies that the
immutable compiled inputs did not change during compilation.
Default native and canonical builds use the committed pin/tree in
cases/shared-build-reference.json, materialized from Git blobs; no live or
untracked shared source is used. At each substantial case boundary, review
newer verified shared fixes with /root/recon_lib_maintenance and deliberately
refresh with:

    make refresh-shared-build RECON_COMMIT=<reviewed-commit>

Old archives remain available and the selected revision/tree is in each build
receipt. The native input signature covers nested headers, C inputs and compiler
flags; unchanged inputs retain incremental objects. Setup/hash failures stop before
the dependent phase or acceptance capture. --native-only supports infrastructure
revalidation without another original invocation. Snapshot setup time is separate
from compilation where measured. Historical receipts refer to their immutable
archives; they do not require later shared worktrees to retain the same hash.

evidence/native-build-sigbus preserves the Clang SIGBUS/invalid-UTF8 crash,
intermediate and stable retry accounting, and a native-only revalidation against
unchanged original0910 inputs/outputs. The crash cause is unestablished. Its
revalidation.json binds compiled shared contents and native artifact hashes to
the original evidence; the earlier setup failure remains explicitly recorded.
