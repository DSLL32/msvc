# VC5.10 C1.DLL

`binary.c` is the function/export registry for the original
`toolchains/5.10/bin/C1.DLL` (SHA-256
`7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d`).
The reconstruction contains 630 application functions / 182,177 code bytes and
6,949 bytes of owned text data. Application code is 56.53% of the original
322,288-byte text extent; that denominator includes runtime code and text data,
and is not a classified application-only denominator. Ten original CRT/support
owners remain separate, without application credit. MSVCRT.LIB is
an import archive here; shared identification reports zero static CRT matches.

All five cached cases complete natively: DLL attach/export lookup/detach, the
minimal C compilation `int main () { return 0; }`, the language workload
below, the declarations workload described next, and the types workload below. The minimal invocation forwards the actual
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
