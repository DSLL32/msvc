#ifndef LEXER_H
#define LEXER_H
#include "tokens.h"
#include "macros.h"
#include "macro-expand.h"
#include <recon.h>

typedef struct StringToken {
    unsigned char *text;
    unsigned short bytes, flags;
} StringToken;

typedef union TokenValue {
    unsigned char bytes[8];
    struct { unsigned low, high; } words;
    double floating;
    StringToken string;
    char *identifier;
    IrToken *literal;
    struct FrontendSymbol *symbol;
} TokenValue;
#pragma pack(4)
typedef struct LexicalToken {
    unsigned char category, code;
    unsigned short flags;
    TokenValue value;
    struct FrontendSymbol *lookup_symbol;
} LexicalToken;
#pragma pack()

#define SCANNER_SUPPRESS_POP 0x10000000UL
#define SCANNER_LOOKAHEAD_FULL 0x40000000UL
extern unsigned scanner_flags;
extern struct FrontendSymbol *last_lookup_symbol;
extern unsigned char current_token_code;
extern TokenValue current_token_value;
extern LexicalToken initial_token_stack[3];
extern LexicalToken *token_stack_top, *token_stack_read;
extern int token_stack_count;
extern unsigned parser_flags;
extern unsigned char pch_output_written;
unsigned char read_numeric_character(void);
int __stdcall integer_digit_value(int character);
unsigned __stdcall convert_integer_digits(char *digits, int length, int radix);
unsigned char __stdcall classify_decimal_integer(unsigned value);
unsigned char __stdcall classify_radix_integer(unsigned value);
unsigned char __stdcall classify_long_integer(unsigned value);
unsigned char __stdcall classify_unsigned_integer(unsigned value);
typedef struct DeclarationCode {
    unsigned char token;
    unsigned short value;
} DeclarationCode;
extern DeclarationCode declaration_codes[];
extern unsigned declaration_code_count;
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
unsigned char *__stdcall ensure_string_space(unsigned char *cursor);
unsigned char *__stdcall convert_string_segment(unsigned char *text, int wide, unsigned bytes);
unsigned short __stdcall decode_escape(unsigned char character, int wide);
void __stdcall convert_pascal_string(StringToken *string);
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
extern char *token_names[153];

void skip_line_comment(void);
void copy_line_comment(void);
void discard_line_comment(void);
void copy_block_comment(void);
void skip_block_comment(void);

unsigned char read_parser_token(void);

void refill_source_boundary(void);

#endif
