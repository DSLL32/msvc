Direct assembler support callback tests
=======================================
These two generated helpers have no public source path in this original
binary: its insertion list is empty and read-only. This is direct code-level
validation, not evidence that any C source reaches the helpers. The original
empty-table proof remains in cases/asm-parser-repair/evidence/.

Each of15units initializes the unchanged real source to the accepted operand
error marker. Debuggers then call one helper with valid bounded token inputs
on a separately allocated private stack; no compiler global/table is changed.
Fresh masked entries agree on inputs before whole-body reconstruction.
Original masked images are independently checked against the accepted-prefix
plus single candidate recipe before final cache freeze.

The diagnostic helper is81bytes; the semantic-value helper is61bytes plus
44bytes of switch data. It returns EDX:EAX/RET4, with only one original-
written semantic word defined for each token. Actual word masks come from
raw stores/table/caller proof. Unwritten words remain raw and unspecified.
Known type pointers are checked by within-process owner identity. Original
Win32 signatures remain unchanged; native RETURN8 transport is confined to
this helper and its sole assembler-parser caller.

All46 public native phases and15direct units pass, plus69public rejection
checks and7direct rejections/one unwritten-word positive control. Original
compiler, Rust graph/ABI/reference/extent/ownership, state/alias and both
grammar audits pass. Native fixture execution never invokes Wine.

Run make -j4 native-direct-check, or normal check for both suites. Adoption
checks include direct result controls. Original-only captures stay explicit.
validation.json preserves initial setup failures, superseded observer repeat,
raw actual observations, stable compiled inputs and split measured times.
Public-case extent union stays239135; direct-inclusive union239321/277234
(86.324549%). No instruction/branch coverage or completion claim.
