Prototype output
================

Original CL /nologo /Zg test.c produces360 bytes of prototypes. The source
contains callbacks, a callback-returning function, stdcall/cdecl declarators,
a pointer-to-array return and struct arguments/results. Local header/source
and C1.ERR hashes are recorded with the exact captured single-entry environment.
Two standalone untouched originals match all bytes and empty artifact set.

Original CL prepare_execution duplicates descriptor1 to descriptor2 before
launch; the phase preserves that alias explicitly. No stream content is changed.
This case follows comment preservation in the authorized queue.
