#include "rcdll.h"
#include <recon.h>

HINSTANCE dll_instance;

/* Application DllMain, original RVA8b29..8b46. */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        dll_instance = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}
