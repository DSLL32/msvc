C13232.EXE reconstruction (project vc1.0, toolchain 1.0-nt)

Current status
--------------
Ten cases are complete: minimal C,
optimized Win32 hello, DLL, numeric/varargs, PCH creation and dependent PCH use,
a minimal do-while loop,nested SEH inline assembly and browser output. There are657 reconstructed
functions and763 accepted original frontiers. Browser definitions/references,
scopes, macros and its filtering pass produce exact raw .sbr output;
cases/browser/ retains fresh frontier and final-file evidence. The assembler case includes
operands,immediates and forward labels; see cases/inline-asm/. Unvisited functions retain named trapping stubs.
The do-while trial reconstructs exactly one function, begin_do_statement; its
source, original captures and current exit terminal are durable in cases/do-while/.
cases/do-while/validation.json records the one-function workflow trial timings.

Accepted original code extents cover 205217 of 277234 code-section bytes,
74.023027%. This is a clipped extent union including shared text data, accepted
CRT and import thunks; it is not branch coverage. scripts/coverage.py reports the
source/CRT/import breakdown from the original-toolchain build and metadata.

Native build and fixtures
-------------------------
Run from this directory:

  make -j4
  make -j4 check
  make -j4 replay
  make -j4 check CASE=numeric-varargs
  make -j4 check CASE=pch-use
  make -j4 native-check
  make -j4 native-adoption-check

The default builds an instrumented native i386 executable using Clang -O0 -g.
check and replay use the same native entry-trace and final-output validation.
native-check builds and checks an uninstrumented executable. Native build and
fixture targets do not depend on Wine, an original executable, a debugger, or
reference capture. Shared lib/recon sources are compiled directly at final link;
no archive is built. Target runtime.o and shared shims are not instrumented.

scripts/check-native.py reads immutable original-backed fixture manifests and
checks input/header/message-catalog hashes, status, exact stdout/stderr, the
complete work-file set and artifact bytes. It always executes an i386 ELF. The
original CL invocation's MSC_CMD_FLAGS is preserved, with portable toolchain
paths; compatibility configuration is supplied through separate RECON variables.
The BINMODE.OBJ default is retained with RECON_DEFAULT_FMODE=0x8000.

scripts/native-traces.py validates all 840 native application definitions and a
cached projection of the original accepted first-entry histories. The projection
excludes matched CRT, target CRT overrides and imports on both sides. There are
643 accepted application entries: minimal 172, hello-win32 249, DLL 1, numeric 103,
PCH-create 43, PCH-use 7, do-while 1 SEH 10 and inline assembly33. Browser contributes27 subsequent application entries. Every case seeds the previously accepted application
prefix. The shared seen-state descriptor is supplied even though current C1
cases have no child processes. New or missing application definitions fail before
launch. The cache is not silently refreshed. Explicit --write regeneration reads
acceptance metadata and an existing original-toolchain map; it executes nothing.

store_extended has an existing private ESI/x87 naked ABI. Its native-only trace
marker preserves registers, flags and floating-point state before the first fstp;
the marker is empty for the original Win32 compiler. No target function or ABI
wrapper was added. The native parser's redundant tentative initializer_temporary
is declared extern, sharing its established owner in initializers.c.

Case                  irex  irsy  irin  irgl  Other outputs
minimal                866    13     3    50
hello-win32           2066   422    46   629
hello-dll             2129   499    49   669
numeric-varargs       2330   535   149   402  irdb 51, msvc.pdb 574
pch-create              94    33     3    68  state.pch 38164
pch-use               1071   116    47    87
do-while               888    39     3    50
seh                   1134   114     6    79
inline-asm            1100   107     6    75
browser                971    69     6    79  test.sbr236

All four primary intermediates match raw bytes. Numeric stderr is the captured
line-24 C4069 CRLF warning; other streams are empty and all cases exit 0. Only the
independently identified timestamp DWORDs at irdb+14 and msvc.pdb+48 are ignored,
after raw cross-output equality is enforced. The absolute path remains exactly
N:\numeric-varargs\msvc.pdb. No path or pointer normalization is performed.

PCH creation and use run at N:\pch in separate private host namespaces. The actual
native producer's PCH must match the immutable reference byte for byte before it
is copied to the consumer. Selecting pch-use automatically runs the producer.
The consumer's PCH input remains unchanged. There are no captured process doubles
or external Windows dependencies in these ten completed frontend cases.

Each run keeps status, streams, trace and artifacts under ~/.ai-tmp/vc1.0-c1/.
Receipts split setup, native execution and output checks; trace-cache preparation
is measured separately. cases/native-all-validation.json records the adoption
build, complete suites, timings, exclusions and rejection evidence. Rejection
checks mutate private copies only: IR bytes, complete file set, entry order,
unmapped definition, timestamp correlation and generated PCH dependency.

Original compiler and audits
----------------------------
These are explicit separate operations, never fixture prerequisites:

  make -j2 audit-win32
  python3 scripts/coverage.py

The canonical Win32 source still builds with the original CL and LINK through
Wine using ~/.ai-tmp/vc1.0-c1/wine. recon.h is empty for that build. Source remains
period-style Win32 C with original calls and ABIs. scripts/wine-tool.py is used
only for this explicit compilation. PE/index helpers in scripts/images.py do
not execute any target.

The call-graph audit checks all 657 accepted source functions, direct/tail calls,
indirect-call presence and complete instruction boundaries. State audits check
268 defaults/aliases, 105 PCH roots, 12 PCH callbacks, the 21-entry shape-cache
boundary, 72 scalar options, 21 option targets, 17 pragma rules, five hash
callbacks, typed builtins and debug schemas. Grammar auditing independently
compares all 376 states, 57904 actions/timings, 227 productions and 34216 gotos.

Explicit original-only reference utilities remain beside fixture captures,
including ../fixtures/pch/capture-original.py and run-phase.c. Normal tests never
call them. The old paired-Wine debugger and replay implementations were removed.
Prior full-state matching evidence and timing accounting are preserved in
cases/historical-validation.json, cases/historical-fast-replay-validation.json
and ../replay-history.json; those are historical receipts, not executable paths.

Source organization and reconstruction invariants
------------------------------------------------
Reconstructed C/H files live in src/, tooling in scripts/, metadata in cases/.
The Makefile stays here and products stay in build/.

options/arguments/option-data handle the original command grammar and defaults;
frontend/initialization own startup and translation-unit control. source-input,
preprocessor-directives, macro-expand, lexer and pragma implement source/header
processing. parser and parser-grammar implement complete accepted LR semantics
using logical grammar tables rather than the original packed representation.
types/parameters/declarations/scopes own symbols, type interning and storage;
expressions/conversions/calls/initializers build semantic trees. global-ir,
control-ir, ir and their contextual formats serialize intermediates. debug-info
and program-database implement encountered /Zi records. pch, pch-files,
pch-reader and pch-relocate implement original serialization and relocation.
runtime.c contains encountered old CRT variants; stubs.c names unreconstructed
entries. Data live with their subsystem and are defined only as encountered.
There are no copied image sections, snapshots, embedded original bodies or
runtime reads of original image data in the reconstructed program.

Important shared storage remains explicit: the preprocessing FILE pointer and
/Zg declaration-output pointer occupy byte 128 of LexerTextStorage; the PCH event
writer shares the permanent allocator's page count. MacroFrame has the observed
20-byte layout and 256-entry nesting bound; include eviction aliases frame[1]'s
source cursor. Declaration flags' preceding byte aliases the high byte of the
active parameter context. The 21-entry function-shape cache and PCH callback
identities are checked against originals. Existing numeric and PCH schemas use
typed records and logical formats, preserving exact serialized bytes.

Inline assembly now has a separate82-rule semantic parser, generated136-state
logical grammar, contextual register/mnemonic dictionaries and instruction IR
encoding. All6936 actions/errors,136 lookahead timings/defaults,83 production
rows and3128 gotos match both the model and linked original-compiler tables.
Dictionary,operand-rule,layout and format audits cover all encountered data.
The fixture completes with exact instruction and label IR records.

Remaining uncovered families include browser/preprocessed output, automatic or
debug PCH, inline assembly, SEH builtin diagnostics, inline expansion, based pointers, legacy CodeView
and error recovery. The active queue continues with inline assembly, browser output and preprocessing.

Browser and preprocessor output fixtures
---------------------------------------
The completed cases/browser and queued cases/preprocessor preserve untouched CL child
invocations, exact environments, dependencies and independent original C1
replays for /FR,/E,/EP and/P. Browser now owns the full8192-byte symbol bitset;
PCH transfers retain their observed8191-byte payload. Source-input/PCH users
share browser_output and browser_pending_flags with the browser writer.
The graph audit rejects external code-section switch tables missing from masks.
