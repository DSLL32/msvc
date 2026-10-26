# VC1.0 C2 reconstruction

Checkpoint1438 completes the public fixed-stride verifier/checksummer case.
There are1323 reconstructed App functions and five unchanged historical CRT
bodies. App code occupies350472/399693 original code-section bytes(87.6853%);
adding916 owned text-data bytes gives351388/399693(87.9145%). These clipped
extent unions use section VirtualSize(raw size only if zero), not branch
coverage. Five historical CRT overrides contribute1776B separately, zero App
credit. Available App/classified CRT/import extents cover377762/399693(94.5130%).
A reliable non-CRT denominator remains unavailable;55 pending App rows and21931B
outside available extents need investigation. No only-CRT claim is made.

`binary.c` is the sole registry. All28 cached IO cases plus one INTERNAL DIRECT
case and225 narrow units pass. Native fixtures never run Wine. Inputs preserve
actual frontend IR, child MSC_CMD_FLAGS, stable S cwd and explicit entry seeds.
Only established COFF timestamp bytes4..8 are normalized; listings/OMF are exact.

Untouched CL `/O2 /Ob0 /G5 /FAc /Z7 /W3 verify.c` compiles a3756B fixed-stride
sample-table verifier with callback-dependent values, bounded signed narrowing,
shift/mask fields, affine indices and disjoint-storage contracts. Actual001232
ex/sy/in/gl/db inputs are4024/1512/24/423/403B. Read-only CL observation stops
before C2, parent exit unobserved. Untouched CL completion and standalone C2
produce matching7154B object and17593B listing, empty streams/status0. No source
revision or private state was needed. Normalized object SHA256:
`ac56554dcdba83c456ba81667c4c5b99694df996013be5085dbd493daa1a5923`. Listing SHA256:
`31874b8444ca8ea227ad42cbd4eadf2c96274c53502e7ff0a74045b8dd22ffc2`.

Six whole App functions add424B: sign-bit lookup8, ordinary line listing44,
conversion-owner exchange33, Pentium gap fill50, comparison-branch retarget219
and multiplication-overflow safety70. All have fresh original/native matching,
canonical compilation and focused graph/RET checks. The actual conversion
34/type102 around E/type104 becomes E/type102, retaining both owners. The
branch case has loadNULL/mode0 and reversed conditions4/5: a real label is
inserted after the destination branch, with its owner chain and reference count
preserved. A separate cold unit checks compatible-label reuse without insertion.

The overflow predicate receives actual arguments16,4,unsigned1,width4. Original
719b8..719fe tests byte/word MUL/IMUL overflow but returns0 for dword operands;
this limitation is deliberately retained. Cold controls cover signed/unsigned
byte/word boundaries and the dword limit. Shared exact shipped-library matching
finds no overlapping runtime body; the current App classification also rests on
its compiler-specific representation contract and induction-analysis caller,
not its provisional name. No new CRT override or shared shim was introduced.

Full `make -j4 audit check unit` passes in2.043s(phases unsplit), including1328
paired graphs/accepted extents and132 scoped RET contracts. Global RET remains
unresolved at finish_debugging's indirect tail. A changed listing hash rejects.
The final concurrent IO takes37.071ms: setup12.362/execution19.819/checks4.889;
five units take33.793ms: setup8.610/execution21.378/checks3.805. All measured
attempts: combined builds21.381s, registry/runtime/case3.668s, W3 six-source
compile8.119s, untouched CL2.336s, CL transport1.822s(including two prelaunch cwd
conflicts), baseline0.475s(including one prelaunch authoring failure), fresh
masked originals7.629s, original input/return observations2.077s, focused graph/
RET0.340s, runtime identity0.321s, native pairs0.418s, GDB0.440s, units/build0.660s
(including a missing test prototype), rejection0.047s. GDB retained successful
state observations before deliberately stopping at the next trap; its teardown
SIGKILL is diagnostic completion, not fixture success. Two small descriptor-only
builds, analysis/editing/authoring and scratch observer compilation are unmeasured.
Combined phases are unsplit and overlapping intervals are not additive elapsed.
Raw references and completion/time/coverage summaries remain under
`~/.ai-tmp/msvc2/vc1.0/c2/stride-verifier/`.

Use `make -j4 native win32`. During a case use current-case replay, necessary
builds and targeted checks; full gates, README and commit follow completion.

The preceding region-query case adds3App/249B with internal-DIRECT reach only.
The new DIRECT case borrows genuine compiler region owners at the decoder's
first eviction lookup(RVA3b210), without private-state changes or fabricated
graphs. Untouched CL `/O2 /Ob0 /Oa /Oi /G5 /Z7 /W3 decode.c` supplies actual
6894B source and000844 ex/sy/in/gl/db inputs(9634/2046/9/221/457B). Read-only
CL observation stops before C2; parent exit is unobserved. Separate CLI probes
complete but reach only the pending50B Pentium gap helper, so they remain
exploration rather than a frozen tiny CLI case. This fixture makes no claim
that ordinary compilation reaches the three query entries.

Three whole App functions add249B: register_has_region_uses105B,
machine_region_is_ancestor38B and find_register_preservation_region106B.
Fresh masked original/native frontiers qualify each; original-toolchain/W3
compilation and focused graph/RET0 checks pass. At the real anchor CX=402f,
region flags50, three live owners supply236B of scalar/relationship results.
Checks include null lists, strict ancestry/self exclusion and16 actual existing
summary IDs, including a positive preservation owner for4024. Returned owners
are compared by actual-chain indices; before/after snapshots check defined
fields and aliases within each process, skipping padding/uninitialized fields.

The original typed probe terminates abruptly with an empty decode.obj and no
streams. Native completion explicitly publishes its status then uses `_exit`,
matching that boundary. An initial native attempt returned normally and App
cleanup deleted the incomplete object; its failed receipt remains intact.
The corrected run reuses the unchanged original. Altering one expected ancestor
result rejects with125. The fixture retains the actual empty artifact assertion
and strict unexpected-output check; it does not claim a completed compilation.

Final `make -j4 audit check unit` passes in2.063s(phases unsplit), including1322
paired graph/accepted extent checks and126 scoped RET contracts. Global RET
remains unresolved at finish_debugging's indirect tail. The current DIRECT run
takes47.330ms: setup15.507/execution27.294/checks4.530. All measured query attempts,
including the failed cleanup run: combined builds17.341s, registry/runtime2.022s,
original helpers12.357s(including unused initial variant), original references/
frontiers6.510s, focused graph/RET0.172s, native replays0.385s, original W3
compile2.448s, intentional rejection0.054s. Earlier decoder exploration costs
4.452s untouched CL plus11.912s original transport/reference/observations.
Command phases overlap and are not additive task elapsed; analysis/editing/
authoring/observer compilation are unmeasured. Raw evidence and completion/time/
coverage summaries remain under `~/.ai-tmp/msvc2/vc1.0/c2/adaptive-packets/`.
Use `make -j4 native win32`; the typed original probe additionally builds with
`make cases/0027-internal-region-queries/.build/original-direct.dll
ORIGINAL_DIRECT_FLAGS="--include ../../../lib/recon/include"` on one shell line.
During cases use current-case replay/builds/targeted checks; full gates, README
and commit follow completion.

The metric reducer uses untouched CL `/O2 /Ob2 /Oi /G5 /Oy- /Z7 /W3`. It combines
2x2 projection, iterative bounded state, callback-preserved noncommutative values,
integer confidence weights, window validation and early diagnostic returns.
The initial4495B draft reached one pending save; the coherent6206B expansion also
reaches a positive floating branch exit. Actual000680 ex/sy/in/gl/db sizes are
6853/2062/108/304/441B. Read-only CL observation preserves genuine transport and
stops before C2, with parent exit unobserved. Separate untouched CL completion
and standalone original C2 both produce the same8178B object, normalized SHA256:
`04aff2c092b9837d7ef03185095dd698dc9208dc42de5fcddcb85be901411066`.
Streams are empty/status0; the object is the sole output. No private flags or
state were forced. The other proposed motion/region paths remain hypotheses.

Three whole App routines add1044 code bytes: resident x87 save339, direct branch
exit46 and its newly discovered exit-store child659. All have fresh masked
original/native qualification, original-compiler builds and focused graph/RET
checks. The save receives four actual arguments, with number61, resident slot0,
width8, homeNULL and flags1c; both sides create its declaration home and insert
one FST, retaining reload and changing flags to1e. Address owners are compared
by defined contents and copy ownership, not unrelated pointer values. The
80-bit spill body is reconstructed but not claimed executed by this case.

The actual branch exit has contextdepth0 and one width8/slot0 resident use with
flags6c and an EBP backing operand. It inserts a real FSTP at the existing target
without changing the resident list/context. A separately labelled cold control
checks the original one-result rule: select the oldest through previous_selected,
exchange it and store only that value. This differs from the full context-drain
routine and is deliberately preserved. Virtual-address register-use bookkeeping
keeps its unvisited child named/trapping. No shared API or CRT work was needed.

One live-state test initially assumed a copied address retained pointer identity;
its corrected assertion checks distinct owned copies with equal defined fields.
The first case-end batch also exposed manually authored expected DATA retaining
a raw timestamp while declaring the existing zero range. Expected clock bytes
are now zero; raw original captures remain untouched. The selected original's
normalized hash already matched. An altered nonclock object byte100 rejects.
No target/output compensation or additional normalization was introduced.

The final full audit checks1319 paired graphs/accepted source-runtime extents on
each side and123 scoped RET contracts. Global RET remains unresolved at
finish_debugging's indirect tail; other obligations remain strict. Use
`make -j4 native win32`. During a case use current-case replay, necessary builds
and focused checks; full gates/README/commit follow completion.

Final `make -j4 audit check unit` took2.445s, phases unsplit. Concurrent metric IO
took55.460ms: setup14.289/execution37.081/checks4.090; its4 units92.432ms:
setup9.095/execution79.470/checks3.867. All measured attempts, including failures
and repeats: combined native/canonical/case builds12.577s, registry/runtime/case
builds2.170s, W3 compile2.288s, untouched CL4.199s, original transport/reference/
observation/frontier captures11.951s, focused graph/RET0.169s, native replays0.386s,
units0.211s, GDB0.657s and expected object rejection0.064s. Both case-end batches
sum4.540s including the failed expected-timestamp batch. Phases overlap and are
not additive task elapsed; analysis/editing/authoring/observer compilation are
unmeasured. Raw evidence/completion/timing summaries remain under
`~/.ai-tmp/msvc2/vc1.0/c2/metric-reducer/`.

The preceding resampler completes3828B object and adds10App/1469B. Its full
object oracle caught an older reversed-transfer assignment: original37b2d..37b40
selects FCOMP to inherit the removed store's pop; the incorrect FCOM differed at
only object byte1005. Actual six-argument original observation and corrected
native watchpoint prove comparison15C80→15D80, store=first=last and reversed1.
The distinct legitimate combine_float_store_load FCOM remains unchanged. The
full resampler object is the durable actual-path regression; its constructed
pop-transition control is cold. Its raw DoubleBits reader uses the proven8B
EAX:EDX POD ABI; 12B Extended remains hidden-result/RET4. First merge/reuse checks
on that case reject, whereas this metric case reaches a real positive exit.

The preceding DLL adapter uses public/Oa with a disjoint-storage contract,
imports, real inline assembly/inlining and an available generated listing
sidecar. It completes3378B object plus16887B exact listing, adding6App/415B.
Earlier failed imported-pointer initializer and absent logical listing-source
probes remain failures; no missing-source fallback is claimed.

Earlier cases include calibration(3960B object/five exact warnings,12App/1448B),
sparse numeric interpretation(6083B object,6App/1121B), tiled initialization/
checksum(4796B object,12App/2233B), protected floating filter(5273B object),
C++ ledger(7810B object/18463B listing), text-record assembler(3102B object) and
SEH packet(3723B object). Calibration pins the actual consumed6019B C23.ERR:
`c0246e0cc265b95a932d82e3d2b4e2f7b42ad0f6c1aca2011524a32ab13cf1e5`.
No diagnostic text is embedded in App source. Far-pointer lowering's deliberate
internal-error stubs remain unforced; other unvisited App children and unsupported
OMF alignment/public-kind paths remain named traps.

Original eight-byte name POD returns and raw DoubleBits use per-object
`-freg-struct-return` for complete App caller/definition and narrow harness
closure; cases inherit binary flags. Twelve-byte Extended keeps its ordinary
hidden result pointer. `src/`
contains App reconstruction, `crt/runtime.c` the five older justified CRT
overrides and `harness/` test/native glue. Generic tooling and shared shims
remain outside this binary. Migration's historical unaccepted SEH snapshot and
invocation remain under `~/.ai-tmp/msvc2/vc1.0/c2/standardization/before/` and
`paused-wip.patch`; superseded proof trees remain in Git history.
