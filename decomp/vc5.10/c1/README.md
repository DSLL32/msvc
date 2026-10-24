# C1.DLL groundwork

Target: `toolchains/5.10/bin/C1.DLL`, SHA-256
`7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d`.
The lifecycle-only first case is complete. Minimal C compilation is in progress,
all initialization callbacks and main are complete. After internal-switch parsing, the captured GetSystemInfo response, all nine pool bindings and a proved64KiB commit, the full command-line parser, include-alias root and checksum generator now reach the genuine public MSVCRT.setlocale entry. Native execution stops before its unsupported body; no locale success is claimed. Its contextual option fields and complete grammar are modeled with individual defaults, snapshot/restore and BSS-reset proofs; initialized state is1836 known/4712 UNMODELED bytes. The native i386
harness and actual Win32 DLL compile the same reconstructed source; genuine
thiscall members and their callers use small C++ translation units. The lifecycle
fixture resolves both compiler exports; the minimal case calls InvokeCompilerPass.
AbortCompilerPass remains a named INT3 stub.

The next72-byte multibyte-scanner candidate and its512 contextual initialized
lexer-table bytes are staged. Own original normal/restore captures reach12f3e
and isolate the distinct setlocale/_setmbcp effects; native shared support and
full paired acceptance remain pending. See `cases/minimal/MULTIBYTE-SCANNER.md`.
Accepted counts below are unchanged.

Accepted ownership is69 application functions /4737 original code bytes plus16
owned switch-table bytes,2 CRT support
functions /451 bytes and two6-byte import thunks:5216 of the `.text` virtual
extent's322288 bytes (1.618%). That denominator includes CRT and unclassified
code; it is not non-CRT coverage. The original17-DWORD PCH-context initializer
and its tail wrapper were explicitly accepted as one ordered JOINT two-function
closure after separate non-promotable prospective input evidence.

`make -j4 native original` builds both targets. `make audit` uses the common Rust
CFG tool with imports, callee cleanup, ownership and whole-extent checks.
`python3 scripts/check-exports.py` separately checks exact names, ordinals,
internal module name and exported stdcall ABIs. `make -j4 fixtures`
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
their historical roles were not relabeled. Only four initialized bytes and 496
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

The six-element context-record reset (RVA12389, 27 bytes) is accepted. The
72-byte typed array has per-element zero-default and nonzero reset/restore
proofs. No later consumer has established its domain; it remains named
`contexts` and does not authorize interpreting other unmodeled bytes.

The three independent one-byte callbacks at RVA123a4/123a5/123a6 are accepted
in order, each through its own fresh matching pair and graph/ABI audit.
They introduce no new data fields or fixture behavior.

The first pool wrapper (RVA123a7) owns two chunks totaling 21 bytes. It calls
a separate constructor and then registers the destructor; the constructor and
registration helpers remain named traps. Fresh evidence matches the constructor
frontier including its actual caller/return site, without consuming pool state.

The first buffered-output constructor (RVA123ae, 58 bytes) is accepted. Its
nine touched fields are recovered individually: data/cursor/capacity, relocation
callback/context, FILE stream, success flag, mode byte and signed descriptor
sentinel. Only 33 original bytes join the model; the native compact structure
has three padding bytes. The original filename/other storage remains UNMODELED.
The nonzero auxiliary probe never invokes its callback or dereferences its fake
stream sentinel. It verifies genuine constructor execution and BSS clearing of
every mapped field before the next named registration trap.

Cleanup registration (RVA123f6 plus cold30407, 63 bytes) is accepted, with
matching count1, destructor identity in slot0 and 31 remaining zero slots. The
overflow diagnostic call remains a separate variadic named trap; its source
literal is kept at the call. Next frontier is callback1241c.

The second output-buffer constructor (RVA12423, 58 bytes) is accepted, reaching
callback1246b with two registered destructors. `output_buffers[]` contains only
the encountered typed instances. The paired auxiliary probe independently seeds
and observes every encountered constructor, with distinct interior allocation
identities and field values; the original-only debugger accepts its exact typed
object/constructor/destructor table as a hashed case input. The original gaps
remain absent. Normal/auxiliary controls reject44/93 corruptions.

The local audit wrapper accepts an explicit `--auditor` path for approved
immutable tool binaries. Audit receipts bind the actual path/hash; checker-only
refreshes issue fresh validation without rerunning unchanged execution evidence.

The third wrapper/constructor are accepted separately, reaching callback124ba
with three destructor identities registered. Auxiliary typed reset/restore
validation rejects100 corruptions. `capture-native.py --reuse-build-from DIR`
can reuse the exact verified normal ELF and immutable compiled sources for an
auxiliary capture: candidate, Win32 DLL and all owned source contents must still
match. Shared worktree drift does not overwrite the recorded inputs. Five
negative controls reject changed ELF/candidate/owned source and missing/corrupt
archived content.

All eight encountered output-buffer wrappers and constructors are accepted in
separate stages. Each constructor retains a real nine-field C body and its
object has independent nonzero reset/restore evidence. Eight real destructor
identities are registered; no destructor body is accepted. The 496 modeled
original BSS bytes occupy520 native bytes including24 natural padding bytes.
The next frontier is callback12645. Original/native normal captures remain
explicit function-acceptance work; the native lifecycle fixture uses cached
references and never invokes Wine.

Empty callback12645 is accepted; durable coverage is28 application functions /
1331 original bytes,2 CRT-support functions /451 bytes and one6-byte import
thunk (1788 total owned bytes). The full .text extent remains322288 bytes,
including CRT/unclassified code; this is not a non-CRT percentage.

Callback12646 is accepted after the orchestrator's concrete review of the
29-byte trial. It copies a recovered signed64 numeric alternative and sets
an availability byte;16 original tail bytes remain UNMODELED. Fresh normal,
positive and negative auxiliary pairs match callback12663, with actual original
24-byte records retained separately and no native whole-record equivalence
claim. See `cases/minimal/VALUE-COPY-REVIEW.md`. The candidate-inclusive audit
passes31 source functions /1817 bytes; `functions.json` now includes that31-function
accepted prefix. Cached native lifecycle fixtures still execute in about4ms.

Empty callback12663 is accepted separately, reaching12664. Its fresh candidate
pair and17 affected value-boundary controls pass;47 unchanged generic controls
reuse explicitly identified prior results. Durable coverage is32 source
functions /1818 original bytes.

The PDB rolling-signature initializer12664 (11 bytes) is accepted, reaching
callback1266f. Its unsigned32-bit field has loaded zero-default evidence and
independent nonzero BSS-clear/callback checks; actual consumers pass it to the
imported SigForPbCb routine, which is not reached yet. Fresh normal/positive/
negative pairs and all changed transport/state controls pass. Durable coverage
is31 application functions /1372 bytes plus2 CRT functions /451 bytes and the
6-byte import thunk:1829 owned bytes. The517 modeled BSS bytes occupy544 native
bytes including27 padding bytes. Numerical value tails remain UNMODELED.

The split PDB wrapper1266f is accepted (21 bytes in two chunks), stopping
at its separate named helper12676. No persistent PDB context field is consumed
yet. Fresh caller/return-site and stale-prefix guards pass;45 unchanged generic
controls reuse explicitly hashed prior results with byte-identical validators
and mutation bodies. Source functions34; owned original bytes1850.

The first persistent PDB-context helper12676 (43 bytes) is accepted, reaching
callback126af with nine cleanup identities. Six typed fields occupy24 native
bytes; the original+0x10 gap remains UNMODELED. Fresh normal/clear/retain pairs
prove the separate session lifetime and both reuse branches without invoking
a provider. All changed transport/state controls passed with measured times;
see `cases/minimal/PDB-CONTEXT.md`. Durable coverage is33 application functions /
1436 bytes,2 CRT functions /451 bytes and the6-byte import thunk (1893 total).
The cached native lifecycle fixture passed in5.65ms under concurrent checks.

The second split PDB wrapper126af is accepted separately (21 original bytes).
Fresh graph/ABI/state evidence reaches its named helper126b6. Four affected
controls reran;45 unchanged generic controls reuse prior passing results with
identical checker and field-contract hashes. Source functions36; owned bytes1914.

The second persistent PDB helper126b6 is accepted (43 original bytes), reaching
callback126ef. Both contexts now have independent typed48-byte total state,
nonzero preservation/reset/retain proofs, separate storage identities and two
unmodeled4-byte gaps. Fresh first/second-context mutation guards pass; unchanged
core controls reuse explicitly identified prior results. Durable coverage is
35 application functions /1500 bytes plus2 CRT functions /451 bytes and the
6-byte import thunk (1957 total). The next callback begins the genuine thiscall
pool initialization sequence.

The first pool wrapper126ef is accepted (13 bytes), stopping at genuine
thiscall member126fc. Actual receiver/stack/return-call and RET4 evidence is
bound to both targets; four typed pool fields have zero-default and nonzero
BSS-reset proofs. Original object extent remains unknown. Durable coverage:
36 application functions /1513 bytes,2 CRT functions /451 bytes and6 import
bytes (1970 total,38 source functions). See `cases/minimal/POOL-INITIALIZERS.md`.

Pool member126fc is accepted (22 bytes), reaching12712. Fresh natural and
nonzero proofs verify actual thiscall entry/return, four known writes and
adjacent state. Durable coverage:37 application functions /1535 bytes plus
2 CRT /451 bytes and6 import bytes (1992 total,39 source functions).

Pool step pool-wrapper2 is accepted, reaching1271f; 2 separately encountered pool objects
have exact receiver/stack/return/default/nonzero reset proofs. Durable coverage:
38 application functions /1548B +2 CRT /451B +6B import =2005B (40 source functions).

Pool step pool-wrapper3 is accepted, reaching1272c; 3 separately encountered pool objects
have exact receiver/stack/return/default/nonzero reset proofs. Durable coverage:
39 application functions /1561B +2 CRT /451B +6B import =2018B (41 source functions).

Pool step pool-wrapper4 is accepted, reaching12739; 4 separately encountered pool objects
have exact receiver/stack/return/default/nonzero reset proofs. Durable coverage:
40 application functions /1574B +2 CRT /451B +6B import =2031B (42 source functions).

Pool step pool-wrapper5 is accepted, reaching12746; 5 separately encountered pool objects
have exact receiver/stack/return/default/nonzero reset proofs. Durable coverage:
41 application functions /1587B +2 CRT /451B +6B import =2044B (43 source functions).

Pool step pool-wrapper6 is accepted, reaching12753; 6 separately encountered pool objects
have exact receiver/stack/return/default/nonzero reset proofs. Durable coverage:
42 application functions /1600B +2 CRT /451B +6B import =2057B (44 source functions).

Pool step pool-wrapper7 is accepted, reaching12760; 7 separately encountered pool objects
have exact receiver/stack/return/default/nonzero reset proofs. Durable coverage:
43 application functions /1613B +2 CRT /451B +6B import =2070B (45 source functions).

Pool step pool-wrapper8 is accepted, reaching1276d; 8 separately encountered pool objects
have exact receiver/stack/return/default/nonzero reset proofs. Durable coverage:
44 application functions /1626B +2 CRT /451B +6B import =2083B (46 source functions).

Pool step pool-wrapper9 is accepted, reaching1277a; 9 separately encountered pool objects
have exact receiver/stack/return/default/nonzero reset proofs. Durable coverage:
45 application functions /1639B +2 CRT /451B +6B import =2096B (47 source functions).

Pool step pool-empty is accepted, reaching1277b; 9 separately encountered pool objects
have exact receiver/stack/return/default/nonzero reset proofs. Durable coverage:
46 application functions /1640B +2 CRT /451B +6B import =2097B (48 source functions).

The33-byte reset1277b is accepted under the reviewed exact20-byte empty-record
prototype representation. Six original DWORD stores (including repeatedfirst)
and four count5 copies establish that byte extent; no larger record layout is
claimed. Source uses a named BYTEarray and bounded clears, retaining the
redundant first4 clear. All20 bytes join known BSS/reset state and remain
outside the6548-byte initialized snapshot. Fresh nonzero before-clear and
callback probes prove full20 reset with whole neighboring-state comparisons.
Prototype negatives16/28, generic47/100 and4 formal/trace controls perpair pass.
See `EMPTY-PROTOTYPE-REVIEW.json` and evidence/empty-prototype-* for immutable
raw sources, field ownership and timings. Durable49 source functions:47 app /
1673B +2 CRT /451B +6 import =2130B; next frontier1279c.

The PCH tail wrapper1279c and real thiscall member11e2a are jointly accepted
(10+18 original bytes), reaching127a6. The68-byte typed file/relocation context
has exact zero defaults, nonzero before-clear and actual member reset proofs;
pointed image/arena layouts and any larger context extent remain unknown.
Actual ECX/formal-this, inherited original return versus native CALL, RET0,
ESP+4/EAX-this/EDI and whole neighboring state are checked. Strict Rust auditing
passes51 source functions with all import/RET/ownership/extent checks. Fresh
core47/100 and new PCH34/43 controls pass; prior unchanged pool negatives are
explicitly reused while all nine current entry/return pairs remain checked.
See `cases/minimal/PCH-STATE-ANALYSIS.md` and `evidence/pch-joint-*` for original/
native timings, immutable proof and the explicit two-function acceptance.
Native cached lifecycle fixture4.36ms. Independent orchestrator review passed;
historical prospective evidence remains raw-only, never relabeled acceptance.

The final empty initializer127a6 is accepted, reaching genuine main127a7 with
argc/argv and actual Invoke caller/return verified. The already reconstructed
wrapper's known pass_mode store is proved from its original instruction and
actual BOOL value; unknown state remains trapped before main's body. Full
core47/100 controls and new main16/16 plus compiled-source8 controls pass.
`make -j4 native-recorded` freezes actual source CONTENT before compilation,
checks that it stayed stable, and binds it to the retained ELF. Captures can
reuse that manifest after shared-only worktree drift; owned-source/content/ELF
changes reject. The normal original capture was reused after a native-only
build, with its historical native hash retained honestly. No original rerun
was needed. Evidence and measured build/capture/fixture/control times are in
`cases/minimal/evidence/last-initializer-*`.

Full main is accepted (137 hot+27 cold original bytes), reaching named helper
12830. Its new52-byte initialized projection and12-byte commandline BSS fields
have canonical defaults, typed firstcopy and distinct nonzero restore/reset
proofs;6496 initialized bytes remain UNMODELED. Actual main entry independently
anchors all initialization checks before main's legitimate phase write. New
boundary controls50/65 and core47/100 pass; see `cases/minimal/MAIN.md` and
`evidence/main-first-helper-*`. Original/native builds3.822/1.899s, captures
1.067/0.517s normal and1.619/1.364s restore, audit0.018s, cached lifecycle5.11ms.
Core controls22.42/73.71s and new controls19.33/37.94s are separate from fixtures.
A tool-only update retained an identical audit report and reused execution raw.

The67-byte work-buffer initializer12830 is accepted, reaching the genuine
reserve_work_buffer12873 trap with cdecl reserve0x500000/commit0x4000. Four
contextual BSS pointer fields (two data/limit pairs) gain exact zero defaults and
nonzero owned-pointer reset proofs. Actual main entry, actual work entry and
allocator frontier are independently observed; main's phase write is validated
before work begins, and no state stores occur before the first allocation call.
All54 source functions pass strict graph/import/RET/ownership/extents auditing;
new work41/46 and main50/65 controls pass, with unchanged generic results reused
explicitly. Original/native builds3.156/1.918s, original/native captures1.068/0.552s
normal and1.720/1.232s restore, audit0.021s, cached native lifecycle3.90ms.
Control runtimes22.29/37.95s (work) and18.98/38.72s (main) remain separate from
fixtures. See `cases/minimal/WORK-BUFFERS.md` and `evidence/work-allocator-*`.

The reserve helper12873 (62hot+30cold bytes) is accepted through two real
VirtualAlloc pairs and main's next commandline-helper128b1. Actual API inputs,
RET16/EDI, loadedproviderhFile/IAT, reservation/commit protections, all16KiB
zeroed committed bytes and exact owned base/limit identities match. Reserved
remainders remain opaque;64KiB alignment and DECOMMIT are not claimed. Existing
shared provider suffices for this reached scope. Finite SDK/ordinarycallee
preservation witnesses enable common Rust cached-import resolution; all55 source
functions pass strict auditing. Fresh allocation53/53 and core47/100 controls
pass; cached native lifecycle4.56ms. See `cases/minimal/RESERVE-WORK-BUFFER.md`
and `evidence/reserve-main-switches-*` for raw bindings and separate timing.

The internal-switch scanner128b1 (79hot+84cold bytes) is accepted, reaching
its separate parser12900 with actualargv[1] "-il"/NUL andmode0. Independent real
scanner entry anchors the earlier allocator/main continuation; no modeled or
original state changes before the parser. All56 source functions pass strict
Rust audits. Focused27/27 controls are fresh; unchanged core47/100 and allocation
53/53 proofs are hash-bound reused, not rerun. Builds4.150/1.904s, normalcaptures
1.468/.800s,restore1.869/1.639s,audit.026s,nativecachedlifecycle4.13ms. New
controls24.35/34.18s are separate. See `cases/minimal/SWITCH-SCANNER.md` and
`evidence/scanner-parser-*`; the getenv/token branch has not yet run.

The internal parser12900 is accepted:216codebytes and16 separately owned mode
jump-table bytes; adjacent4paddingbytes staymasked/uncredited. Three contextual
atoi-result fields (defaults0/0/100) expand known initializedstate64B, leaving
6484B UNMODELED with exact distinct nonzero snapshot/restore proof. Allten natural
argv calls are independently recorded with actualpointer/bytes/mode/return/stack;
they exercise unrecognized mode0 only. Canonicalgetenv MSC_CMD_FLAGS is actually
observedNULL, reaching mainhelper12998; prefix/atoi branches are not executed
credit. Strict57-source audit2877ownedB passes, fresh focused42/54 controls pass,
unchanged priorprotocol/old-field/allocation results explicitly reused. See
`cases/minimal/INTERNAL-SWITCH-PARSER.md` and `evidence/internal-parser-main-*`.

The one-byte helper12998 is accepted with independent entry/return proof before
the real GetSystemInfo call. The shared SYSTEM_INFO-only profile reproduces all36
recorded bytes; no MEMORYSTATUS is invented or supplied. Fresh normal/restore
proofs and76 focused controls pass. See `cases/minimal/SYSTEM-INFO.md`.

The arena wrapper is accepted with all six32-byte BSS objects and their typed
20-byte descriptors defined at use. The next real primary-initializer entry
precedes descriptor processing. See `cases/minimal/ARENAS.md`.

Primary arena initialization129db is accepted through the genuine member entry.
Actual preprologue ECX, descriptor/NULL stack slots, complete formals and state
match. See `cases/minimal/PRIMARY-ARENA.md`.

The genuine arena reservation member now reaches its size helper after the sole
verified descriptor-pointer store. Allocation is still pending; see
`cases/minimal/ARENA-MEMBER.md`.
