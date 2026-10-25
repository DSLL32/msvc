/* Target-specific declarations for the already retained embedded RC CRT startup.
 * Shared imported CRT implementation/compatibility still belongs to lib/recon. */
#ifndef VC510_RC_STARTUP_H
#define VC510_RC_STARTUP_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <excpt.h>
#include "../src/rc.h"
#include <recon.h>

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
