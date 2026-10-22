#ifndef RCPP_LEXER_H
#define RCPP_LEXER_H
#include "preprocessor.h"
#include <stdio.h>
#include <stddef.h>
#include <recon.h>
/* Values are dispatch categories in the original lexer, not output tokens. */
enum RcCharacterClass {
    LC_SPACE=0, LC_CR=1, LC_SLASH=2, LC_END=3, LC_STAR=4, LC_NEWLINE=5,
    LC_ESCAPE=6, LC_CHARACTER=7, LC_STRING=8, LC_DOT=9, LC_BANG=10, LC_HASH=11,
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
extern char rc_text_buffer[2048];
unsigned __stdcall rc_escape_value(int);
extern unsigned char rc_macro_hash;
extern int rc_macro_name_length;
typedef struct RcMacro {
    struct RcMacro *next;
    char *name, *body;
    signed char arguments;
    unsigned char flags;
} RcMacro;
typedef struct RcMacroFrame {
    unsigned char *saved_cursor;
    char *argument_work, *output_base;
    RcMacro *macro;
    unsigned char argument_count, argument_index;
    unsigned short reserved;
} RcMacroFrame;
/* Original storage starts at 010169c8. The old limit permits frame 64,
 * which overlaps definition workspace and the first three symbol buckets.
 * Express that actual shared storage as a typed union, including the first
 * active frame's saved cursor used by include-buffer management. */
typedef struct RcMacroNames {
    RcMacroFrame frames[64];
    char *definition;
    int formal_count;
    RcMacro *buckets[256];
} RcMacroNames;
typedef union RcMacroStorage {
    RcMacroNames names;
    RcMacroFrame expansion[65];
} RcMacroStorage;
extern RcMacroStorage rc_macro_storage;
#define rc_macro_frames rc_macro_storage.expansion
#define rc_macros rc_macro_storage.names.buckets
#define rc_define_storage rc_macro_storage.names.definition
#define rc_define_argument_count rc_macro_storage.names.formal_count
#define rc_saved_input_cursor rc_macro_frames[1].saved_cursor
void rc_read_macro_parameters(void);
unsigned rc_read_macro_body(void);
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
/* Compiler-checked native layouts and the original shared-state aliases. */
typedef char RcMacroLayout[(sizeof(RcMacro)==16 && offsetof(RcMacro,flags)==13)?1:-1];
typedef char RcMacroFrameLayout[(sizeof(RcMacroFrame)==20 && offsetof(RcMacroFrame,argument_count)==16)?1:-1];
typedef char RcMacroStorageLayout[(sizeof(RcMacroStorage)==2312 && offsetof(RcMacroNames,definition)==1280 && offsetof(RcMacroNames,buckets)==1288)?1:-1];
typedef char RcSavedCursorAlias[((sizeof(RcMacroFrame)+offsetof(RcMacroFrame,saved_cursor))==20)?1:-1];
typedef char RcLastFrameAlias[((64*sizeof(RcMacroFrame)+offsetof(RcMacroFrame,saved_cursor))==offsetof(RcMacroNames,definition) && (64*sizeof(RcMacroFrame)+offsetof(RcMacroFrame,argument_count))==(offsetof(RcMacroNames,buckets)+2*sizeof(RcMacro *)))?1:-1];
typedef char RcInputLayout[(sizeof(RcInput)==40 && offsetof(RcInput,bytes)==20 && offsetof(RcInput,wide)==24)?1:-1];
typedef char RcFileLayout[(sizeof(FILE)==32 && offsetof(FILE,_bufsiz)==24)?1:-1];
extern RcInput rc_inputs[10];
extern int rc_input_depth, rc_line_number, rc_input_encoding, rc_expanding_macro;
extern unsigned char *rc_input_cursor;
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
