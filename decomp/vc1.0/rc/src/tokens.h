#ifndef RC_TOKENS_H
#define RC_TOKENS_H
#include "resource.h"
#include <recon.h>
#define TOKEN_NUMBER 20
#define TOKEN_STRING 21
#define TOKEN_WIDE_STRING 23
#define TOKEN_WIDE_PREFIX 137
#define TOKEN_LITERAL_ONLY 0x8000
void __cdecl scan_wide_string(void);
void __cdecl scan_ansi_string(void);
void __cdecl scan_kanji_string(void);
BOOL __stdcall scan_keyword_or_symbol(unsigned int);
WORD __stdcall lookup_keyword(char *);
void __cdecl scan_expression(void);
void __cdecl scan_number_token(void);
void __cdecl scan_number_literal(void);
char __cdecl next_numeric_character(void);
unsigned long __cdecl scan_hex_number(void);
unsigned long __cdecl scan_octal_number(void);
unsigned long __cdecl scan_decimal_number(void);
unsigned long __cdecl expression_value(void);
unsigned long __cdecl expression_factor(void);
unsigned long __stdcall apply_operator(unsigned long, char, unsigned long);
BOOL __stdcall expression_operator(char *);
void __cdecl skip_expression_space(void);
#endif
