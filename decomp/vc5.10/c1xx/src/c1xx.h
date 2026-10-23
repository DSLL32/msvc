#ifndef C1XX_RECONSTRUCTION_H
#define C1XX_RECONSTRUCTION_H

#include <windows.h>
#include <stdlib.h>
#include <recon.h>

typedef void (__cdecl *Initializer)(void);
typedef BOOL (WINAPI *RawDllEntry)(HINSTANCE, DWORD, LPVOID);

#ifdef __cplusplus
extern "C" {
#endif
extern int process_attached;
extern int compiler_mode;
extern HINSTANCE dll_module;
extern RawDllEntry raw_dll_entry;
extern int dll_adjust_fdiv;
extern Initializer crt_initializers[2];
extern Initializer *onexit_begin, *onexit_end;

__declspec(dllimport) extern int _adjust_fdiv;

BOOL WINAPI _DllMainCRTStartup(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI _CRT_INIT(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
void __cdecl _initterm(Initializer *, Initializer *);
int initialize_compiler_pass(void);
int cleanup_compiler_pass(void);
void WINAPI AbortCompilerPass(DWORD);
int WINAPI InvokeCompilerPass(int, char **, int);

#ifdef __cplusplus
}
#endif

#endif
