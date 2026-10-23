#ifndef C1_RECONSTRUCTION_H
#define C1_RECONSTRUCTION_H
#include <windows.h>
#include <recon.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
int WINAPI InvokeCompilerPass(int, char **, int);
void WINAPI AbortCompilerPass(int);
extern HINSTANCE c1_dll_instance;
typedef BOOL (WINAPI *C1DllEntry)(HINSTANCE, DWORD, LPVOID);
typedef void (__cdecl *C1Initializer)(void);
extern int c1_crt_attached;
extern C1DllEntry c1_raw_dllmain;
extern int c1_crt_adjust_fdiv;
extern C1Initializer *c1_onexit_begin, *c1_onexit_end;
extern C1Initializer c1_initializers[1];
BOOL WINAPI c1_crt_entry(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI c1_crt_init(HINSTANCE, DWORD, LPVOID);
#endif
