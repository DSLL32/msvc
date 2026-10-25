# VC5.10 C1.DLL

`binary.c` is the function/export registry for original
`toolchains/5.10/bin/C1.DLL` (SHA-256
`7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d`).
The migration preserves 82 accepted application functions / 7,037 code bytes.
Historical linked startup support remains separately classified: two CRT
functions / 451 bytes; two import thunks / 12 bytes. Owned text data is 161
bytes. The original text extent is 322,288 bytes; its non-CRT denominator has
not been classified. Initialized state retains 4,200 unmodeled bytes, and the
literal object's separate 16-byte tail remains unmodeled.

`make -j4 check unit` builds one native i386 `.build/libbinary.so` and discovers
all numbered cases. `make -j4 win32 audit` verifies original VC5 compilability
and shared graph/import/RET/ownership/extent checks. Runtime identification and
compiler launching use shared `recon-tool`; live finite graph contracts are in
`policy-data.json`, with finite original/provider/SDK premises in
`import-contracts.json`. Original compiler artifacts live under `build/`.
The MSVCRT import library has no linked static CRT bodies here: the shared
identifier honestly reports zero matches. The two historical startup owners
remain explicit registry CRT rows and are included in strict source graph checks.

The lifecycle case attaches, resolves both typed exports without invoking them,
and detaches. The minimal case attaches and invokes the actual supported direct
11-argument `_InvokeCompilerPass@12` contract with BOOL 1. It remains partial at
`push_packing(0, NULL)`, RVA `0xd49b`. Its captured logical S: cwd is preserved in
an isolated native scratch namespace, never the immutable original directory.
Final IR from the different baseline cwd is not merged into this partial oracle.
Both cases use cached original observations and run entirely without Wine.

Minimal's actual HOST CRT argv/environment differ from the Invoke vector. Their
private raw files remain immutable, hash-checked readonly dependencies in
`~/.ai-tmp/msvc2/vc5.10/c1/minimal-packing-frame-normal-001/`; they are not committed
or printed. This makes that case intentionally dependent on retained private
scratch. Its SYSTEM_INFO bytes are a genuine fixture input; the locale/x87 host
profiles bind the same own original reference identity. Adapter file loading uses
host libc before target invocation, preserving target shim FILE/lowio state.

The unaccepted 39-byte `_push_packing` trial and owned research were saved before
migration in `~/.ai-tmp/msvc2/vc5.10/c1/standardization-migration-20261025/`
(`snapshot.json`, scoped patch, `worktree/`, `build-snapshot/`). It had no fresh
matching original/native acceptance pair. The runnable source restores its real
named INT3 stub from accepted commit `ba144072`. Existing committed evidence is
available in Git history; per-function snapshots are not invocation cases.
No new functions or cases were accepted during migration.
