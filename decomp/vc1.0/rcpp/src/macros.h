#ifndef RCPP_MACROS_H
#define RCPP_MACROS_H
#include "condition.h"
#include <recon.h>
extern char *rc_expansion_work, *rc_macro_output_start, *rc_argument_work;
extern unsigned rc_argument_count;
extern int rc_expansion_line;
void __stdcall rc_push_macro(RcMacro *);
void rc_start_macro_body(void);
int rc_finish_macro_body(void);
void __stdcall rc_read_macro_arguments(RcMacro *, int);
void __stdcall rc_stringize_argument(unsigned char, const char *);
char *rc_macro_argument(void);
void __stdcall rc_compact_macro_output(char *, char *);
void rc_start_macro_argument(void);
char *__stdcall rc_copy_macro_quote(char *, unsigned char);
char *__stdcall rc_substituted_argument(int *);
#endif
