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
extern short pch_conditional_depth;

char *read_angle_include_filename(void);

char *trim_filename_space(char *begin, char *end);

#endif
