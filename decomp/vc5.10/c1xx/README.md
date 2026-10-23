# C1XX.DLL

The first case is complete: load/CRT attach, resolve both compiler exports by
name and ordinal without calling them, then unload/CRT detach. The first real
minimal C++ compilation is now active: InvokeCompilerPass has its full wrapper,
and reaches the initializer stub. AbortCompilerPass remains an INT3 stub.

`make -j4 native` builds the i386 Linux static harness. `make -j4 fixtures`
replays its cached original oracle with native instrumentation and no Wine.
`make -j4 original` separately builds the real DLL with MSVC 5.10; `make audit`
compares its graph/ABI/imports using the shared Rust auditor. Audit/build targets
may use Wine explicitly; fixture dependencies do not. Fixtures inspect the
already-built Win32 DLL's exports without executing it.

Accepted code is **two application functions (207 bytes)**, **two target CRT
functions (451 bytes)** and **one genuine linker-generated import thunk (6 bytes)**.
The 664 accepted bytes are 0.0884% of the 751,216-byte code extent; this is not
a non-CRT coverage percentage. The frontend driver and initializer remain stubs.

## Target and ABI

Target `toolchains/5.10/bin/C1XX.DLL`, SHA-256
`e27df3bb9a37058c85dad3fb7d1ed30b9e52d38fb6c0f66c8fdba0a11111d3d9`.
Preferred/observed base `0x10400000`; entry RVA `0x2e3c6`; no TLS directory.
The export module name is `c1n32_l.dll`.

| Ordinal | Exact name | RVA | ABI |
| --- | --- | --- | --- |
| 1 | `_AbortCompilerPass@4` | `0x903ad` | stdcall, one DWORD, ret 4 |
| 2 | `_InvokeCompilerPass@12` | `0x384d4` | stdcall, argc/argv/BOOL, ret 12 |

For native builds, target translation units use `-Dmain=c1xx_main`; the host is
compiled separately. Source keeps canonical main and the Win32 symbol remains
_main. This explicit native symbol projection adds no target wrapper or graph edge.

CL statically pushes `(pass.flags >> 3) & 1`, argv and argc. The third argument
is a Boolean mode, not a host callback table. The real invocation is captured under
`cases/minimal/contract.json`: argc26, Boolean1, raw vectors/context and a return0.
A direct original DLL host reproduces all four CL-produced IR files byte-for-byte.
`cases/minimal/invoke-validation.json` records the wrapper promotion and11 rejected
input/state/ELF/extent controls. The initializer snapshot schema remains pending
common policy; no frontend data section has been bulk copied. Repeated invocation resets frontend state and runs `.initfe`
callbacks; cancellation, SEH, PDB/COM and resource lookup need later cases.

## First-case evidence

`cases/lifecycle/validation.json` records immutable original/native captures and
the exact trial audit. `functions.json` records each promotion in order.
`cases/lifecycle/reference.json` is the native fixture oracle; no fixture silently
captures original output. Captures preserve actual loaded target/MSVCRT bytes,
raw state and streams, relocated code snapshot, full mask readback and bindings
to the compiled native/Win32 artifacts and configuration.

The canonical shipped MSVCRT is 277,776 bytes, SHA-256
`e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210`.
Original hosts use an adjacent verified copy and `msvcrt=n`; DLLLOAD hFile bytes
prove the provider actually loaded. The entry guard is outside all 26,460 HIGHLOW
relocations. At its first trap, all 751,216 loaded code bytes are saved and masked;
only complete accepted/trial hot and cold chunks are restored from that snapshot.
No loader data exemptions or nonpreferred-base certification are claimed.

The native host registers the DLL's verified identity through `lib/recon`, calls
the same stdcall lifecycle source as the real Win32 DLL, and checks post-return
state. Attach count goes 0→1→0; the empty 128-byte onexit allocation is freed while
its end pointer intentionally retains the original dangling identity. Raw callback
and initializer slots stay NULL. The saved module is the registered DLL, thread
notifications are disabled, LastError stays 203 and caller x87 precision stays
53 bits. Host-only observation preserves LastError. The application entry trace
is the original-derived first-encounter `DllMain`; CRT/import/shim entries are
filtered on both sides.

Two-chunk CRT functions are single logical vertices in the Rust audit. Import
`_initterm` is an ordinary external declaration; LINK emits its six-byte thunk.
Separate source/import counts prevent treating that thunk as handwritten code.
The full-case acceptance rejects 20 altered completion/state/identity/mask/audit
proofs. Earlier CRT-entry and initializer gates retain their own negative controls.
Source contents are frozen by compiler wrappers. `scripts/check-history.py` and
`check-completion-negative.py --historical` revalidate the committed completion
receipt against immutable execution snapshots and a fresh static Rust audit.
They cannot promote functions, change raw receipts or execute either binary.
Current native fixture execution remains a separate check.

## Boundaries still pending

The saved module feeds LoadStringA (256-byte buffer); resources will need actual
encountered IDs/encoding/capacity behavior before native support is claimed.
MSVCRT FILE, `_mbctype`, SEH/longjmp and wide-character imports require provider
contract checks. MSPDB50 and OLE methods are in-process stateful interfaces, not
subprocess responses. Only the first lifecycle subset is implemented.

`cases/plan.json` retains source-only C++ seeds copied from vc1.0; it contains no
old IR/oracle reuse. Original builds/captures use the shared prefix and fixed R:/S:
drives, under `S:\vc5.10\c1xx`, with no private mappings or server operations.
