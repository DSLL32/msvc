C13232.EXE reconstruction (project vc1.0, toolchain 1.0-nt)

Current status
--------------
The first three captured cases are complete. The assigned fourth case,
numeric-varargs (/Zi /Od /G5 /W3), is in progress: 510 reconstructed source
functions and 609 accepted trap frontiers. All three earlier raw-output
regressions pass. Every new numeric frontier replays with exact pending
buffers, visible files and stdout/stderr except the two proven database
timestamp fields. The current paired trap is pdb_argument_list_index at original 0x49760.

The minimal-C case originally required 179 source functions and 258
frontiers. All 258 frontiers replay successfully against the final build,
with exact pending-buffer and visible-file comparison. Both executables
exit 0 with empty stdout/stderr and produce the captured raw intermediates:
irex 866, irsy 13, irin 3, and irgl 50 bytes.

The optimized Win32 hello case uses the captured CL /O2 /G5 frontend
invocation and original SDK headers. All 251 additional frontiers replay
successfully, then both executables exit 0 with empty stdout/stderr. The
four intermediate files match the capture byte for byte: irex 2066, irsy 422,
irin 46, and irgl 629 bytes. The unmodified original independently reproduces
all three captures. No fixture intermediate bytes are linked into the compiler.

The DLL case uses the captured CL /O2 /G5 /LD /Fehello.dll hello-dll.c
frontend invocation, including -D_MT. Its one additional frontier reconstructs
the complete __declspec attribute parser. Both executables exit 0 with empty
stdout/stderr and produce the captured raw intermediates: irex 2129, irsy 499,
irin 49, and irgl 669 bytes. The new frontier and all 509 earlier frontiers
replay successfully against this build on their recorded cases.

Unvisited functions remain individually named trapping stubs for subsequent
cases. The accepted functions retain their complete control flow and calls
to those entries.

Build and differential workflow
-------------------------------
  make -j2
  make check
  make replay
  make check CASE=hello-win32
  make replay CASE=hello-win32
  make check CASE=hello-dll
  make replay CASE=hello-dll

Builds use the original CL.EXE and LINK.EXE through Wine, with a private
prefix at ~/.ai-tmp/vc1.0-c1/wine. The original toolchain is read-only.
GNU Make can compile independent source files in parallel.

compare.py masks every original and reconstructed code section with INT3,
then restores only the functions in functions.json's accepted prefix. It
launches the two executables concurrently and requires the same logical
trap. check verifies the current frontier and audits the reconstructed
call graph. replay repeats every recorded frontier before restoring the
current accepted set. An unfinished matched trap is reported explicitly.
Each matching frontier also requires identical visible working-directory
files and raw file contents. The debugger reads each intermediate FILE's
pending _base.._ptr bytes with ReadProcessMemory and compares them exactly,
without flushing or modifying the compiler. This also detects divergence
before buffered output reaches its visible file.

The case comes from ../fixtures/minimal-c/manifest.json, captured at the
original CL CreateProcessA boundary. Each executable receives only the
MSC_CMD_FLAGS environment entry, no command-line arguments, and an isolated
working directory containing test.c. The portable temporary prefix is ir.
The debugger itself inherits the build environment; the debugged compiler
gets an explicit, double-NUL-terminated single-entry environment block.

When the case completes, compare.py checks exit 0, empty stdout/stderr,
and raw byte equality of irex, irsy, irin, and irgl against the shared
capture. It also repeats the case with the unmodified original binary.
No intermediate-file bytes are linked into the reconstructed compiler.

Source organization
-------------------
c1.c: process entry and structured exception wrapper.
options.c / option-data.c / options.h: frontend option handling and its
  typed grammar, defaults, warning settings, and option state.
arguments.c: MSC_CMD_FLAGS tokenization and argv extension.
characters.c / characters.h: lexical character classes and code-page
  lead-byte initialization.
checksums.c: checksum transform table initialization.
packing.c: structure-packing stack.
memory.c / vectors.c: original permanent and temporary allocation schemes.
diagnostics.c: typed diagnostic vocabulary lookup table.
frontend.c / initialization.c: translation-unit driver, startup state,
  intermediate stream creation, and builtin type initialization.
preprocessor-directives.c: directive classification/dispatch, conditional
  state, skipped source, source macro definitions/replacement encoding, and include filename/search
  handling.
pch.c: precompiled-header include-event playback and ownership state.
pp-expression.c: separate preprocessor expression parser, defined operator, integer
  evaluation, short-circuit state, and evaluator arena constants.
preprocessor.c / keywords.c: identifier/keyword interning and vocabulary.
lexer.c / token-data.c / tokens.h: token pushback, lexical source dispatch,
  line beginnings, and named token categories.
types.c / parameters.c: type and shape interning, function types,
  descriptor stack, and parameter-list construction.
parser.c / parser.h: LR control flow, grammar semantic actions, declarator
  construction, and parameter-scope setup.
declarations.c: symbol declaration, storage validation, type-shape folding,
  qualifier propagation, redeclaration lookup, tag definition and enum constants.
hash-table.c / hash-table.h: generic arena-backed maps, insertion, collision
  lookup, and pointer-identity hashing/equality.
parser-grammar.c: logical token shifts, reduction exceptions, production
  lengths, and nonterminal gotos, decoded from the encountered yacc tables.
  The original compressed action/check layout is not reproduced.
scopes.c / builtins.c / builtin-data.c: active/completed symbol scopes,
  scope cleanup and serialization, sentinel objects,
  and typed intrinsic definitions.
constants.c / ir.c / ir-formats.c: constant nodes, recursive expression emission,
  and schema-driven IR writer.
conversions.c: pointer, qualifier, function-prototype, and base-type compatibility
  checks, explicit casts and constant payload conversion.
expressions.c / calls.c: semantic expression construction, member selection,
  argument conversion and function-call context.
initializers.c / initializer-records.c: scalar and braced initialization,
  aggregate traversal, runtime zero fill and static initializer records.
literal-names.c: pooled literal names and string-literal serialization.
expression-records.c: named token-to-record rules, sharing declaration meanings
  with token-data.c rather than duplicating the original combined vocabulary.
control-ir.c / control-formats.c: source and option control-record schemas,
  compact state encoding, deferred pragmas, and warning override lists.
intermediate.c: intermediate stream opening and expression-header reservation.
runtime.c: encountered old CRT variants whose shipped LIBC implementations
  differ, including binary-mode open, stream seek/buffering, timezone
  initialization, and the original US daylight-saving rules.
floating.c: startup floating-point control.
macros.c: predefined names, timestamp formatting, macro interning, -D, and
  whitespace preservation.
macro-expand.c: predefined/object/function macro dispatch, typed rescan frames,
  saved physical-input alias, replacement entry, and deferred frame restoration.
source-input.c: file opening and overlapping buffered include windows.
global-ir.c / global-formats.c: global/symbol IR schema serialization.
pragma.c / pragma.h: complete pragma dispatcher, pragma-name lookup, warning
  controls, and listing/segment state.
option-state.c: optimization-state bit packing and pragma optimization rules.
The expression writer and function serializer share the original expression
count and call/inline state and the shared FrontendSymbol layout; interned
identifier globals are shared with
predefined-macro initialization.
stubs.c: named, trapping entries for functions not yet reconstructed.

Data are defined in the owning subsystem as they are encountered. There
are no copied image sections, address-indexed data snapshots, embedded
original executable bodies, or runtime loads of original image data.
Original addresses in comments and functions.json are comparison evidence.

identify-runtime.py matches the shipped LIBC.LIB machine code and validates
REL32 destinations against imports and other matched symbols. It can
match individual public functions when a neighboring function in the same
library section differs. Recognition alone does not accept a function:
compare.py advances the accepted list only after its matching trap.

audit-graph.py compares direct call and tail-jump targets and the presence of indirect
calls for every accepted source function. It identifies switch tables so
their address entries are not mistaken for instructions or call edges.

audit-state.py checks the initial bytes and address aliases of 168 encountered
frontend state objects, 72 scalar options, the 21 option-table targets, 17 pragma optimization
rules, and five hash callback identities, including PCH ownership, function-directory state,
parser defaults, and source/include state. These checks read the binaries
for verification only; reconstructed code does not load original image data.

The preprocessing FILE pointer and /Zg declaration-output pointer are the
same field at byte 128 of the 132-byte lexer workspace, represented by
LexerTextStorage. The PCH event writer also shares the permanent allocator's
page count; no second counter is defined for that use.

MacroFrame is a 20-byte typed record with 256 entries, matching the encountered
nesting bound. Include eviction's saved input cursor aliases frame[1]'s
source cursor. The explicit format_text source entry overrides an ambiguous
library-pattern match for its small variadic wrapper; its actual callee is a
compiler string formatter, separate from the shipped CRT scanning routines.

The pragma dispatcher shares the default code-segment symbol with function
declaration setup. PLMN/PLMF flags and their active type-attribute state have
one owner each; stack-check and native-caller normalized defaults start at 1,
as in the original, before command-line startup overwrites them.

The C grammar includes all 227 production records. The optional trailing-comma
productions 225/226 were absent in the initial logical table; the first SDK enum
exposed that omission. audit-grammar.py independently checks every state's
terminal actions and lookahead timing, every production referenced by those
actions, and every used nonterminal goto: 57,904 actions, 227 productions and
34,216 gotos. The clean -j2 build and full 64-frontier hello replay pass, followed
by exact minimal-C raw-output regression.

EnumeratorSymbol represents the original 28-byte enum-constant variant rather
than allocating the 44-byte general symbol record. The increment helper's ABI
is (value,name), verified from both its reached body and its parser caller.
The debugger records registers, callers and frame words at traps to help locate
future divergences without changing the debugged program.

Checkpoint 261 adds the full source macro-replacement scanner, including formal
markers, stringizing, token pasting and Microsoft #@ characterizing. Declaration
handling now covers tag references/forward declarations, ordinary identifier
lookup, scope buckets, storage validation, declarator prefixes, abstract
cast declarators, circular parameter lists, parameter-type hashing and the
canonical explicit (void) descriptor. The original segment setter is shared by
code and data declarations; cleanup registration shares scope_cleanup_stack.
All 83 hello frontiers replay with exact pending/visible intermediate bytes,
and the minimal-C four-file regression remains raw-byte identical.

Checkpoint 272 completes compatible redeclaration checking and commitment:
base/shape comparisons, default integer promotions, nested prototype merging,
storage-class compatibility and inheritable storage modifiers. The prototype
merger retains the original workspace-to-existing-descriptor copy direction
in its nested replacement path. The declaration flags' preceding byte aliases
the high byte of active_parameter_context; a shared union represents both
views. The original /Zg subrule establishes the option name prototypes_only,
replacing the earlier provisional syntax_only name. Scalar option defaults now
participate in the same bidirectional byte-address alias audit as parser state.
All 94 hello frontiers replay with exact buffered/visible file bytes, followed
by raw-byte exact minimal-C regression.

Checkpoint 285 adds formal-list encoding, source macro undefinition, expanded
identifier interning and inhibition markers, array-parameter adjustment,
trigraph recovery, physical source-boundary checks, quoted macro text, line
comments, warning dispatch/settings and mutable type descriptions. Newly
reached warning paths establish that diagnostics, parser recovery, IR error
suppression and process exit all share the one error_count at original 0x5b228;
the earlier provisional current_function and separate driver counter are now
one object. All 108 hello frontiers replay with exact pending/visible bytes;
minimal-C still exits0 with all four captured raw intermediates identical.

Checkpoint 298 reaches structure/union member lists. Base qualifier propagation,
member declaration/name checking, effective qualifiers, data-member placement,
packing/alignment, explicit padding members, target-width sum overflow and
member-range insertion are reconstructed as distinct original graph entries.
AggregateContext now names its tail, byte size, alignment and pending bitfield
state. A symbol's associated union represents its encountered member-list,
enum-name-table and deferred-initializer variants at the original shared offset.
All 121 hello frontiers replay with exact pending/visible file bytes, and the
minimal-C regression retains identical raw files.

Checkpoint 310 adds aggregate completion/anonymous member copying, pointer and
nested-array sizes, physical line continuation, continued identifier interning,
pragma-list validation and callback dispatch, and #pragma function's builtin
lookup/external conversion. The reached constant-expression helper at 0x2c8f0
returns a four-byte integer value, rather than a token pointer; its name, return
type and grammar union view now express that established meaning. The shared
browser_filename pointer is included in the address/default audit. All 133
hello frontiers replay with exact buffered/visible bytes; minimal-C remains
raw-byte identical.

The intrinsic-name audit also checks all 51 lookup suffixes and the 29 initial-
character boundaries against the original reader's separate table view. The
reconstruction reuses its typed builtin vocabulary rather than defining a
second copy of those spellings.

Checkpoint 320 adds full bitfield packing, #pragma pack parsing/validation,
function-like macro actual collection and storage, per-argument rescan input,
raw/expanded substitution selection, identifier adjacency, and replacement
redefinition comparison. Symbol metadata now gives bitfield offset/width a
typed view of the original shared word. MacroArgument represents the packed,
span-linked argument stream, with zero/'A' continuation markers. All 143 hello
frontiers replay with exact pending/visible bytes; minimal-C still matches the
four captured raw intermediates.

Checkpoint 324 adds string-literal scanning, workspace bounds and wide-character
conversion, plus quoted-include lookup through the source directory chain.
StringToken gives the scanner's pointer/length/flags payload a typed view.
The expanded default-state audit establishes that text_work_cursor initially
points to expression_storage; the reconstruction now preserves that initialized
relationship before parser startup. All 147 hello frontiers replay with exact
buffered/visible bytes, and minimal-C retains its four raw captured files.

Checkpoint 341 adds the whole binary-expression constructor, integer promotion,
record-operand validation, common arithmetic type selection and integer/short-
circuit constant folding. Pointer arithmetic retains byte scaling and the
original graph's conversion/floating helpers as named traps until reached.
Parameter stack sizing, local extern/static declaration processing, identifier
values and dereferences, switch setup and its shared work-list allocator now
match. The former provisional declaration-finalizer stub is established as
based-expression normalization, and its name and node type are corrected.
Label/parameter state and the encountered integer/extended-zero defaults join
the address/default audit. All 164 hello frontiers replay with exact buffered
and visible bytes; minimal-C still produces the four captured raw files.

Checkpoint 351 adds sorted signed/unsigned case chains, case-label sharing,
integer-width conversion, named function designator resolution and the full
prototype-driven argument checker. ArgumentNode preserves both the circular
source-order link and reverse parser chain; CallContext is a 36-byte nested
prototype/diagnostic frame. The old sizeof/alignof pre-action names are now
corrected to prepare_named_function_call(char *) and prepare_call_expression.
The case-limit array has five entries; its following word is the separately
owned function_scope_level, as the byte-alias audit establishes. All 174 hello
frontiers replay with exact pending/visible bytes, and minimal-C remains
raw-byte identical. Current frontier is address-taking for BeginPaint's paint
argument, in the whole unary-expression constructor.

Checkpoint 357 reconstructs whole unary expressions, struct/union operand
permissions, modifiable lvalues and call completion. Call completion checks
prototype/intrinsic arity, computes actual and declared argument stack sizes,
and restores the enclosing call context. The parser intentionally discards
its return value. Address-taking now owns typed /qc inline-region and scope
records at their encountered consumers; no data sections were imported.
The -Za/-Ze option field is named extensions_enabled after checking the
original option actions. All 180 hello frontiers replay with exact pending
and visible IR bytes, the 143-state/default/alias audit passes, and minimal-C
still exits successfully with all four raw fixture files identical.
Current matching frontier is binary operand conversion, original 0x3ddf0.

Checkpoint 372 adds binary operand/category checks, cv conversion checks,
expression-statement output, char/wchar string types, pooled literal naming
and literal symbol creation. Encoded names combine length, the generated
two-byte checksum, and a bounded escaped prefix. Shared formatting workspace
and the existing auxiliary IR output stream remain single objects across
these consumers; the 148-state/default/alias audit covers them. The dedicated
literal-names module owns the readable number/character encoding logic.
Literal initializer headers and compressed integer output now reach the
general initializer-value writer at 0x3b3a0. All 195 hello frontiers replay
with exact pending/visible bytes; minimal-C retains all four raw fixture files.

Checkpoint 383 reconstructs the complete general initializer-value writer,
including constant/relocation classification and integer, floating and string
record branches. Counted text and compressed short output are accepted, as
are element sizing and switch completion. HelloWindowProc is traversed and
the case reaches WinMain's WNDCLASS aggregate initializer. Declaration setup
and aggregate frames now preserve the original mutable symbol/type copies,
all twenty variant copy sizes, the 32-level frame layout and initial depth -1.
The 159-state/default/alias audit verifies that data. All 206 hello frontiers
replay with exact pending/visible IR bytes; minimal-C remains byte-identical.
Current matching frontier is aggregate value emission at 0x301a0.

Checkpoint 395 reconstructs aggregate target selection and descent, runtime
and static initializer checks, address normalization, lvalue construction,
and assignment emission. WinMain now emits the first WNDCLASS member
assignment and reaches aggregate traversal advancement at 0x30970. The
32-frame initializer stack shares four preceding state words with its
negative-index sentinel; a typed union preserves these aliases. All 218
hello frontiers replay with exact pending and visible intermediate bytes;
the minimal-C regression retains all four captured raw files.

Checkpoint 407 completes WNDCLASS aggregate zero initialization. Traversal
advances array/member positions, counts remaining storage, closes explicit
and implicit braces, emits runtime zero fill and rewrites intrinsic string
operations where applicable. Finalization preserves incomplete-array type
updates and the original cleanup entries. Three more contextual state
objects are audited. All 230 hello frontiers and the minimal-C raw-output
regression pass. The next function at 0x29970 selects a member with . or ->;
its provisional function-call name and argument-list prototype are corrected
to select_member_expression(operation, operand, name).

Checkpoint 419 reconstructs member selection, member address/model handling,
and the explicit-cast pipeline: declarator cleanup, cast validation, scalar
and pointer classification, null-pointer conversion and constant payload
conversion. Float branches retain named conversion entries and the existing
normalized /Op option. Type-name cast semantic values now use their actual
FrontendSymbol pointer. All 242 hello frontiers replay with exact pending
and visible bytes; minimal-C retains all four captured raw files. The current
matching trap is the void-pointer classifier at 0x40cd0.

Checkpoint 428 completes the optimized Win32 hello case. The final entries
handle void-pointer classification, array decay, short-circuit branches,
long-literal classification, loop setup/cleanup, representation casts and
symbol-name/string output. The optional quick-compiler loop stacks are
contextual typed objects, sharing the existing normalized_Oq option.
A clean make -j2 check CASE=hello-win32, all 251 hello frontiers and all 258
minimal-C frontiers pass. The 428 function call graphs, 168 initial-state and
alias checks, option/pragma identities, and logical grammar audit pass.
Both final runs and independent unmodified-original runs produce identical
raw captures with successful exit and empty stdout/stderr.

Checkpoint 429 completes the DLL frontend case. parse_declspec retains its
cdecl storage-vector ABI, identifier-mode flag, all attribute dispatch and
error paths. It consumes the existing typed keyword/attribute vocabulary;
no new implementation data are introduced. The data audit now independently
checks all six attribute-table rows, including semantic names, integer fields
and the sentinel. The immutable shared fixture and its source are unchanged.
A clean original-toolchain make -j2 check CASE=hello-dll, the new DLL frontier,
all 258 minimal-C frontiers and all 251 hello-win32 frontiers pass, including
successful exits and raw capture comparison against the unmodified original.
All 429 function call graphs, 168 state defaults/aliases, option and callback
identities, six declspec rows and the logical grammar audit pass.

Checkpoint 453 adds native /Zi database startup, allocation and loading,
CodeView record schemas and LF_TYPESERVER output, decimal floating scanning,
the complete original CRT __strgtold12 state machine and extended-value
storage. PDB creation uses the typed 64-byte header, twelve-byte allocation
nodes and 116-byte database context; no opaque binary data are imported.
The private ECX-method ABI is expressed with fastcall and an unused EDX
formal when stack operands follow. Ten-byte x87 transfers and the original
ESI/ST(0) store tail have documented narrow assembly exceptions because this
compiler's C scalar types cannot express them; decisions remain C.

The immutable numeric fixture now includes irdb and msvc.pdb. Independent
original replay uses a private baseline prefix and canonical N:\numeric-varargs
cwd; original/reconstructed labels have separate prefixes with the same DOS
path. No path bytes are normalized. The timestamp signature at PDB+48 is
written from the begin_program_database time(0) argument by
create_program_database, read by program_database_signature, and copied to
the LF_TYPESERVER payload; the debug stream encoder places it at irdb+14.
Each run's two signatures must agree before those four-byte fields are
normalized. All other auxiliary bytes and all four main IR files stay exact.
Independent original replay confirms the exact six-file set, main IR hashes,
51-byte irdb, 574-byte PDB and the expected C4069 CRLF warning. Auxiliary
reconstruction completion is still pending later debug/type writers.

At this checkpoint all 36 additional numeric frontiers replay, all 453
call graphs match, 177 state defaults/aliases and 13 debug schemas pass,
and all logical grammar checks pass. The three completed cases exit 0 and
match their captures and independent untouched originals again. Full old-case
frontier replay and the final clean build remain required at case completion.

Checkpoint 477 adds generic work-stack read/seek and initializer byte/block
output, scalar/string completion, incomplete-array inference, double
conversion, the external diagnostic catalog, and the compiler's complete
small formatter. Both processes now emit the exact captured C4069 warning
at line 24, including CRLF, then continue into the variadic macros.

The newly reached buffer flush proved that startup/source compiler_stack and
initializer_buffer both name original pointer 0x5f4f4. They now share one
object and the existing sixteen-byte WorkingStack layout, with contextual
2048-byte linked WorkingBlock records. Their terminal used-byte field is
shared by the writer, reader and finalizer. The original reset method is
retained rather than introducing a second reset/release function.

Masked-original single-step analysis identified a prior extent that ended
inside initialize_work_stack's final RET 4. Masking changed its operand to
0xCCCC on the existing-buffer branch. Its extent now includes all three RET
bytes. A new graph-audit instruction-boundary lookahead found and corrected
two equivalent unvisited RET operand truncations in intern_keyword and
skip_inactive_literal. All 477 original and reconstructed source extents
pass that guard. No code from those previously reconstructed functions was
replaced or skipped.

All 66 additional numeric frontiers replay after these fixes. Stdout/stderr
are now compared at every matched trap as well as at successful exit.
The three earlier cases independently reproduce their captures again.
All 477 call graphs, 184 state defaults/aliases and the unchanged table and
grammar audits pass. The current accepted original code-extent union is
168095/277234 bytes (60.6329%), including shared text data, CRT and import
thunks, clipped to code-section VirtualSize. This is not branch coverage.
A clean build and full older-case frontier replay remain required before
the fourth case is accepted as complete.

Checkpoint 487 adds pointer qualification/model classification, both sizeof
constructors, complete-type sizing, signed subtraction and signed/unsigned
multiplication range checks, dereference type propagation, declaration debug
type preparation, and the whole legacy/PDB type-emission dispatcher.
The special-name audit now checks all fourteen original destination cells and
spellings; an old audit entry incorrectly assigned name_unnamed to the
alignment-member cell, although the reconstructed source already used distinct
correct identifiers. Both cells and their table references now have independent
alias checks. All eighteen width/range table values are checked semantically
against the three original parallel arrays.

All 76 numeric frontiers since the prior completed-case boundary replay with
matching logical traps, bytes and diagnostics. The 487 source call graphs,
instruction boundaries, 198 state defaults/aliases, option/pragma identities,
logical grammar and contextual table audits pass. Accepted original code-extent
union is 169602/277234 bytes (61.1765%, not branch coverage). The fourth case
continues into native PDB type record construction; final output acceptance,
clean build and full older-case replay remain pending.

Checkpoint 510 adds native PDB scalar/pointer classification, nested type
resolution, array and function records, numeric leaves, record padding,
hash interning and list allocation. The whole legacy deferred flush is also
reconstructed, preserving its /Zi early exit and every legacy call target.
Character literals retain escaped-byte ordering, wide forms and DBCS tails;
enum expressions and explicit nested initializer braces now execute too.
Extended subtraction and its shared precision/range tail retain ten-byte
values through documented narrow x87 transfers and arithmetic, since the
original compiler cannot express that scalar type or ESI/ST(0) ABI in C.

All 23 frontiers added since checkpoint 487 replay with matching bytes and
diagnostics; checkpoint 487 had replayed the preceding 76 numeric frontiers.
The current build passes all 510 source call graphs and instruction-boundary
checks, 201 state/default/alias checks, 58 primitive debug codes and the
remaining data, option and grammar audits. Accepted extent union is
173101/277234 bytes (62.4386%, not branch coverage). New PDB records are now
interned in memory; final database writing, case acceptance and full older
frontier regressions remain pending.
