# VC1.0 C2 reconstruction

Checkpoint 1383 completes the tiled integer record initializer/checksum case.
There are 1268 reconstructed App functions and five unchanged historical
handwritten CRT overrides. App code occupies 343678/399693 original code-section
bytes (85.9855%); adding 916 distinct owned text-data bytes gives an accepted App
extent union of 344594/399693 (86.2147%). These are unions clipped to original
code-section VirtualSize (raw size only if zero), not branch coverage. The five
CRT overrides contribute 1776 bytes separately and receive zero App credit.
The available App, classified CRT and import union is 370968/399693 (92.8132%).
A reliable non-CRT denominator remains unavailable; 97 pending App rows and
28725 bytes outside that available union still need investigation. No only-CRT
remainder claim is made.

`binary.c` is the sole registry. All 21 cached IO cases complete and 142 narrow
units pass. Fixtures retain actual frontend IR, the single MSC_CMD_FLAGS child
transport, stable S cwd, explicit first-entry seeds and complete object/listing/
PDB oracles. Only established COFF timestamp bytes 4..8 are normalized; OMF bytes
are exact. Native fixtures do not run Wine.

The tiled-record source uses untouched CL `/O2 /Ob0 /G5 /Oi /Z7`. It combines
bulk fills/copies, nested record initialization, invariant integer constants and
dictionary addresses, checksums and an external readiness callback. Read-only
shared DIRECT observation retained the actual five C1 channels and C2 invocation
before CL started the backend. That support capture does not claim parent CL
completion; an ordinary untouched CL baseline and independent standalone C2
both succeed. Source 2902B has no headers; actual ex/sy/in/gl/db inputs are
2861/875/15/370/369B. The fixture keeps observed prefix 001848 and exact child
environment/cwd. The complete 4796B object matches CL and native reconstruction.
Normalized SHA256:
`e14e87a75e4f42d478f8883aab22047d75b110342fc4d9a7dd91b90aa61f6396`.
Streams are empty, status 0 and the object is the sole generated output.

Twelve whole App functions add 2233 code bytes: bulk-memory insertion (788),
its separate copy-tree tail wrapper (5), region cleanup (106), block removal
(119), work-edge unlinking (125), optional edge removal (41), machine-region
entry preparation (183), insertion cursor preparation (74), rematerialization
range recording (202), region-register sets (358), deferred-definition
replacement (97) and recursive complement extraction (135). Fresh masked-original/
native evidence reaches all twelve in order and completes. Original compiler
builds, W3 compiles and individual graph/RET checks pass. The shared generation
counter remains one actual storage owner used by both selector and range recorder;
WorkItem and machine-header aliases remain intact. No new CRT body, shim or
assembly bridge was added.

The first machine-region preparation takes its unchanged reference-count exit.
Its unvisited label-retargeting child remains a named trap. Actual inputs,
result nodes, list lifetimes and owner aliases are checked through typed native
entry hooks against bounded original observations. Cold controls cover optional
edge absence, list splicing, marker walks, register add/removal with a preserved
definition, and complement search/removal. Unvisited floating fill and diagnostic
children remain traps; these controls do not claim execution of those branches.

Use `make -j4 native win32` for native and original-toolchain builds. During a
case use current-case replay, necessary builds and focused checks; complete
regression/audit and commits follow case completion. The completed-case
`make -j4 audit check unit` passes 1273 paired graphs, 1273 accepted source/runtime
extents on each side and 76 scoped RET contracts. Global RET comparison remains
unresolved for finish_debugging's indirect tail; other checks are not waived.
An independently changed byte100 object expectation rejects on its artifact hash.

The case-end combined audit/full-fixture command took 1.994s, with phases unsplit.
Cached tiled IO took 31.512ms: setup13.490/execution12.372/checks5.650; its16-unit
run took59.205ms: setup7.942/execution47.609/checks3.654. Across all measured
attempts, native/canonical/case builds took76.307s, registry/runtime preparation
14.075s, W3 commands15.132s, untouched CL2.264s, original captures/transport18.024s,
focused graph/RET0.746s, native replays0.829s, units0.520s and GDB diagnostics0.425s.
The private Ghidra analysis attempts took78.611s: the first exited0 despite import
failure, and the second timed out general analysis but produced six selected
function analyses. Generated decompiler output was analysis only. Totals include
failed/repeated commands and can overlap; they are not additive task elapsed.
Analysis/editing, observer compilation and fixture authoring were not separately
measured. Raw evidence, timing and completion summaries remain under
`~/.ai-tmp/msvc2/vc1.0/c2/tiled-records/`.

Failed attempts are retained there: a wrong Node+0x10 interpretation that crashed
after enabling the tail wrapper; an optional flow remover incorrectly associated
with a mandatory remover, caught by the next original trap; two test-only missing
field/prototype declarations; and the final raw timestamp hash mismatch. The last
was resolved by binding the existing timestamp comparison rule to retained fresh
raw output; all4796 object bytes already matched outside that clock field.

Earlier completed cases include the protected floating filter (5273B object),
C++ ledger (7810B object/18463B listing), text-record assembler (3102B object) and
SEH packet (3723B object). Original eight-byte name POD returns use per-object
`-freg-struct-return` for the App owner and narrow harness access; cases inherit
binary flags. `src/` contains App reconstruction, `crt/runtime.c` the five older
justified CRT overrides and `harness/` test/native glue. Generic tooling and shared
shims remain outside this binary. OMF alignment8 retains recovered EAX/ECX inputs
on its native symbol; other nonstandard alignments and unsupported OMF public
kinds still trap. Migration's historical unaccepted SEH snapshot and invocation
remain under `~/.ai-tmp/msvc2/vc1.0/c2/standardization/before/` and
`paused-wip.patch`; superseded proof trees remain in Git history.
