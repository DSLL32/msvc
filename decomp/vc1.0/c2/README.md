# VC1.0 C2 reconstruction

Checkpoint 1410 completes the mixed-precision coefficient-bank/waveform case.
There are1295 reconstructed App functions and five unchanged historical
handwritten CRT overrides. App code occupies346871/399693 original code-section
bytes (86.7844%); adding916 distinct owned text-data bytes gives an accepted App
extent union of347787/399693 (87.0135%). These are unions clipped to original
code-section VirtualSize (raw size only if zero), not branch coverage. The five
CRT overrides contribute1776 bytes separately and receive zero App credit.
The available App, classified CRT and import union is374161/399693 (93.6121%).
A reliable non-CRT denominator remains unavailable;79 pending App rows and25532
bytes outside that available union still need investigation. No only-CRT
remainder claim is made.

`binary.c` is the sole registry. All24 cached IO cases complete and188 narrow
units pass. Fixtures retain actual frontend IR, single MSC_CMD_FLAGS child
transport, stable S cwd, explicit first-entry seeds and complete object/listing/
PDB oracles. Only established COFF timestamp bytes4..8 are normalized; OMF bytes
are exact. Native fixtures do not run Wine.

The waveform module uses untouched CL `/O2 /Ob2 /Oi /Op /G5 /Z7`. It combines
single-precision coefficient storage, double polynomial/predictor state, small
inlined coefficient helpers, constant unsigned conversion and fabs/minimum
folding, runtime-dependent samples, bounded output and unsigned control/checksum
lanes. Read-only shared DIRECT observation retained the actual five C1 channels
and C2 invocation before CL started the backend. That support capture does not
claim parent CL completion; an ordinary untouched CL baseline and independent
standalone C2 both succeed. Source4240B has no headers; actual ex/sy/in/gl/db
inputs are3793/1334/91/541/365B, prefix001344. Complete3303B object normalized
SHA256:`6082173f6f7412633973528d64742a508fcfa0dbf010302f076d2e15c24983b8`.
Native and original streams are empty, status0 and the object is the sole output.
A changed byte100 object expectation rejects; no new normalization was added.

Nine whole App functions add624 code bytes: raw float input(34),80-bit multiply
(79), divide(79), absolute value(76), unsigned conversion(51), minimum(102),
conversion to single(35), conversion from single(32) and machine narrowing
predicate(136). Actual arguments, defined output bytes, pointer aliases and
returns were observed in the original and checked with native typed entry
hooks. The first machine predicate takes the register-number4000 shortcut;
its full use-chain body has separate width/operand/dead-result controls, not
an execution-coverage claim for that first call. Register-number classes stay
distinct from numerically equal operand-property bits.

The existing intrinsic interpreter's explicit unknown-data trap caught newly
encountered fabs ID17. Only its contextual16-byte descriptor was added:
double result1008, unary recipe u, empty runtime name, flags20. Original node/
result alias observation and native unary-owner checks pass. No new function
or text-data credit is assigned for this descriptor.

Original-toolchain/W3 builds and fresh masked-original/native pairs pass.
Seven new arithmetic/conversion bodies use the established narrow exception
for80-bit x87 primitives and the private ESI/ST0 store/normalizer ABI, which
this compiler's C floating types cannot express. Tag selection and minimum
control remain C; no assembly algorithm or new wrapper vertex was introduced.
Focused controls cover operand order, signs, signed zero, mixed tags, unsigned
boundaries and encountered rounding. Raw-input NaN payload control tests byte
transport only; no floating-arithmetic NaN or universal environment equivalence
is claimed. The bounded overflow/conversion controls are separate from the real
finite coefficient inputs. Maximum and other unvisited helpers remain traps.

The final audit retains1300 paired graphs,1300 accepted source/runtime extents
on each side and103 scoped RET contracts. Global RET comparison remains
unresolved for finish_debugging's indirect tail; other checks are not waived.
No new CRT body or shared shim was needed. The first transport attempt retained
an obsolete TMP/TEMP parent path and correctly failed; only the intended parent
overlay was corrected before obtaining actual child transport. A cold test's
host tmpfile FILE ABI mismatch was replaced with supported binary fopen glue;
the real-input reader control already passed. Both failures and the intentional
intrinsic data trap/debugger stop remain in scratch evidence.

Use `make -j4 native win32` for native and original-toolchain builds. During a
case use current-case replay, necessary builds and focused checks; complete
regression/audit, README updates and commits follow case completion. The case-end
`make -j4 audit check unit` took2.037s, phases unsplit. Cached waveform IO took
35.448ms: setup17.354/execution13.342/checks4.752; its19-unit run took51.352ms:
setup9.027/execution37.838/checks4.487. Across all measured attempts, native/
canonical/case builds took35.520s, registry/runtime preparation11.673s, W3
commands13.422s, untouched CL0.937s, original captures/transport14.092s, focused
graph/RET0.569s, native replays0.567s, current-case units0.426s, native debugging
0.188s and expected rejection0.034s. Totals include failures/repeats and overlap;
they are not additive task elapsed. Analysis/editing, observer compilation and
capture authoring were not separately measured. Raw evidence, timing and
completion summaries remain under `~/.ai-tmp/msvc2/vc1.0/c2/mixed-wave/`.

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
