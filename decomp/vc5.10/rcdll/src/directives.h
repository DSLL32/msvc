#ifndef RECONSTRUCTED_RC_DIRECTIVES_H
#define RECONSTRUCTED_RC_DIRECTIVES_H
#include "preprocess.h"
#include <recon.h>

enum RcDirective {
    PP_IF,PP_ELIF,PP_ELSE,PP_ENDIF,PP_IFDEF,PP_IFNDEF,PP_DEFINE,
    PP_INCLUDE,PP_PRAGMA,PP_UNDEF,PP_LINE,PP_UNKNOWN,PP_ERROR,PP_IDENT
};
typedef struct RcTokenInfo {
    const WCHAR *description;
    unsigned char kind;
} RcTokenInfo;
typedef struct RcIntegerType {
    unsigned short flags;
    unsigned short other2;
    unsigned key4,key8,other12;
    unsigned short other16,other18;
} RcIntegerType;
typedef struct RcIntegerToken {
    unsigned char token,flags;
    unsigned short other2;
    RcIntegerType *type;
    unsigned other8;
    int value;
    unsigned other16;
} RcIntegerToken;
extern int rc_expanding_macro,rc_in_directive,rc_header_mode;
extern WCHAR rc_source_marker[257];
extern WCHAR *rc_cpp_path_characters;
WCHAR __cdecl rc_read_nonspace(void);
int __cdecl rc_resume_input(void);
void __stdcall rc_read_identifier(WCHAR);
unsigned char __stdcall rc_directive_code(const WCHAR *);
void __stdcall rc_expect_end(const WCHAR *);
int __stdcall rc_skip_inactive(int);
int __cdecl rc_push_condition(void);
int __cdecl rc_evaluate_condition(void);
int __cdecl rc_parse_conditional_expression(void);
int __cdecl rc_parse_logical_or(void);
int __cdecl rc_parse_logical_and(void);
int __cdecl rc_parse_bitwise_or(void);
int __cdecl rc_parse_bitwise_xor(void);
int __cdecl rc_parse_bitwise_and(void);
int __cdecl rc_parse_equality(void);
int __cdecl rc_parse_relational(void);
int __cdecl rc_parse_shift(void);
int __cdecl rc_parse_additive(void);
int __cdecl rc_parse_multiplicative(void);
int __cdecl rc_parse_unary(void);
int __cdecl rc_parse_primary(void);
int __stdcall rc_accept_expression_token(unsigned char token);
extern int rc_expression_parentheses;
extern unsigned char rc_expression_token;
void __cdecl rc_parse_define(void);
void __cdecl rc_parse_macro_parameters(void);
unsigned __cdecl rc_read_macro_replacement(void);
int __stdcall rc_find_macro_parameter(const WCHAR *name);
void __stdcall rc_check_macro_space(WCHAR *cursor);
WCHAR *__stdcall rc_copy_replacement_quoted(WCHAR *cursor,WCHAR delimiter);
extern int rc_macro_parameter_count;
extern WCHAR *rc_macro_replacement;
unsigned char __cdecl rc_next_token(void);
void __cdecl rc_skip_line(void);
WCHAR *__cdecl rc_parse_angle_name(void);
int __cdecl rc_search_current_directory(void);
void __cdecl rc_search_include_directories(void);
UINT __cdecl rc_parse_pragma(void);
int __stdcall rc_change_codepage(UINT);
WCHAR __cdecl rc_get_logical_character(void);
typedef union RcTokenExtra {
    struct { unsigned char hash,length; } identifier;
    unsigned short string_length;
} RcTokenExtra;
extern unsigned char rc_token_kind;
extern RcTokenExtra rc_token_extra;
#define rc_token_hash rc_token_extra.identifier.hash
#define rc_token_length rc_token_extra.identifier.length
#define rc_string_length rc_token_extra.string_length
extern void *rc_token_value;
extern RcTokenInfo rc_token_info[256];
extern int rc_number_buffer_active;
void __cdecl rc_line_comment(void);
void __cdecl rc_block_comment(void);
int __cdecl rc_open_source_frame(void);
void __stdcall rc_copy_quoted(WCHAR delimiter);
unsigned char __cdecl rc_read_character_constant(void);
void __cdecl rc_read_string(void);
unsigned __stdcall rc_escape_character(unsigned character);
unsigned char __stdcall rc_read_number(WCHAR first);
unsigned char __stdcall rc_read_fraction(WCHAR *end);
int __stdcall rc_match_logical_character(unsigned character);
void __cdecl rc_skip_block_comment(void);
void __cdecl rc_skip_line_comment(void);
int __stdcall rc_read_macro_identifier(WCHAR first);
int __stdcall rc_expand_macro(RcMacro *macro);
extern WCHAR *rc_number_buffer;
extern WCHAR rc_string_buffer[2045];
#define rc_number_spelling_address_identity (rc_string_buffer[1024])
unsigned __stdcall rc_parse_integer_value(const WCHAR *text,int radix);
unsigned char __stdcall rc_based_integer_kind(int value);
unsigned char __stdcall rc_decimal_integer_kind(int value);
unsigned char __stdcall rc_long_integer_kind(int value);
unsigned char __stdcall rc_unsigned_integer_kind(unsigned value);
void * __stdcall rc_store_integer(unsigned token,const unsigned *value);
RcIntegerType *__stdcall rc_intern_type(RcIntegerType *type);
int __stdcall rc_equal_type(const RcIntegerType *left,const RcIntegerType *right);
int __stdcall rc_digit_value(int character);
#endif
