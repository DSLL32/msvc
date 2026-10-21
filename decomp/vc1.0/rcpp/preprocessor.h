#ifndef RCPP_PREPROCESSOR_H
#define RCPP_PREPROCESSOR_H
#include "rcpp.h"
typedef struct RcOption {
    const char *pattern;
    void *destination;
    signed char again;
    unsigned char action;
} RcOption;
typedef struct RcSuboption {
    int letter, action;
    int *destination;
} RcSuboption;
typedef struct RcStringOptions {
    int remaining;
    char *values[100];
} RcStringOptions;
typedef struct RcOptions {
    int extensions, zE, debug_information, prototype_output, zI, zc;
    int pf, keep_comments, preprocess_lines, preprocess_plain, unsigned_char;
    int packing, intrinsics, loop_optimization, xc, H, stack_check;
    int pascal_calls, char_type, Fs, R;
    char *path_characters, *codepage, *input_name, *output_name, *debug_file;
    char *error_file, *il_file, *version, *A, *Q, *unmatched_input;
} RcOptions;
extern char *rc_current_option;
unsigned __stdcall rc_parse_option_number(char *);
void __stdcall rc_apply_option(RcOption *, char *, int);
int __stdcall rc_match_suffix(const char *, const char *);
extern RcOptions rc_options;
extern RcStringOptions rc_definitions, rc_include_directories;
extern char rc_diagnostic_buffer[256], rc_source_name[128];
extern char *rc_message_format;
extern int rc_error_count, rc_preprocessing_started, rc_input_depth;
char *rc_next_argument(void);
int __stdcall rc_parse_option(RcOption *, char *, char *(__cdecl *)(void), int);
char *__stdcall rc_message(unsigned);
void rc_format(char *, const char *, ...);
void __stdcall rc_fatal(unsigned);
BOOL WINAPI rc_control_handler(DWORD);
void __stdcall rc_initialize_preprocessor(char *, char *, RcStringOptions *);
void rc_emit_tokens(void);
#endif
