#ifndef KEYWORDS_H
#define KEYWORDS_H
#include "preprocessor.h"
#include <recon.h>

enum KeywordToken {
    TOK_DISABLED_KEYWORD = 1,
    TOK_AUTO = 11,
    TOK_STATIC = 12,
    TOK_EXTERN = 13,
    TOK_INLINE = 14,
    TOK_REGISTER = 15,
    TOK_TYPEDEF = 16,
    TOK_FORTRAN = 17,
    TOK_PASCAL = 18,
    TOK_CDECL = 19,
    TOK_FASTCALL = 20,
    TOK_SYSCALL = 21,
    TOK_EXPORT = 25,
    TOK_ASM = 26,
    TOK_BASED = 27,
    TOK_NEAR = 28,
    TOK_FAR = 29,
    TOK_FAR16 = 30,
    TOK_HUGE = 31,
    TOK_CONST = 32,
    TOK_VOLATILE = 33,
    TOK_CHAR = 34,
    TOK_INT = 35,
    TOK_FLOAT = 36,
    TOK_DOUBLE = 37,
    TOK_SHORT = 38,
    TOK_LONG = 39,
    TOK_VOID = 40,
    TOK_SIGNED = 43,
    TOK_UNSIGNED = 44,
    TOK_ENUM = 45,
    TOK_STRUCT = 46,
    TOK_UNION = 47,
    TOK_BREAK = 97,
    TOK_CASE = 98,
    TOK_CONTINUE = 102,
    TOK_DEFAULT = 103,
    TOK_DO = 105,
    TOK_ELSE = 107,
    TOK_EXCEPT = 111,
    TOK_FINALLY = 112,
    TOK_FOR = 113,
    TOK_GOTO = 115,
    TOK_IF = 116,
    TOK_RETURN = 127,
    TOK_SIZEOF = 132,
    TOK_STDCALL = 133,
    TOK_SWITCH = 135,
    TOK_TRY = 137,
    TOK_WHILE = 141,
    TOK_LEAVE = 149,
    TOK_DECLSPEC = 151
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
