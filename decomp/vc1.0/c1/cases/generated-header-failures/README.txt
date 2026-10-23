Generated-header ingestion failures
===================================
One audit.c completes compiled_prefix before including generated.h. The first
phase omits that header and expects C1083; the second supplies the captured
header ending inside PAIR(7, and expects C1057. Untouched CL /FRaudit.sbr /Z7
/Od /G5 /W4 /c exits2; each actual C1 child exits1. Two independent child
transports agree on five complete partial IR files and exact fatal diagnostics.
The browser temporary and final output are absent after fatal shutdown.

Fixtures execute native C1 only, with the exact sole MSC_CMD_FLAGS environment
and original CL descriptor alias. The missing header is explicitly absent,
not synthesized by a process double. No backend invocation, program runtime
or recovery-tail completion is claimed. Original observation separately
proves compiled_prefix reaches and returns from function finalization.

Status: complete at992 accepted entries. Six whole frontend functions add
433bytes; identified _strerror CRT extent adds224bytes. All44native phases,
61 oracle rejection tests, strict originalcompiler/Rust graph/ABI/reference/
ownership/extent checks,290state/default/alias checks and both grammars pass.
validation.json preserves the failed remove/_unlink graph audit, fixedcall
identity, full128byte static _strerror proof and stable finalsource relink.
