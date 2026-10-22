#ifndef RCPP_TOKENS_H
#define RCPP_TOKENS_H
#include <recon.h>
/* Token numbers retained by the compiler-derived resource preprocessor. */
enum RcTokenKind {
    TK_INVALID, TK_NEWLINE, TK_IDENTIFIER, TK_STRING,
    TK_FLOAT, TK_DOUBLE, TK_LONG_DOUBLE, TK_INT, TK_LONG, TK_UINT, TK_ULONG,
    TK_AUTO, TK_STATIC, TK_EXTERN, TK_REGISTER, TK_TYPEDEF,
    TK_FORTRAN, TK_PASCAL, TK_CDECL, TK_INTERRUPT, TK_SAVEREGS, TK_LOADDS,
    TK_EXPORT, TK_ASM, TK_NEAR, TK_FAR, TK_HUGE, TK_CONST, TK_VOLATILE,
    TK_CHAR, TK_INT_TYPE, TK_FLOAT_TYPE, TK_DOUBLE_TYPE, TK_SHORT, TK_LONG_TYPE,
    TK_VOID, TK_SIGNED, TK_UNSIGNED, TK_ENUM, TK_ENUM_TAG, TK_STRUCT,
    TK_STRUCT_TAG, TK_UNION, TK_UNION_TAG, TK_AND, TK_LOGICAL_AND,
    TK_AND_ASSIGN, TK_ASSIGN, TK_BIT_FIELD, TK_COLON, TK_COMMA,
    TK_DECREMENT, TK_DIVIDE, TK_DIVIDE_ASSIGN, TK_EQUAL, TK_BANG, TK_DEREFERENCE,
    TK_GREATER, TK_GREATER_EQUAL, TK_INCREMENT, TK_LBRACKET, TK_LSHIFT_ASSIGN,
    TK_LSHIFT, TK_LESS, TK_LESS_EQUAL, TK_MINUS, TK_MINUS_ASSIGN, TK_MODULUS,
    TK_MODULUS_ASSIGN, TK_MULTIPLY, TK_MULTIPLY_ASSIGN, TK_NOT_EQUAL, TK_OR,
    TK_OR_ASSIGN, TK_LOGICAL_OR, TK_DOT, TK_PLUS, TK_PLUS_ASSIGN, TK_ARROW,
    TK_POST_DECREMENT, TK_POST_INCREMENT, TK_QUESTION, TK_RSHIFT_ASSIGN,
    TK_RSHIFT, TK_TILDE, TK_UNARY_MINUS, TK_XOR, TK_XOR_ASSIGN, TK_ARGUMENT,
    TK_ASSIGN_OP, TK_BREAK, TK_CASE, TK_CAST, TK_STORAGE_CLASS, TK_CONSTANT,
    TK_CONTINUE, TK_DEFAULT, TK_DIV_OP, TK_DO, TK_ELLIPSIS, TK_ELSE, TK_ENTRY,
    TK_EOF, TK_EQUALITY_OP, TK_UNUSED_104, TK_FOR, TK_ARGUMENT_MARKER,
    TK_GOTO, TK_IF, TK_INCREMENT_OP, TK_INITIALIZING, TK_LBRACE, TK_LINE,
    TK_LPAREN, TK_INTRINSIC, TK_MODIFIER, TK_MODULE, TK_RBRACKET, TK_RBRACE,
    TK_RELATIONAL_OP, TK_RETURN, TK_RPAREN, TK_SELF, TK_SEMICOLON, TK_SHIFT_OP,
    TK_SIZEOF, TK_STRUCT_OP, TK_SWITCH, TK_UNUSED_128, TK_TYPE, TK_TYPEDEF_NAME,
    TK_UNARY_PLUS, TK_WHILE
};
typedef struct RcTokenInfo {
    const char *name;
    unsigned category;
} RcTokenInfo;
extern RcTokenInfo rc_token_info[133];
extern unsigned char rc_token_kind;
typedef struct RcLexeme {
    void *value;
    union {
        struct { unsigned char hash, name_length; } identifier;
        unsigned short string_length;
    } detail;
} RcLexeme;
extern RcLexeme rc_current_token;
#define rc_token_value rc_current_token.value
#define rc_token_hash rc_current_token.detail.identifier.hash
#define rc_token_length rc_current_token.detail.identifier.name_length
#define rc_token_string_length rc_current_token.detail.string_length
unsigned char rc_next_token(void);
#endif
