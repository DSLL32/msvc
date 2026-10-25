/* Declarations for the retained binary-specific startup in startup.c. */
#ifndef C1_CRT_STARTUP_H
#define C1_CRT_STARTUP_H
#include "c1.h"
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef BOOL (WINAPI *C1DllEntry)(HINSTANCE, DWORD, LPVOID);
extern int c1_crt_attached;
extern C1DllEntry c1_raw_dllmain;
extern int c1_crt_adjust_fdiv;
extern C1Initializer *c1_onexit_begin, *c1_onexit_end;
extern C1Initializer c1_initializers[1];
BOOL WINAPI _DllMainCRTStartup(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI _CRT_INIT(HINSTANCE, DWORD, LPVOID);
#ifdef __cplusplus
}
#endif
#endif
