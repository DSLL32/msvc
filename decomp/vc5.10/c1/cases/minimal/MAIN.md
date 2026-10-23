# Main and initialized state

`main` owns137 hot bytes127a7..12830 and27 cold bytes30420..3043b. Its first
callee is the named work-buffer initializer12830. Source keeps all later calls
as separate named pending functions and preserves the real Win32 APIs.

Initialization is validated at actual main entry, not against state legitimately
changed later by main. Original entry127a7 is independently trapped/snapshotted;
GDB records actual native preprologue PC, raw cdecl argc/argv, vector bytes/NUL,
formal inventory, all currently modeled compiler state and CRT/module/error/x87
state. Checks bind both caller CALL instructions and returns. The genuine helper
frontier12830 then proves main changed only compiler_phase to1. Original full
BSS/snapshot/globals and all other original initialized bytes remain identical;
all native modeled neighbors remain identical. Helper-as-entry snapshots reject.

The typed initialized projection now has52 encountered bytes: first_call,
compiler_phase, C9IL option, compiler_flags and SDK SYSTEM_INFO36. Exact canonical
defaults include page4096 and allocation granularity65536. Main operands and
option table entries establish roles; field-map.json records addresses/aliases.
All fields join the6548-byte allocation's typed snapshot/restore. Its other6496
bytes remain UNMODELED; they are never represented as a native blob or claimed
equivalent. Normal firstcopy compares the entire ORIGINAL snapshot with its
ORIGINAL source and the52 known native fields with their known defaults.

The labeled restore auxiliary uses distinct nonzero live/saved values for every
new field, including owned pointer sentinels. It proves typed restore and reset
without using those pointers in an API. Twelve commandline BSS bytes also gain
nonzero reset proof. Initializer proofs use actual entry snapshots; main's phase
write is checked separately. No new unknown semantic consumer is allowed past
the named frontier.

Shared DebugBreak is typed but deliberately unsupported125 for the unvisited
-debugBreak% branch; no exception behavior is claimed. Actual GetSystemInfo has
not been reached. Both original5.10 and native i386 builds pass, with full strict
function-graph/import/RET/ownership/extent auditing:53 source functions/2323B.

Fresh main/boundary controls50/65 and generic47/100 pass. Frozen proof, actual
raw identities, source-content snapshots and times are in
`evidence/main-first-helper-{normal,clear-aux}/acceptance-proof.json`.
A graph-tool-only publication produced a byte-identical report; current
validation references the new auditor without original or native recapture.
