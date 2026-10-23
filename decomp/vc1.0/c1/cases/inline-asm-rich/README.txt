Substantial inline-assembly case
================================

Actual CL /nologo /c /Z7 /O2 /G5 /W3 asm-rich.c.
Scaled register addressing, member/array offsets, BYTE/WORD/DWORD/QWORD PTR,
OFFSET, TYPE/LENGTH/SIZE, x87 stack-register arithmetic and exchanges.
Original compiled and linked program exits0 while checking integer and floating
results. CL child transport and all raw frontend outputs are frozen before
reconstruction. Relative paths; no path, pointer or clock normalization.
Explicit original capture remains outside native fixtures.

Complete at887 accepted entries,760 source functions. Twelve new functions
add2594 frontend extent bytes; total225662/277234 (81.397664%), not branch
coverage. Whole original/native first-entry sequence and all five raw IR files
match. All25 native phases and11 meaningful oracle/dependency rejection checks
pass, plus six strict graph/ABI/mask checks. See validation.json and evidence/.
