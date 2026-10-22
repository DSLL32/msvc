/* Declaration meanings of lexical codes. The executable shares one token
   vocabulary with expression opcodes; declarations retain their own meanings
   here, rather than importing the unrelated expression entries. */
#include "parser.h"

DeclarationCode declaration_codes[] = {
    { TOK_AUTO, 4 }, { TOK_STATIC, 3 }, { TOK_EXTERN, 1 },
    { TOK_REGISTER, 5 }, { TOK_TYPEDEF, 7 }, { TOK_FRIEND, 0x10 },
    { TOK_INLINE, 0x11 }, { TOK_VIRTUAL, 0x12 },
    { TOK_CHAR, 0x41 }, { TOK_INT, 0x64 }, { TOK_FLOAT, 0x104 },
    { TOK_DOUBLE, 0x108 }, { TOK_SHORT, 0x42 }, { TOK_LONG, 0x44 },
    { TOK_VOID, 0x80 }, { TOK_SEGMENT, 0x86 }, { TOK_SEGNAME, 0x69 },
    { TOK_SIGNED, 0x8000 }, { TOK_UNSIGNED, 0x4000 },
    { TOK_CLASS, 0x408 }, { TOK_ENUM, 0x200 },
    { TOK_STRUCT, 0x402 }, { TOK_UNION, 0x404 },
    { TOK_FORTRAN, 0x30 }, { TOK_PASCAL, 0x20 }, { TOK_CDECL, 0x10 },
    { TOK_FASTCALL, 0x40 }, { TOK_SYSCALL, 0x50 }, { TOK_STDCALL, 0x60 },
    { TOK_THISCALL, 0x70 }, { TOK_INTERRUPT, 0x100 },
    { TOK_SAVEREGS, 0x200 }, { TOK_LOADDS, 0x800 }, { TOK_EXPORT, 0x400 },
    { TOK_NEAR, 1 }, { TOK_FAR, 2 }, { TOK_FAR16, 3 }, { TOK_HUGE, 3 },
    { TOK_CONST, 0x1000 }, { TOK_VOLATILE, 0x2000 },
    { TOK_PRIVATE, 1 }, { TOK_PROTECTED, 2 }, { TOK_PUBLIC, 3 },
    { TOK_MODIFIER, 0x9e }, { TOK_OLDCALL, 0 }, { TOK_SYSAPI, 0xcb },
    { TOK_CONVERSION_MODIFIER, 0x9e },
    { TOK_CONVERSION_CONST, 0x1000 }, { TOK_CONVERSION_VOLATILE, 0x2000 }
};
unsigned declaration_code_count = sizeof(declaration_codes) / sizeof(declaration_codes[0]);
