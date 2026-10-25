/* Declarations for the retained startup.c variant only; no shared CRT replacement. */
#ifndef C1XX_CRT_STARTUP_H
#define C1XX_CRT_STARTUP_H
#include "c1xx.h"
#include <recon.h>

typedef BOOL (WINAPI *RawDllEntry)(HINSTANCE, DWORD, LPVOID);
#ifdef __cplusplus
extern "C" {
#endif
extern int process_attached;
extern RawDllEntry raw_dll_entry;
extern int dll_adjust_fdiv;
extern Initializer crt_initializers[2];
extern Initializer *onexit_begin, *onexit_end;
__declspec(dllimport) extern int _adjust_fdiv;
BOOL WINAPI _DllMainCRTStartup(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI _CRT_INIT(HINSTANCE, DWORD, LPVOID);
void __cdecl _initterm(Initializer *, Initializer *);
#ifdef __cplusplus
}
#endif
#endif
