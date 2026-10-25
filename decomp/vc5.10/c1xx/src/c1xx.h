#ifndef C1XX_RECONSTRUCTION_H
#define C1XX_RECONSTRUCTION_H

#include <windows.h>
#include <stdlib.h>
#include <recon.h>

typedef void (__cdecl *Initializer)(void);

#ifdef __cplusplus
extern "C" {
#endif
extern int compiler_mode;
extern HINSTANCE dll_module;


BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
int initialize_compiler_pass(void);
int cleanup_compiler_pass(void);
void WINAPI AbortCompilerPass(DWORD);
int WINAPI InvokeCompilerPass(int, char **, int);

#ifdef __cplusplus
}
#endif

#endif
