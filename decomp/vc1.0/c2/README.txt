C23232.EXE reconstruction, vc1.0 / original1.0-nt toolchain

Current checkpoint:1128 accepted frontiers and1049 reconstructed source functions.
Accepted original code extents cover310296/399693 bytes (77.633584%). This is
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
contains twelve complete objects: minimal286, hello2289, loops620, pressure879,
aliasing600, floating979, i386572, deep x87947, i486883, combined2125 and G5 joins1972 and G3 deep-call1009 bytes.
Root reviewed the next single inliner/bitfield/dense-switch case. See cases/queue.json,
cases/combined-completion.json and cases/native-all-validation.json.

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
Scheduled x87 input/operand arrangement and exchange (including explicit
slot1..8 aliases) are now recovered, as is i386 rematerialization cost.
Other generated-call and inliner/debug/assembly families still contain traps.

Additional cases require the orchestrating agent's review. Shared lib/recon/
changes belong to its dedicated maintainer. See cases/shared-prefix-validation.json
for migration timings, negative checks and exact scope.

Migration validation measured a32.569s complete original rebuild,1.096s native
relink,4.887s graph/coverage audit and2.860s selected /W3 checks. The final native
suite wrapper took0.464s. Detailed nine-pipeline/eight-entry-capture and per-case
setup/execution/check timings, including known failures/unmeasured limits, are in
cases/shared-prefix-validation.json. Original and native case timing ledgers
are separate; analysis wall time was not measured.

Final CPU/x87 queue checkpoint1089 preserves1010 whole source graphs and all
nine exact object oracles. Fifteen original/canonical fastcall cleanup ABIs are
independently checked in cases/resumed-frontier-abi-validation.json. The generic
cycle helper returns AX; its declaration now records the unsigned-short ABI.
The i486 register-pair helper provisional *_lea names actually test PUSH pairs;
source comments preserve that distinction without altering cached identities.
Completion evidence/timings and unstarted next-case proposals are recorded in
cases/x87-completion.json, cases/i486-completion.json and cases/queue.json.

The default make audit now invokes tooling/graph-audit's common Rust CFG engine
through the shared quiet build helper, then the existing scoped RET ABI proof.
Enabled flags: --check-ownership --check-extents --check-accepted-extents. They
check application ownership, whole tails/switch exclusions and all accepted
CRT/import extents on both images. The original Python graph checker remains
available explicitly as make audit-python; make audit-negatives checks current
scratch mutations and the21 focused graph/SEH/extent safeguards. make audit-data
retains the contextual ISA/intrinsic/radix proofs. No original recapture occurs.
Global --check-ret remains off: finish_debugging's unresolved indirect callback
tail has no inferred cleanup contract. The existing15 application ABI proofs
remain enabled; a changed RET8->RET4 is independently rejected. See
cases/rust-audit-integration.json for current hashes, parity and measured times.

Combined /Oi /O2 /Ob0 /G3 case completed with14 whole application additions
and3270 accepted original code bytes beyond1089. New contextual descriptors
cover strlen/strcpy/strcmp; generic candidate sizing is now recovered, while
G3 rematerialization cost and G5 scheduled x87 arrangement remain traps.
Fourteen independent original/canonical RET cleanup proofs are recorded in
cases/combined-final-abi-validation.json. The final native ten-object suite
wrapper took0.555s (setup0.164s; case execution/check splits in receipt), final
graph/coverage2.202s, contextual data2.036s, rejection suite15.720s and selected
original /W3 checks8.684s. Build phases and analysis are not inferred; complete
attempt/repeat and capture setup/execution/check timing is retained in
cases/combined-completion.json. No follow-on case has started.

G5 branch joins complete at1107: scheduled FXCH162 bytes, prior-load scan117,
input arrangement792 and ordinary operand arrangement199. All eleven whole
objects pass; see cases/joins87-completion.json for timings and slot8 proof
limits. Synthetic all-slot checking preserves named live aliases without C
array overruns or a tighter diagnostic range.

G3 deeper calls complete at1108 with the full708-byte i386 recreation-cost
function and59136 independently matched opcode/operand forms. All twelve
objects pass; cases/remat386-completion.json records complete timings,
unsplit rebuilds, failures and proof scope. The remaining-code inventory
separates provisional driver entries, library/import identities and unclassified
code/data/alignment; it does not attribute unknown bytes to application C.

The reviewed /O2 /Ob2 /G5 inline-records case is active at1112. Original
candidate creation is confirmed by explicit entry probe; bitfield reads,
aggregate argument sizing/storage and candidate construction now match.
Twelve complete native objects plus the thirteenth named frontier pass.
See cases/inlrec-first-checkpoint.json for data-frontier and timing limits.

Inliner binding checkpoint1118 adds diagnostic naming/filtering, warning-state
lookup, scope merge and ordered argument binding/factory. Their6 CFG cleanup
proofs, full-body limits and split timing provenance are recorded in
cases/inlrec-binding-checkpoint.json. Visible warning output, C++ undecoration
and additional override state paths remain explicitly trapping when unreached.

Inline evaluation checkpoint1122 preserves pairwise alias checks, return
counting and the argument prefix. The provisional inline-storage kind is now
16 bits, matching original caller/consumer evidence; see
cases/inlrec-evaluation-checkpoint.json for ABI and unvisited child limits.

Materialization checkpoint1127 adds406-byte argument evaluation/insertion,
word type selection, reference refresh and two original relocation functions.
All13 native regressions and five cleanup proofs pass; the current frontier
is the complete inline-body substitution routine. Detailed limits/timings are
in cases/inlrec-materialization-checkpoint.json.

The full1500-byte inline-body substitution routine is recovered at1128,
including its bounded12-way directive data. Its named child frontier is node
substitution; whole-case completion remains pending. Source/ABI/data limits
and repeated-attempt timing are in cases/inlrec-substitution-checkpoint.json.

Inliner node checkpoint1135 preserves node substitution, binding/site recycling,
whole alias-use scan and distinct matching-parent setup/recursive graph entries.
The scan uses named remapped opcode flags; its independent182-row/910-value
proof rejects both mistaken original/normalized bit substitutions. Failed
native crash and repeat timings remain in cases/inlrec-node-checkpoint.json.
All13 native regressions, full graphs/extents, six cleanup ABIs and original
/W3 pass; current frontier is bitfield assignment lowering, not object completion.

Inline-records checkpoint1143 adds bitfield updates, constant factoring, the
original signed-overflow checker, dense-switch table generation/range lookup,
explicit block copies and special table-label region references. All13 native
regressions and1064 graphs pass. Eight cleanup ABIs and2700 arithmetic
boundary comparisons pass; short-mask rejection and corrected383-byte
switch generator proof remain in cases/inlrec-bitfield-switch-checkpoint.json.
Current frontier is the Pentium fallthrough-jump pairing helper.

Inline-records is complete at1146 accepted entries/1067 source graphs. Its
1554-byte object matches original CL/C1/C2 and standalone C2 after only the
four-byte COFF timestamp normalization; all13 native cases now complete.
Original /W3, data audits, graph/extent audits and focused negatives pass.
cases/inlrec-completion.json records timings/failures/limits; scripts/inventory.py
refreshes the application/library/import/unclassified inventory for the next case.
