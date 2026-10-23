Mac P-code/native entry aliases

This explicit COFF input defines _pfunc with the native legacy Type0x8020 and
its __nep_pfunc native counterpart. Startup uses a type8 jump-table reference;
the function contributes both native-bit and P-code type20 packed references,
and data contributes an ordinary type3 code pointer. These test inputs derive
the type/prefix semantics from original LINK, not a modern C++/Mac compiler.

Two untouched-original captures succeed and full2MB outputs agree beyond the
structural COFF timestamp. Fresh original/native GDB entries identify the two
native alias helpers before reconstruction; full terminal streams/status/output
match. C preserves the original WORD buffer-length arithmetic, local128-byte
name buffer/heap fallback, shared18-byte COFF symbol scratch and type20 no-op.
No Macintosh runtime execution or output patching/extra normalization is claimed.
Native fixtures execute only ELF with captured references. See validation.json
and ../m68k-dupcon/family-validation.json.
