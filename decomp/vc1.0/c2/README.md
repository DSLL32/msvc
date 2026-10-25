# VC1.0 C2 reconstruction

Checkpoint 1356 completes the text-record assembler case. There are 1241
reconstructed application functions and five historical handwritten CRT overrides.
Application code occupies 337988/399693 original code-section bytes (84.5619%);
including 916 distinct owned text-data bytes, its accepted extent union is
338904/399693 (84.7911%). These are byte extents, not branch coverage. The five
CRT overrides contribute 1776 bytes separately and receive zero application credit.
The union of available App, classified CRT and import boundaries is 365278 bytes
(91.3896%). A reliable non-CRT denominator remains unavailable; 116 pending App
rows and 34415 bytes outside that available union still need investigation.

`binary.c` is the sole function registry. All 18 cached IO cases complete, with
99 narrow unit tests. Native fixtures retain actual frontend IR, the sole
MSC_CMD_FLAGS transport, stable S cwd, explicit first-entry seeds and complete
object/listing/PDB oracles. Only established COFF timestamp bytes 4..8 are
normalized; OMF bytes are exact. Fixtures do not run Wine.

The text-record source uses untouched CL `/O2 /Ob0 /G5 /Oi /Z7`. A shared
read-only observation retained the actual C1-produced files and C2 invocation at
CreateProcessA; the parent CL exit was not observed. The fixture preserves the
actual absolute intermediate prefix and all five channels unchanged. Standalone
original C2 supplies its own successful status, streams and sole 3102-byte object.
The complete normalized object SHA256 is
`3ba363a288a0beb3975c4065aafed505d178847e99705b17e06a65d42b4f9e03`.
Seven whole App functions add 2627 code bytes: tail-position/address checks,
memcmp/strcat selection, a zero-result operand helper and name-decoding support.
The generated string-literal name takes the decoder's original early-failure
path; operator/truncation controls are separate cold coverage. Its token child
and the tail-label lookup remain named traps. Typed data checks independently
compare all 41 operator spellings and state initializers in the original PE,
canonical PE and native ELF. Native callers of the original eight-byte POD return
use per-object `-freg-struct-return`; narrow native test access lives in `harness/`.
Case objects retain the common compiler flags. No assembly bridge was needed.

The earlier SEH source uses untouched CL `/O2 /Ob0 /G3 /Z7` transport. Its
3723-byte object retains normalized SHA256
`9a41bdaf2a709cd7a76d71a3e6fda69213daecd459efc395be945395eb79fc8e`.
Scope ownership/unwinding, protected register flow, i386 exception selection,
scope-table initializers and frame ranges are reconstructed. Defined fields and
ownership aliases are checked separately from cold controls. The original frame
range accumulator and comparison-condition map retain their nonobvious behavior.

Use `make -j4 native win32` for native and original-toolchain builds. During a
case, run its replay and relevant narrow controls; full regression/audit and
commits follow case completion. The completed-case `make -j4 audit check unit`
passes 1246 paired graphs, 1246 source/runtime extents per side and 49 scoped RET
contracts. Global RET comparison remains unresolved for finish_debugging's
indirect tail; its uncertainty does not waive the scoped contracts or other checks.

The completion batch took 18.110 seconds including rebuilds, audit and fixtures;
its phases were not individually timed. The cached text-record IO fixture took
25.442 ms: setup 12.437, execution 9.100, checks 3.904. Fresh original completion
took 1.079 seconds and its native command 0.060 seconds. Across all measured case
work, combined native/canonical build attempts took 175.047 seconds, a native-only
build 6.368, original compiler/probe commands 14.211, original reference attempts
15.437, seven focused graph audits 0.430 and native debugger attempts 0.585.
These totals include failed/repeated attempts. Analysis/editing and unwrapped
small rebuilds were not separately measured. Full logs, ordered acceptance,
negative controls and timing details remain in
`~/.ai-tmp/msvc2/vc1.0/c2/textrec/`; the earlier SEH records remain in `seh-resume/`.

`src/` contains App reconstruction; `crt/runtime.c` contains only the five older
justified CRT overrides, unchanged by this case. Shared shims and generic
execution/capture/audit machinery remain outside the binary. The observed OMF
alignment-8 missing-return boundary uses the same native symbol with recovered
EAX/ECX regcall inputs; other nonstandard alignments still trap. OMF public kinds
outside 1/3/4 also remain unsupported. Migration's historical paused SEH snapshot
and invocation remain under `~/.ai-tmp/msvc2/vc1.0/c2/standardization/before/` and
`paused-wip.patch`. Superseded proof trees remain in Git history, not fixtures.
