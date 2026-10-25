# VC5.10 C1.DLL

`binary.c` is the function/export registry for original
`toolchains/5.10/bin/C1.DLL` (SHA-256
`7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d`).
The current prefix contains 86 accepted application functions / 10,654 code bytes.
Historical linked startup support remains separately classified: two CRT
functions / 451 bytes; three import thunks / 18 bytes. Owned text data is 161
bytes. The original text extent is 322,288 bytes; its non-CRT denominator has
not been classified. Initialized state retains 4,032 unmodeled bytes, and the
literal object's separate 16-byte tail remains unmodeled.

`make -j4 check unit` builds one native i386 `.build/libbinary.so` and discovers
all numbered cases. `make -j4 win32 audit` verifies original VC5 compilability
and shared graph/import/RET/ownership/extent checks. Runtime identification and
compiler launching use shared `recon-tool`; live finite graph contracts are in
`policy-data.json`, with finite original/provider/SDK premises in
`import-contracts.json`. Original compiler artifacts live under `build/`.
Application source stays in `src/`; the two historical startup functions and
headers are isolated in `crt/`, with a per-file rationale and no App credit.
Native DLL adapters/profile setup live in `harness/dll-cases.c`; numbered cases
hold invocation descriptors, captured inputs/results and narrow tests.
MSVCRT.LIB is an import library here: shared identification honestly reports
zero static matches, while the historical CRT rows retain strict graph checks.

The lifecycle case attaches, resolves both exports without invoking them, and
detaches. Minimal invokes the actual supported direct 11-argument
`_InvokeCompilerPass@12` contract with BOOL 1. It remains partial at
`create_input()`, RVA `0x13f0e`. Its fresh reference uses logical cwd
`T:\vc5.10\c1\0001-minimal-c`, preserved in an isolated native scratch namespace.
Final IR from other invocations is not merged into this oracle. Both cases run
entirely natively against cached original observations.

`make -j4 original-host` builds the original-only `harness/win32-host.c`.
Shared `recon-tool original capture --host build/c1-host.exe --progress
--native-msvcrt` owns transport, staging and masking. The host forwards actual
argc/argv to the three-argument stdcall export and uses the real loader lifecycle.
Native execution binds that same vector; its finite environment models only the
observed absent `MSC_CMD_FLAGS` query. New queries require input evidence.
No private historical HOST vectors or error seeds are used. SYSTEM_INFO and
qualified locale bytes remain explicit fixture/profile inputs, rather than
claims about a complete Windows environment.

The former unaccepted 39-byte `_push_packing` trial and owned research remain
saved in `~/.ai-tmp/msvc2/vc5.10/c1/standardization-migration-20261025/`.
It is now accepted after a fresh shared masked-original/native pair and focused
alignment inheritance/override test. Historical proof remains in Git history;
new progress reports/logs are under scratch `resume-minimal-20261025/`.
No new functions or cases were accepted during migration.
