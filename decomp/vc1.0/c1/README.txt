C13232.EXE reconstruction (project vc1.0, toolchain 1.0-nt)

Current status
--------------
The captured minimal-C case is complete: 179 reconstructed source functions
and 258 accepted trap frontiers. Every frontier was replayed in order with
exact pending-buffer and visible-file comparison. The reconstructed frontend
exits 0 with empty stdout/stderr and produces all four captured intermediate
files byte for byte: irex 866, irsy 13, irin 3, and irgl 50 bytes. The unmodified
original independently produces the same files and process result.

The active second case is fixtures/hello-win32, the captured CL /O2 /G5
hello.c frontend invocation. There are now 324 reconstructed source functions
and 405 accepted frontiers. The case opens the original SDK's windows.h,
reads its include guard, evaluates architecture-selection #if expressions,
expands _MSC_VER through the typed macro-frame/rescan machinery, and handles
the SDK warning pragma. The case now handles SDK enum/typedef declarations, function
prototypes and nonempty macro replacements. Its current matching trap is
make_binary_expression, original VA 0x3c750.
All 147 hello frontiers replay successfully with exact pending-buffer/visible-file
checks. The earlier 200-function checkpoint also replayed all 258 minimal-C
frontiers; its current regression still completes with the four raw files.

Build and differential workflow
-------------------------------
  make -j2
  make check
  make replay
  make check CASE=hello-win32
  make replay CASE=hello-win32

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
  checks, with the diagnostic token vocabulary they consume.
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

audit-state.py checks the initial bytes and address aliases of 109 encountered
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
