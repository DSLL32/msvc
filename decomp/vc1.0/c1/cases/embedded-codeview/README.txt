Embedded /Z7 CodeView type reconstruction
=========================================

The original CL invocation is /nologo /c /Z7 /Od /G5 /W3 types.c. Its actual
C1 child argv/environment and complete input/output inventories are frozen in
evidence. Two separate untouched original C1 runs reproduce all five raw IR
outputs, status and streams using relative input names and the portable ir
prefix. No absolute paths or clocks occur in these output oracles. /Yd is not
selected because this standalone case does not use a precompiled header.

The source covers recursive/nested records, a union, signed/wide enum values,
arrays, bitfields, a packed record, qualified pointers and function pointers.
The case is complete at852 accepted entries:19 new whole source functions.
All five raw outputs match, including643-byte embedded type data. The full
normal suite now has21 cases. validation.json records the accepted order,
fresh masked-original/native evidence, graph/state/grammar audits and measured
build/reference/native time, including failed shared-startup attempts.
Original capture is explicit and separate. Native fixtures never invoke Wine.

The signed/wide enum constants occur in source, but no enum-typed object forces
an enum debug record. Enum-member serialization remains a named trapping path;
this source case does not claim that unvisited branch is verified.
