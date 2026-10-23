#include "c1.h"
#include <recon.h>

HINSTANCE c1_dll_instance;

/* Original application DllMain, RVA 0x1218e..0x121b1. */
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    c1_dll_instance = module;
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(module);
    return TRUE;
}
