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
/* PCH event headers precede their variable, nul-terminated textual payload. */
typedef struct PchInputRecord {
    unsigned kind, bytes, value;
    char text[1];
} PchInputRecord;
typedef char PchInputTextOffsetMustBe12[(offsetof(PchInputRecord, text) == 12) ? 1 : -1];
extern PchInputRecord *pch_input_record;
extern short pch_saved_page_count;
extern int pch_input_changed, pch_abort_on_file_change;
extern int permanent_page_count;
void record_pch_string_event(unsigned kind, unsigned value, char *text);
int resume_pch_source_input(void);
unsigned __stdcall source_file_timestamp(char *filename);
int evaluate_preprocessor_expression(void);
unsigned char __stdcall classify_directive(char *name);
void undefine_source_macro(void);
void define_source_macro(void);
void process_pragma(void);
int push_conditional_state(void);
void capture_directive_text(void);
char *read_include_name(void);
char *read_angle_include_name(void);
char *trim_filename_space(char *begin, char *end);
int __stdcall skip_conditional_block(unsigned flags);
unsigned char read_skipped_directive(void);
void __stdcall skip_directive_literal(unsigned char delimiter);
void __stdcall finish_directive(char *name);
unsigned char next_directive_character(void);
int __stdcall record_pch_condition(int condition);
int process_pch_directive(void);
void warn_ident_requires_string(void);
#endif
