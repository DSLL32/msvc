Legal386 diagnostic-monitor disassembly corpus; inspect only, never execute.

The primary original-compiled monitor.obj contains558 .text bytes. The companion
contains90. Both sources use ordinary VC1 inline assembly; the companion needs
14 explicitly documented fixture-only bytes because VC1 rejects selector:offset
syntax (C2415/C2400). Those bytes are legal far CALL/JMP ptr16:32; their offset is
little-endian12345678 and selector1234. The rejected source/streams are retained
under companion/assembler-rejection. None of this input assembly is reconstructed
LINK implementation code.

Original CL /nologo /c /Od /G3 built each object twice. Builds differ only in the
structural COFF TimeDateStamp at4..7; the frozen first object itself is unchanged.
Untouched LINK -dump /disasm <object> runs twice per object with identical complete
streams/status, then explicit fresh masked captures establish accepted entry order.
Active native-only references are cases/shared-prefix/dump-disasm-x86-monitor and
its -extra companion. Those fixtures never run Wine, an original or input code.

The primary legal ARPL encoding63C3 makes the original second callback inspect the
following BOUND opcode and longjmp; preserve its actual illegal-instruction line
and subsequent RET/BOUND decoding. Original repnz and ST-index quirks likewise
remain exact. The normal exception registration/current saved scope are both0;
public longjmp support proves zero handler unwind, not general SEH compatibility.

81 application C functions,4787 actual instruction bytes plus75alignment; two CRT
support bodies188extent bytes are separate. 14 typed contextual tables/arrays are
independently byte/pointer audited. All46cached native cases and727app entries pass.
The x87 correction history is explicit: first full output exposed writing size
atoperand2+9c; actual original store isoperand1+78. Both corrected complete224-byte
poststates and original/canonical displacements match; failed output is retained.

validation.json, timing.json and components.json record tests, measured costs and
limits. progress.json gives ordered per-function fresh evidence. First25 entries
have firstentry/stack/stream evidence;56 later entries also have scalar state
samples. No claim of branch coverage. The five unentered old decoder seeds include
486-only and obsolete/reserved/error handlers and are not silently accepted.
