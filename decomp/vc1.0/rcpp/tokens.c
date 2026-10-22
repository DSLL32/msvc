/* Token descriptions shared by scanner output and diagnostic formatting.
 * Category fields are compiler token groups retained by this preprocessor.
 * Slot zero is invalid; valid token descriptions occupy original indices 1..132.
 * The original bytes before index one are the preceding string object's tail,
 * not a valid descriptor, so no raw padding/preceding data is reproduced. */
#include "tokens.h"
#include <stddef.h>
RcTokenInfo rc_token_info[133] = {
    {NULL,TK_INVALID},
    {"newline",TK_NEWLINE}, {"identifier",TK_IDENTIFIER}, {"string",TK_STRING},
    {"float constant",TK_CONSTANT}, {"double constant",TK_CONSTANT},
    {"long double constant",TK_CONSTANT}, {"int constant",TK_CONSTANT},
    {"long constant",TK_CONSTANT}, {"unsigned int constant",TK_CONSTANT},
    {"unsigned long constant",TK_CONSTANT},
    {"auto",TK_STORAGE_CLASS}, {"static",TK_STORAGE_CLASS},
    {"extern",TK_STORAGE_CLASS}, {"register",TK_STORAGE_CLASS},
    {"typedef",TK_STORAGE_CLASS}, {"fortran",TK_MODIFIER}, {"pascal",TK_MODIFIER},
    {"cdecl",TK_MODIFIER}, {"interrupt",TK_MODIFIER}, {"saveregs",TK_MODIFIER},
    {"loadds",TK_MODIFIER}, {"export",TK_MODIFIER}, {"_asm",TK_ASM},
    {"near",TK_MODIFIER}, {"far",TK_MODIFIER}, {"huge",TK_MODIFIER},
    {"const",TK_MODIFIER}, {"volatile",TK_MODIFIER},
    {"char",TK_TYPE}, {"int",TK_TYPE}, {"float",TK_TYPE}, {"double",TK_TYPE},
    {"short",TK_TYPE}, {"long",TK_TYPE}, {"void",TK_TYPE}, {"signed",TK_TYPE},
    {"unsigned",TK_TYPE}, {"enum",TK_ENUM}, {"enum tag",TK_ENUM_TAG},
    {"struct",TK_STRUCT}, {"struct tag",TK_STRUCT_TAG}, {"union",TK_UNION},
    {"union tag",TK_UNION_TAG}, {"&",TK_AND}, {"&&",TK_LOGICAL_AND},
    {"&=",TK_ASSIGN_OP}, {"=",TK_ASSIGN}, {"bit field",TK_BIT_FIELD},
    {":",TK_COLON}, {",",TK_COMMA}, {"--",TK_INCREMENT_OP}, {"/",TK_DIV_OP},
    {"/=",TK_ASSIGN_OP}, {"==",TK_EQUALITY_OP}, {"!",TK_BANG}, {"*",TK_DEREFERENCE},
    {">",TK_RELATIONAL_OP}, {">=",TK_RELATIONAL_OP}, {"++",TK_INCREMENT_OP},
    {"[",TK_LBRACKET}, {"<<=",TK_ASSIGN_OP}, {"<<",TK_SHIFT_OP},
    {"<",TK_RELATIONAL_OP}, {"<=",TK_RELATIONAL_OP}, {"-",TK_MINUS},
    {"-=",TK_ASSIGN_OP}, {"%",TK_DIV_OP}, {"%=",TK_ASSIGN_OP},
    {"*",TK_MULTIPLY}, {"*=",TK_ASSIGN_OP}, {"!=",TK_EQUALITY_OP},
    {"|",TK_OR}, {"|=",TK_ASSIGN_OP}, {"||",TK_LOGICAL_OR},
    {".",TK_STRUCT_OP}, {"+",TK_PLUS}, {"+=",TK_ASSIGN_OP}, {"->",TK_STRUCT_OP},
    {"--",TK_INCREMENT_OP}, {"++",TK_INCREMENT_OP}, {"?",TK_QUESTION},
    {">>=",TK_ASSIGN_OP}, {">>",TK_SHIFT_OP}, {"~",TK_TILDE},
    {"unary minus",TK_UNARY_MINUS}, {"^",TK_XOR}, {"^=",TK_ASSIGN_OP},
    {"argument",TK_ARGUMENT}, {"assign op",TK_ASSIGN_OP}, {"break",TK_BREAK},
    {"case",TK_CASE}, {"cast",TK_CAST}, {"storage class",TK_STORAGE_CLASS},
    {"constant",TK_CONSTANT}, {"continue",TK_CONTINUE}, {"default",TK_DEFAULT},
    {"div op",TK_DIV_OP}, {"do",TK_DO}, {"...",TK_ELLIPSIS}, {"else",TK_ELSE},
    {"entry",TK_ENTRY}, {"end of file",TK_INVALID}, {"equ op",TK_EQUALITY_OP},
    {NULL,TK_UNUSED_104}, {"for",TK_FOR}, {"argument",TK_ARGUMENT_MARKER},
    {"goto",TK_GOTO}, {"if",TK_IF}, {"incr op",TK_INCREMENT_OP},
    {"initializing",TK_INITIALIZING}, {"{",TK_LBRACE}, {"line",TK_LINE},
    {"(",TK_LPAREN}, {"intrinsic",TK_INTRINSIC}, {"modifier",TK_MODIFIER},
    {"module",TK_MODULE}, {"]",TK_RBRACKET}, {"}",TK_RBRACE},
    {"rel op",TK_RELATIONAL_OP}, {"return",TK_RETURN}, {")",TK_RPAREN},
    {"_self",TK_SELF}, {";",TK_SEMICOLON}, {"shift op",TK_SHIFT_OP},
    {"sizeof",TK_SIZEOF}, {"struct op",TK_STRUCT_OP}, {"switch",TK_SWITCH},
    {NULL,TK_UNUSED_128}, {"type",TK_TYPE}, {"typedef name",TK_TYPEDEF_NAME},
    {"unary plus",TK_UNARY_PLUS}, {"while",TK_WHILE}
};
unsigned char rc_token_kind;
RcLexeme rc_current_token;
