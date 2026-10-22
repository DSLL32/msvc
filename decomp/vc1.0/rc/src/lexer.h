#ifndef RC_LEXER_H
#define RC_LEXER_H
#include "driver.h"
enum ResourceToken {
    TOKEN_LPAREN = 1, TOKEN_RPAREN = 2, TOKEN_OR = 12,
    TOKEN_BEGIN = 13, TOKEN_END = 14, TOKEN_COMMA = 15,
    TOKEN_NOT = 16, TOKEN_AND = 17, TOKEN_EQUALS = 18,
    TOKEN_PLUS = 101, TOKEN_MINUS = 102, TOKEN_EOF = 127
};
struct LexOperator { char spelling; int token; };
extern struct LexOperator operator_tokens[];
extern unsigned char token_delimiters[128];
extern char source_character, source_filename[MAX_PATH];
extern FILE *lexer_input;
extern int source_line, lexer_status;
struct LexToken {
    DWORD value;
    int line, position;
    DWORD is_long;
    WORD length;
    BYTE kind, flags;
    unsigned int attributes;
    char spelling[4098];
    WCHAR text[4098];
};
extern struct LexToken current_token;
int __cdecl advance_character(void);
int __cdecl read_source_character(void);
int __cdecl read_buffered_character(void);
void __stdcall lexical_error(char *, char *);
int __cdecl rc_tolower(int);
unsigned int __stdcall read_file(FILE *, void *, unsigned int);
#endif
