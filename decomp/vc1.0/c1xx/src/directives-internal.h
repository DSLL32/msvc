#ifndef DIRECTIVES_INTERNAL_H
#define DIRECTIVES_INTERNAL_H
#include "lexer.h"
#include "source-input.h"
#include <recon.h>
enum PreprocessorDirective {
    PP_IF, PP_ELIF, PP_ELSE, PP_ENDIF, PP_IFDEF, PP_IFNDEF,
    PP_DEFINE, PP_INCLUDE, PP_PRAGMA, PP_UNDEF, PP_LINE,
    PP_UNKNOWN, PP_ERROR, PP_IDENT
};
extern unsigned conditional_branch_states[32];
extern short pch_include_count;
int evaluate_preprocessor_expression(void);
unsigned char __stdcall classify_directive(char *name);
void undefine_source_macro(void);
void define_source_macro(void);
void process_pragma(void);
int push_conditional_state(void);
void capture_directive_text(void);
char *read_include_name(void);
int __stdcall skip_conditional_block(unsigned flags);
void __stdcall finish_directive(char *name);
unsigned char next_directive_character(void);
int __stdcall record_pch_condition(int condition);
int process_pch_directive(void);
void warn_ident_requires_string(void);
#endif
