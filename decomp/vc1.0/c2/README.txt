C23232.EXE reconstruction, vc1.0 / original1.0-nt toolchain

Current checkpoint:1076 accepted frontiers and997 reconstructed source functions.
Accepted original code extents cover296904/399693 bytes (74.283012%). This is
an extent union of source, shared text data, CRT and import thunks, clipped to
original VirtualSize (raw-size fallback when zero), not branch coverage.

Build and validate from this directory:
    make -j4 native native-trace
    make -j4 check
    make -j4 win32
    make -j4 audit coverage

check/replay and named-profile checks run native i386 -O0 -g only. They compare
cached original first-entry traces, status, streams and complete object bytes.
Unknown process dependencies fail; no fixture invokes Wine or refreshes original
references. Only identified COFF timestamp bytes4..7 are normalized. The suite
contains seven complete objects: minimal286, hello2289, loops620, pressure879,
aliasing600, floating979 and i386572 bytes. Deep x87 currently ends at the
matching named trap select_runtime_intrinsic. Its original complete947-byte
object is captured for the later completion check. The approved883-byte i486
case remains queued. See cases/queue.json and cases/native-all-validation.json.

Original builds and explicit captures use the repository shared prefix
~/.ai-tmp/vc1.0-capture/wine. Fixed drives are R:repo and S:~/.ai-tmp/msvc2.
case_paths.py rejects original paths outside these roots. Per-case canonical
cwd is S:\vc1.0\c2\<case>; native cases map the same namespace into isolated
host directories. The C2 capture lease is S:\vc1.0\c2\capture.lock. Never remap
drives or stop the shared server. The former C2 prefix/server were verified
unused by compiler/case processes and retired; no compatibility route remains.

Original-only operations, outside fixtures:
    make -j4 build/debug-run.exe build/capture-cl.exe
    python3 scripts/capture-original.py --profile x87 --receipt cases/x87-before.json
    python3 scripts/debug-frontier.py --profile x87 --reference cases/x87-before.json

The first command builds original-only tools. capture-original masks all code,
restores accepted whole functions/shared text, and records the next original
trap. --entries additionally captures explicit first-entry plans with one-byte
restoration and EIP rewind. Incorrect entry orders/restoration bytes are rejected.
Fresh transport/entry evidence for the shared mappings is under
cases/shared-prefix-evidence/. native-traces.py --write projects those references
offline; routine one-function acceptance uses --extend-before/--extend-after.
Always obtain matching fresh original/native evidence, preserve whole bodies and
ABIs/callee graphs, retain named child traps, and audit before committing.

All nine source/frontend pipelines were recaptured through original CL's
CreateProcessA thunk, then independently checked with standalone original C2.
Their actual command/environment blocks and four backend IR inputs are pinned
in C2-owned fixtures. Source headers are conservatively pinned for hello.
Minimal/hello use C2-owned capture copies; sibling fixtures were not changed.
All nine IR streams and timestamp-normalized objects match the previous bytes.
The old raw transport bundles were removed without relabeling them; independent
ABI/extent/data analysis is under cases/analysis/, with historical bytes in Git.
Timing-only historical ledgers explicitly retain overlap and unmeasured limits.

Canonical graph safeguards include calls/tail calls/indirect edges, disconnected
SEH handlers, inline switches, overlapping instruction streams and instruction
extent boundaries. Root's audit optimization is unchanged: historical graph-only
comparison10.663s to2.703s (3.95x), all1342 old graph sets identical and21 focused
checks passed. See cases/audit-performance-validation.json and check-audit-graph.py.
Independent contextual-data proofs cover x87 forms, conditions/register encodings,
stack effects, costs, stack operands and intrinsic records/operator maps. Library
identity/call-data observations remain separate from application acceptance.

Source remains original-style Win32 C with individual ABIs and original-toolchain
compilability. The documented raw80-bit x87 boundaries in extended.c retain narrow
assembly bridges. No source/body/acceptance/ABI changes occurred during migration.
Nine encountered math descriptors are typed and contextual; unknown IDs trap.
Unvisited generated-call metadata, x87 input arrangement/exchange (including the
original slot1..8 alias contract), and i386 rematerialization cost remain traps.
The completed /O1 /G3 input did not reach that anticipated cost function.

Continue the approved deep-x87 case, then /O2 /G4 floating stores/comparisons.
Pause after both complete; no further cases are authorized. Shared lib/recon/
changes belong to its dedicated maintainer. See cases/shared-prefix-validation.json
for migration timings, negative checks and exact scope.

Migration validation measured a32.569s complete original rebuild,1.096s native
relink,4.887s graph/coverage audit and2.860s selected /W3 checks. The final native
suite wrapper took0.464s. Detailed nine-pipeline/eight-entry-capture and per-case
setup/execution/check timings, including known failures/unmeasured limits, are in
cases/shared-prefix-validation.json. Original and native case timing ledgers
are separate; analysis wall time was not measured.
