C13232.EXE reconstruction (project vc1.0, toolchain1.0-nt)
========================================================

Fifty-eight native phases are complete: minimal C, optimized Win32 hello, DLL,
numeric/varargs, explicit PCH creation/use, do-while, nested SEH, inline
assembly, browser output, /E, /EP, /P, preserved comments, /Zg prototypes,
automatic-PCH creation, reuse and changed-macro rebuild, embedded /Z7 types,
/O2 /Ob1 plus /Ob2 small functions with early returns, and the combined
enum/long-record/literal/floating case, debug-bearing PCH creation/reuse, and
richer indexed/member/OFFSET/x87 inline assembly, plus the TLS/static/pragma/
automatic-bitfield checksum case, the deliberate diagnostic/recovery unit, and
the named-base relocatable record arena, /Zi PCH library binding, the legacy callback adapter, assembler expression/directive checksum, browser-bearing timestamp PCH create/reuse, bounded assembler error recovery, configurable calibration tables, typed packet-view/callback adapters, normal/expected-error arena API prototype phases, portability-header diagnostics, explicit-base/goto traversal, typed x87/bit-scan arithmetic, generated-header shutdown failures, protocol-header parser repair, assembler operand diagnostics/SEG, and standalone QuickC regions/loops/memory initialization.

There are 924 accepted source functions and 1057 accepted original entries.
Of these,913 are application functions; ten historical runtime.c bodies and
the historical __strgtold12 body in floating.c are CRT, with no application credit.
Accepted original extents cover 246991/277234 code-section bytes (89.091165%):
224253 application bytes,22552 CRT bytes and186 import-thunk bytes. This is a
union clipped to code-section VirtualSize, including shared text data; it is
not branch coverage. Of this union,245156bytes have public-case evidence and
1835 bytes have separate direct-test evidence for assembler support, expression
templates, database object methods and legacy segment/type helpers; no public
trigger is established for those fourteen functions. Unvisited
functions retain named trapping stubs.

Native build and checks
-----------------------
Run from this directory:

  make -j4
  make -j4 check
  make -j4 check CASE=numeric-varargs
  make -j4 check CASE=pch-use
  make -j4 native-check
  make -j4 native-adoption-check

The native i386 build uses Clang -O0 -g. check/replay compare cached original
first-entry projections, status, raw streams, complete file sets and artifacts.
Shared lib/recon C sources are linked directly; target runtime.o and shims are
excluded from application instrumentation. The original BINMODE.OBJ default is
retained through RECON_DEFAULT_FMODE=0x8000. Fixtures never invoke Wine or
recapture references. Unknown definitions and dependencies fail.

cases/original-entry-references.json binds 58 completed phase histories to fresh
shared-prefix original first-entry captures. All 899 publicly reached accepted application entries
are checked with accepted-prefix seeding and exact expected order. Every native
case checks status, raw streams and complete artifacts. Earlier migration checks
are recorded in cases/shared-prefix-validation.json; automatic-PCH completion and
current58 public plus35 direct regression receipts are in
cases/ide-debug-pch/validation.json.
The native cache is not silently refreshed. Regenerate it explicitly with
scripts/native-traces.py --write after a verified original-toolchain map and
acceptance metadata change.

Case                  irex  irsy  irin  irgl  Other outputs
minimal                866    13     3    50
hello-win32           2066   422    46   629
hello-dll             2129   499    49   669
numeric-varargs       2330   535   149   402  irdb63, msvc.pdb574
pch-create              94    33     3    68  state.pch38156
pch-use               1071   116    47    87
do-while               888    39     3    50
seh                   1134   114     6    79
inline-asm            1100   107     6    75
browser                971    69     6    79  test.sbr245
preprocessor-E           -     -     -     -  stdout381
preprocessor-EP          -     -     -     -  stdout292
preprocessor-P           -     -     -     -  test.i381
comments                 -     -     -     -  stdout337
prototypes               -     -     -     -  stdout360
automatic-pch-create    179    67     6    77  state.pch38021
automatic-pch-reuse     999    67     6    85  stdout26; state.pch unchanged
automatic-pch-changed   179    67     6    77  state.pch38022
embedded-codeview     1196   177    74   332  irdb643
inlining-ob1          1369   223    15   162
inlining-ob2          1369   223    15   162
combined-types        1611   186   198   484  irdb1261
debug-pch-create       443    87    26   462  irdb619; debug.pch40167
debug-pch-use         1212   108     6   215  irdb67; debug.pch unchanged

All primary intermediates match raw bytes. Numeric stderr retains the captured
line24 C4069 CRLF warning. Only identified timestamp DWORDs at irdb+14 and
msvc.pdb+48 are ignored, after raw equality between both fields is checked.
No path/pointer normalization occurs. The exact PDB path is
S:\vc1.0\c1\numeric-varargs\msvc.pdb. The actual native PCH producer must
match the reference before its bytes are supplied to the consumer; selecting
a dependent case runs its producer first. Header mtimes are pinned input data
for automatic PCH. Original CL's stdout/stderr alias is preserved where required.

Original builds and reference capture
-------------------------------------
These explicit operations are separate from fixtures:

  make -j2 audit-win32
  python3 scripts/coverage.py
  python3 scripts/build-capture-helper.py

Original builds/captures use ~/.ai-tmp/vc1.0-capture/wine. Fixed R: maps to the
repository and S: to ~/.ai-tmp/msvc2. No tool remaps those drives or stops the
shared server. Relative paths are used where output permits them. Observable
absolute case paths use S:\vc1.0\c1\<case>; C1 serializes each fixed original
cwd and snapshots its complete contents. Native runs use independent host
namespaces through per-process RECON_DRIVE_R/S mappings.

src/capture-original.c is a separate original-only debugging utility, never
linked into C1 or executed by fixtures. It accepts an ordered entry plan,
restores exactly one instruction byte on a matched breakpoint, rewinds EIP,
and retains register/stack/pending-buffer evidence at a terminal trap.
Wrong entry order is rejected. scripts/capture-frontier.py validates the
plan, frozen source dependencies and exact one-entry MSC_CMD_FLAGS environment.
Full-reference captures and masked captures are explicit, never silent.

Original compiler audits check778 function graphs and instruction boundaries,
286 state/default aliases,105 PCH roots and12 callbacks, the21-entry function
shape cache, scalar options, pragmas, intrinsic groups and serialization data.
The C grammar audit checks376 states,57904 actions/timings,227 productions and
34216 gotos. The assembler audit checks136 states,6936 actions/errors,
136 lookahead/default timings,83 productions and3128 gotos, plus contextual
operand/register/mnemonic tables. Logical model and linked C rows both match.

Source layout and active queue
-------------------------------
Source lives in src/, scripts in scripts/, metadata in cases/. Function
ownership, extents, accepted order and text data are recorded in functions.json.
Frontend subsystems keep their contextual data and semantic wire structures;
no copied image sections or runtime original-image reads are used by C1.
store_extended retains its documented ESI/x87 ABI and a native-only entry marker
that preserves registers, flags and floating state. The source also compiles
with the original Win32 compiler; recon.h is empty there.

The approved queue is cases/approved-queue.json. The legacy PDB object workflow
is complete; the next substantial residual case awaits review. Existing sources,
original options/environments, dependencies, outputs and acceptance ranges are
durable. Independent grammar/layout/ABI analysis remains available in scripts/
and cases/analysis/. Measured builds, setup, reference capture, native execution
and checks are reported separately in current validation receipts.

Completed substantial case
--------------------------
cases/combined-types-literals completes the approved single combined case.
Its original /Z7 /O2 transport and raw oracles are frozen; ten new functions
cover floating arithmetic/comparison/conversion, richer escape/literal handling
and a continued debug field list. Its C enum declarations lower to int and do
not reach enum debug writers. That case is finished; the debug-PCH workflow
below is now active.

Shared graph audit
------------------
audit-win32 invokes tooling/graph-audit directly with CFG discovery and RET,
function-reference, ownership and whole-extent checks. Its Cargo prerequisite
is built quietly through the shared Make include. scripts/audit-text-masks.py
retains only the original external-switch-table mask assertion, which is not
yet in the shared engine. State/data/grammar checks remain separate.

The stronger RET check found the historical PCH pointer-array helper had two
missing unused string arguments. The repaired function/caller preserve four
slots and RET16. cases/pch-abi-correction.json and cases/graph-audit record the
old candidate error and current validation. Historical raw evidence is unchanged;
derived entry projections pin its original receipt hashes and document the
logical-name correction. scripts/check-graph-audit.py checks scratch mutations,
including a regression to the old RET8 and an omitted external switch mask.

Completed debug-PCH workflow
---------------------------
cases/debug-pch completes the approved producer/consumer workflow. The /Z7
/Ycdebug.h producer emits an exact40167-byte PCH; /Yudebug.h restores its
LF_PRECOMP type range/checksum/object record and consumes the actual native
producer output after exact comparison. Header/PCH/object mtimes and canonical
S paths remain explicit input data. Consumer verifies the PCH is unchanged.
All24 native phases and10 oracle/dependency rejections pass. The fresh producer
all-call debugger proof also matches every one of six enum-sort operand pairs
in order, alongside the normal original-backed first-entry trace. /Yd was not
needed for this separate-debug mode. No extra case is queued.

Completed richer inline assembly
--------------------------------
cases/inline-asm-rich exercises scaled/register-plus-displacement addressing,
struct member/array offsets, BYTE/WORD/DWORD/QWORD PTR, OFFSET/TYPE/LENGTH/SIZE,
and explicit/implicit x87 stack forms. Original CL accepts /Z7 /O2 /G5 /W3;
the original-built program checks integer/floating results and exits0. Twelve
new whole functions cover member-scope search, expression arithmetic, scale
encoding, operand sizes and x87 normalization. All five raw frontend outputs,
pending buffers, status/streams and first-entry order match originals without
normalization. All25 native phases,11 oracle/dependency rejections and six
strict graph/ABI/mask checks pass. No further case is registered yet.

Completed storage/pragma/runtime-initialization case
--------------------------------------------------
cases/storage-pragmas combines initialized TLS records/scalars, persistent local
statics, nested named packing, alloc_text, explicit intrinsic restoration and
parameter-dependent automatic bitfield/record initialization. Original CL /Z7
/O2 /G5 /W3 accepts all forms; the original-linked checksum program exits0.
Eighteen new functions add2177 frontend extent bytes. Whole fresh original/native
entry order, buffers, streams and five raw IR files match with no normalization.
All26 native phases,11 oracle/dependency rejections, six strict graph/ABI/mask
checks,287 state defaults/aliases and both grammar/table suites pass. No shared
shim change was required. Further work awaits the next root-reviewed case.

Flag-word identity audit
-----------------------
The original scanner word is 0x6bb44 and the separate parser word is 0x6bb48.
Their upper WORDs are 0x6bb46 and0x6bb4a. scripts/audit-flag-words.py
(Python capstone 5.x) checks raw original operand bytes independently of the
source address annotations, inventories every accepted direct access on both
sides and compares normalized per-function bit masks. It also checks the
PCH serialized block's final DWORD slots and their portable C offsets.
The normal audit-win32 target includes this check. For deliberate private
word/mask/offset mutations run scripts/check-flag-words.py --directory PATH.

cases/storage-pragmas/flag-identity/ contains the correction evidence.
The earlier storage matching-output receipts are retained, but are insufficient
to establish these two internal word identities: their caller and callee shared
the wrong parser/scanner choice. The corrected reconstruction additionally
compares both complete 32-bit words at actual original/native function entry
and caller-return boundaries. This correction earns no new-function credit.

Completed named-base record arena
--------------------------------
cases/based-record-arena completes the supported named-base pointer workflow:
two arenas, offset-linked records, pointer arithmetic/member access, ordinary
address conversions and a checked base switch. Original CL /Z7 /Od /G5 /W3
compiles and links the silent checksum program. Eight new whole functions add
1136 accepted frontend extent bytes. Five raw intermediates, entry order,
pending buffers and streams match without normalization. All 28 native phases,
20 oracle/dependency rejections, six graph/ABI/mask checks, three flag-word
mutations, 288 state defaults/aliases and both grammar suites pass. Unsupported
based(void), __self and __segment probes are preserved as rejected syntax;
their remaining compatibility helpers are neither CRT nor completed.

Completed /Zi PCH library binding
--------------------------------
cases/pdb-pch-binding uses public /Yc /Yu /YlBindingArchive with real PDB
enum/record/function-pointer/based-link types. Original compile/link and the
checksum program succeed. Ten whole functions add1557 frontend extent bytes;
both phases match exact streams, IR and39884-byte PCH. Only the proven PDB
creation-signature DWORD is clock-normalized after raw cross-output equality.
Consumer uses actual native producer PCH/PDB, validates the complete captured
backend input contract before using its frozen object, and proves raw signature
propagation. Every enum-sort operand pair matches original order. All30 native
phases and25 oracle/dependency rejections pass, with original compilation and
strict graph/ABI/state/flag/grammar checks. A failed pack(push/pop) preflight
exposed original unrelocated retired packing pointers; it remains explicit
evidence, while the successful source uses pack(2)/pack() and fresh captures.

Completed legacy callback adapter
--------------------------------
cases/legacy-callback-adapter combines old-style narrow/default-promoted
parameters, nested callback redeclarations, array parameter adjustment,
register parameters, explicit indirect calls and accepted void* round trips.
Original /Z7 /Od /G5 /W4 compile/link and the checksum program succeed; six
intentional warnings and five raw IR files match exactly. Nine whole functions
add1242 frontend extent bytes. All31 native phases,28 oracle/dependency
rejections, six graph/ABI/mask checks, three flag mutations,288 state/alias
checks and both grammar suites pass. Some initially proposed helpers remain
trapping because this case did not reach them.

Completed assembler expression/directive checksum
------------------------------------------------
cases/inline-asm-expressions combines binary constants, HIGH/LOW/NOT,
ALIGN/EVEN, _emit NOP, SHORT and DS-addressed reads in one safe checksum
function. Original compile/link/runtime succeed; five raw files and empty
streams match. Seven whole functions add785 frontend extent bytes. All32
native phases,30 rejection checks, six graph/ABI/mask checks, three flag
mutations,289 state/alias checks and both grammar suites pass. The graph audit
caught and corrected the original comment-skip callee before acceptance.

Completed browser-bearing timestamp PCH
---------------------------------------
cases/browser-pch-timestamp compiles one /FR+/Z7 explicit PCH producer and
consumer with nested macro API and pinned-mtime __TIMESTAMP__ strings.
All browser/PCH/IR bytes match exactly; actual native producer PCH/browser
outputs feed the consumer, with a strictly validated frozen backend object.
Four application functions add196 frontend bytes; independently matched
_fstat and its import add662 support bytes. A read-only metadata witness
proves the actual descriptor, return and timestamp at the native CRT seam.
All34 native phases,32 rejections, six graph/ABI/mask checks, three flag
mutations,289 state/alias checks and both grammars pass.

Completed assembler diagnostic/recovery unit
-------------------------------------------
cases/asm-diagnostic-recovery intentionally reports four bounded assembler
errors, requiring C1exit4 and exact diagnostics/full partial browser/IR files.
Five whole functions add427 frontend bytes. Original/native function-finish
observations prove recovery into valid recovered_tail; no backend or runtime
success is claimed. All35 native phases,35 rejection checks, six strict graph/
ABI/mask checks, three flag mutations,289 state/alias checks and both grammar
suites pass. Diagnostic grammar audit now checks50 logical token names.

Completed configurable calibration table
----------------------------------------
cases/calibration-table consumes48 public-D coefficients, verifies integer-
derived gains and partial packed/shared-bitfield/anonymous records, and tests
local thread-static zero initialization and retention. Eight whole functions
add771 frontend extent bytes, including48 original switch-table bytes. All
five rawIR files and one exact C4051 warning match; untouched checksum exits0.
The failed original anonymous-union-tail variant is preserved as a target
limitation. All36 native phases,38 rejections, six graph/ABI/mask checks, three
flag mutations,289 state/alias checks, Vector layout and both grammars pass.
Listing pragmas reach the reconstructed helper with Fs0; active listing output
is untested, and byte coverage remains an extent union, not branch coverage.

Completed typed packet-view/callback adapter
-------------------------------------------
cases/packet-view-adapter combines safe byte views, a read-only pointer dropping
const, defined unsigned negation and correctly typed stdcall callbacks. Three
whole functions add296 frontend extent bytes. Untouched checksum exits0; all
five rawIR files and five warnings match. All37 native phases,41 rejections,
six graph/ABI/mask checks, three flag mutations and state/grammar audits pass.
The original parameter-cache unwritten-local branch remains documented, not
claimed exercised. Provisional prefix helpers stay trapping.

Completed arena API declaration/prototype workflow
-------------------------------------------------
cases/arena-api-prototypes uses one named-based record/callback API source for
normal compilation and public /Zg. Normal original checksum exits0; /Zg keeps
its original C2213 limitation, C1exit1 and complete prototype output. Two whole
functions add217 frontend extent bytes. All39 native phases,45 rejections,
six graph/ABI/mask checks, three flag mutations and state/grammar audits pass.
Prototype-only output has no backend/runtime success claim. Unsupported initial
__far/__loadds forms remain failed preflight evidence, not accepted syntax.

Completed portability-header diagnostics/recovery
------------------------------------------------
cases/portability-diagnostics reproduces six errors/seven warnings and all
six partial artifacts at C1exit6, with no backend/runtime success claim. Eight
whole functions add447 frontend extent bytes. Original/native observations
prove cast_probe and recovered_tail finish, including full scanner/parser
words and reset symbol. All40 native phases,49 rejections, six graph/ABI/mask
checks, three flag mutations and state/grammar audits pass.

Completed explicit-base and goto traversal
-----------------------------------------
cases/explicit-base-control proves public movedbase :> (void*)0 behavior with
outer typed casts, named-based return links and bounded goto traversal. Three
whole functions add766 frontend extent bytes. Numeric and int*-null variants
were rejected and remain evidence; only void*-null preserves the required
marker. Untouched checksum exits0 and all five rawIR files/streams match.
All41 native phases,52 rejections, graph/ABI/mask/state/grammar audits pass.
The old provisional member-expression name is corrected at fixed entry3f820
without rewriting old raw evidence or changing its two-pointer RET8 ABI.

Completed typed x87 and integer bit scans
----------------------------------------
cases/typed-x87-bitscan uses real typed int/double memory operands, balanced
x87 arithmetic, nonzero bsf/bsr and two-operand imul for an exact checksum93.
Three whole functions add114 frontend extent bytes. Untouched program exits0;
all five rawIR files and silent streams match. All42 native phases,55
rejections and graph/ABI/mask/state/grammar checks pass. Failed shared-source
freeze and subsequent stable relink/revalidation remain explicit evidence.

Generated-header fatal shutdown
--------------------------------
cases/generated-header-failures has two strict expectedfailure phases: a missing
header and an incomplete functionlike macro invocation. Both originals/C1 exit1
(CLexit2), leave five exact partial IR files and discard browser temporary/final
files. Six whole frontend bodies add433bytes; the identified _strerror CRT extent
adds224bytes. All44 native phases and61 oracle rejection checks pass, with the
strict originalcompiler/graph/ABI/state/grammar checks. No runtime claim is made.

Protocol-header parser repairs
------------------------------
cases/protocol-parser-repair exercises real LR token insertion and declaration
synchronization, with four exact errors and recovered_tail finalization/browser
marker. Three whole functions add800 frontendbytes. The exact nested semantic
union required shared native RETURN8 transport after stackinput checks caught
a hidden-resultpointer ABI mismatch; Win32 signature and graph stay faithful.
Raw actualregister/RET/caller proofs and old C2282 vararg repair are retained.
All45 native phases,65 rejections and strict canonical audits pass.

Assembler operand diagnostics and SEG
-------------------------------------
cases/asm-parser-repair freezes five bounded syntax/operand errors with exact
partial files and recovered_tail finish/browser evidence. Two whole functions
add142 frontendbytes. Actual caller inputs, consumed operand state and SEG
EAX pointer identity/RET4 are independently proved. All46 native phases and69
rejection checks pass. The generated insertion list is empty in the original;
its two helpers remain unaccepted nonCRT support, with raw sentinel/read-only
reference proof retained rather than forcing a public source path.

Direct assembler support
-------------------------
cases/asm-direct-callbacks validates two generated insertion callbacks that
the original empty table cannot reach from source. Fifteen explicit native
units compare cached original direct-call inputs/state/diagnostics/partial
files and actual EDX:EAX/RET/caller contract. Two bodies plus shared switch
data add186bytes to direct-inclusive extent only; public extent is unchanged.
Original-defined word masks and pointer owners are proved independently.
Normal check includes these Wine-free units; native-adoption-check includes
seven rejection controls and an original-unwritten-word positive control.

Public hdrstop PCH workflow
---------------------------
cases/pragma-hdrstop-pch adds bare /Yc-/Yu create/reuse with an explicit
hdrstop pragma filename and postboundary namedpacking reuse. Two frontend
bodies add218bytes, plus261bytes of identified original LIBC support.
Actual native producer PCH/browser/IR contract precedes frozen backend-object
input to consumer. No backend is executed by fixtures; all PCH/IR/browser
bytes and immutable dependencies match. Final live-root native rebuild and
48public/15direct units plus rejection/static checks pass.

Damaged dispatch-header diagnostics
-----------------------------------
cases/dispatch-type-diagnostics adds one bounded expected-error /FR /Z7
workflow with incomplete-record arithmetic, pointer-shape conversion and
function-designator operand diagnostics. Three frontend functions add147B.
Exact partial IR/browser files, four errors/three warnings and recovered_tail
finalization match. Current full-suite and ABI/data rejection receipts, plus
read-only inventory of older CRT overrides, are bound in validation.json.

Prototype-scope packet registry
-------------------------------
cases/prototype-scope-registry adds successful prototype-scope tag binding,
typed callback/factory and pointer-to-array API checksum with exact C4115.
The enclosing-scope helper adds41frontend bytes; owned globals/chain/returned
object and RET state are independently proved. Final50public/15direct and
rejection/static receipts are in that case's validation.json.

Generated bulk registry browser limit
-------------------------------------
cases/browser-index-limit uses66000 generated public external declarations
to cross the real shared browser-index limit. Exact once C4041, completeIR
and73B partialSBR agree; original checksum runtime succeeds. One38B frontend
body and live FILE/global ownership are proved. Source/count/order/hash,
full staged-browser bytes, timings and storage are retained in that case.

Malformed callback-body semantic recovery
-----------------------------------------
cases/callback-body-recovery adds two absent parameter-list definitions,
exact expected C2054s/exit2 and recovered_tail finalization. One69B frontend
body clears real context globals and updates scanner upperword masks;
actual two-call inputs/return/wholeword observations match. Largest residual
application/support regions and honest padding candidates are also retained.

Direct reusable expression templates
------------------------------------
cases/expression-templates tests the 12-byte reusable-zero constructor and the
493-byte recursive tree copier through a separate external unit harness. The
real minimal compiler context supplies interned types and arena ownership;
owned integer/unary/binary trees use mode6, string and extended payloads use
mode2. Six units compare two raw original baselines, exact field definedness,
parent links, payload/tail bytes, allocator transfer, and individual return ABIs.
Their cached native runner needs neither Wine nor GDB. Public extent is unchanged.
Mode6 payloads would index beyond the six pools and remain explicitly held;
other unproved allocator contexts are not covered. Run independently with:

  make -j4 native-expression-tests
  python3 scripts/check-expression-tests.py
  python3 scripts/check-expression-rejections.py

The first string trial exposed a missing stdcall callee prototype. The retained
failure and repaired stack/return proof document the fix; pch.h now supplies the
existing copy_bytes_end declaration. No CRT function was reconstructed.

Public malformed database cleanup
---------------------------------
cases/legacy-pdb-object freezes a public malformed-magic msvc.pdb input. The
original and reconstruction emit C2470, exit1 (CLexit2), and leave five identical
partial IR files. Its 27-byte list destructor adds one frontend function. Fresh
entry and twice original/native return observations prove the actual embedded
list, its one live12-byte buffer, exact free ownership/order, unchanged owner
fields and ECX-this/RET0/nonvolatile ABI. Unwritten constructor fields remain raw;
freed block-list pointers are never followed. The cached fixture uses no Wine or
debugger. scripts/check-pdb-cleanup.py checks frozen observations and nine damaged
ownership/ABI controls.

Direct database object methods
------------------------------
The same case has six external units for the353-byte object-open and52-byte
reload methods: existing writable/readonly files, creation, readonly absence,
reload success and a truncated32-byte header. Each starts at the actual natural
constructor with its116/12/12-byte allocations; no target globals or unwritten
fields are initialized by the driver. Two original baselines and fresh native
entry/return proofs bind actual formals, linked record/type-index owners,
store-defined fields, own padding/tails, descriptor positions and error states.
Complete files/streams match raw bytes; the fixed signature is a direct input.
There is no established public trigger for these methods. Both preserve their
own x87 control word; original027f/native037f are recorded, not declared equal.
These methods perform integer/file operations and no x87 arithmetic.

Run cached units with make -j4 native-pdb-tests followed by
python3 scripts/check-pdb-objects.py. They need neither Wine nor GDB and are part
of the normal full check. Explicit debug-pdb-unit.py is acceptance tooling only.
Twelve damaged observation/cache controls and one real unknown-SIGTRAP control
are retained. The first zero-argument driver incorrectly filled three canary
slots with argument placeholders; the failed run is preserved, and only the
affected reload baselines were repeated after fixing the external frame builder.
The earlier coverage origin label placed1824 bytes of old __strgtold12 in its
frontend bucket; current accounting correctly classifies it as CRT. No CRT body
was added or changed, and historical receipts remain unchanged.

Standalone QuickC region and loop workflow
-----------------------------------------
cases/quickc-regions is a direct C1 frontend-interface case with genuine parsed
-qc/-Ol/-Oe, nested loops, branches, inline functions, tracked short parameters,
aggregate initialization and floating-point expressions. It makes no CL producer
or C2 dialect compatibility claim. Two original runs pin each source revision;
v1 remains unchanged and v2 adds an actually used short-parameter helper.
Four observed option/normalization words are all1. No target globals are seeded.

The case adds22 application functions/4520 bytes: query ownership, parameter
tracking, nested region selection/propagation, compact loop operands/back edges,
and inline memory initialization. Complete original/native IR, status, streams
and fresh22-function first-entry order match. Total accepted application count
is893; total code-extent coverage is245552/277234 bytes(88.57%), including CRT and
imports. Application-only accepted extents are222814 bytes; no CRT body changed.

The first loop-exit continuation caught an incorrect descriptor arena assignment:
the original uses pool3 with loop lifetime, not recycled expression pool2. Failed
raw observations and the corrected continuation remain in the case evidence.
The builtin helper has a separate original undefined default for operations other
than172/173: two local stack words are consumed uninitialized. Win32 source keeps
that bug; the exact native path exits125. A small compiled guard and genuine
QuickC control check it without inventing values or claiming public reachability.

Run the ordinary native fixture with make -j4 check CASE=quickc-regions. All
fixtures use cached references; explicit capture/debug scripts remain acceptance
analysis tools. No private prefix, original child execution, or Wine is involved
in fixture runs. See the case validation receipt for elapsed accounting.

Legacy declarations and counted literals
-----------------------------------------
cases/legacy-segment-literals has a valid named-base record/count-string/callback
program and an explicit scalar-base diagnostic variant. Parsed -m/-Zf/-Jd reach
six functions/414 bytes, with exact status0/1, complete streams/files and fresh
entry order. No CRT/shim work or parser-global seeding is involved. The original
rejected segment/extended-identifier trial and an initially wrong expected0 trial
remain frozen. No public producer for dormant segment-token handlers is claimed.

Direct legacy declaration support (2026-10-23)
--------------------------------------------
One real parsed source context supplies named base_pointer/object_base owners
at finish_declarations. The external-unit driver runs three whole segment
modifier helpers (366 bytes), proving actual symbol lookup/name arguments,
reference-count deltas, based-symbol identity, AX16 results, RET8/8/4 and
nonvolatile/stack-canary preservation. Complete files, streams and pending IR
match the originals. No parser globals or token stream are manufactured.
Original first-entry traps and the ordinary shared native instrumentation agree
with each accepted prefix. scripts/check-support.py reuses the existing GDB
owned-stack driver and cached original receipts; it never runs Wine.
The shipped keyword table still provides no identified producer for these
legacy segment grammar actions. Direct support is not public reachability.
See cases/legacy-type-support/manifest.json and validation.json. No CRT code
was reconstructed or changed.

The same direct workflow also covers initializer address sizing, a typed
symbol-leaf constructor, function-designator decay, type-attribute removal and
strict suffix matching (five functions/373 bytes). Method inputs are actual
parsed object/function types, symbol pointers and interned name/substrings.
Only initialized returned-node fields and their actual owner links are read;
the temporary description depth returns to its real prior value. Eight direct
support functions total739 bytes. Segment-stage validation.json remains pinned;
the later checkpoint is type-validation.json. Public reachability is not claimed.

IDE progress with -Yd debug/browser PCH (2026-10-24)
--------------------------------------------------
The standalone C1 -ide/-Yd/-Z7/-FR workflow creates and reuses a guarded
header PCH with nested macro headers, records, enums and callback functions.
The native consumer must use the actual native producer's debug.pch, equal
to the frozen original bytes; no producer.obj or backend execution is needed.
Two untouched original runs per phase agree on complete PCH/IR/browser files
and raw IDE progress streams. Six functions/286 bytes are newly covered:
report_ide_source, ide_source_name, print_ide_status, report_ide_file_end,
report_ide_line and restore_pch_separate_debug. The source-name buffer and
variadic progress writer were previously unlisted application callees, not CRT.
IDE progress retains the original signed sixteen-line group and initial -1.
Fresh original and ordinary shared native entry traces agree for both phases.
Twelve named application stubs remain; unclassified bytes are not counted CRT.
See cases/ide-debug-pch/manifest.json and validation.json.
