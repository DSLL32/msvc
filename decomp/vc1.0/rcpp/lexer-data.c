/* Character classification tables encountered by preprocessor initialization.
 * ASCII rows are explicit lexical categories. Unassigned high bytes are errors
 * until a selected multibyte codepage marks its lead bytes. */
#include "lexer.h"
#define BAD8 LC_BAD,LC_BAD,LC_BAD,LC_BAD,LC_BAD,LC_BAD,LC_BAD,LC_BAD
#define IDENT8 LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER
unsigned char rc_character_class[256] = {
    LC_END,LC_BAD,LC_BAD,LC_BAD,LC_BAD,LC_BAD,LC_BAD,LC_BAD,
    LC_BAD,LC_SPACE,LC_NEWLINE,LC_SPACE,LC_SPACE,LC_CR,LC_BAD,LC_BAD,
    BAD8,
    LC_BAD,LC_BAD,LC_END,LC_BAD,LC_BAD,LC_BAD,LC_BAD,LC_BAD,
    LC_SPACE,LC_BANG,LC_STRING,LC_HASH,LC_BAD_PUNCTUATION,LC_PERCENT,LC_AND,LC_CHARACTER,
    LC_LPAREN,LC_RPAREN,LC_STAR,LC_PLUS,LC_COMMA,LC_MINUS,LC_DOT,LC_SLASH,
    LC_DIGIT,LC_DIGIT,LC_DIGIT,LC_DIGIT,LC_DIGIT,LC_DIGIT,LC_DIGIT,LC_DIGIT,
    LC_DIGIT,LC_DIGIT,LC_COLON,LC_SEMICOLON,LC_LESS,LC_EQUAL,LC_GREATER,LC_QUESTION,
    LC_AT,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,
    IDENT8, IDENT8,
    LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_LBRACKET,LC_END,LC_RBRACKET,LC_XOR,LC_IDENTIFIER,
    LC_BAD_PUNCTUATION,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,
    IDENT8, IDENT8,
    LC_IDENTIFIER,LC_IDENTIFIER,LC_IDENTIFIER,LC_LBRACE,LC_OR,LC_RBRACE,LC_TILDE,LC_BAD,
    BAD8,BAD8,BAD8,BAD8,BAD8,BAD8,BAD8,BAD8,
    BAD8,BAD8,BAD8,BAD8,BAD8,BAD8,BAD8,BAD8
};
#undef BAD8
#undef IDENT8

/* Character flags combine identifier, digit-radix and numeric suffix classes.
 * Keep these semantic combinations alongside the scanner, rather than copying
 * a raw block from the executable. */
#define I 16
#define H (16|8)
#define S (16|32)
#define HS (16|8|32)
#define Z8 0,0,0,0,0,0,0,0
unsigned char rc_character_flags[256] = {
    128,0,0,0,0,0,0,0, 0,64,128,64,64,0,0,0,
    Z8, 0,0,128,0,0,0,0,0,
    64,0,0,0,0,0,0,0, 0,0,128,0,0,0,0,0,
    31,31,30,30,30,30,30,30, 28,28,0,0,0,0,0,0,
    0,H,HS,H,HS,H,H,I, S,I,I,I,I,I,I,S,
    I,S,I,I,I,I,I,I, I,I,I,0,0,0,0,I,
    0,H,HS,H,HS,H,H,I, S,I,I,I,I,I,I,S,
    I,S,I,I,I,I,I,I, I,I,I,0,0,0,0,0,
    Z8,Z8,Z8,Z8,Z8,Z8,Z8,Z8, Z8,Z8,Z8,Z8,Z8,Z8,Z8,Z8
};
#undef I
#undef H
#undef S
#undef HS
#undef Z8
char rc_macro_name[512];
unsigned char rc_macro_hash;
int rc_macro_name_length;
FILE *rc_output;

/* Shared string storage also contains the two 1K macro-work windows. */
char rc_text_buffer[2048];
