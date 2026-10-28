# VC5.10 C1.DLL

`binary.c` is the function/export registry for the original
`toolchains/5.10/bin/C1.DLL` (SHA-256
`7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d`).
The reconstruction contains 852 application functions / 226,531 code bytes and
10,057 bytes of owned text data. Application code is 70.29% of the original
322,288-byte text extent; that denominator includes runtime code and text data,
and is not a classified application-only denominator. Ten original CRT/support
owners remain separate, without application credit. MSVCRT.LIB is
an import archive here; shared identification reports zero static CRT matches.

All twenty cached cases complete natively: DLL attach/export lookup/detach, the
minimal C compilation `int main () { return 0; }`, the language workload
below, the declarations workload described next, the types workload below, the scalars workload below, the preprocessing workload below, the declarators workload below, the Win32 extensions workload below, the storage-initialization diagnostic workload below, its positive extended-C counterpart, the pragma-configuration workload below, the header-directives workload below, the preprocessed-output workload below, the assembly-operands workload below, the constant-expressions workload below, the declaration-diagnostics workload below, browser output the callback/union workload and prototype output described below. The minimal invocation forwards the actual
11 arguments to the three-argument stdcall `_InvokeCompilerPass@12` export with
BOOL 1. It exits zero and matches all four original outputs: `minimalex` (1,187
bytes), `minimalgl` (51), `minimalin` (3), and `minimalsy` (13), including normal
compiler finalization, arena release and registered cleanup callbacks.
No clock normalization is required by these outputs.

The language case compiles a C89 header and source using function-like macros,
an enum and structure, initialized record and character arrays, function
parameters and calls, pointer arithmetic, member access, local variables,
for/while loops, switch, continue/break, comparisons and a conditional return.
Its original and native executions exit zero with 541 first entries and match
all four artifacts: `languageex` (1,728 bytes), `languagegl` (172), `languagein`
(84), and `languagesy` (228). The cached native replay took 18.209 ms; the
explicit final reference capture took 1,068.472 ms. The case adds 23 focused
tests. Retained raw artifacts and intermediate failures are in scratch
`language-case-20261026/`, outside the checkout.

The declarations case combines conditional branches, token pasting and
stringification, packed bitfields, record/union initialization, adjacent escaped
narrow strings, wide strings, double constants, do/goto control flow, casts,
member access and `sizeof`. Both executions exit zero with 595 first entries
and exact artifacts: `declarationsex` (1,685 bytes), `declarationsgl` (215),
`declarationsin` (111), and `declarationssy` (205). The final reference capture
took 1,336.283 ms; the matching native run took 21.185 ms. Six focused tests
cover conditional arithmetic, promotions, dirty bitfield/label storage,
adjacent literal ownership and finite extended-precision edges/status. Raw
artifacts and preserved intermediate failures remain in scratch
`declarations-case-20261026/`.

The types case combines explicit cdecl callback typedefs, indirect calls,
multidimensional arrays, pointer differences and casts, linked record types,
static address initializers, aggregate parameters/returns and record copies.
Both executions exit zero with 537 first entries and exact artifacts:
`typesex` (2,073 bytes), `typesgl` (404), `typesin` (134), and `typessy` (512).
The final reference took 1,587.743 ms; the matching native run took 20.877 ms.
Two focused tests cover tag-attribute write extent and deferred literal-buffer
ownership. The latter also verifies that literal output leaves pending ordinary
initialization untouched. Raw artifacts and intermediate failures remain in
scratch `types-case-20261027/`. This case adds 18 application functions and
2,901 code bytes; wider semantic and option domains remain guarded.

The scalars case combines float/double initializers and expressions, explicit
64-bit integer suffixes, signed/unsigned division and remainder, bitwise compound
assignments and shifts, casts, volatile short loads, post-increment/decrement,
logical expressions and function results. Its actual `-ZB64` invocation enables
the target's 64-bit type configuration. Both executions exit zero with 501 first
entries and exact artifacts: `scalarsex` (2,276 bytes), `scalarsgl` (413),
`scalarsin` (124), and `scalarssy` (511). The final reference took 1,665.561 ms;
the matching native run took 21.823 ms. Four focused tests cover finite float
conversion, overlapping input/output, precision-controlled extended subtraction
and division operand order, neighboring bytes and x87 stack depth. The case also
corrects the numeric scanner's `int64`/`int128` remap distinction. Raw artifacts
and intermediate failures remain in scratch `scalars-case-20261027/`.

The preprocessing case combines nested and angle includes, repeated inclusion
with pragma once, command-line definitions, function-like expansion, token
pasting, stringification, conditional expressions, comments, line splicing and
logical source locations. Its `-E` invocation exits zero with 282 first entries,
exact 765-byte stdout, empty stderr and no output files. The final reference
took 1,825.183 ms; its matching native run took 18.496 ms. Two focused tests
cover diagnostic-text allocation/lifetime and bounded include-name trimming.
The case adds 25 application functions and 7,147 code bytes. Current semantic
projections retain explicit guards for unknown punctuation entries, wider
rescan states, recorded line payloads and debug-expression contexts. Raw
artifacts and preserved failures remain in scratch `preprocessor-case-20261027/`.

The declarators case combines nested record/union initializers, arrays of
callbacks, pointers to callback arrays and rows, callback-returning functions,
K&R parameter declarations, aggregate returns, static local objects and a
variadic call. Both executions exit zero with 565 first entries and exact
artifacts: `declaratorsex` (2,615 bytes), `declaratorsgl` (779), `declaratorsin`
(399), and `declaratorssy` (598). The final reference took 1,819.666 ms; its
matching native run took 21.005 ms. Five focused tests cover parameter-list
links, suppressed diagnostics, opaque symbol-copy bytes, declaration-number
encoding and collision-chain lookup. The case adds 19 application functions,
2,126 code bytes and 35 bytes of owned text data. It also recovers the finite
86-string compiler-message catalog using source STRINGTABLE resources and the
existing native resource loader. Raw artifacts and retained failures are in
scratch `declarators-case-20261027/`. At that checkpoint, all eight cached cases and their units
passed in 0.713 s; the complete graph/ABI audit passed in 1.246 s.

The Win32 extensions case combines stdcall/fastcall callback declarations and
calls, exported and thread-local objects, volatile unsigned state, inline MOV/INC
assembly, `__try`/`__except`/`__leave` and nested `__finally` cleanup. Both
executions exit zero with 525 first entries, the same 78-byte warning stream,
and exact artifacts: `extensionsex` (2,502 bytes), `extensionsgl` (412),
`extensionsin` (54), and `extensionssy` (593). The final reference took
2,117.958 ms; the matching native run took 19.599 ms. Six focused tests cover
assembler operand construction, register spelling and encoding, symbol reference
flags, displacement wrapping and record write extent. The case adds 49 App
functions, 14,766 code bytes and 1,484 owned text-data bytes. Its assembler LR
generator emits encountered logical grammar relations; unmodeled opcode,
register, operand and record-capacity domains remain guarded. The diagnostic
path uses a contextual resource string and shared bounded ASCII `_mbstrlen`
support, without CRT body credit. Raw artifacts and retained intermediate
failures remain in scratch `win32-extensions-case-20261027/`. All nine cached
cases and their units passed in 0.235 s; the final
original/native build took 4.031 s and the complete graph/ABI
audit passed in 1.378 s.

The storage-initialization case combines named code/data sections, static string
pointer tables, aggregate/bitfield/array initializers and scalar conversions.
The original rejects its nonconstant aggregate initializers under the selected
C configuration: both executions exit 11 with 551 first entries and the exact
583-byte diagnostic stream. All four artifacts match: `storageex` (1,132 bytes)
and empty `storagegl`, `storagein`, and `storagesy`. The final original took
1,994.363 ms; its matching native run took 20.980 ms. This case adds 19 application
functions and 2,514 code bytes. A focused test checks fresh/recycled segment
symbol writes, and the existing snapshot test covers the new segment size,
cache selector and declaration-context map while retaining unknown-slot sentinels.
All ten cached invocations and units passed in 0.234 s; the full graph/ABI audit
passed in 1.366 s. Raw artifacts and retained failures remain in scratch
`storage-case-20261027/`. The compile rejection is the expected result of this
invocation, not evidence of successful runtime aggregate compilation.

The positive storage counterpart uses explicit `-Ze` mode with the same coherent
source workload. It exercises the actual runtime bitfield temporary, generated
identifier sequence, signed-difference overflow check and intrinsic-call handling.
Both executions exit zero with 552 first entries, empty stdout/stderr and exact
artifacts: `storageex` (2,124 bytes), `storagegl` (226), `storagein` (138), and
`storagesy` (350). The final reference took 2,124.276 ms; the matching native run
took 21.138 ms. This case adds 5 application functions and 1,045 code bytes, with
focused signed-width/unsigned-bypass tests and affected counter/marker snapshot
coverage. All eleven cached invocations and units passed in 0.408 s; the complete
graph/ABI audit passed in 1.377 s. Raw artifacts and retained intermediate failures
remain in scratch `extended-storage-case-20261027/`. The preceding rejection
oracle is preserved separately, with no relabeling or diagnostic suppression.

The pragma case combines compiler messages, warning disable/restore, stack
checks, inline depth/recursion, automatic inlining, optimization controls,
intrinsic off/on selection, linker comments and named CODE/CONST/BSS sections.
Both executions exit zero with 472 first entries and exact outputs: `pragmaex`
(1,344 bytes), `pragmagl` (226), `pragmain` (23), and `pragmasy` (64).
The final reference took 1,941.802 ms and the matching native run 21.020 ms.
It adds 12 application functions and 1,731 code bytes. Focused tests cover
warning-number normalization, sparse option bits, copy alignment/tails and
nonzero restoration of inline controls and optimization masks. All twelve
cached invocations and units passed in 0.429 s; the complete graph/ABI audit
passed in 1.385 s. Retained raw outputs and intermediate failures are in scratch
`pragma-configuration-case-20261027/`. Other intrinsic-name buckets, diagnostic
conversions and pragma domains retain their explicit unsupported boundaries.

The original extended-precision boundary uses ten-byte x87 values that VC5
cannot express with its ordinary `long double`. Tiny documented assembly
primitives transfer the actual implicit x87/register inputs and status; finite
sign/range decisions remain readable C. Noncanonical and nonfinite domains
remain unsupported. Compiler support routines retain original-only CRT
classification, with no native bodies or application credit.

`make -j4 check unit` builds one native i386 `.build/libbinary.so`, discovers all
numbered cases and checks their cached original invocations. The minimal case
also has 60 focused tests. `make -j4 win32 audit` verifies original VC5
compilability and shared graph/import/RET/ownership/extent checks. Runtime
identification and original compiler launching use shared `recon-tool`.
Live finite graph contracts are in `policy-data.json`; small root-level witness
files describe only the qualified original/provider/SDK ABI facts they consume.
Logs, disassembly and intermediate observations stay outside the checkout.

Application source is in `src/`. The historical linked startup implementation
is isolated in `crt/`, with its per-file rationale and no App credit. Native DLL
adapters and focused test glue are in `harness/`. Numbered cases contain only
invocation descriptors, captured inputs/results and tests. The target-specific
parser generator emits readable token, shift/goto and production relations;
it does not embed addresses or copied binary sections.

`make -j4 original-host` builds `harness/win32-host.c`. Shared
`recon-tool original capture --host build/c1-host.exe --progress
--native-msvcrt` owns staging, masking and capture. The typed host forwards its
real argc/argv and uses the actual loader lifecycle. The minimal reference uses
logical cwd `T:\vc5.10\c1\0001-minimal-c`, preserved in an isolated native
workspace. Its finite consumed environment models only absent `MSC_CMD_FLAGS`;
new environment consumers require their actual input evidence. No historical
private HOST vectors or error seeds are used. SYSTEM_INFO and qualified locale
bytes remain explicit profile inputs, not a complete Windows-state claim.

`symbol-aliases.json` records observed code-free linker identities. Each physical
method body has one canonical registry owner. Output-buffer destruction likewise
has one outlined ECX receiver method and eight real callback prefixes, preserving
the original tail/fallthrough graph. Its explicit `Destroy` spelling keeps the
containing BSS a trivial C storage owner.

The header-directives case combines a forced header, quoted/angle include
search, include aliases, repeated once/guarded headers, `defined` conditions,
and logical `#line`/`__FILE__`/`__LINE__` handling. Both executions exit zero
with 565 first entries and exact outputs: `headersex` (1,435 bytes), `headersgl`
(204), `headersin` (45), and `headerssy` (62). The final reference took
2,005.455 ms; the matching native run took 21.111 ms. Two focused tests cover
arena string-copy alignment/tails and newest matching alias links/delimiters;
the existing dirty-state restore test covers the selected forced-header name.
This case adds four application functions, 1,443 code bytes and 56 text-data
bytes. Raw artifacts and intermediate failures remain in scratch
`header-directives-case-20261027/`.

The preprocessed-output case uses `-E -C` with guarded includes, token pasting,
stringification, conditional branches, preserved block/line/macro comments,
pragma output and logical source markers. Both executions exit zero with 279
first entries and identical 593-byte stdout; stderr and file outputs are empty.
The final reference took 1,756.385 ms; the matching native run took 21.021 ms.
One focused test checks comment probing, newline position, context line updates
and preservation of unrelated lexer flags. This case adds four application
functions, 955 code bytes and 36 text-data bytes. Raw streams and preserved
intermediate failures remain in scratch `preprocessed-output-case-20261027/`.

The assembly-operands case combines register saves, scaled indexed memory,
explicit DWORD operands, immediate arithmetic, three-operand IMUL, shifts,
conditional branches, local assembly labels, SETcc and C-variable operands.
Both executions exit zero with 444 first entries and exact artifacts:
`assemblyex` (1,556 bytes), `assemblygl` (107), `assemblyin` (27), and
`assemblysy` (85). The final reference took 1,740.497 ms; the matching native
run took 21.024 ms. Three focused tests cover dirty expression-node extent,
third-operand initialization, scale/width writes and signed immediate edges.
This case adds seven application functions and 972 code bytes. All fifteen
cached invocations and units passed in 0.666 s; the complete graph/ABI audit
passed in 1.418 s. Raw artifacts and preserved intermediate failures remain
in scratch `assembly-operands-case-20261027/`. Unknown mnemonic, register,
operand and encoding relations retain explicit unsupported boundaries.

The constant-expressions case combines floating addition/multiplication/division,
explicit narrowing and integer conversions, logical constant simplification and
signed/unsigned 64-bit shifts. Both executions exit zero with 442 first entries
and exact outputs: `constantsex` (1,481 bytes), `constantsgl` (281),
`constantsin` (121), and `constantssy` (136). The final reference took
1,487.726 ms; the matching native run took 21.018 ms. Three focused tests cover
extended arithmetic precision/input/storage/stack effects, truncation toward
zero with decimal overflow, and complete 64-bit radix spellings. This case adds
three application functions and 611 code bytes. It also corrects the existing
logical shortcut predicate and preserves the compiler shift helpers' BYTE count.
Raw artifacts and preserved failures remain in scratch
`constant-expressions-case-20261027/`. Unwritten conversion-failure locals and
wider unmodeled semantic domains remain guarded.

The declaration-diagnostics case exercises malformed initializer recovery,
missing record members, argument-count checking and incompatible initialization.
Both executions return the actual error count 4 with 453 first entries and the
same 246-byte diagnostic stream. All artifacts match: `diagnosticsex` (1,195
bytes), empty `diagnosticsgl` and `diagnosticsin`, and `diagnosticssy` (50).
The final reference took 1,853.766 ms; the matching native run took 21.008 ms.
Four application functions add 264 code bytes and 52 bytes of switch data. A focused
test checks dynamic diagnostic selectors, vararg consumption, truncation and
untouched output neighbors. All seventeen cached cases and units passed in
1.315 s; the full graph/ABI audit passed in 1.422 s. Scratch
`declaration-diagnostics-case-20261027/` retains the raw reference, artifacts,
failures and explicit additive-asset derivation. The original resource asset
and target inputs/outputs remain unchanged; a source-validated native catalog
is added under a separate asset name. No output normalization is applied.
Suppression-mode counting and unknown token-name cells remain unsupported.

The browser-information case compiles macros, enum and record declarations,
member references, local and static variables, functions, calls and an if/else
branch with `-FRbrowser.sbr`. Both executions exit zero with 575 first entries
and exact artifacts: `browser.sbr` (612 bytes), `browserex` (1,690), `browsergl`
(161), `browserin` (22), and `browsersy` (233). The final reference capture took
2,073.069 ms; its matching native run took 21.050 ms. The case adds 37 application
functions / 4,485 code bytes and 250 bytes of owned switch data. Four focused
tests cover record encodings, generation/ID assignment, declaration categories,
suppressed locals, scope-depth markers and untouched record/object neighbors.
All eighteen cached cases and their units passed in 1.632 s; the complete
original/native graph and ABI audit passed in 1.452 s. Raw artifacts and retained
failures are in `browser-information-case-20261027/` and
`browser-resume-20261028/` under the binary's scratch directory. Browser reuse,
error output, wider symbol categories and unencountered browser slots remain
outside the exercised domain. The source documents and zeroes the original's
uninitialized 512-byte write-error padding; that error path was not exercised.

The callback/union case combines repeated function and callback declarations,
K&R/default-promotion compatibility, anonymous union members, array/function
decay, aggregate parameters and returns, typed callback arguments and a variadic
external call. Both executions exit zero with 557 first entries and exact
artifacts: `callbacksex` (2,474 bytes), `callbacksgl` (357), `callbacksin` (128),
and `callbackssy` (664). The final reference took 2,183.331 ms; the matching
native run took 18.591 ms. Three application functions add 1,862 code bytes;
existing storage and type owners are retained. Three focused checks cover nested
record-frame/member-allocation boundaries, opaque 52-byte symbol copies and
simple prototype selection without input mutation. All nineteen cached cases
and units passed in 1.715 s; the full graph/ABI audit passed in 1.453 s.
Scratch `callback-union-case-20261028/` retains the raw artifacts, references
and failed native catchup attempts. The original nested-prototype merge has a
reversed copy from fresh arena storage into an existing parameter list. Its
Win32 source preserves that defect; native guards this unexercised incidental
storage domain before the read or effect. No zero initialization or comparison
mask is substituted for it.

The prototype-output case uses `-Zg` with stdcall/fastcall declarations, named
record parameters and returns, callbacks, pointers to arrays, K&R parameters
and a variadic declaration. Both executions exit zero with 411 first entries,
exact 799-byte stdout, empty stderr and no output files. The final reference
took 1,941.670 ms; its matching native run took 22.769 ms. The case adds 21
application functions / 3,046 code bytes and 257 bytes of owned switch data,
including declaration/type printers, handle-backed text composition and
brace-balanced token skipping. Two focused tests cover scalar/tag spelling and
buffer growth, prefix retention and handle reuse; the dirty snapshot test also
covers the formatter's actual state. All twenty cached cases and units passed
in 0.387 s, the full graph/ABI audit in 1.477 s and the final original/native
build in 1.804 s. Raw references, artifact directories and failed guard catchup
attempts remain in scratch `prototype-output-case-20261028/`. This case unifies
the prototype-mode option and lexer spelling workspace with their existing
physical owners. Unvisited pointer-qualifier and formatter diagnostic domains
remain behind genuine pending children; no clock normalization is used.

The completed cases exercise bounded domains. Larger readable parser, lexer,
IR writer, PCH, debug and recorded-input bodies include unexercised branches;
completion does not establish every option, grammar reduction or operand shape.
Unrecovered initialized fields, partial token/symbol payloads and object gaps
remain unmodeled. Unsupported semantic reads trap in both builds; native-only
guards cover genuine host ABI/layout differences. Opaque bytes may be retained
or copied within established owners without assigning meanings or defaults.
The authentic 16-byte type transfer includes its separate cache bytes.

The earlier unaccepted packing WIP and migration research are preserved under
`~/.ai-tmp/msvc2/vc5.10/c1/standardization-migration-20261025/`; packing was then
qualified in this case. Intermediate current-case originals, failures, native
matches and measured logs are under scratch `resume-minimal-20261025/`.
The committed `capture.bin` contains the completed invocation/IO oracle only.
