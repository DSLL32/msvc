# VC1.0 C2 reconstruction

Checkpoint1416 completes the generated DLL telemetry-adapter case. There are
1301 reconstructed App functions and five unchanged historical handwritten CRT
bodies. App code occupies347286/399693 original code-section bytes(86.8882%);
adding916 distinct owned text-data bytes gives348202/399693(87.1174%). These
are clipped extent unions using section VirtualSize(raw size only if zero),
not branch coverage. The five CRT overrides contribute1776 bytes separately
and receive zero App credit. Available App/classified CRT/import extents cover
374576/399693(93.7159%). A reliable non-CRT denominator remains unavailable;
74 pending App rows and25117 bytes outside available extents need investigation.
No only-CRT remainder claim is made.

`binary.c` is the sole registry. All25 cached IO cases complete and197 narrow
units pass. Fixtures preserve actual frontend IR, single MSC_CMD_FLAGS child
transport, stable S cwd, explicit first-entry seeds and complete output oracles.
Only established COFF timestamp bytes4..8 are normalized; listings/OMF are exact.
Native fixtures never run Wine.

The adapter uses untouched CL `/O2 /Ob2 /Oa /G5 /Z7 /FAcs /W3`. Its explicit
contract makes input records, output and DLL storage disjoint. Real imported
data/functions, small inlined encoders with typed assembly parameters/local
labels, callback-surviving values and an unsigned packet checksum form one
workload. The available generated-telemetry.c is an actual listing dependency,
not an include or a missing-source fallback. Source4485B and sidecar3454B are
pinned alongside actual ex/sy/in/gl/db3386/1190/18/493/405B, prefix000368.
Read-only CL observation captures the genuine pre-C2 invocation/IR; its parent
exit is unobserved. Ordinary untouched CL and independent standalone C2 both
succeed. Only telemetry_encode is emitted as a function: the helpers really
inline. Imports and expanded assembly labels remain visible in the object/listing.
Complete3378B object normalizedSHA256:
`8e45943204da52ae5d762f0c2c63efd3e75f005f35c5ec8eeebcce1cefa77767`.
The16887B listing is exactSHA256:
`113525ab9d5f32df7f973830396b210b367eb0494435999ea27910b724e4bb2c`.
Streams are empty/status0, with no other outputs. Changed object byte100 rejects.

Six whole App routines add415 code bytes: imported-symbol alias construction83,
its newly discovered descriptor clone49, assembly parameter pin35, assembly
symbol substitution41, inline-argument global-load predicate101 and transfer
call-boundary search106. The alias owns a12B contextual link; target names retain
source-string suffix identity at+6. Exact first44B kind1 clone and both genuine
fastcall arguments are observed. Assembly callbacks consume actual parameterid
A6 and its binding; substitution returns materialized storageidF3. The first pin
already has flag0 cleared, so state-changing flag controls are separately cold.
The first global-argument predicate returns1 for the real opcode55 tree; cold
controls cover arity, short circuit and non-global storage. The first transfer
search walks nine preceding records to an indirect call. Cold controls separately
cover forward stores, direct100-class call skips and the kind14/2D/register4
marker. No claim is made that the actual first search executes all those paths.

Fresh masked-original/native pairs, original-toolchain/W3 builds and focused
whole-function graph/RET checks pass. Case-end full audit retains1306 paired
graphs and1306 accepted source/runtime extents on each side, plus109 scoped RET
contracts. Global RET remains unresolved at finish_debugging's indirect tail;
other graph/extent obligations stay strict. No new CRT/shim or assembly ABI
exception was needed. Initially unindexed clone_descriptor remained a named
trap until its own fresh frontier. Earlier failed compiler probes retained static
imported-pointer initializer rejection, reserved assembler formal spelling, and
fatal missing logical listing source. A successful smaller source reached only
three helpers; the coherent public/Oa expansion supplied two more families.
Descriptor-field/stale-frontier authoring failures were corrected before tests.
All raw failed and successful evidence remains in owned scratch.

Use `make -j4 native win32` for native/original builds. During a case use only
current-case replay, necessary builds and focused checks; full gates/README/
commit follow completion. The final `make -j4 audit check unit` took2.019s,
phases unsplit. Cached adapter IO took41.080ms: setup14.094/execution22.187/
checks4.799; its nine-unit run took38.002ms: setup9.207/execution24.959/checks3.835.
Across all measured attempts including failures/repeats: native/canonical/case
builds30.023s, registry/runtime preparation3.642s, W3 commands6.025s, untouched
CL7.958s, original transport/reference/frontier captures14.027s, focused graph/
RET0.329s, native current-case replays0.382s, units0.131s and expected rejection
0.051s. Phases can overlap and are not additive task elapsed. Analysis/editing,
observer compilation and capture authoring are unmeasured. Raw evidence and
completion/timing summaries: `~/.ai-tmp/msvc2/vc1.0/c2/dll-adapter/`.

Earlier cases include calibration(3960B object/five exact warnings,12App/1448B),
sparse numeric interpretation(6083B object,6App/1121B), tiled initialization/
checksum(4796B object,12App/2233B), protected floating filter(5273B object),
C++ ledger(7810B object/18463B listing), text-record assembler(3102B object) and
SEH packet(3723B object). Calibration pins the actual consumed6019B C23.ERR:
`c0246e0cc265b95a932d82e3d2b4e2f7b42ad0f6c1aca2011524a32ab13cf1e5`.
No diagnostic text is embedded in App source. Far-pointer lowering's deliberate
internal-error stubs remain unforced; other unvisited App children and unsupported
OMF alignment/public-kind paths remain named traps.

Original eight-byte name POD returns use per-object `-freg-struct-return` for
the App owner and narrow harness access; cases inherit binary flags. `src/`
contains App reconstruction, `crt/runtime.c` the five older justified CRT
overrides and `harness/` test/native glue. Generic tooling and shared shims
remain outside this binary. Migration's historical unaccepted SEH snapshot and
invocation remain under `~/.ai-tmp/msvc2/vc1.0/c2/standardization/before/` and
`paused-wip.patch`; superseded proof trees remain in Git history.
