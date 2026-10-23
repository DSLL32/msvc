#ifndef RECONSTRUCTED_RCDLL_H
#define RECONSTRUCTED_RCDLL_H
#include <windows.h>
#include <recon.h>

typedef int (WINAPI *RC_DIAGNOSTIC)(DWORD, DWORD, char *);
/* The help case observes only NULL; resource callback record ABI is unrecovered. */
typedef LPVOID RC_RESOURCE;
BOOL WINAPI recon_dll_entry(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI rcdll_crt_init(HINSTANCE, DWORD, LPVOID);
extern HINSTANCE dll_instance;
extern int dll_attach_count;
extern BOOL (WINAPI *dll_raw_main)(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
extern HWND rc_window;
extern RC_DIAGNOSTIC rc_diagnostic;
extern RC_RESOURCE rc_resource;
int __cdecl rc_main(int, char **);
BOOL WINAPI Handler(DWORD);
int WINAPI RC(HWND, DWORD, RC_DIAGNOSTIC, RC_RESOURCE, int, char **);
#endif
