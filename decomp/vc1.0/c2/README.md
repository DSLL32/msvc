# VC1.0 C2 reconstruction

Checkpoint1349 completes the structured-exception case. There are1234
reconstructed application functions and five historical handwritten CRT overrides.
Application code occupies335361/399693 original code-section bytes (83.9046%);
including916 distinct owned text-data bytes, its accepted extent union is
336277/399693 (84.1338%). These are byte extents, not branch coverage. The five
CRT overrides contribute1776 bytes separately and receive zero application credit.
The union of available App, classified CRT and import boundaries is362651 bytes
(90.7324%). A reliable non-CRT denominator remains unavailable;119 pending App
rows and37042 bytes outside that available union still need investigation.

`binary.c` is the sole function registry. All17 cached IO cases now complete,
with82 narrow unit tests. Native fixtures retain exact frontend IR, the sole
MSC_CMD_FLAGS transport, stable S cwd, explicit first-entry seeds and complete
object/listing/PDB oracles. Only established COFF timestamp bytes4..8 are
normalized; OMF bytes are exact. Fixtures do not run Wine.

The SEH source uses untouched CL `/O2 /Ob0 /G3 /Z7` transport, with five actual
backend IR channels and the source file. Its3723-byte object matches normalized
SHA256 `9a41bdaf2a709cd7a76d71a3e6fda69213daecd459efc395be945395eb79fc8e`,
with exit0, empty streams and no unexpected outputs. Reconstruction covers scope
ownership and unwinding, protected flow/register exclusion, i386 exception-record
selection, scope-table initializers and frame ranges. Real-input typed entry
checks retain defined fields and ownership aliases; independent cold controls do
not imply that every branch was executed by this source. The frame-range length
accumulator and the16-byte comparison-condition map preserve their original
nonobvious behavior.

Use `make -j4 native win32` for native and original-toolchain builds. During a
case, run its replay and relevant narrow controls; full regression/audit and
commits follow case completion. The completion run of `make -j4 audit check unit`
passes1239 paired graphs,1239 source/runtime extents and42 scoped RET contracts.
Global RET comparison remains unresolved for finish_debugging's indirect tail;
its uncertainty does not waive the scoped contracts or other graph checks.
The full completion batch took2.511s, unsplit. The final native SEH fixture took
21.182ms: setup12.244ms, execution4.991ms, checks3.947ms. Fresh original completion
took1.228s and its paired native command.054s. Original `/W3` compilation of the
final changed module took1.269s. Earlier builds, bounded live-input observations,
failed test-glue attempts and their split timings remain under
`~/.ai-tmp/msvc2/vc1.0/c2/seh-resume/`; analysis time was not separately measured.

`src/` contains App reconstruction; `crt/runtime.c` contains only the five older
justified CRT overrides, unchanged by this case. Shared shims and generic
execution/capture/audit machinery remain outside the binary. The observed OMF
alignment8 missing-return boundary uses the same native symbol with recovered
EAX/ECX regcall inputs; other nonstandard alignments still trap. OMF public kinds
outside1/3/4 also remain unsupported. Migration's former paused SEH candidate is
now reconstructed; the unchanged historical snapshot and invocation remain under
`~/.ai-tmp/msvc2/vc1.0/c2/standardization/before/` and `paused-wip.patch`.
Superseded proof trees remain in Git history rather than fixture payloads.
