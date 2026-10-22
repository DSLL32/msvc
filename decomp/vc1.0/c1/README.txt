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
hello.c frontend invocation. There are now 200 reconstructed source functions
and 280 accepted frontiers. The case opens the original SDK's windows.h,
reads its include guard and empty macro definition, skips its inactive
RC_INVOKED block, and enters the #if expression parser. Its current matching
trap is parse_pp_bitwise_or, original VA 0x18810. All 22 new frontiers and all 258
minimal-C frontiers replay successfully with exact pending-buffer/visible-file
checks. Minimal-C still completes with the captured raw files.

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
  state, skipped source, source macro definitions, and include filename/search
  handling.
pch.c: precompiled-header include-event playback and ownership state.
pp-expression.c: separate preprocessor expression parser and evaluation state.
preprocessor.c / keywords.c: identifier/keyword interning and vocabulary.
lexer.c / token-data.c / tokens.h: token pushback, lexical source dispatch,
  line beginnings, and named token categories.
types.c / parameters.c: type and shape interning, function types,
  descriptor stack, and parameter-list construction.
parser.c / parser.h: LR control flow, grammar semantic actions, declarator
  construction, and parameter-scope setup.
declarations.c: symbol declaration, storage validation, type-shape folding,
  qualifier propagation, and redeclaration lookup.
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
macros.c: predefined names, timestamp formatting, macro interning, and -D.
source-input.c: file opening and overlapping buffered include windows.
global-ir.c / global-formats.c: global/symbol IR schema serialization.
option-state.c: optimization-state bit packing.
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

audit-state.py checks the initial bytes and address aliases of 39 encountered
frontend state objects, including PCH ownership, function-directory state,
parser defaults, and source/include state. These checks read the binaries
for verification only; reconstructed code does not load original image data.

The preprocessing FILE pointer and /Zg declaration-output pointer are the
same field at byte 128 of the 132-byte lexer workspace, represented by
LexerTextStorage. The PCH event writer also shares the permanent allocator's
page count; no second counter is defined for that use.
