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
