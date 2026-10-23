Ordinary malformed-C diagnostics and parser recovery
===================================================

One /Z7 /Od /G5 /W3 source deliberately contains duplicate member, negative
array bound, conflicting basic type, missing initializer token, unresolved
object/function and too-few call arguments. Six errors and one warning occur
in a fixed original order. C1 exits 6; CL reports its separate driver exit 2.
Five raw partial IR files and exact stdout are required, with absent object/
EXE/PDB files explicitly checked. No normalization applies.

The untouched CL child transport was frozen before reconstruction. Two
independent originals reproduced all five partial files and exact streams.
The final masked original first-entry plan accepts entries 905 through 913,
with exact native first-entry order and terminal status 6. The expected error
exit is part of this case's oracle; every older success case still requires 0.

Original/native function-finish observations show broken_initializer,
implicit_call and recovered_tail in order, each with matching symbol kind,
flags, index and attributes, followed by clearing the active function symbol
at return. Both complete scanner/parser words match at those boundaries too.
This proves semantic completion of the valid tail beyond the pragma marker.

The earlier storage scanner/parser word mistake and six older tag/declaration
mask mistakes are corrected and independently documented under
../storage-pragmas/flag-identity/. Old output-matching receipts remain as
historical output evidence only. These repairs earn no function credit.

Native fixtures never invoke Wine. Original captures/builds are explicit,
use the shared prefix and fixed R:/S: drives, and occur outside fixtures.
validation.json records the eight new functions, failed probes/corrections,
complete checks and measured timings. Expected raw evidence retains CRLF.
