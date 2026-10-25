#ifndef C2_CRT_H
#define C2_CRT_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <excpt.h>
#include <io.h>
#include <ctype.h>
/* This target imports the FILE array; the SDK accessor adds a graph vertex. */
#undef _iob
__declspec(dllimport) extern FILE _iob[];
/* Its character macros also import data directly rather than SDK accessors. */
#undef _pctype
__declspec(dllimport) extern unsigned short *_pctype;
#undef MB_CUR_MAX
#define MB_CUR_MAX __mb_cur_max
__declspec(dllimport) extern int __mb_cur_max;
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

void mainCRTStartup(void);
void c2_set_argv(void);
void c2_set_default_precision(void);
int c2_math_error(struct _exception *);
int c2_register_exit(C2Initializer callback);
C2Initializer c2_register_onexit(C2Initializer callback);
#endif
