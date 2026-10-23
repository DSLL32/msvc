#ifndef VC510_RC_H
#define VC510_RC_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <excpt.h>
#include <recon.h>

typedef int (WINAPI *DiagnosticCallback)(HWND, unsigned, char *);
/* Only NULL is observed for this slot in the first case. */
typedef void *ResourceCallback;
/* The six stack slots and cleanup are evidenced by both original modules. */
int WINAPI RC(HWND, unsigned, DiagnosticCallback,
                                   ResourceCallback, int, char **);
BOOL WINAPI Handler(DWORD);
int rc_main(int, char **, char **);
int WINAPI rc_diagnostic(HWND, unsigned, char *);
void mainCRTStartup(void);
void set_argv(void);
void set_default_precision(void);
unsigned int __cdecl _controlfp(unsigned int, unsigned int);
struct _exception;
int math_error(struct _exception *);
struct _startupinfo { int newmode; };
__declspec(dllimport) void __cdecl __set_app_type(int);
__declspec(dllimport) int *__cdecl __p__fmode(void);
__declspec(dllimport) int *__cdecl __p__commode(void);
__declspec(dllimport) char ***__cdecl __p___initenv(void);
__declspec(dllimport) void __cdecl __getmainargs(int *, char ***, char ***, int, struct _startupinfo *);
__declspec(dllimport) void __cdecl __setusermatherr(int (__cdecl *)(struct _exception *));
__declspec(dllimport) extern int _adjust_fdiv;
void __cdecl _initterm(void (__cdecl **)(void), void (__cdecl **)(void));
int __cdecl _XcptFilter(unsigned long, struct _EXCEPTION_POINTERS *);
#endif
