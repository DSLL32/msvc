The original VC1.0 NT compiler produced inputs/x86-rich.obj from src/x86-rich.c
with /c /Od /G3. The661-byte COFF object contains181 code bytes, four external
functions and an ordinary relocation. It is disassembly input, never executed.
The original-only capture outside fixtures exited0 with the exact streams in
manifest.json. Native fixtures use that fixed reference and never invoke Wine.

The32-bit inline assembler rejects MOV AX,[BX+SI+4] with C2432. Five annotated
fixture-only _emit bytes (67 66 8B 40 04) encode that address/operand-size override
form; the untouched original dumper confirms its decoding. These input bytes are
not reconstruction code or a replacement for recovered C implementations.

Preserve original output quirks: F3 MOVSB is rendered "repnz movs"; the D8 C1 and
D9 C9 forms assembled with ST(1) are rendered st(2). The corpus keeps the actual
output rather than correcting its spelling or architectural interpretation.
Compiler rejection evidence and original capture timings remain under the
manifest's private scratch directory. No volatile bytes or output are normalized.
