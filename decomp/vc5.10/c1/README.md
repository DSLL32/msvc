# C1.DLL groundwork

Target: `toolchains/5.10/bin/C1.DLL`, SHA-256
`7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d`.
The lifecycle-only first case is complete. Minimal C compilation is now in progress. The native i386 harness
and actual Win32 DLL compile the same reconstructed C. The lifecycle fixture resolves both compiler exports without invoking them.
The new minimal case invokes the reconstructed pass wrapper and currently traps
in its named pass initializer; AbortCompilerPass remains an INT3 stub.

Accepted ownership is four application functions (`DllMain`,
`InvokeCompilerPass`, the typed pass initializer and checked allocator, 569 original bytes), two linked CRT support functions
(451 bytes), and one genuine import thunk (6 bytes). Their union is 1,026 of the
`.text` virtual extent's 322,288 bytes (0.318%); that denominator includes CRT and unclassified code, not just application
code. This is lifecycle coverage, not compiler-pass coverage.

`make -j4 native original` builds both targets. `make audit` uses the common Rust
CFG tool with imports, callee cleanup, ownership and whole-extent checks.
`python3 scripts/check-exports.py` separately checks exact names, ordinals,
internal module name and both still-trapping export ABIs. `make -j4 fixtures`
runs only Linux code against the cached original status, streams and application
entry trace. The host also asserts the observed attach/detach state.

The final measured incremental original build took 2.824s, native build 1.270s,
Rust audit 0.012s, explicit masked-original capture 0.566s and native GDB capture
0.146s. The native fixture took 0.0042s (setup 0.0016s, execution 0.0021s,
checks 0.0002s), excluding build. Twenty completion corruption controls pass.
Evidence, source snapshots, audit bindings and timings live under
`cases/lifecycle/evidence/`; large immutable original analysis artifacts remain
hash-addressed in the documented scratch locations. See
`cases/lifecycle/PROTOCOL.md` for acceptance and provenance details.

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
transport for the completed attach/resolve/detach case. CRT entry, CRT_INIT and DllMain are accepted; both compiler exports still trap.
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

## Minimal compiler-pass case

`cases/minimal/reference/` freezes a fresh direct-DLL reference for the supported
11-argument no-header/no-debug/PDB-switch case, producing `minimalex`, `minimalgl`,
`minimalin` and `minimalsy`. The separate `driver-contract/` freezes actual CL's
26-argument default invocation, Boolean1, return0, observed stdcall cleanup12,
canonical loaded C1/MSVCRT identities and captured IL before C2 starts. Driver
defaults include an include path and `-Fd`; they are not requirements of the
smaller separately validated case. Neither capture is reconstruction acceptance.

The pass export is accepted with all 172 hot/cold bytes and complete Win32
try/filter/handler/cleanup source. Fresh masked/native evidence reaches the same
named initializer with matching actual caller arguments, pinned source bytes,
module/heap/CRT state, LastError0 and x87 control027f. Eighteen corruption controls
pass. Native SEH currently supports only the observed ordinary path and fails
on actual faults; no exceptional path equivalence is claimed.

Target source uses conventional `_DllMainCRTStartup`, `_CRT_INIT` and `main`.
Explicit MAP symbol aliases preserve historical CRT logical IDs. The native
compiler aliases target-TU `main` to `c1_main`, leaving the Linux host entry
separate; the Win32 DLL retains literal `main`. These internal names are
conventional reconstruction names, not a claim of recovered debug symbols.

Use `python3 scripts/audit-trial.py --case minimal`, then the explicit original
and native capture scripts with `--case minimal`; cached fixture runs never call
those original transports. Immutable earlier lifecycle evidence remains intact.

Initializer/allocator staging completed after the reviewed two-body trial:
`evidence/initializer-staged-promotion/` contains a fresh initializer-only natural
allocator INT3 frontier, followed by `evidence/allocator-staged-promotion/` with
the real allocator as the only new candidate and genuine first-copy state.
The older two-body normal/restore captures remain explicitly auxiliary evidence;
their historical roles were not relabeled. Only four initialized bytes and 160
BSS bytes are currently modeled; other compiler state remains UNMODELED. All 34
initializer slots are represented, and execution stops at the first unreconstructed
callback. See `cases/minimal/INITIALIZER-REVIEW.md` for the original review packet.

The first context reset (RVA12365,18 bytes) is accepted. Its object/value/linked
saved-frame state is mapped as a typed12-byte BSS object; the domain of the
value remains unrecovered. Normal fresh evidence reaches callback12377. A
separate labeled auxiliary code probe seeds all three context fields nonzero
at the real callback entry and verifies their clearing, also checking repeat-
pass BSS clearing and retained snapshot identity. Normal/auxiliary validation
rejects22/39 corruptions. See `evidence/context-reset*` under the minimal case.

The second context reset (RVA12377, 18 bytes) is accepted separately, reaching
callback12389. Both context objects have original/native zero defaults and
independently seeded nonzero callback and BSS-restore checks. Their object/value/
frame roles use mapped storage/allocation identities, never cross-process pointer
values. Evidence lives under `evidence/current-context-reset*`.
