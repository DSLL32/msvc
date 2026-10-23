# C1.DLL groundwork

Target: `toolchains/5.10/bin/C1.DLL`, SHA-256
`7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d`.
The lifecycle-only first case is in progress. The reviewed CRT entry is accepted
as one support function owning 224 original code bytes; no application function
is accepted yet. `make groundwork` checks
the original PE; `make inspect` prints metadata. `make -j4 native original` builds
the static Linux harness and real Win32 DLL with named INT3 stubs. `make audit`
invokes common Rust CFG discovery with explicit DLL paths and checks, currently
with zero accepted vertices. Fixtures remain blocked until the case is accepted.
See `cases/lifecycle/PROTOCOL.md` for the review and fresh-evidence gate.

## Binary and host contract

PE32 i386 DLL, preferred base `0x10600000`, image size `0x9b000`; executable
`.text` RVA `0x1000`, virtual size `0x4eaf0` (raw size `0x4ec00`). It has ten
sections, base relocations, resources, and no TLS directory. The sole debug
directory entry is type 4 (miscellaneous), not a CodeView symbol record. Inventory
does not establish application/CRT extents or any coverage percentage.

| Ordinal | Exact exported name | RVA | Stack cleanup |
| --- | --- | --- | --- |
| 1 | `_AbortCompilerPass@4` | `0x43975` | `ret 4` at `0x43a42` |
| 2 | `_InvokeCompilerPass@12` | `0x12229` | `ret 12` at `0x122a2` |

The export directory's internal module name is `c1n32_l.dll`; neither export is
forwarded. Keep the actual filename, internal name, decorated symbols and ordinals
as distinct metadata.

Static evidence supports a three-slot stdcall invocation
`int InvokeCompilerPass(int argc, char **argv, int mode)`; these are descriptive
names, not a recovered header. CL's call at VA `0x4056c6` pushes
`((pass.flags >> 3) & 1)`, `argv`, then `argc`. C1 stores argument 3 at RVA
`0x5c08c` and calls its C front end at RVA `0x127a7` with only argc/argv. This is
not a callback-table argument. The CL worker captured the actual minimal-pass
argument vector; that compile is queued second. Abort's four-byte slot is not read by its
observed body; preserve it and the stdcall cleanup regardless. The cancellation
body signals application state and waits on a semaphore; it is not equivalent to
unconditionally exiting the host.

There are 125 imported symbols: MSVCRT.dll (88), KERNEL32.dll (27), mspdb50.dll
(9), and USER32.dll (1: `LoadStringA`). The first and third providers are shipped
beside C1.DLL; Windows supplies KERNEL32 and USER32. The inventory lists every
symbol. MSVCRT/PDB provider initialization belongs to the host dependency record,
not C1 application coverage. This toolchain includes CL.EXE, C2.EXE, headers,
libraries and `MSVCRT.LIB`. The CL worker reports original `/?` exit 0 with version
11.00.7022, and `/nologo /Bd /c test.c` exit 0 producing a 275-byte COFF object.
This proves the original compiler runs, not a reconstruction build. The later
original-only debugger capture in
`../cl/cases/original-probes/host-contract-2/` records load event 22, CRT attach
trap 23, invoke 25 with 27 arguments and Boolean 1, and detach 27. Its loaded DLL
file hash matches this target, and all four pre-C2 IR files were preserved.

The resource tree has 141 RT_STRING blocks and one RT_VERSION item, all language
1033. Observed resource calls at RVA `0x25f74` and `0x4617b` pass the DLL's stored
HINSTANCE to `LoadStringA`, with capacity 256 and respectively a stack buffer or
global buffer at `0x57e40`. A native module identity must select C1's resources,
not the harness executable's resources. C1 has no direct imports of
`GetModuleFileName`, `GetModuleHandle`, `LoadLibrary` or `GetProcAddress`; this
inventory does not rule out dependency-mediated filename behavior.

## DLL lifecycle requiring an agreed harness

The PE entrypoint is RVA `0x1202a`, a CRT DLL startup wrapper with stdcall
three-slot cleanup. Its application DllMain at `0x1218e` stores the HINSTANCE at
`0x5c040` and calls `DisableThreadLibraryCalls` for process attach, then returns
TRUE. The CRT wrapper separately allocates its exit-callback table and invokes
`_initterm`; detach includes callback teardown. Do not identify loading the DLL
with invoking its compiler export or skip initialization to reach that export.

`InvokeCompilerPass` calls a separate initializer at `0x122a5` before calling
the compiler. On first use it copies initialized data `[0x5e000,0x5f994)` into a
heap snapshot. A later call restores that snapshot and zeros the front-end BSS
`[0x50000,0x5a06c)`, retaining the snapshot pointer at `0x57818`. Initialization
then walks the 34 pointer slots `[0x69000,0x69088)` in `.initfe`. These are
per-invocation operations, distinct from process attach. Export cleanup calls
`0x2387a` after compilation; its complete contract is not reconstructed. The
export's SEH wrapper catches exception code `0xe0005031`, preserves its returned
status, and runs cleanup. Native translation must eventually preserve this
control flow; no general SEH shim is requested or invented during groundwork.

Proposed native transport is a small harness statically linking the reconstructed
DLL C, explicitly executing the agreed attach/export/detach lifecycle. The
original-toolchain target remains a real Win32 DLL preserving its exports. Harness
functions are transport, outside the target graph. Start with a fresh process
for one invocation, then add a deliberate same-process two-invocation case to
exercise the observed reset behavior. Do not use a fresh process to claim that
reset path has been tested. The orchestrator reports root approval of this
preferred transport; only the lifecycle first case is now released.

For original captures, a debugger must observe the actual DLL load/base and trap
at its CRT entry before target instructions execute. Use the staged load-event
entry guard described in the project README, then mask from a relocated loaded
snapshot at the first entry trap. Nonpreferred-base behavior remains untested;
an offline-masked image needs relocation-aware treatment of fixup sites. An EXE-only
entry trap or a breakpoint installed after LoadLibrary returns misses the DLL's
initialization. Record DLL identity/hash plus RVA (`runtime_pc - loaded_base`),
not the preferred VA alone; retain CL and dependent-module identities separately.
Native instrumentation must map those target identities consistently and exclude
harness/shim/import entries without excluding target initializers.
`scripts/dll-capture.c` now implements the original-only guarded loader and masker;
its baseline records both lifecycle calls and its masked mode restores explicit
ranges from the relocated snapshot. `scripts/native-host.c` is the initial native
transport, currently trapping at its CRT entry counterpart. The CRT entry is accepted; CRT_INIT, DllMain and both compiler exports still trap.
Nonpreferred-base behavior remains untested.

## Seed cases and release conditions

`cases/plan.json` fixes a proposed order: minimal C, comment preservation,
numeric/varargs, embedded CodeView types, then repeated minimal invocation. Only
source inputs were copied from vc1.0; their hashes and provenance are recorded.
No vc1.0 expected output, IR, trace or acceptance record is a 5.10 oracle. The
source files are case inputs, separate from the target INT3 stubs in `src/`.

Original work uses the shared prefix `~/.ai-tmp/vc1.0-capture/wine`, R: for the
repo and S: for shared scratch, under `S:\vc5.10\c1`. Prefer relative case paths;
do not create a prefix or change mappings. Native fixture execution must never
launch Wine. Future reference capture must freeze the real CL-to-DLL arguments,
environment, streams, input bytes, IR and any auxiliary dependency operations.
Compare native regressions only with those explicit 5.10 captures; new function
acceptance still requires fresh matching masked-original/native evidence.

Potential shared work is module/resource identity for
`LoadStringA`, provider-scoped imported CRT state and verified PDB-interface
boundaries. Existing tracing supports the initial per-DLL static native harness;
multi-module native tracing is only needed if a later case requires it. These
are investigation items, not permission to replace opaque calls
with success stubs. Contact `/root/recon_lib_maintenance` for concrete shim needs
and the vc5.10 orchestrator for host, graph, filtering and acceptance policy.
