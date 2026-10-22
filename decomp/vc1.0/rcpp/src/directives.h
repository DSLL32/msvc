#ifndef RCPP_DIRECTIVES_H
#define RCPP_DIRECTIVES_H
#include "lexer.h"
#include "tokens.h"
#include <recon.h>
enum RcDirective {
    PP_IF, PP_ELIF, PP_ELSE, PP_ENDIF, PP_IFDEF, PP_IFNDEF, PP_DEFINE,
    PP_INCLUDE, PP_PRAGMA, PP_UNDEF, PP_LINE, PP_UNKNOWN, PP_ERROR, PP_IDENT
};
extern int rc_in_directive, rc_in_condition_expression, rc_header_mode;
extern int rc_condition_state[32];
unsigned char rc_read_nonspace(void);
unsigned __stdcall rc_read_identifier(unsigned char);
unsigned rc_resume_input(void);
void rc_skip_line(void);
int rc_evaluate_condition(void);
void __stdcall rc_expect_end(const char *);
int __stdcall rc_skip_inactive(int);
int rc_push_condition(void);
void rc_parse_define(void);
char *rc_parse_angle_name(void);
int rc_search_current_directory(void);
void rc_search_include_directories(void);
void rc_parse_pragma(void);
void rc_remove_macro(void);
unsigned char rc_get_logical_character(void);
unsigned char rc_close_include(void);
int __stdcall rc_expand_macro(RcMacro *);
void __stdcall rc_copy_quoted_text(unsigned char);
unsigned char rc_parse_character(void);
void rc_parse_string(void);
unsigned char __stdcall rc_parse_number(unsigned char);
unsigned char __stdcall rc_read_floating(char *);
int __stdcall rc_match_character(int);
void rc_skip_line_comment(void);
void rc_skip_block_comment(void);
void rc_discard_block_comment(void);
void rc_discard_line_comment(void);
int __stdcall rc_read_expanded_identifier(unsigned char);
#endif
