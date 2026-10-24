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

Debug/listing case setup (1149 accepted; no new application body yet)
The original-validated dbglist /Od /G3 /Zi /FAcs case freezes five IR channels,
source and the pre-C2 PDB. Object timestamp bytes4..7 are the only normalization;
the .cod and retained .pdb are exact output oracles. Strict multi-artifact checks
reject corruption, missing/extra artifacts and changed PDB input. Native fixtures
now include13 completed cases and dbglist's named initialize_codeview_output trap.
Two exact LIBC bodies and SetStdHandle were accepted separately with real call
observations, not application credit. The actual freopen close transition exposed
a shared lowio mismatch; commit410fea95 fixes it. Original/native transient state,
the earlier mismatch and failed setup attempts are retained in cases/dbglist-*.
See cases/dbglist-plan.json for capture/build/fixture/check timings and limits.

CodeView prefix checkpoint1162 adds four readable C writers (272 bytes), with
fresh original/native progression to write_segment_listing. One original debug
segment global7914c had incorrectly become two C globals; operand validation now
binds23 accepted function/storage pairs across nine segment globals, rejects a
second zero-valued storage and checks actual original/native segment identity.
The same domain proof corrected initializer kind10 to the original type stream.
Nine isolated original/native counted-string shapes cover null/255/clamp/spare
byte behavior. CRT printf internals retain exact archive/relocation/per-body
frontier evidence; native ABI matching is only at the public printf seam. Actual
FILE,24 buffered bytes, descriptorposition0 and result24 match; ten proof
negatives pass. Thirteen cases complete and dbglist still traps as expected.
Full receipts, failed attempts and timing limits: cases/dbglist-codeview-checkpoint.json.

Listing/output checkpoint1172 adds ten application bodies (2072 bytes), reaching
write_initializer_assembly with fresh original/native evidence. All14 native
cases pass (13 completed objects, one expected debug/listing trap); selected
original /W3 modules and ten cleanup ABI checks pass. The full common-data
writer retains unvisited declaration/alignment paths and typed element-size data.
COFF fill uses one contextual80-byte scratch. Its unusual original <=80 direct
write loop is preserved;102 isolated actual-C/original probes compare buffered
block rings/counters and mocked direct-write requests/80 defined scratch bytes.
Oversized reads are not claimed matching. A separate closed-target scope/operand
contract and readonly unaccepted trailer-helper analysis document the limit.
Coverage is318796/399693 original code bytes (79.760216%), not branch coverage.
See cases/dbglist-listing-checkpoint.json for split timings, repetitions and limits.

Initializer/listing checkpoint1180 adds eight complete application bodies and
2079 original code/text-data bytes. Driver/header/item/scalar/padding/directive/
ORG/string functions pass fresh ordered original/native frontiers.18 listing
cleanup ABIs,15 directive classes and48 isolated string quoting/wrapping/column
shapes pass; the original10-DWORD table and all256 C-locale printability entries
are checked independently. All14 native cases pass (13 complete, debug trap).
Coverage320875/399693=80.280365% is code-extent union, not branch
coverage. Debug input_counted_string700b0 is next; full debug artifacts still pending.
Retained unvisited pointer-to-ALIGN%d behavior and case-specific COFF fill limits
are explicit. See cases/dbglist-initializer-checkpoint.json for failures/timings.

Inline-assembly checkpoint1186 adds six whole application bodies and2306
original code/text-data bytes. Serialized assembly becomes typed machine IR,
including symbol-relative base/index aliases and x87/high-byte/branch forms.
Twenty-two contextual operand descriptors and23 grammar switch targets are
independently checked.57 whole-instruction and80 operand/id/first-diagnostic
probes pass alongside24 cleanup ABI checks and selected original /W3 builds.
All14 native cases pass,13 complete and dbglist at its expected allocation trap.
Coverage323181/399693=80.857308% is extent union, not branch
coverage. See cases/dbglist-assembly-checkpoint.json for exact seams and timings.

Numeric listing checkpoint1206 adds three whole application bodies (972 bytes):
physical-register selection, COFF auxiliary-symbol construction and floating
initializer listing. Seventeen reached CRT bodies are independently indexed,
archive/relocation-identified and masked, with zero application coverage credit.
Their actual internal arguments/results/buffers are observed; native matching
uses the public printf seam rather than invented internal native entries. The
real1.25 call matches complete FILE buffer, return, descriptor position and x87
state after separately proved startup53/%g fixes. A separately proved%Lg
exact-double promotion adapter closes the prior negative-zero width10 failure.
All120 bounded width/repetition/column probes and20 numeric-proof negatives pass.
Twenty-seven cleanup ABIs, original /W3 modules, graph/data/negative checks and
all14 native cases pass (13 complete plus debug's named source-listing trap).
Coverage327863/399693=82.028707% is extent union, not branch coverage;
3710 of the4682 newly accepted bytes are CRT support. Debug artifacts remain
pending. See cases/dbglist-numeric-checkpoint.json for failures and split timings.

Source-interleaving checkpoint1213 adds seven whole application bodies and1144
original bytes: line collection/predecessor, cached-file switching, local/name
formatting and source-line output. The original twentieth-cache limit is checked
after the twenty-first row writes into four adjacent live globals. A contextual
typed union preserves those aliases and order without C array bounds overflow.
Eighteen whole-original/native cache states,16 real function/storage operand
bindings and a split-storage rejection pass; file/diagnostic mocks are explicitly
control probes, not real failure-path runtime acceptance.152 symbol-format/flag
states verify the original%ld+ label spelling and signed local offsets. All34
cleanup ABIs, selected original /W3 modules and graph/data/negative checks pass.
All14 native cases pass (13complete+debug at named read_listing_source stub).
Coverage329007/399693=82.314927% is code-extent union, not branch
coverage. See cases/dbglist-source-checkpoint.json for failures and split timings.

Checkpoint1219: source line reading, encoded-byte rows and relative tab spacing
are reconstructed (four whole app bodies873B plus150B exact CRT support).
All14 cached native cases pass; debug output is still pending instruction text.
Actual entry observations now cover all byte-writer/spacing formals and the
next PUSH EBP instruction pointer/defined record, stdout, column and x87 state.
Default audit rejects missing formal/hidden-input metadata; isolated actual-C
control probes cover462 driver opcode/mode states and154 byte/spacing shapes.
These mocked child/printf probes do not claim unvisited real-case coverage.
See cases/dbglist-reader-checkpoint.json for raw failures, scope and all measured
build/reference/debug/fixture/check times, including repeats.

Checkpoint1222 adds the full instruction-text, special-form and mnemonic writers.
Typed opcode spellings/condition names match every original/canonical index;
2772 special-parent and915 mnemonic-parent shapes pass with explicit child mocks.
The isolated FADD failure caught a wrong raw/normalized property substitution
before acceptance. Actual operand entry includes all3 formals and alias/record/
stdout/x87 observations. All14 native cases pass; debug artifacts remain pending.
See cases/dbglist-text-checkpoint.json for scope, raw failures and split timings.

Checkpoint1225 reconstructs the complete operand/prefix/register-name bodies.
400 operand,288 prefix and116 register lookup shapes pass;60 typed register
entries include originalNULL slots and6 explicitly shared segment identities.
All16 known listing-family entry formals and next comment state are observed.
Original/W3, graph/ABI/negatives and14native cases pass; debug artifacts pending.
See cases/dbglist-operand-checkpoint.json for limits and all measured timings.

Checkpoint1228 adds whole instruction comments, comment separators and size names.
95 parent shapes,30 actual-padding/zero-return shapes and15 type/default shapes
pass, plus all10 original size dispatch destinations. All14 native cases pass.
20 actual listing-family/next-boundary formals are observed. Next displacement
input0/hint0 leaves nonzero formatting unexercised; debug artifacts still pending.
See cases/dbglist-comment-checkpoint.json for scope, failures and split timings.

Checkpoint1230: write_listing_displacement (125 bytes) and typed fastcall
coff_output_debug_pair(char*) (60 bytes) accepted. Its first guard is false;
child-mocked shape probes do not claim execution of cold children. All four
actual next coff_begin_lines inputs, defined listing buffer and x87 state match.
A handle10/12 mismatch exposed wrong uses of the existing primary/current/
diagnostic filename globals. Data identity proof, failed attempts and timing
ledger: cases/dbglist-coff-filename-checkpoint.json. Original850c8 starts1;
this is PE data, not synthetic seeding. All14 native cases and canonical
graph/extent/ABI/negatives pass;13 finish and dbglist stops at begin-lines.
Coverage333620/399693=83.4691% is accepted code extent, not branch coverage.
Canonical/W3 builds pass. No target-local CRT was added.

Checkpoint1233 reconstructs the whole COFF begin-lines253-byte function,
debug-record65-byte callback, and ordinary100-byte line recorder. Actual
register/stack formals, original/canonical CFG cleanup, native first entries
and named traps match.192/144/96 controlled original/canonical shapes cover
block growth, free-list reuse, auxiliary chains, line-zero sentinel and WORD
wrap; child mocks remain distinct from real child acceptance. All14 native
cases pass; dbglist now stops at coff_line_marker.13 completed output cases
remain exact. Coverage334038/399693=83.5736% code extent, not branch coverage.
Data/ABI limits, original references and separate timing receipts are recorded
in cases/dbglist-lines-checkpoint.json. The earlier informal begin-lines251
count was arithmetic only; its accepted6eca0..6ed9d extent was always253.

Checkpoint1236 adds whole.lf30-byte and.ef56-byte boundary writers plus
83-byte end_debug_function. Actual ABI inputs include a typed FunctionSymbol
pointer with current_symbol/current_function.symbol identities and complete
parent-read field proof; children remain named typed traps.128 boundary
shapes retain untouched auxiliary bytes and original WORD masking. All14
native cases and canonical graph/extent/ABI checks pass,13 output cases
complete; dbglist reaches write_codeview_function69650. Original/W3 builds
pass.169new extent bytes; code-extent coverage is in
cases/dbglist-line-end-checkpoint.json with split measured timing/limits.

Checkpoint1239 adds216-byte CodeView procedure records,28-byte address-pair
relocations and207-byte recursive lexical-scope output.450 packed-record and
48 recursive-chain controls match original/canonical C. Actual pointer/scalar
inputs and shared scope-head aliases match;13 finished native output cases
stay exact, dbglist reaches write_debug_symbol69930. Whole function graphs,
extents, ABI and original/W3 builds pass.451new code-extent bytes and timing
limits/failed host gate are recorded in cases/dbglist-procedure-checkpoint.json.

Checkpoint1240 reconstructs the whole484-byte CodeView symbol dispatcher.
2496 original/canonical states exercise qualification, distinct70/780/8000
attribute domains, tags, seven writer routes and recursive alias restoration,
with explicit child mocks. First actual unnamed kind3/tag31 is suppressed;
this is not real cold-alias branch coverage. Actual pointer/head identity and
all encountered parent fields pass, plus original/canonical cleanup, graph,
extents, typed cursor/allocation storage identities and original/W3 builds.
All14 native cases pass;13 outputs complete, dbglist advances to its next
writer trap. Evidence/extent coverage/timing: cases/dbglist-dispatch-checkpoint.json.

Checkpoint1243 adds186-byte CodeView labels,143-byte frame variables and
57-byte S_END (historical begin_debug_function name). Actual label list member1
and parameter member0/type/offset/context match; S_END has a proved genuine
zero-input ABI and explicit empty observation.144 label and1152 frame
controls compare packed bytes, raw offset/wrap, function/role adjustment and
post-select input order with child mocks. All14 native cases and canonical
graph/extent/ABI/W3 checks pass;13 finished outputs remain exact. Debug case
now reaches listing_size_suffix2a840.386new bytes, coverage and full timing
ledger: cases/dbglist-label-frame-checkpoint.json.

Checkpoint1246 adds54-byte b/w/d listing suffixes,138-byte module-debug
finalization and110-byte S_UDT records. Global typedef_DebugValue(type4117)
was observed in actual global_scope bucket41/member0; undefined descriptor
allocation words remain raw rather than compared or normalized. Module has
a genuine zero-input proof, visits global symbols and retains db FILE ownership.
Three suffix values/four diagnostic controls, six module controls and12 typedef
record shapes pass with explicit child mocks. All14 native cases, canonical
graph/extent/ABI and original/W3 builds pass.13 outputs remain complete; dbglist
now reaches write_codeview_local_data697c0.302new extent bytes and detailed
timing/limits: cases/dbglist-module-typedef-checkpoint.json.

Checkpoint1248 adds191-byte local-data/TLS and134-byte S_COMPILE records.
Actual adjustments global bucket49/member0/type4109 and defined fields/mode
match; compiler entry has a genuine zero-input proof and reuses shared original
852b8 compiler_identification storage.768 data-record and30 compiler-record
controls compare packed bytes, name/counts/relocations/call order and controlled
post-select loads; children/TLS/failure paths remain explicitly scoped. All14
native cases and canonical graph/extent/ABI/W3 pass,13 outputs complete.
Next db FILE-reader69ba0;325new code-extent bytes and full timing ledger in
cases/dbglist-data-compile-checkpoint.json.

Checkpoint1250 adds the48-byte CodeView db-header wrapper and29-byte
initializer-chain release. Actual binary FILE owner/defined state and three-node
ECX chain/free-head inputs match original/native.9 wrapper and30 whole-list
original/canonical controls verify AL width, handover, exact+8 writes and preserved
remaining node bytes. Allocation and release share the existing static free list.
An old unaccepted trap lacked provisional metadata: strict cache rejected it;
flag-only correction and fresh identical-mask/frontier evidence are retained.
All14 native cases pass,13 complete outputs remain exact; debug case reaches
coff_write_lines6f040. Canonical graph/extent/ABI and reconstructed-body/W3 pass;
existing return-valued INT3 stubs still emit C4035 under W3 and are not disguised.
77new bytes:336232/399693=84.1226% accepted code extent, not branch coverage.
Full failed-attempt/timing ledger: cases/dbglist-input-release-checkpoint.json.

Checkpoint1251 completes debug-asm-listing with the whole200-byte COFF line
writer. Actual contribution/base0, circular26-record block and auxiliary owners
12/23 match;160 original/canonical controls cover debug/address rules, rings,
wraparound, six-byte write requests and free-list splice with explicit child
mocks. All14 native cases now exit0 with exact streams/artifacts. Debug object
2280 bytes matches after only its established timestamp4..7; listing7635 CRLF
bytes and complete564-byte PDB are exact, PDB unchanged from input. Original
compiler/W3,1141 app graphs and extent/ABI/storage checks pass. Coverage is
336432/399693=84.1726% accepted code extent, not branch coverage. Completion,
failed helper invocation and split timing: cases/dbglist-completion-checkpoint.json.
Remaining inventory keeps154 pending app/support rows and54544 unclassified
bytes separate from proven CRT; next single-case proposal is pending review.

The fifteenth case, omf-typed-debug, is a derived standalone original C2 case.
Ordinary original CL /omf and /Bd /omf silently omit this internal-default
option; both genuine COFF transports are retained as rejected OMF probes.
Actual baseline CL /Od /G3 /Z7 frontend IR is unchanged; only supported backend
-omf is appended to the actual MSC_CMD_FLAGS. Untouched C2 reaches the OMF
module opener with coff_object0 and produces a repeat-exact2594-byte object,
all OMF checksums valid, no normalization/PDB/side artifacts. A separate exact
stream/inventory capture repairs an initial helper log-path overlap without
relabeling old raw evidence. Native matches omf_open_module;14 complete cases
remain exact. OMF artifact/input negatives reject false COFF clock exemptions.
No new function accepted: cases/omf-capture-checkpoint.json records provenance,
all failed probes, original/native observations and measured timings.

Checkpoint1252 reconstructs the whole1173-byte OMF module opener. Actual
zero-input ABI matches; real case currently executes its prefix through the
first child, then both sides trap at omf_initialize_records6a9a0.128 controlled
original/canonical states cover later names/segments/groups/fixup threads and
COMDAT branches with explicit child mocks, not actual branch coverage. Those
controls caught an extra class-index write on a named COMDAT path; it is removed.
Contextual data uses one named8-byte group storage/pointer identity, typed
counters/threads and72-byte Segment format unions. An actual PE initial-value
mismatch in listing_column is fixed toFFFFFFFF; failed observations retained.
All15 native cases pass (14 complete), canonical graphs/extents/ABI/storage and
original/W3 builds pass.337605/399693 accepted extent bytes; app/support-only
319305B and1137 functions exclude the5 historical handwritten CRT bodies.
Full failed/repeated build/control and timing ledger: cases/omf-open-checkpoint.json.

Checkpoint1254 adds34-byte OMF cache initialization and31-byte module header.
Four typed1004-byte BSS wire-record buffers retain all bytes except the exact
tags reset; four poisoned full-pool controls and original/native entry agree.
The actual ECX source pointer aliases source_filename, omftypes.c NUL bytes
match; header requests THEADR80/contextNULL and forwards the returned record
unchanged before flush. Those children remain typed traps, verified separately
at their own entries. All15 native cases pass (14 complete), next record begin
6b480. Canonical graphs/extents/ABI/data and original/W3 pass.65new extent bytes;
full layout provenance and split timings: cases/omf-cache-header-checkpoint.json.

Checkpoint1256 adds463-byte OMF record dependency/reuse dispatch and37-byte
first-type flush. Actual80/NULL factory and95 flush inputs match; four defined
zero20-byte cache headers/current-slot0 ownership are observed on both sides.
608 whole original/canonical factory controls cover every dispatch class,
short-circuit order, reuse/allocation and context-read timing, with explicit
child mocks. TypeC3 retains the original required context+4 read; no NULL guard
is invented.42 flush controls preserve all pool bytes and select first matching
slot, independently of child return.15 native cases pass (14 complete); next
cache lookup6bd50. Original/W3 and canonical graph/extent/ABI/storage pass.
500new bytes; split timings and proof limits: cases/omf-begin-flush-checkpoint.json.
The broad608-case model is explicit/audit-data; routine audit checks actual ABI.

Checkpoint1258 reconstructs the whole471-byte OMF cache lookup and its300-byte
jump data, plus83-byte first-free/current-successor allocation. The C uses named
record classes and typed Segment COMDAT byte69 storage, not copied jump data.
1677 whole original/canonical controls cover every dispatch slot, ring order,
signed capacities/fixup limits, context/group identity and unsigned positions,
padding and overflow.288 allocator states verify all occupancy masks and actual
returned-child pointer propagation. Child initialize/pad behavior is mocked;
actual80/NULL case only searches empty records. No oversized-write claim or
invented guard. All15 native cases pass (14 complete), next initializer6c060.
Its real ECX pool-slot0/header, EDX80 and stackNULL/RET4 ABI are already observed.
Original/W3 and graph/extent/ABI/data pass:339024/399693 accepted extent bytes;
app/support320724B and1143 functions exclude5 historical CRT bodies. Full timing
ledger/proof limits: cases/omf-find-allocate-checkpoint.json. Broad models remain
explicit/audit-data; routine audits check actual ABI and dispatch data only.

Checkpoint1260 adds483-byte OMF record initialization and96-byte mutable counted
string append.144 full original/canonical controls compare all prefix classes,
returned flush slot, every child argument/count/cursor/prefix and payload-counter
reset; typed named children remain traps.48 string controls preserve warning
before source truncation, the actual byte255 mutation, NULL/empty/255 boundaries
and unchanged tails. Actual case initializes slot0/type80/NULL and appends the
same source_filename-owned omftypes.c pointer. Next byte writer6b650 remains a
named trap;15 native cases pass (14 complete). Original/W3 and graph/extent/ABI/
data checks pass.339603/399693 accepted extent bytes, app/support321303B and1145
functions excluding5 historical CRT bodies. Split timings and branch/mock limits:
cases/omf-initialize-string-checkpoint.json.

Checkpoint1262 adds75-byte OMF byte append and11-byte current-record flush
wrapper.79 bounded original/canonical copy/error controls and32 tail-transfer
controls pass. The wrapper now explicitly returns the child EAX pointer; original
callers may ignore it. Actual byte-source/stackcount10 and current34-byte record
header/payload/cursor/owner inputs are observed independently. Capacity-error
controls stop at actual diagnostic727, not an invented returning handler or
unsafe oversized copy. Original/W3, graph/extent/ABI/data and15 native cases
pass (14 complete); next omf_flush_record6c330 with full pointer input already
observed.339689/399693 accepted extent bytes; app/support321389B and1147functions
exclude5 historical CRT bodies. Timings: cases/omf-bytes-wrapper-checkpoint.json.

Checkpoint1263 reconstructs the full197-byte record flusher.336 original/
canonical controls preserve dependency short-circuit order, cached pre-write
type, tag clearing before pending fixups and current-record changes by children.
Fresh original/native match writer6c440; original/W3, graph/extent/ABI and15
cached cases pass (14 complete).339886/399693 accepted extent bytes; app/support
321586B and1148 functions exclude5 historical CRT bodies. The next writer's
full record input matches, but actual object_file._bufsiz4096 vs native0 holds
its ABI/acceptance. Failed raw evidence is cases/omf-write-record-abi/PENDING.json;
shared maintainer is investigating preserved metadata on FILE reuse. No oracle
exclusion/target forcing or writer promotion. Timings: cases/omf-flush-checkpoint.json.

While writer acceptance is held,67 original-only serializer controls validate
length/checksum/fwrite request bytes and genuine capacity/short-write diagnostic
frontiers; they are analysis, not native equivalence or application credit.
The exact first15-byte THEADR equals the immutable original object prefix.
scripts/observe-scalar-entry.py --native-only --original-receipt OLD --receipt NEW
reuses path/hash-bound original input without overwriting failed observations.
The known mismatch is still rejected, and reusing a native receipt is rejected
before debugger execution. Analysis/timing state: cases/omf-wire-analysis-state.json.
Inventory now separately reports1148 app/support functions321586B and5 historical
CRT bodies1776B; driver metadata remains unchanged.163 pending driver rows and
51081 unclassified bytes/700 ranges retain unknown identity. A reliable non-CRT
denominator is unavailable; all original code remains399693B.

Checkpoint1264 reconstructs the whole151-byte wire serializer:16-bit length,
8-bit additive checksum, exact fwrite request and original cursor/count changes.
67 whole original/canonical controls and10 actual record/FILE ABI negatives pass;
short-write/capacity branches stop at genuine named diagnostic boundaries. The
shared retained-FILE correction cabec0d10 exactly matches the passing candidate
hash; native-only retry resolves the observed4096-vs0 input hold. Failed raw
observations remain unchanged. A former10000-step emulator limit stopped the
canonical /Od978-byte checksum loop; equal50000 limits pass, with source unchanged.
Original/W3, final full graph/extent/ABI/data audit and15 native cases pass
(14 complete); first15-byte THEADR equals frozen original output, full OMF still
pending. Next omf_output_record6b860.340037/399693 accepted extent bytes;
app/support321737B and1149 functions exclude5 historical CRT bodies. Honest
failed/repeated/candidate/final timing ledger: cases/omf-wire-checkpoint.json.

Checkpoint1266 reconstructs178-byte module-directive dispatch plus44-byte switch
data and78-byte COMENT wrapper. The callback now has its proved fastcall pointer
ABI; raw historical old-name evidence remains untouched. Actual kind6/head-owned
20-character user comment and lowCL classDF/text inputs match.290 dispatcher and
153 comment controls verify all11 kinds, single compiler_identification address
mutation, prefix/counter order and source aliasing. Unsupported kinds stop at
actual294 diagnostic with the original undefined-after-fatal class assumption;
no default init/guard, original /W3 passes. Signed kind7 lengths only have request-
shape proof under child mocks, no huge-copy execution claim. All15 native cases
pass (14 complete), original graph/extent/ABI/data pass; next omf_name_index6a940.
340337/399693 accepted extent bytes; app/support322037B and1151functions exclude5
historical CRT bodies. Full timing/scope: cases/omf-directive-comment-checkpoint.json.

Checkpoint1268 adds49-byte name-index wrapper and191-byte name interner.72+268
whole original/canonical controls and6+10 actual ABI negatives pass. The actual
empty name and4-byte write-only destination have proved caller-local ownership
(original child entryESP+8, native &caller.index); prior uninitialized bytes are
not read/dumped/seeded.16 typed BSS buckets and PE next-name1 match. Contextual
12-byte name/index/next nodes preserve arena7 allocation/string copy, first-five-
signed-byte hash, all16 buckets, chain/prefix equality, index wrap and post-child
state reload. Old nodes/strings and output guards are checked. Original/W3 and
full graph/extent/ABI/data pass;15 native cases pass (14 complete). Next segment
definition6a890.340577/399693 accepted extent bytes; app/support322277B and1153
functions exclude5 historical CRT bodies. Timings/scope: cases/omf-name-interner-checkpoint.json.

Checkpoint1269 reconstructs the whole176-byte SEGDEF32 writer.164 whole
original/canonical controls and8 actual descriptor ABI negatives pass. Source
byte53 is now correctly named Segment.combine, proved by SEGDEF bit placement
and accepted PUBLIC/COMMON/COMDAT listing paths; historical evidence retains its
legacy key with an explicit refinement. Cached prefix state survives alignment
children; class/empty-name values reload after index children. All15 native cases
pass (14 complete); next alignment6a310 with actual ECX4 already observed.
Original/W3 passes all9 renamed modules; graph/extent/ABI/data pass.340753/399693
accepted extent bytes; app/support322453B and1154functions exclude5 historical
CRT bodies. Scope/timings: cases/omf-segment-checkpoint.json.

Checkpoint1271 adds54-byte alignment mapping and62-byte index writer. Actual
alignment4 maps5; original1/2/4/16 return1/2/5/3. Original unsupported RET leaves
EAX unassigned: Win32 source keeps the missing return, native exact unsupported
branch traps (verified by controlled GDB input3). Public reach of other values
is unproved; any real such input holds for register-ABI review, not a guessed
result. Original/W3 passes. Index signed<128 and high-byte truncation/count wrap
match52 whole controls; actual slot0/index3/count5/cursor28 observed. A mistaken
observer zero-length assumption is retained as failure; corrected native-only
retry binds the unchanged original995500 prefix. Full audits and15 cached native
cases pass (14 complete); next group definition6a850.340869/399693 accepted
extent bytes, app/support322569B and1156functions exclude5 historical CRT bodies.
Detailed limits/failed attempts/timings: cases/omf-alignment-index-checkpoint.json.

Checkpoint1273 adds52-byte group declaration and67-byte fixup-thread writer.
Actual group data/default ownership and8-byte descriptor match; DOS1 branch
skips explicit member append.32 whole group controls verify group store before
lookup, cold member path and post-child reloads.32 thread controls verify byte
CL/DL/tag truncation versus full32-bit returned thread and target, callee-saved
registers/RET8; all4 actual formals observed. Original/W3, graph/extent/ABI/data
and15 cached native cases pass (14 complete). A source-scope assertion failure
from counting subsequent functions' returns is retained and fixed without target
changes. Next selection callback6b6a0.340988/399693 accepted extent bytes;
app/support322688B and1158functions exclude5 historical CRT bodies. Detailed
branch limits and measured attempts: cases/omf-group-thread-checkpoint.json.

Checkpoint1274 reconstructs31-byte typed fastcall segment selection: exactly
combine7 chooses COMDAT32, all other bytes LEDATA32.16 original/canonical
controls/5 actual ABI negatives pass; actual debug segment combine0 observed.
Initial observer guessed2; retained raw observations both0, corrected parser
validates immutable bindings without recapture. All15 cached native cases pass
(14 complete), next data callback6b6c0. Original build/W3 reconstructed modules
and graph/extent/ABI/data checks pass. Existing return-valued INT3 stubs produce
known C4035 warnings under a broad W3 request; reviewed separately, no fabricated
returns.341019/399693 accepted extent bytes; app/support322719B/1159functions
exclude5 historical CRT bodies. Receipt: cases/omf-select-checkpoint.json.

Ordinary audit retains actual ABI/negatives/extent/storage/layout proofs while
older whole-body open/COFF-lines/comment/bytes/name-index/wire semantic sweeps
remain explicit in audit-data. New body controls still run at acceptance. Five
full/default models pass once on the same snapshot:10.372s total vs1.648s for
structural modes. Full adjusted audit+coverage20.055s vs prior30.058s, graph-only
split unmeasured; no source build in those batches. See ordinary-audit-model-
modes.json. Prior group-control prose corrected64->32; raw results always32.

Checkpoint1276 adds36-byte data callback and29-byte record-space helper. Actual
source is the initialized4-byte signature caller-local (original entryESP+4,
native &signature/sizeof4); current A1 slot0/count0/cursor28/debug context match.
16 forwarding and50 signed-capacity original/canonical controls plus8+6 actual
ABI rejects pass. Equality renews; original signed32 predicates and full child
return pointer preserved. Actual970>4 skips renewal, cold branch controlled
separately. Raw GDB signed-pointer observation failure retained; corrected
native-only observer reuses original, no recapture. Original/W3/ordinary full
audits and15 native cases pass (14 complete), next mark-symbol6ab20.341084/
399693 accepted extent bytes; app/support322784B and1161functions exclude5
historical CRT bodies. Split timings/limits: cases/omf-data-reserve-checkpoint.json.

Checkpoint1277 reconstructs407-byte symbol marking with all original storage/
COMDAT/function/weak dispatch edges and named unaccepted emitter children.126
whole original/canonical controls and8 actual ABI rejects pass, including
post-child reloads and full bounded state/callee-saved checks. Actual global
_omf_callback kind1/flags980/output4/attributesa3/segment2 has closed bucket57
member0 owner; only consumed ranges/name observed. Cold COMDAT/function branches
are controlled separately. Original/W3/full audits and15 cached native cases
pass (14 complete), next public emitter6aef0.341491/399693 accepted extent bytes;
app/support323191B and1162functions exclude5 historical CRT bodies. The55.297s
audit command includes126 canonical recompiles/link/index, so build/check
within it is unsplit, not audit-only; model2.073s. Other measured/failed attempts
and scope: cases/omf-mark-checkpoint.json.

Checkpoint1278 adds206-byte public emitter.83 whole original/canonical controls
and8 actual ABI negatives cover cached entry offset, post-child record/type
reloads, LE32/count wrap, kind1/3/4 tag selection and genuine label diagnostic638.
Actual _omf_callback offset136/type1014/global owner and data_segment match.
Original unknown kinds use an uninitialized local type: Win32 source retains
that assumption, no extra input/default value. Native exact unsupported kind
traps (controlled1->2 proof); real other kind holds, public reach is unproved.
The old uninitialized local is neither observed as ABI input nor seeded.
Original/W3/full audits and15 native cases pass (14 complete); next external
6ad80.341697/399693 accepted extent bytes; app/support323397B/1163functions
exclude5 historical CRT bodies. Timings/limits: cases/omf-public-checkpoint.json.

Checkpoint1279 adds64-byte EXTDEF writer with32 whole original/canonical
controls/7 actual ABI negatives. The encountered next-external85078 counter
is one typed unsigned PE1 storage, distinct from name/group/segment counters.
Actual _omf_external descriptor/global owner/name context and counter1 match;
old Symbol.field14 is write-only and is never captured/seeded. Counter ordering/
wrap/child mutations, post-string state reload and zero type byte preserved.
Correction: prior mark/public prose called byte13 padding; it is field0d naming
discriminator. Retained actual raw0 now explicitly checked, no raw evidence
changed. Original/W3/full audits and15 native cases pass (14 complete), next
relocation6afc0.341761/399693 accepted extent bytes; app/support323461B/1164
functions exclude5 historical CRT bodies. Timings/scope: omf-external-checkpoint.json.

Checkpoint1280 adds489-byte relocation dispatcher plus32-byte8-slot switch.
71 original/canonical whole-body controls and10 actual ABI negatives pass,
including cached kind/flags, post-child operand reloads, exact child ABIs,
LE32 fixup, signed remainder with ONE zero byte, sentinel and zero-local.
Actual3 formals/caller aliases and owned38-byte type6 datum/repeat1 match.
Existing absolute-reference callback6b2c0 reused after duplicate preflight
rejection; initial WIP build/static-check failure retained distinctly. Child
semantics remain mocked in controls, no oversized-copy/fatal-return claims.
Original/W3/full audits/fresh pair and15 native cases pass (14 complete), next
byte-array6b3d0.342282/399693 accepted extent bytes; app/support323982B and1165
functions exclude5 historical CRT bodies. Audit57.156s includes126 canonical
recompiles, not audit-only. Timings/scope: omf-relocation-checkpoint.json.

Checkpoint1281 adds172-byte repeated-byte writer.132 original/canonical
whole-body controls/9 actual ABI rejects cover record capacity splits, repeated
source rewind/null padding, unsigned chunk bounds, child Segment mutations,
position reload and final restoration. Actual4 arguments/38 owned bytes/repeat1
match. No repeat0/nonprogress success or invented guard; children mocked in
controls. Original/W3/full audits and15 native cases pass(14 complete), next
pad6b2a0.342454/399693 extent bytes; app/support324154B/1166functions exclude5
historical CRT. Timings/limits: cases/omf-byte-array-checkpoint.json.

Checkpoint1282 adds28-byte zero-padding leaf.49 whole original/canonical
controls/8 actual ABI negatives check unsigned count wrap, exact zero writes,
bounded payload guards/1004 boundary and RET0. Actual current slot0/request2/
count38/cursor66 comes from omf_find_record(return6bf15), correcting preliminary
status inference of a direct null-array call. Old destination is write-only,
not observed or seeded. Original/W3/full audits and15 native cases pass(14
complete), next datum6b2d0.342482/399693 extent bytes; app/support324182B and
1167functions exclude5 historical CRT. Timing: cases/omf-pad-checkpoint.json.

Checkpoint1283 adds245-byte datum writer.277 original/canonical controls/9
actual ABI rejects cover LE integer widths, unchanged floating bit copies,
append ABI, count wrap and signed width reloaded after child mutation. Actual
type2/width4/value3/count40/cursor68 and two caller-owned formals match; only
active source fields observed, destination old bytes write-only. Checker-only
NUL literal length failure retained; no target change for it. Original/W3/full
audits and15 native cases pass(14 complete), next absolute-reference6b2c0.
342727/399693 extent bytes; app/support324427B/1168functions exclude5 historical
CRT. Timings/limits: cases/omf-datum-checkpoint.json.

Correction to checkpoint1281 prose: byte-array controls total68, not132.
The retained raw validation always reports68:4x2x2x2x2=64 core states plus4
positions. No behavior/check failure or missing case; prior raw evidence and
checkpoint retained. See cases/omf-byte-array-count-correction.json.

Checkpoint1284 adds16-byte absolute-reference adapter with15 forwarding
controls/8 actual ABI rejects. Actual displacement-4 and lookup-owned symbol
id118/bucket54 match; childtype9/displacement/relative0 andRET8 preserved, no
pointee dereference or child behavior inferred. Original/W3/full audits and15
native cases pass(14 complete), next fixup6b990.342743/399693 extent bytes;
app/support324443B/1169functions exclude5 historical CRT. Timings/scope:
cases/omf-absolute-checkpoint.json.

Checkpoint1285 adds840-byte fixup constructor and contextual24-byte typed
entry, preserving reserved bytes and high old flags.296 whole original/
canonical controls/10 ABI negatives cover relative0->bit0, index/code truncation,
lookup cached flags/addend/thread reloads and true diagnostic/cold label paths.
Actual4 formals/id118/storage4/section2/current count124/segment2 match. First
model exposed wrong new section-to-thread selection; corrected2=data,4=const,
6=bss, with failed model/wrong-body WIP build retained and no earlier promotion.
Only accepted same-domain thread initialization exists, already correct. Original/
W3/full audits/fresh pair and15 native cases pass(14 complete), next allocator
6bd10.343583/399693 extent bytes; app/support325283B/1170functions exclude5
historical CRT. No real old entry dump/seed or cold execution claim. Corrected
build41.159s includes126 canonical recompiles; audit41.853s separately. Details:
cases/omf-fixup-checkpoint.json.

Checkpoint1286 adds54-byte no-formal fixup address allocator and encountered
typed250x24 BSS pool, no raw data copy/initializer.24 controls/5 ABI rejects
cover valid slots/counter increment and actual upper-bound diagnostic; memory
hooks reject old-entry reads/writes. Actual counter0, no hidden inputs. Signed
negative counts remain outside known zero/reset/+1 lifecycle, no new guard.
Raw-file BSS-counter proof mistake retained/corrected to loader-zero membership,
without target change. Original/W3/graph/layout/ABI/focused controls/fresh pair
and15 native cases pass(14 complete), next output_noop_function68d70.343637/
399693 extent bytes; app/support325337B/1171functions exclude5 historical CRT.

Ordinary audit now uses16 additional existing-pattern --abi-only paths after
actual ABI/negative/extent/storage proofs; public/alignment guard source and
receipt hashes retained. Full semantic grids remain default and explicit under
audit-data; function/control-loop ASTs unchanged. Host mode verification5.926s
includes2 incompatible guard-option rejects. Next required1286 audit20.691s
with0 recompiles versus prior full-grid1285 batch41.853s; this includes a new
54-byte function, no separate profiling run. Newly changed contracts retain
focused full controls at acceptance. See cases/omf-new-model-modes.json and
cases/omf-fixup-allocator-checkpoint.json for timings/scope.

Checkpoint1287 refines output_noop_function from a naked/no-argument stub
to empty4-argument fastcall C matching originalRET8(3bytes). All4 unused inputs
are observed through caller aliases, frame0/line34; filename contents unused.
12 full controls/9 ABI rejects check stack cleanup/saved registers/guards and
no non-stack access; no assembly adapter or output-flags assertion. Constructors
assign typed callback directly. Original/W3/ordinary audits/fresh pair and15
native cases pass(14 complete), next local-external6adc0.343640/399693 extent
bytes; app/support325340B/1172functions exclude5 historical CRT. New full
controls separate from historical structural audit21.257s. Timings/scope:
cases/output-noop-begin-checkpoint.json.

Checkpoint1288 adds64-byte LEXTDEF emitter,32 full original/canonical
controls/7 actual ABI rejects. Actual _omf_adjust/id124/flags1/attrs6031 and
mutable external counter2 match, sharing existing85078 domain; original initial1
unchanged, no invented init/counter/helper. Old Symbol.field14 output not read
or seeded; contextual name-provider fields/owned name only. Original/W3/ordinary
audits/new focused controls/fresh pair and15 native cases pass(14 complete),
next debug filename wrapper6b730.343704/399693 extent bytes; app/support325404B/
1173functions exclude5 historical CRT. Audit21.908s, historical full grids stay
explicit. Timings/scope: cases/omf-local-external-checkpoint.json.

Checkpoint1289 adds whole5-byte filename tail adapter as typed C; actual
omftypes.c ECX pointer aliases current/listing/machine record owner.15 forwarding
controls/10 ABI rejects, original/W3/graph/extents/ABI/storage checks, fresh pair
and15 native cases pass(14 complete), next fixup writer6c400. Return-only audit
first rejected tail JMP; corrected exact target/extent proof verifies accepted
header RET0. Failure3.481s retained; corrected ordinary audit22.226s. Physical
tail/call return frames are not input identity.343709/399693 extent bytes;
app/support325409B/1174functions exclude5 historical CRT. Full object pending.
Timings/limits: cases/omf-filename-checkpoint.json.

Checkpoint1290 adds55-byte signed pending-fixup traversal with dynamic bound
reload after each child. Actual count4/typed pool identity matched; entry fields
not consumed/dumped/seeded.105 full controls/7 ABI rejects cover zero/negative
reset,1..250 valid traversal, child count changes and stride24; memory hooks
reject pool-field accesses. Original/W3/ordinary audits/fresh pair and15 native
cases pass(14 complete), next named serializer6c680.343764/399693 extent bytes;
app/support325464B/1175functions exclude5 historical CRT. Audit23.551s, focused
controls separate; full object still pending. cases/omf-fixup-writer-checkpoint.json.

Checkpoint1291 adds whole306-byte FIXUPP32 serializer in typed C. Actual
sole pointer/slot0/count4/type9/location120/flags7/addend40 and conditional
fields match; no padding/opaque entry dump.888 independent wire/control states
plus8 ABI rejects cover flags/threads/indices/addend suppression/signed type
bits and child mutations, cached flags versus later reloads. Original/W3/
ordinary audits/fresh pair and15 native cases pass(14 complete), next public
symbol callback6b710.344070/399693 extent bytes; app/support325770B/1176functions
exclude5 historical CRT. New full controls20.839s, ordinary structural audit
23.228s, full object pending. cases/omf-fixup-entry-checkpoint.json.

Checkpoint1292 adds28-byte conditional public callback as typed fastcall C.
Actual current-function alias/flags1/segmentID5 match;54 full controls/7 ABI
rejects verify bit20 gating, lookup-result forwarding and no re-test after child
mutation. Original/W3/ordinary audits/fresh pair and15 native cases pass
(14 complete), next line callback6b740.344098/399693 extent bytes; app/support
325798B/1177functions exclude5 historical CRT. Ordinary audit23.526s, full
object pending. Timings/limits: cases/omf-symbol-callback-checkpoint.json.

Checkpoint1293 adds64-byte source-line callback. Actual line34/offset0 and
caller/current-symbol context match.180 full controls/9 ABI rejects prove saved
inputs, current-symbol pointer/flags reload after lookup, low16 line, selected
normal/COMDAT writer and RET4 child/RET0 parent. Both writers still named traps,
actual normal branch advances to6b780. Original/W3/ordinary audits/fresh pair
and15 native cases pass(14 complete).344162/399693 extent bytes; app/support
325862B/1178functions exclude5 historical CRT. Audit23.369s, full object pending.
cases/omf-line-callback-checkpoint.json retains split timings and scope.

Checkpoint1294 adds110-byte LINNUM32 writer. Actual word34/offset0/stack
segment and caller lookup result/closed hashID5 match; no segment payload or
incoming record input.216 full six-byte wire controls/8 ABI rejects verify
count/cursor guards, input preservation across begin95 and RET4. Original/W3/
ordinary audits/fresh pair and15 native cases pass(14 complete), next size
reference6bd00.344272/399693 extent bytes; app/support325972B/1179functions
exclude5 historical CRT. Audit23.894s, full object pending.
cases/omf-line-entry-checkpoint.json retains split timings and limits.

Checkpoint1295 adds16-byte CodeView size-reference adapter: symbol/type11/
displacement/relative0 permutation matches. Actual displacement-20=26-length46
in write_codeview_function, original69704..69713 and closed globalID124 prove
caller/owner.15 controls/8 ABI rejects, original/W3/ordinary audits/fresh pair
and15 native cases pass(14 complete), next CodeView block69b20.344288/399693
extent bytes; app/support325988B/1180functions exclude5 historical CRT. Ordinary
audit24.252s, full object pending. cases/omf-size-checkpoint.json.

Checkpoint1296 adds119-byte lexical S_BLOCK32 writer. Actual offset20/bytes65/
functionstart0 and lexical scope20..85/debugoffset122 match. Packed24-byte C
record emits only23 initialized bytes, no alignment padding input/output.240
full wire/order controls/10 ABI rejects prove pre-child offset cache and final
debug global reload across all callbacks. Original/W3/ordinary audits/fresh
pair and15 native cases pass(14 complete), next relative reference6bcf0.344407/
399693 extent bytes; app/support326107B/1181functions exclude5 historical CRT.
Audit24.202s, full object pending. cases/codeview-block-checkpoint.json.

Checkpoint1297 adds16-byte relative operand adapter: symbol/type9/
displacement/relative1 permutation, childRET8/parentRET0 match. Actual
displacement1=encoder cursor-buffer/caller offset and global symbolID124
aliases observed.15 controls/8 ABI rejects, original/W3/ordinary audits/fresh
pair and15 native cases pass(14 complete), next global CodeView data69f30.
344423/399693 extent bytes; app/support326123B/1182functions exclude5 historical
CRT. Audit24.542s, full object pending. cases/omf-relative-checkpoint.json.

Checkpoint1298 adds191-byte global data/TLS CodeView writer with existing
shared typed record layout. Actual sole symbolID116/name_omf_unknown/flags2304/
type117/coff0/debugoffset559 match; inactive nonCOFF offset and descriptor tails
not dumped.768 full independent wire controls/10 ABI rejects, original/W3/
ordinary audits/fresh pair and15 native cases pass(14 complete), next module
finalizer6a9d0.344614/399693 extent bytes; app/support326314B/1183functions
exclude5 historical CRT. Full new controls19.541s, structural audit25.780s;
names>255 original length behavior still unexercised, no guard/normalization.
Full exact object pending. cases/codeview-global-data-checkpoint.json.

At the unaccepted module-finalizer input, exact buffered output comparison
found original1994/native1996 bytes, first difference636 in LPUBDEF at635.
Original group0/segment1 became native group94c/segment1: an older default
segment group was misclassified as an empty C string. Original84fd4 points at
a two-DWORD zero descriptor84fc8..84fd0, whose number field is at+4. It now
uses typed OmfGroup ungrouped_segment and the same bound pointer; type/8-byte
extent/offsets, distinct-zero/string/null negatives and real initializer use
are checked. No string padding or record/checksum compensation.

Fresh finalizer input now matches all1994 defined bytes, FILE state and7
segment rows; unchanged original failed-prefix bytes also match the new
reference exactly. Failed raw prefixes and observation remain under
cases/omf-module-end-abi; passing evidence under omf-module-end-abi-corrected.
Original/native builds,/W3,affected full prefix initializer,ordinary graph/
extents/ABI/storage and15 native cases pass(14 complete). Ordinary audit
26.752s, real identity/path.606s; full measured attempts/splits retained in
cases/default-segment-group-checkpoint.json. Accepted1298/344614 bytes and
app1183functions remain unchanged; finalizer6a9d0 still unaccepted, full exact
OMF object pending.

Checkpoint1299 adds whole253-byte module finalizer after the separately
recorded default-group repair. Actual no-formal FILE/full1994-byte prefix/
7-segment inputs match.336 full controls/10 ABI rejects prove conditional
flush order,header rewrites,post-child global/list/order reloads,restore-end
seek,MODEND/finalflush/close and started0; ignored errors remain ignored.
Original/W3/ordinary audits/fresh pair and15 native cases pass(14 complete),
next26-byte MODEND writer6aad0.344867/399693 extent bytes; app/support326567B/
1184functions exclude5 historical CRT. Full controls6.248s, structural audit
26.330s; full exact object pending. cases/omf-module-end-checkpoint.json.

The derived standalone OMF typed-data/debug case completes at1300. The final
26-byte MODEND writer has actual no-formal/caller proof and54 full controls/
5 ABI rejects. Original/W3/ordinary graph/extents/RET/ABI/storage pass. Fresh
masked original and native both exit0 with empty streams and exact2594-byte
omftypes.obj SHA df5cd7020fa23f611e9f0782c135ac2cee61cbc5b73b906028c9d9a8fb8cf6e9.
No normalization. All15 cached native cases complete; strict OMF artifact/input
negatives remain. The case adds49 application/support functions and8461 code/
text-data extent bytes since1251, no branch-coverage claim.

Accepted union344893/399693 bytes(86.2895%); app/support326593B/1185functions
exclude5 historical CRT bodies. Remaining46,092 unclassified bytes/145 pending
driver rows are not assigned to CRT or treated as measured bodies; non-CRT
denominator unavailable. Final structural audit27.135s; native OMF setup.014184s/
execution.005937s/check.001161s,15-case wrapper.848s. Full measured attempts,
splits/oracle references/limits: cases/omf-completion-checkpoint.json. The earlier
default-group divergence and true8-byte storage repair remain preserved, with
no record-length/checksum compensation.

Approved COMDAT/communal case captured with unchanged originalCL /O2 /Ob0 /Gy
/G5 /Z7 source/options; exact original C2 transport plus supported standalone
-omf yields1962B packets.obj, no normalization. Actual COMDAT/CEXTDEF/LINSYM/
COMDEF/LIDATA counts3/3/3/1/2; CL itself did not emit -omf. At1301 the85B
COMDEF writer has actual soleECX symbol/owned name/type4107/size68/counter1
proof,192 full original/canonical controls and8 ABI negatives. Fresh original/
native advance to named communal-size encoder6c600;15 completed cases and
new16th expected frontier pass. Full ordinary audit26.912s, build2.892s,
controls3.517s, fresh original1.654s/native.241s, native suite wrapper.872s.
See cases/comdat-communal-checkpoint.json for complete capture/repeat timings
and limits; exact completed object pending, no child semantic coverage claim.

At1303 the signed communal-size encoder116B and iterated-data writer195B
are reconstructed. Their153/1152 whole original/canonical controls and6/8
ABI negatives pass; original compiler/W3/ordinary graph/extents/ABI/storage,
fresh matching and16 native cases pass. Actual LIDATA source is caller-owned
zero,repeat1016/width1/combine2; coldCOMDAT/synthetic negative cases remain
model controls only. Iterated body ends at6b293RET8, excluding padding from
earlier196B provisional envelope. Next CodeView register-location pass2b1b0.
Split timings/extent/current limitations: cases/comdat-size-checkpoint.json
and cases/comdat-iterated-checkpoint.json. Exact final object still pending.

At1304 the153B CodeView register-location pass is accepted: actual27-node
closed register lists/two eligible temporaries/no-formal ABI match;384 whole
controls/7 negatives preserve saved home+next across representative lookup,
signed cost/conflicts gating and outputregister low6bits. Original/W3/fresh
matching and16 native cases pass. The corrected ordinary audit27.615s follows
a retained test-only BSS/raw-file assertion failure; no target behavior changed.
Next expand_zero_extension at24740,1213B analysis body, still named trap.
See cases/comdat-debug-register-checkpoint.json including failed timings.

At1305 the1213B zero-extension expansion is accepted as readable C in
src/zero-extension.c.768 independent ISA-plan original/canonical controls
cover register/highbyte/alignment/addressalias/followingAND forms,8 ABI
negatives, exact child arguments/list effects/RET0. Actual indexedglobal
MOVZXW4 dest1 context matches; other shapes are cold controls. No new data,
assembly or helper edges. Original/W3/audit/fresh pair/native16 cases pass.
New source list caused one full41.012s build; focused controls14.706s,
ordinary audit28.596s, fresh original1.200s/native.220s, suite1.019s.
Next local OMF name6ae50; exact final object pending. Split receipt:
cases/comdat-zero-extension-checkpoint.json.

At1307 local-name wrapper49B and CEXTDEF writer66B are accepted. Actual
_packet_step name aliases the compiler workspace, then sourceid161 resolves
closed segmentid183/nameindex17/counter4.72/64 whole original/canonical
controls and6/7 ABI negatives, original/W3/audits/fresh pair/native16cases
pass. No outputfield14 or uninitialized localindex input snapshots. Next
COMDAT selection mapper6c2f0. Exact1962B object pending; split receipts in
cases/comdat-local-name-checkpoint.json and comdat-external-checkpoint.json.

At1309 the57B COMDAT selection mapper and110B LINSYM writer are accepted.
Actual selection2 is lowCL and returnlowAL; original caller bytes prove this
unsignedchar ABI.1536 full byte/upperECX/diagnostic controls retain exact541
error edge and post-callAL0, no error execution claim. Initial test-only
actual1 assertion and midcall callee-saved assertion failures retained, corrected
without target changes or original recapture. LINSYM actual line14/offset0/
closed segment183 and216 wholecontrols/8 negatives pass. Original/W3/audits/
fresh pairs/native16cases pass; next CodeView register69880. Fullcontrols
29.058s/5.700s and all failed timing receipts in cases/comdat-selection-
checkpoint.json and cases/comdat-line-checkpoint.json. Final object pending.

COMDAT/communal OMF completes at 1311: exact 1962-byte object SHA256
2dcb42c74d07c84c7aa7f25a1724dd70b43ea06b1dca747a5e257ff544557783,
without normalization, empty streams and exit 0. Eleven new functions cover
2214 bytes, including the CodeView register writer (124) and register-number
helper (46). Original /W3, graph/ABI/storage audits, focused whole-function
controls and all 16 cached native cases and negatives pass.

Actual SEGDEF alignment 8 exposed the original missing-return EAX behavior:
the record pointer survives the default RET and its low three bits enter the
attribute. Native regcall makes EAX/ECX explicit C formals on the same symbol,
with compiler tracing and RET 0. Win32 retains one fastcall formal and the
missing C return; narrow assembly transports EAX at the two callers and the
callee. Other unsupported native alignments still trap.

The genuine adjacent empty-name DWORD and four 1004-byte records now have
one 4020-byte contextual owner. VC1 lacks align/bss_seg; the approved OMFREC
initialized-zero section changes the file representation, preserving original
loader values. Native uses alignment 8. Actual PE/ELF member identities, all
slot strides and address remainders, zero fields and canonical relocations are
checked. The storage change adds no function or code credit. Historical
alignment-4 ABI evidence keeps its original defined-branch scope; conditional
EAX evidence is separate.

Whole-function controls caught a count increment moved after the alignment
call despite matching final bytes. The corrected count is 3 before the COMDAT
call, with cursor offset 25. Failed guard/partial-object evidence, placement
probes, comparisons and repeated checks are retained. Final audit plus coverage
took 29.545 seconds (Rust alone was not timed); the 16-case wrapper took 1.000.
COMDAT setup/run/check took .0141/.00819/.00106 seconds. Accepted extent union
is 347107/399693 = 86.8434%, not branch coverage. Application/support source
covers 328807 bytes in 1196 functions; five historical CRT bodies cover 1776
bytes separately. A reliable non-CRT denominator is unavailable because 43887
bytes remain unclassified. Split timings, failures and the next-work inventory
are in cases/comdat-completion-checkpoint.json and remaining-code-inventory.json.

SEH packet case starts at 1311 from unchanged original /O2 /Ob0 /G3 /Z7.
One CL workflow (2.979s) and standalone C2 (.838s) agree on the full 3723-byte
COFF/CodeView object after the established timestamp exemption. Actual output
has a two-entry 24-byte exception scope table, three code relocations, handler/
unwind references and S/F/T debug sections. Five captured IR channels and the
owned source are pinned; no PDB or extra artifact. No target output execution.

At 1312 the reached 44-byte partial-write predicate is accepted. Actual tagged
CX 0x8024 matches the caller scalar; both accepted lookups consume only low16,
so the original partial SI assignment introduces no hidden high-word input.
160 whole original/canonical controls check optional miss versus required
re-query, the second result's bit40, call order, high-word poisons, boolean EAX,
RET 0 and preserved records/nonvolatile registers. Original /W3 and ordinary
audits pass; all 17 cached native cases match. Next assembly-label extractor
27850 remains a named trap; protected-function envelopes are still provisional.
Build2.709s, full controls3.483s, audit/coverage30.922s, original1.036s/native
.212s, suite1.029s. Full split receipt: cases/seh-partial-register-checkpoint.json.

At 1313, the 194-byte assembly-reference extraction pass is accepted. Actual
Node and forwarded left-pointer aliases match the caller; a NULL local head is
output storage. Seventy-two whole original/canonical controls verify tail
sentinel linking, typed marker construction, symbol flag writes, duplicate
aliases and exact pointer/stack/nonvolatile behavior. The actual case stops at
the first visitor call, so its list transformation is not yet exercised by the
case. Visitor and collection callback remain named traps. Original /W3, audits
and all 17 native cases pass. A missing intern_leaf header caused one 1.079s
failed build; the corrected header dependency rebuild took 40.336s. Controls
2.074s, audit/coverage31.163s, fixture wrapper.984s. Full receipt and retained
failure: cases/seh-assembly-references-checkpoint.json.

At 1314 the 117-byte machine-symbol visitor is accepted. All three formals
and actual operand slots match: operation1/count2, based symbol in operand1,
ignored register operand0, named callback and opaque parent-owned local-head
context. Four hundred whole original/canonical controls cover all operand
classes, aliases, optional slots, callback/global substitution and mutations
that distinguish cached slots/count from fresh later operands. The 231 named
signed counts match existing original data. Original /W3, ordinary audits and
all 17 native cases pass; the actual callback still traps, so substitution and
callback-body execution remain unclaimed. Build3.012s, controls7.982s,
audit/coverage31.672s, suite1.052s; cases/seh-machine-symbol-checkpoint.json.

At 1315 the whole 126-byte collection callback is accepted. The actual kind-1
symbol and initially NULL parent-owned head have matching register, caller and
storage identities. The 216 whole original/canonical controls and eight ABI
rejects cover pointer duplicates, kind bypasses, diagnostic and constructor
calls, and the head reload after constructors. Other paths are controlled
rather than claimed executed. Original /W3, ordinary audits and all 17 native
cases pass. A stale parent audit assertion was corrected to allow the accepted
callback, retaining its failed 30.625s log. Shared header changes caused full
canonical/trace rebuilds inside the 64.515s audit and 7.262s check batches;
the actual fixture loop took .689s. Normal native-only refresh took 5.960s.
Full evidence/timing: cases/seh-collect-reference-checkpoint.json.

At 1316 the whole 49-byte assembly-label normalization callback is accepted.
The actual kind-1 symbol/cached slot identity and unused NULL context match.
The 360 whole original/canonical controls cover label redirects, self/alias
references, full identifiers, wrapping counts and post-lookup mutations; the
first actual entry skips that label branch. Original /W3 and ordinary audits
pass. Function input and fresh frontier evidence use the exact earlier native
ELF with published shared revision 604a9954c. A later shared perror candidate
triggered a trace rebuild during check; its 17-case pass is candidate-only,
later published unchanged as READY 45f7a2ace, with exact ELF/source hashes
preserved separately. No original recapture or suite rerun for publication.
The next 28-byte flow-reference callback remains a named trap. Build took
4.843s, controls 6.586s, audit/coverage 32.098s, original 1.350s/native .274s.
The 7.263s check includes a full trace rebuild; actual fixture loop .706s.
Receipt, source bindings and limits: cases/seh-normalize-assembly-checkpoint.json.

At 1317 the whole 28-byte assembly flow-reference callback is accepted. Both
actual register formals match: a scope-owned kind-1 symbol/cached caller slot
and the opaque current work-block context. Ninety whole original/canonical
controls cover the kind gate, exact lookup/connect forwarding, retained context,
post-lookup target changes and full zero return despite child clobbers. The
first actual entry skips the label path. Original /W3, audits and all 17 native
cases pass. Build inputs/ELFs are explicitly bound: new-function matching used
published 45f7 sources; the later fixture relink used unchanged candidate bytes
subsequently published as 574227e0e. Earlier input observation used the recorded
604 ELF. Publication required no recapture or rerun. Build 35.242s includes
a shared-header dependency rebuild; controls 1.728s, audit/coverage 32.413s.
Check 3.137s includes shared CRT relink; actual fixture loop 1.174s. Next is
the 385-byte inline-assembly frame-register allowance scan. Full evidence:
cases/seh-reference-assembly-checkpoint.json.

At 1318 the whole 385-byte inline-assembly frame-register allowance scan is
accepted. The actual Function/current-function identity and five-work graph
match, including two assembly blocks and active local-symbol operand aliases.
The 370 whole original/canonical controls and eight ABI rejects verify explicit
ESP writes, implicit ESP mask, DATA, EBP local exceptions, all 231 existing
count/implicit-property identities and fresh reads after child mutations.
Original /W3, graph/ABI/storage audits, fresh pair and all 17 native cases pass
on explicitly frozen published 574227 inputs. A missing owning header caused
one canonical link failure; fixed fastcall declarations and retained failure
logs. Corrected build 4.561s, full controls 18.821s, audit/coverage 33.844s;
fixture wrapper 1.056s. Next SEH preparation wrapper is only 69 bytes: the
earlier 1616-byte analysis envelope contains several separate functions, each
still trapped. Receipt: cases/seh-frame-allow-checkpoint.json.

At 1319 the 69-byte SEH preparation wrapper is accepted; its established
provisional logical name prepare_inline_function is retained. The original
1616-byte envelope is several functions, never one accepted extent. The sole
opaque Function matches its caller/current-function identity; no payload or
old reset-cell inputs exist. Four typed contextual owners are distinct, and
90 whole original/canonical controls verify zeroing, reset/collect/reset/rewrite
order, saved pointer, child mutations, partial stops and RET0/nonvolatile.
The actual case stops at the separate 11-byte reset helper; collect/rewrite
remain named traps. Original /W3, audits, fresh pair and all 17 cached expected
terminals pass. Build 12.341s includes native header fanout, controls 1.824s,
audit/coverage 33.588s, fixture wrapper 1.023s. Explicit source/ELF identity,
corrected extents and split timing: cases/seh-prepare-checkpoint.json.

At 1320 the separate 11-byte scope-stack reset is accepted. Exact MOV/RET
proves no formal, hidden input or old cursor read. The actual wrapper caller
matches; both known callers ignore EAX, so no zero/preserved return is invented.
Twenty-five whole original/canonical controls and eight ABI rejects check
poisoned old values/registers, RET0/nonvolatile and unchanged adjacent storage.
The cursor is an original initialized-data zero DWORD; an initial checker
BSS assumption failed before semantic controls and was corrected without a
source change. Its failed log is retained. Original /W3, audits, fresh pair
and all 17 expected native terminals pass. Build 2.610s, full controls .803s,
audit/coverage 33.979s, fixture wrapper .971s. Next discovery pass is a separate
550-byte function, still trapped. Receipt: cases/seh-reset-checkpoint.json.

At 1321 the whole 550-byte scope-discovery pass is accepted. The actual
Function and 55-expression input graph match, including Node/label identities
and conditional markers. Thirty-one whole original/canonical controls and
12 ABI rejects cover nested scopes, label IDs, finally counts, return markers,
handler-block splices, cached predecessor reuse, exact diagnostic stops and
fresh/cached values across child mutations. Original malformed-list assumptions
remain; there is no invented cursor initializer or repair. Actual execution
stops at the independent allocator, so transformation/diagnostic bodies are
controlled rather than claimed executed. Original /W3, audits, fresh pair and
all 17 cached native terminals pass. Audit/coverage 35.022s and fixture wrapper
1.034s are separate from the 40.600s header-dependency build. A failed source
preflight mistakenly continued into that build; stable exact owner-confirmed
bytes, honest postbuild source sampling and ELF/PE hashes are preserved. They
were published unchanged as f086d3fd2. Full receipt: cases/seh-collect-checkpoint.json.

At1322 the whole114-byte exception-scope allocator is accepted. The actual
no-formal entry, arena1/48 request, owned record/arena cursor lifetime and48
cleared bytes before the parent query match original/native. The full48-byte
typed record retains its existing prefix, with contextual constructor-cleared
word36, local-set pointer40 and flags44. No new CRT call edge supplies the clear.
120 whole original/canonical controls and11 ABI rejects cover poison/adjacent
bytes, wrapping IDs, parent IDs, child mutations and fresh list append. Original
full return is retained, while actual native execution stops at the separate
enclosing-query trap. All17 cached expected terminals, original/W3 and structural
audits pass. Build14.323s, audit/coverage34.312s, fixture wrapper1.031s are
separately measured. Receipt: cases/scope-allocate-checkpoint.json.
