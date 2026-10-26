# VC1.0 C2 reconstruction

Checkpoint 1389 completes the sparse-opcode numeric packet interpreter case.
There are 1274 reconstructed App functions and five unchanged historical
handwritten CRT overrides. App code occupies 344799/399693 original code-section
bytes (86.2660%); adding916 distinct owned text-data bytes gives an accepted App
extent union of345715/399693 (86.4951%). These are unions clipped to original
code-section VirtualSize (raw size only if zero), not branch coverage. The five
CRT overrides contribute1776 bytes separately and receive zero App credit.
The available App, classified CRT and import union is372089/399693 (93.0937%).
A reliable non-CRT denominator remains unavailable;93 pending App rows and27604
bytes outside that available union still need investigation. No only-CRT
remainder claim is made.

`binary.c` is the sole registry. All22 cached IO cases complete and150 narrow
units pass. Fixtures retain actual frontend IR, single MSC_CMD_FLAGS child
transport, stable S cwd, explicit first-entry seeds and complete object/listing/
PDB oracles. Only established COFF timestamp bytes4..8 are normalized; OMF bytes
are exact. Native fixtures do not run Wine.

The numeric packet interpreter uses untouched CL `/O2 /Ob0 /G5 /Oi /Z7`.
It combines22 sparse numeric opcodes, eight floating state values, conditional
joins, external query/commit callbacks, checksum/address loops and side-effecting
short-circuit validation. Read-only shared DIRECT observation retained the actual
five C1 channels and C2 invocation before CL started the backend. That support
capture does not claim parent CL completion; an ordinary untouched CL baseline
and independent standalone C2 both succeed. Source4385B has no headers; actual
ex/sy/in/gl/db inputs are4778/1063/12/312/251B. The fixture retains actual prefix
002004 and exact child environment/cwd. The complete6083B object matches CL and
native reconstruction. Normalized SHA256:
`4efd18a9edb83ee7a4c63bf05228815876616f9a020e28629adaabee505e0eef`.
Streams are empty, status0 and the object is the sole generated output.

Six whole App functions add1121 code bytes: expression materialization(43),
sparse-switch splitting(56), dense-prefix selection(131), switch pivot creation
(190), stack-copy interval scanning(227) and its stack-write/ESP-motion child(474).
The two child functions were discovered from actual call edges and remain
individual ABI/graph vertices. Original compiler builds, W3 compiles and focused
per-function graph/RET checks pass. All actual formal inputs and relevant live
owners were observed read-only in the original and checked with native typed
entry hooks. No new CRT body, shim, assembly bridge or contextual data table
was needed. No new floating-branch helper reach is claimed from the source's
floating features.

Narrow controls distinguish dense prefixes from sparse fallback and the six-range
threshold, preserve selector/branch targets and expression-list ownership, and
check private stack-address copies, PUSH/POP/call/arithmetic adjustments, exposed
homes, touching/overlapping memory ranges, indexed addressing and label barriers.
The ordinary final audit retains1279 paired graphs,1279 accepted source/runtime
extents on each side and82 scoped RET contracts. Global RET comparison remains
unresolved for finish_debugging's indirect tail; other checks are not waived.
An independently changed byte100 object expectation rejects on its artifact hash.
Unvisited diagnostic children, floating fill conversion and unsupported OMF
alignment/public-kind paths remain named traps.

Use `make -j4 native win32` for native and original-toolchain builds. During a
case use current-case replay, necessary builds and focused checks; complete
regression/audit, README updates and commits follow case completion. The case-end
`make -j4 audit check unit` took2.438s, with phases unsplit. Cached interpreter IO
took38.029ms: setup12.071/execution21.970/checks3.988; its8-unit run took73.724ms:
setup8.945/execution60.727/checks4.052. Across all measured attempts, native/
canonical/case builds took30.384s, registry/runtime preparation3.562s, W3 commands
9.169s, untouched CL2.219s, original captures/transport10.642s, focused graph/RET
0.345s, native replays0.521s and units0.315s. Totals include failed/repeated
commands and can overlap; they are not additive task elapsed. Analysis/editing,
observer compilation and fixture authoring were not separately measured.
Raw evidence, timing and completion summaries remain under
`~/.ai-tmp/msvc2/vc1.0/c2/sparse-vm/`.

Retained preparation failures include an existing-cwd reservation rejection and
an attempted no-op timestamp-policy derivation. The final fresh capture already
inherited the correct established timestamp rule, so derivation correctly
rejected; pairing that unchanged capture/report directly passed. This was not
an object or reconstruction mismatch. There were no failed target builds or
native behavior controls in this case.

Earlier completed cases include tiled initialization/checksum(4796B object,
12App functions/2233B), protected floating filter(5273B object), C++ ledger(7810B
object/18463B listing), text-record assembler(3102B object) and SEH packet(3723B
object). Original eight-byte name POD returns use per-object
`-freg-struct-return` for the App owner and narrow harness access; cases inherit
binary flags. `src/` contains App reconstruction, `crt/runtime.c` the five older
justified CRT overrides and `harness/` test/native glue. Generic tooling and
shared shims remain outside this binary. Migration's historical unaccepted SEH
snapshot and invocation remain under
`~/.ai-tmp/msvc2/vc1.0/c2/standardization/before/` and `paused-wip.patch`;
superseded proof trees remain in Git history.
