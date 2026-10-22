#ifndef KEYWORDS_H
#define KEYWORDS_H
#include "preprocessor.h"
#include <recon.h>
enum KeywordToken {
    TOK_DISABLED_KEYWORD = 1,
    TOK_AUTO = 11, TOK_STATIC = 12, TOK_EXTERN = 13, TOK_REGISTER = 14,
    TOK_TYPEDEF = 15, TOK_FRIEND = 16, TOK_INLINE = 17, TOK_VIRTUAL = 18,
    TOK_CHAR = 19, TOK_INT = 20, TOK_FLOAT = 21, TOK_DOUBLE = 22,
    TOK_SHORT = 23, TOK_LONG = 24, TOK_VOID = 25,
    TOK_SEGMENT = 26, TOK_SEGNAME = 27,
    TOK_SIGNED = 28, TOK_UNSIGNED = 29, TOK_CLASS = 30, TOK_ENUM = 31,
    TOK_STRUCT = 32, TOK_UNION = 33, TOK_FORTRAN = 41, TOK_PASCAL = 42,
    TOK_CDECL = 43, TOK_FASTCALL = 44, TOK_SYSCALL = 45,
    TOK_INTERRUPT = 46, TOK_SAVEREGS = 47, TOK_LOADDS = 48, TOK_EXPORT = 49,
    TOK_MS_ASM = 50, TOK_BASED = 51, TOK_NEAR = 52, TOK_FAR = 53,
    TOK_FAR16 = 54, TOK_HUGE = 55, TOK_CONST = 56, TOK_VOLATILE = 57,
    TOK_BREAK = 108, TOK_CASE = 109, TOK_CONTINUE = 112, TOK_DEFAULT = 113,
    TOK_DO = 115, TOK_ELSE = 117, TOK_FOR = 121, TOK_GOTO = 123,
    TOK_IF = 124, TOK_MODIFIER = 130, TOK_OLDCALL = 131,
    TOK_RETURN = 135, TOK_SIZEOF = 140, TOK_STDCALL = 141,
    TOK_SWITCH = 143, TOK_WHILE = 146, TOK_PRIVATE = 148,
    TOK_PROTECTED = 149, TOK_PUBLIC = 150, TOK_NEW = 151, TOK_DELETE = 152,
    TOK_THIS = 155, TOK_OPERATOR = 162,
    TOK_CONVERSION_MODIFIER = 173, TOK_CONVERSION_CONST = 174,
    TOK_CONVERSION_VOLATILE = 175, TOK_SEH_TRY = 178,
    TOK_EXCEPT = 179, TOK_FINALLY = 180, TOK_NCTAG = 183,
    TOK_NOVTORDISP = 185, TOK_ASM = 186, TOK_CATCH = 187,
    TOK_TEMPLATE = 188, TOK_THROW = 189, TOK_TRY = 190, TOK_RESUME = 191,
    TOK_NOUNWIND = 192, TOK_LEAVE = 194, TOK_SINGLE_INHERITANCE = 197,
    TOK_MULTIPLE_INHERITANCE = 198, TOK_VIRTUAL_INHERITANCE = 199,
    TOK_SYSAPI = 203, TOK_THISCALL = 204, TOK_DECLSPEC = 205
};
typedef struct KeywordSpec {
    char *name;
    unsigned char token;
    unsigned char reserved[3];
} KeywordSpec;
typedef struct DeclspecSpec {
    char *name;
    unsigned attribute, parameter_mode, reserved;
} DeclspecSpec;
extern KeywordSpec keyword_specs[];
extern DeclspecSpec declspec_specs[];
void __stdcall intern_keyword(char *name);
char *__stdcall intern_identifier(char *name);
extern char *interned_name;
#endif
