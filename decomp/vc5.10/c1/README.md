# VC5.10 C1.DLL

`binary.c` is the function/export registry for the original
`toolchains/5.10/bin/C1.DLL` (SHA-256
`7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d`).
The reconstruction contains 342 application functions / 92,202 code bytes and
3,574 bytes of owned text data. Application code is 28.61% of the original
322,288-byte text extent; that denominator includes runtime code and text data,
and is not a classified application-only denominator. Five original CRT/support
owners / 571 bytes remain separate, without application credit. MSVCRT.LIB is
an import archive here; shared identification reports zero static CRT matches.

Both cached cases complete natively: DLL attach/export lookup/detach, and the
minimal C compilation `int main () { return 0; }`. The latter forwards the actual
11 arguments to the three-argument stdcall `_InvokeCompilerPass@12` export with
BOOL 1. It exits zero and matches all four original outputs: `minimalex` (1,187
bytes), `minimalgl` (51), `minimalin` (3), and `minimalsy` (13), including normal
compiler finalization, arena release and registered cleanup callbacks.
No clock normalization is required by these outputs.

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

The completed case exercises a bounded domain. Larger readable parser, lexer,
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
