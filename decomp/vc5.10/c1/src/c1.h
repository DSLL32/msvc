#ifndef C1_RECONSTRUCTION_H
#define C1_RECONSTRUCTION_H
#include <windows.h>
#include <recon.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
int WINAPI InvokeCompilerPass(int, char **, int);
void WINAPI AbortCompilerPass(int);
typedef BOOL (WINAPI *C1DllEntry)(HINSTANCE, DWORD, LPVOID);
extern int c1_crt_attached;
extern C1DllEntry c1_raw_dllmain;
BOOL WINAPI c1_crt_entry(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI c1_crt_init(HINSTANCE, DWORD, LPVOID);
#endif
