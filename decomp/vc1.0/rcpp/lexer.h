#ifndef RCPP_LEXER_H
#define RCPP_LEXER_H
#include "preprocessor.h"
#include <stdio.h>
/* Values are dispatch categories in the original lexer, not output tokens. */
enum RcCharacterClass {
    LC_SPACE=0, LC_CR=1, LC_SLASH=2, LC_END=3, LC_STAR=4, LC_NEWLINE=5,
    LC_CHARACTER=7, LC_STRING=8, LC_DOT=9, LC_BANG=10, LC_HASH=11,
    LC_PERCENT=12, LC_EQUAL=13, LC_XOR=14, LC_OR=15, LC_AND=16,
    LC_PLUS=17, LC_MINUS=18, LC_LESS=19, LC_GREATER=20, LC_BAD=23,
    LC_RBRACE=24, LC_RBRACKET=25, LC_COLON=26, LC_COMMA=27,
    LC_RPAREN=28, LC_DIGIT=29, LC_LBRACE=30, LC_LBRACKET=31,
    LC_LPAREN=32, LC_QUESTION=33, LC_SEMICOLON=34, LC_TILDE=35,
    LC_INTERNAL_1=36, LC_INTERNAL_2=37, LC_INTERNAL_3=38, LC_INTERNAL_4=39,
    LC_IDENTIFIER=40, LC_AT=41, LC_DBCS_LEAD=42, LC_BAD_PUNCTUATION=43
};
extern unsigned char rc_character_class[256], rc_character_flags[256];
extern char rc_macro_name[512];
extern unsigned char rc_macro_hash;
extern int rc_macro_name_length;
typedef struct RcMacro {
    struct RcMacro *next;
    char *name, *body;
    signed char arguments;
    unsigned char flags;
} RcMacro;
extern RcMacro *rc_macros[256];
RcMacro *rc_find_macro(void);
int __stdcall rc_compare_macro_text(char *, char *, unsigned);
char *__stdcall rc_copy_bytes(const char *, unsigned);
extern FILE *rc_output, *rc_input_file;
unsigned char __stdcall rc_hash_name(const char *);
void __stdcall rc_define_macro(char *, unsigned, int);
typedef struct RcInput {
    int capacity;
    FILE *stream;
    int saved_line;
    char *saved_name;
    unsigned char *saved_cursor, *bytes;
    unsigned short *wide;
    int buffered, encoding;
    long file_position;
} RcInput;
extern RcInput rc_inputs[10];
extern int rc_input_depth, rc_line_number, rc_input_encoding, rc_expanding_macro;
extern unsigned char *rc_input_cursor, *rc_saved_input_cursor;
int __stdcall rc_open_include(char *, int);
int rc_byte_order(void);
int __stdcall rc_detect_encoding(FILE *);
int rc_refill_input(void);
char *__stdcall rc_copy_string(const char *);
char *__stdcall rc_quote_filename(char *, const char *);
void rc_emit_line(void);
int rc_skip_comment(void);
int rc_splice_line(void);
void rc_parse_directive(void);
void rc_begin_line(void);
#endif
