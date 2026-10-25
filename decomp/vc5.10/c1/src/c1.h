#ifndef C1_RECONSTRUCTION_H
#define C1_RECONSTRUCTION_H
#include <windows.h>
#include <recon.h>


#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
int WINAPI InvokeCompilerPass(int, char **, int);
void WINAPI AbortCompilerPass(int);
extern HINSTANCE c1_dll_instance;
extern int c1_pass_mode;
int __cdecl initialize_compiler_pass(void);
int __cdecl main(int, char **);
int __cdecl cleanup_compiler_pass(void);
typedef void (__cdecl *C1Initializer)(void);
#ifdef __cplusplus
}
#endif
#endif
