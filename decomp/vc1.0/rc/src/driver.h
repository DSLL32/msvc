#ifndef RC_DRIVER_H
#define RC_DRIVER_H
#include "rc.h"
#include <stdio.h>
#include <recon.h>
extern FILE *diagnostics, *resource_output, *preprocessor_input;
extern char resource_filename[MAX_PATH], input_filename[MAX_PATH];
extern char preprocessor_options[1032], include_paths[520];
extern char *temporary_input, *temporary_output;
extern unsigned int default_language;
extern int error_count, kanji_mode, verbose;
void __cdecl resource_finish(char *, ...);
void * __stdcall resource_alloc(unsigned int);
BOOL WINAPI interrupt_resource(DWORD);
int __stdcall switch_character(char);
int __cdecl rc_toupper(int);
BOOL __stdcall prepare_input(char *);
BOOL __stdcall initialize_lexer(FILE *);
BOOL __cdecl compile_resources(void);
long __stdcall seek_file(FILE *, long, int);
void __stdcall write_file(FILE *, void *, unsigned int);
#endif
