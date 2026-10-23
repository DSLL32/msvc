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
validation.json records the current accepted range, matched frontier, audit
results and measured build/reference/native time. The completed normal suite
has18 cases; this case joins that suite only after reaching exact completion.
Original capture is explicit and separate. Native fixtures never invoke Wine.
