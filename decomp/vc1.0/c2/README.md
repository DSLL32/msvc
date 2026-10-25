# VC1.0 C2 reconstruction

Checkpoint 1364 completes the C++ ledger/checksum case. There are 1249
reconstructed application functions and five historical handwritten CRT overrides.
Application code occupies 339421/399693 original code-section bytes (84.9204%);
including 916 distinct owned text-data bytes, its accepted extent union is
340337/399693 (85.1496%). These are byte extents, not branch coverage. The five
CRT overrides contribute 1776 bytes separately and receive zero application credit.
The union of available App, classified CRT and import boundaries is 366711 bytes
(91.7482%). A reliable non-CRT denominator remains unavailable; 109 pending App
rows and 32982 bytes outside that available union still need investigation.

`binary.c` is the sole function registry. All 19 cached IO cases complete, with
113 narrow unit tests. Native fixtures retain actual frontend IR, the sole
MSC_CMD_FLAGS transport, stable S cwd, explicit first-entry seeds and complete
object/listing/PDB oracles. Only established COFF timestamp bytes 4..8 are
normalized; OMF bytes are exact. Fixtures do not run Wine.

The ledger source uses untouched CL `/O2 /Ob2 /G5 /Oi /Z7 /FAcs`. It combines
nested class methods, constructor/destructor/operator names, repeated small POD
returns and inline calls, enum constants, external callbacks and volatile sinks.
The original listing proves inline bodies and real qualified names; the source
has no header dependencies. A shared read-only observation retained actual
C1XX-produced files and C2 transport at CreateProcessA. Parent CL exit was not
observed in that support capture; an ordinary untouched CL baseline and separate
standalone C2 run both succeeded. The fixture preserves the actual intermediate
prefix and all five channels. The standalone 7810-byte object matches the full
CL object except its timestamp; the 18463-byte listing is byte-identical.
Normalized object SHA256:
`3c02087e75ae8397a11d08b915cf317882785da308b674960b0fe08e09c7aedb`.
Exact listing SHA256:
`e348e1aaf59431150d15c752eead650e96f02cf5975ec6a3d549ac5ea7237f52`.

Eight whole App functions add 1433 code bytes: demangled listing output, token
parsing, parameter-register assignment, counted constant payload input, typed
temporary interning, high-DWORD operands, CodeView constant records and numeric
leaves. The numeric encoder was discovered through the real writer's call.
Live owner/cursor inputs and cache/scope/reference identity are checked alongside
cold byte, width, wrap and CodeView boundary controls. The original kind8
constant descriptor is exactly 50 bytes. Unsupported diagnostic paths still
reach named traps; their success is not claimed. The predicted secondary-return
register helper was not reached and remains unimplemented.

The earlier text-record case retains its complete 3102-byte object and original
memcmp/strcat selection behavior. Its string-literal name uses early failed
undecoration; ledger now exercises the formerly trapped token parser with real
qualified names. The tail-label lookup remains pending. The earlier SEH case
retains its complete 3723-byte object and scope/owner/frame-range checks.
Original eight-byte name POD returns still use per-object `-freg-struct-return`
for their App owner and narrow native test access in `harness/`; case objects
inherit common flags. No new assembly bridge or shared shim was needed.

Use `make -j4 native win32` for native and original-toolchain builds. During a
case, run its replay and relevant narrow controls; full regression/audit and
commits follow case completion. The completed-case `make -j4 audit check unit`
passes 1254 paired graphs, 1254 source/runtime extents per side and 57 scoped RET
contracts. Global RET comparison remains unresolved for finish_debugging's
indirect tail; its uncertainty does not waive scoped contracts or other checks.
Both independently corrupted object/listing expectations reject on their actual
file bytes. No additional output normalization was introduced.

The completion batch took 2.014 seconds including case relinks, audit and native
fixtures; its phases were not individually timed. Cached ledger IO took37.072ms:
setup15.055, execution17.738, checks4.280. Its14-unit command took42.164ms:
setup7.786, execution30.443, checks3.935. Fresh original completion took0.770s;
paired native command0.063s (setup14.029/execution18.174/checks4.378ms internally).
Across all measured attempts, combined native/canonical builds took128.369s,
descriptor/registry/runtime preparation23.145s, original compiler/W3 commands
12.760s, untouched CL baseline1.802s, original captures14.462s, eight focused
graph/RET checks0.497s, native case replays0.518s, unit commands0.408s, GDB0.404s
and two intentionally rejecting oracle controls0.091s. These totals include
failed/repeated attempts and may overlap; they are not additive task elapsed time.
Shared header changes triggered several full canonical/native rebuilds. Analysis,
editing, initial unwrapped native build and small observer compiles were not
separately measured. Full output/timing records remain in
`~/.ai-tmp/msvc2/vc1.0/c2/ledger/`; older case work remains in its own scratch paths.

`src/` contains App reconstruction; `crt/runtime.c` contains only the five older
justified CRT overrides, unchanged by this case. Shared shims and generic
execution/capture/audit machinery remain outside the binary. The observed OMF
alignment-8 missing-return boundary uses the same native symbol with recovered
EAX/ECX regcall inputs; other nonstandard alignments still trap. OMF public kinds
outside 1/3/4 also remain unsupported. Migration's historical paused SEH snapshot
and invocation remain under `~/.ai-tmp/msvc2/vc1.0/c2/standardization/before/` and
`paused-wip.patch`. Superseded proof trees remain in Git history, not fixtures.
