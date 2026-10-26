# VC1.0 C2 reconstruction

Checkpoint 1401 completes the annotated fixed-point calibration case.
There are 1286 reconstructed App functions and five unchanged historical
handwritten CRT overrides. App code occupies346247/399693 original code-section
bytes (86.6282%); adding916 distinct owned text-data bytes gives an accepted App
extent union of347163/399693 (86.8574%). These are unions clipped to original
code-section VirtualSize (raw size only if zero), not branch coverage. The five
CRT overrides contribute1776 bytes separately and receive zero App credit.
The available App, classified CRT and import union is373537/399693 (93.4560%).
A reliable non-CRT denominator remains unavailable;88 pending App rows and26156
bytes outside that available union still need investigation. No only-CRT
remainder claim is made.

`binary.c` is the sole registry. All23 cached IO cases complete and169 narrow
units pass. Fixtures retain actual frontend IR, single MSC_CMD_FLAGS child
transport, stable S cwd, explicit first-entry seeds and complete object/listing/
PDB oracles. Only established COFF timestamp bytes4..8 are normalized; OMF bytes
are exact. Native fixtures do not run Wine.

Calibration uses untouched CL `/O2 /Ob0 /G5 /Z7 /W4`. It combines bounded unsigned
fixed-point records, checksum loops and callbacks with genuine warning pragmas
(once, disable, default and level4) and a user comment. Read-only shared DIRECT
observation retained the actual five C1 channels and C2 invocation before CL
started the backend. That support capture does not claim parent CL completion;
an ordinary untouched CL baseline and independent standalone C2 both succeed.
Source2716B has no headers; actual ex/sy/in/gl/db inputs are2775/754/12/416/235B.
The fixture retains actual prefix001500 and exact child environment/cwd. Its
3960B object matches CL and native reconstruction. Normalized SHA256:
`232efaed7965b1253a67ba45145b518c7d52d6c0756291cd59e0e490c8d5201f`.
Status0 and empty stdout match; all326 stderr bytes, including five warnings
at lines20/22/39/51/53 and their CRLF endings, remain exact. The object is the
sole generated output.

The actual diagnostic reader opens `R:\toolchains\1.0-nt\bin\c23.err` in mode r;
its first fgets and resource bytes were observed directly. Native replay pins
6019-byte C23.ERR SHA256
`c0246e0cc265b95a932d82e3d2b4e2f7b42ad0f6c1aca2011524a32ab13cf1e5`.
No warning text is synthesized or embedded in App source. New contextual data
are the diagnostic FILE owner, shared384-byte line buffer and20 typed number/
file-offset pairs; existing warning and source-location owners are reused.

Twelve whole App functions add1448 code bytes: COFF comment writer(60), warning
setter(79), warning once-state change(35), diagnostic loader(125), cache lookup
(74), line reader(149), numeric parser(91), offset insertion(54), quoted-message
decoder(73), diagnostic emitter(394), variadic adapter(24) and private formatter
(290). The formatter is App code with its original c/s/d/x and F/l syntax,
not a CRT replacement. Original compiler/W3 builds and fresh masked-original/
native pairs pass. Actual formals, varargs consumed by the source-prefix format,
FILE state, buffer/cache ownership and results were observed and checked with
native typed entry hooks. All four original emitter callers use1000/2000/4000;
its undefined label for other nonzero categories is not given a new fallback.
The original valid-resource/no-EOF-recovery and installed-warning assumptions
are preserved. Failed-open, override-overflow and invalid-category paths are
not claimed as executed by this case.

Narrow controls cover first-match warning order, the fiftieth entry, nearest
cached offsets and distance cutoff, duplicate/full caches, numeric wrap,
quoted escapes, source aliases and private-format modifiers/literals/null
strings. The final audit retains1291 paired graphs,1291 accepted source/runtime
extents on each side and94 scoped RET contracts. Global RET comparison remains
unresolved for finish_debugging's indirect tail; other checks are not waived.
Changed warning stderr and an incorrect C23.ERR dependency hash both reject.
A focused graph check caught canonical /Od strlen/strcpy calls absent from the
original inline emitter; period intrinsic pragmas restore that original graph.
No shared shim, new CRT body or assembly bridge was needed.

Use `make -j4 native win32` for native and original-toolchain builds. During a
case use current-case replay, necessary builds and focused checks; complete
regression/audit, README updates and commits follow case completion. The case-end
`make -j4 audit check unit` took3.297s, phases unsplit. Cached calibration IO took
29.361ms: setup14.508/execution10.057/checks4.796; its19-unit run took46.501ms:
setup9.355/execution33.173/checks3.973. Across all measured attempts, native/
canonical/case builds took112.629s, registry/runtime preparation8.525s, W3 commands
19.166s, untouched CL1.904s, original captures/transport16.221s, focused graph/RET
0.736s, native replays0.742s, current-case units0.528s and expected rejection
controls0.041s. Totals include failed/repeated commands and overlap; they are
not additive task elapsed. Build time includes broad dependency invalidation.
Analysis/editing, observer compilation and capture authoring were not separately
measured. Raw evidence, timing and completion summaries remain under
`~/.ai-tmp/msvc2/vc1.0/c2/calibration/`.

Retained failures include an authoring assertion on Windows slash spelling/raw
CRLF, a missing-case build during preparation, a make invocation from the wrong
cwd, the detected graph mismatch and a descriptor rejection after the final
frontier became implemented but before it was changed to exit0. These are
separate from the passing final object/stream oracle; no mismatch was normalized
away. Far-pointer lowering's deliberate internal-error stubs remain unforced.
Other unvisited App children and unsupported OMF alignment/public-kind paths
remain named traps.

Earlier completed cases include sparse numeric interpretation(6083B object,
6App functions/1121B), tiled initialization/checksum(4796B object,12App/2233B),
protected floating filter(5273B object), C++ ledger(7810B object/18463B listing),
text-record assembler(3102B object) and SEH packet(3723B object). Original
eight-byte name POD returns use per-object `-freg-struct-return` for the App
owner and narrow harness access; cases inherit binary flags. `src/` contains App
reconstruction, `crt/runtime.c` the five older justified CRT overrides and
`harness/` test/native glue. Generic tooling and shared shims remain outside
this binary. Migration's historical unaccepted SEH snapshot and invocation
remain under `~/.ai-tmp/msvc2/vc1.0/c2/standardization/before/` and
`paused-wip.patch`; superseded proof trees remain in Git history.
