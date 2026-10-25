#ifndef RECON_RCDLL_CRT_H
#define RECON_RCDLL_CRT_H
#include <windows.h>
#include <recon.h>

BOOL WINAPI _DllMainCRTStartup(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI _CRT_INIT(HINSTANCE, DWORD, LPVOID);
extern int dll_attach_count;
extern BOOL (WINAPI *dll_raw_main)(HINSTANCE, DWORD, LPVOID);
#endif
