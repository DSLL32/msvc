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
typedef struct RcIntegerToken {
    unsigned char prefix[12];
    int value;
} RcIntegerToken;
extern int rc_expanding_macro,rc_in_directive;
extern WCHAR rc_source_marker[];
extern WCHAR *rc_cpp_path_characters;
WCHAR __cdecl rc_read_nonspace(void);
int __cdecl rc_resume_input(void);
void __stdcall rc_read_identifier(WCHAR);
unsigned char __stdcall rc_directive_code(const WCHAR *);
void __stdcall rc_expect_end(const WCHAR *);
int __stdcall rc_skip_inactive(int);
int __cdecl rc_push_condition(void);
int __cdecl rc_evaluate_condition(void);
void __cdecl rc_parse_define(void);
unsigned char __cdecl rc_next_token(void);
void __cdecl rc_skip_line(void);
WCHAR *__cdecl rc_parse_angle_name(void);
int __cdecl rc_search_current_directory(void);
void __cdecl rc_search_include_directories(void);
UINT __cdecl rc_parse_pragma(void);
int __stdcall rc_change_codepage(UINT);
WCHAR __cdecl rc_get_logical_character(void);
extern unsigned char rc_token_kind,rc_token_hash,rc_token_length;
extern void *rc_token_value;
extern RcTokenInfo rc_token_info[256];
extern int rc_keep_internal_marker,rc_number_buffer_active;
void __cdecl rc_line_comment(void);
void __cdecl rc_block_comment(void);
int __cdecl rc_open_source_frame(void);
void __stdcall rc_copy_quoted(WCHAR delimiter);
unsigned char __cdecl rc_read_character_constant(void);
void __cdecl rc_read_string(void);
unsigned char __stdcall rc_read_number(WCHAR first);
unsigned char __stdcall rc_read_fraction(WCHAR *end);
int __stdcall rc_match_logical_character(unsigned character);
int __stdcall rc_read_macro_identifier(WCHAR first);
int __stdcall rc_expand_macro(RcMacro *macro);
#endif
