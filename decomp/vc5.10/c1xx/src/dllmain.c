#include "c1xx.h"
#include <recon.h>

HINSTANCE dll_module;

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    dll_module = module;
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(module);
    return TRUE;
}
