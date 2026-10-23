#ifndef C2_H
#define C2_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>
#include <excpt.h>
#include <io.h>
#include <recon.h>

typedef void (__cdecl *C2Initializer)(void);
struct _exception;
struct _startupinfo { int newmode; };

/* Internal MSVCRT declarations used by the original embedded startup. */
__declspec(dllimport) void __cdecl __set_app_type(int);
__declspec(dllimport) int *__cdecl __p__fmode(void);
__declspec(dllimport) int *__cdecl __p__commode(void);
__declspec(dllimport) int _adjust_fdiv;
__declspec(dllimport) void __cdecl __setusermatherr(int (__cdecl *)(struct _exception *));
void __cdecl _initterm(C2Initializer *, C2Initializer *);
__declspec(dllimport) void __cdecl __getmainargs(int *, char ***, char ***, int, struct _startupinfo *);
__declspec(dllimport) char ***__cdecl __p___initenv(void);
int __cdecl _XcptFilter(unsigned long, struct _EXCEPTION_POINTERS *);

extern C2Initializer *c2_onexit_begin, *c2_onexit_end;
extern int c2_fmode, c2_commode, c2_adjust_fdiv;
extern int c2_default_math_error, c2_new_mode, c2_wildcards;
extern C2Initializer c2_c_initializers[1], c2_cpp_initializers[1];

void c2_crt_startup(void);
void c2_set_argv(void);
void c2_set_default_precision(void);
int c2_math_error(struct _exception *);
int c2_main(int, char **, char **);
int c2_register_exit(C2Initializer callback);
C2Initializer c2_register_onexit(C2Initializer callback);
void __fastcall c2_read_environment(char *flags);
void c2_initialize_options(void);
void c2_print_banner(void);
FILE *__fastcall c2_open_intermediate(char *base, char *suffix, char *mode, char *description);
void c2_read_header(void);
void c2_reset_symbol_table(void);
void *c2_create_type_table(void);
void c2_initialize_constants(void);
void c2_initialize_opcodes(void);
void c2_initialize_machine(void);
void c2_initialize_trees(void);
void c2_initialize_codegen(void);
void c2_initialize_registers(void);
void c2_initialize_optimizer(void);
void c2_compile_unit(void);
void c2_finalize_output(void);
void c2_flush_debug(void);
void c2_cleanup(void);
BOOL WINAPI c2_console_handler(DWORD control);
void __fastcall c2_report_internal(char *source, int line);
void c2_fatal(int message, ...);
#endif
