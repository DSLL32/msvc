#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H
#include "initialization.h"

#pragma pack(1)
typedef struct PreprocessorSymbol {
    struct PreprocessorSymbol *next;
    unsigned char flags;
    char name[1];
} PreprocessorSymbol;
typedef struct PreprocessorKeyword {
    unsigned char token;
    PreprocessorSymbol symbol;
} PreprocessorKeyword;
#pragma pack()
#define SYMBOL_FROM_NAME(name) ((PreprocessorSymbol *)((char *)(name) - 5))
#define KEYWORD_FROM_NAME(name) ((PreprocessorKeyword *)((char *)(name) - 6))
extern PreprocessorSymbol **preprocessor_buckets;
extern unsigned char *macro_text_begin, *macro_text_next;
void initialize_macro_text(void);
void scan_continued_identifier(char *start, unsigned hash);
unsigned char __stdcall classify_directive(char *name);
void capture_directive_text(void);
int pch_find_conditional(void);
int evaluate_preprocessor_expression(void);
void __stdcall finish_directive(char *name);
int __stdcall pch_resolve_condition(int condition);
int __stdcall skip_inactive_directives(int mode);
unsigned char skip_directive_space(void);
int push_conditional_level(void);
void define_source_macro(void);
char *read_include_filename(void);
void process_pragma(void);
void undefine_source_macro(void);
void warn_ignored_token(void);
extern unsigned conditional_branch_state[32];
extern short pch_include_count;

char *read_angle_include_filename(void);

char *trim_filename_space(char *begin, char *end);

int __stdcall pch_skip_include(char *filename);
int __stdcall open_include_in_source_directory(char *filename);
void __stdcall search_include_directories(char *filename);
void __stdcall pch_record_include(char *name_end);

void read_macro_parameters(void);
unsigned read_macro_replacement(void);
extern int macro_parameter_count;

unsigned char find_conditional_directive(void);

void __stdcall skip_inactive_literal(int delimiter);

int skip_macro_space(void);
void __stdcall scan_macro_argument_identifier(int first_character);

extern int macro_space_seen;
void __stdcall macro_scan_error(unsigned code);

#endif
