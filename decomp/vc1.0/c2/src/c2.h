#ifndef C2_H
#define C2_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
extern int argument_count;
extern char **arguments;
extern char *intermediate_prefix;
extern int compiler_errors, debugger_enabled;
extern FILE *expression_file, *symbol_file, *initializer_file, *global_file;
BOOL WINAPI compiler_control(DWORD event);
void compiler_cleanup(void);
void __fastcall append_environment_arguments(char *flags);
void parse_options(void);
void initialize_debugging(void);
void initialize_limits(void);
FILE *__fastcall open_intermediate(char *prefix, char *suffix, char *mode, char *description);
void initialize_frontend_data(void);
void initialize_backend(void);
void initialize_symbols(void);
void *initialize_output(void);
void initialize_codegen(void);
void initialize_registers(void);
void initialize_segments(void);
void initialize_globals(void);
void compile_functions(void);
void finish_output(void);
void finish_debugging(void);
void finish_globals(void);
void __fastcall compiler_internal_error(char *source, unsigned code);
/* Two register arguments and one callee-cleaned stack argument (RET 4). */
void __fastcall report_context_error(unsigned code, char *source, char *filename);
#endif
