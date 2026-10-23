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
extern char *rc_input_temporary;
extern UINT rc_codepage,rc_saved_codepage;
extern int rc_warnings;
extern WCHAR rc_filename_storage[257],*rc_filename;
extern unsigned rc_preparing_line,rc_prepared_lines;
unsigned __stdcall rc_write_bytes(FILE *,const void *,unsigned);
int __stdcall rc_detect_encoding(FILE *);
int __stdcall rc_is_text_unicode(const void *,unsigned,unsigned *);
int __stdcall rc_read_wide_line(WCHAR *,unsigned,int,FILE *);
WCHAR *__stdcall rc_skip_white(WCHAR *,int);
int __stdcall rc_prefix_equal(const WCHAR *,const WCHAR *);
void __stdcall rc_notify_progress(WPARAM,LPARAM);
void __stdcall rc_input_error(unsigned);
void __stdcall rc_input_fatal(unsigned);
void __stdcall rc_input_warning(unsigned);
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
