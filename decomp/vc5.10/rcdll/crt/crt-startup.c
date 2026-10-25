#include <stdlib.h>
#include "rcdll-crt.h"
#include <recon.h>

typedef void (__cdecl *DLL_EXIT_FUNCTION)(void);
extern __declspec(dllimport) int _adjust_fdiv;
extern void __cdecl _initterm(DLL_EXIT_FUNCTION *, DLL_EXIT_FUNCTION *);

int dll_platform_kind;
int dll_fdiv_adjust;
DLL_EXIT_FUNCTION *dll_exit_end;
DLL_EXIT_FUNCTION *dll_exit_base;
DLL_EXIT_FUNCTION dll_constructors[1];

/* Embedded CRT _CRT_INIT, RVA12bb0..12d1c. */
BOOL WINAPI _CRT_INIT(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    DWORD version;
    DLL_EXIT_FUNCTION *entry;
    if (reason == DLL_PROCESS_DETACH) {
        if (dll_attach_count <= 0)
            return FALSE;
        --dll_attach_count;
    }
    if (dll_platform_kind == 0) {
        version = GetVersion();
        if ((BYTE)version == 3 && (version & 0x80000000UL))
            ++dll_platform_kind;
        else
            --dll_platform_kind;
    }
    dll_fdiv_adjust = _adjust_fdiv;
    if (reason == DLL_PROCESS_ATTACH) {
        if (dll_platform_kind >= 0 && dll_attach_count != 0)
            return TRUE;
        if (dll_platform_kind < 0)
            dll_exit_base = (DLL_EXIT_FUNCTION *)malloc(128);
        else
            dll_exit_base = (DLL_EXIT_FUNCTION *)GlobalAlloc(0x2000,128);
        if (dll_exit_base == NULL)
            return FALSE;
        *dll_exit_base = NULL;
        dll_exit_end = dll_exit_base;
        _initterm(dll_constructors,dll_constructors+1);
        ++dll_attach_count;
        return TRUE;
    }
    if (reason == DLL_PROCESS_DETACH &&
        (dll_platform_kind < 0 || dll_attach_count == 0) && dll_exit_base != NULL) {
        entry = dll_exit_end-1;
        while ((unsigned long)entry >= (unsigned long)dll_exit_base) {
            if (*entry != NULL)
                (*entry)();
            --entry;
        }
        if (dll_platform_kind < 0)
            free(dll_exit_base);
        else
            GlobalFree((HGLOBAL)dll_exit_base);
        dll_exit_base = NULL;
    }
    return TRUE;
}
