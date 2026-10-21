#ifndef LEXER_H
#define LEXER_H
#include "tokens.h"
#include "macros.h"

typedef union TokenValue {
    unsigned char bytes[8];
    struct { unsigned low, high; } words;
    double floating;
    char *identifier;
    IrToken *literal;
    struct FrontendSymbol *symbol;
} TokenValue;
#pragma pack(4)
typedef struct LexicalToken {
    unsigned char category, code;
    unsigned short flags;
    TokenValue value;
    unsigned line;
} LexicalToken;
#pragma pack()

#define SCANNER_SUPPRESS_POP 0x10000000UL
#define SCANNER_LOOKAHEAD_FULL 0x40000000UL
extern unsigned scanner_flags;
extern unsigned token_line;
extern unsigned char current_token_code;
extern TokenValue current_token_value;
extern LexicalToken initial_token_stack[3];
extern LexicalToken *token_stack_top, *token_stack_read;
extern int token_stack_count;
extern unsigned parser_flags;
extern unsigned char pch_output_written;
extern char token_text[128];
int skip_source_comment(void);
unsigned recover_special_character(void);
unsigned recover_trigraph(void);
int consume_line_escape(void);
unsigned char next_source_character(void);
void report_ide_line(void);
void finish_pch_capture(void);
void preprocess_directive(void);
void begin_source_line(void);
void skip_source_line(void);
void __stdcall copy_quoted_literal(int delimiter);
unsigned char __stdcall scan_character_literal(int wide);
void __stdcall scan_string_literal(int wide, int delimiter);
void scan_source_identifier(void);
int scan_macro_identifier(void);
int __stdcall expand_macro(MacroDefinition *definition);
unsigned char close_source_file(void);
void finish_pch_output(void);
int __stdcall match_source_character(int character);
unsigned __stdcall read_number_text(char *destination);
unsigned char __stdcall scan_number(int first_character);
unsigned char __stdcall scan_float_tail(char *destination);
unsigned char __stdcall scan_assembler_number(int first_character);
unsigned char scan_source_token(void);
#endif
