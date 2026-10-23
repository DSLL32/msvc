#ifndef RECONSTRUCTED_RC_COMMAND_H
#define RECONSTRUCTED_RC_COMMAND_H
#include <stdio.h>
#include <setjmp.h>
#include "rcdll.h"
#include <recon.h>

extern jmp_buf rc_abort_context;
extern int rc_result;
extern HANDLE rc_heap;
extern char *rc_include_paths;
extern char rc_error_text[2048];
HANDLE __stdcall rc_initialize(void);
void *__stdcall rc_allocate(int);
int __stdcall rc_release(void *);
int __stdcall rc_is_option(char);
char *__stdcall rc_message(DWORD);
void __cdecl rc_format(char *, DWORD, const void *, ...);
void __stdcall rc_emit(char *);
void __stdcall rc_fatal(char *);
long __stdcall rc_seek(FILE *, long, int);
int __stdcall rc_prepare_input(char *);
int __stdcall rc_preprocess(int, char **, int);
int __stdcall rc_initialize_resource(void);
int __stdcall rc_initialize_input(FILE *);
int __stdcall rc_parse_resources(void);
int __stdcall rc_flush_resources(FILE *);
unsigned __stdcall rc_align_output(FILE *);
void __stdcall rc_cleanup(void);
#endif
