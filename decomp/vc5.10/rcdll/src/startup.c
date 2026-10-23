#include "rcdll.h"
#include <recon.h>

int dll_attach_count;
BOOL (WINAPI *dll_raw_main)(HINSTANCE, DWORD, LPVOID);

/* _DllMainCRTStartup, original RVA 12d20..12ddf (CRT support). */
BOOL WINAPI _DllMainCRTStartup(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    BOOL result = TRUE;
    if (reason == DLL_PROCESS_DETACH && dll_attach_count == 0)
        return FALSE;
    if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH) {
        if (dll_raw_main != NULL)
            result = dll_raw_main(instance, reason, reserved);
        if (result)
            result = _CRT_INIT(instance, reason, reserved);
        if (!result)
            return FALSE;
    }
    result = DllMain(instance, reason, reserved);
    if (reason == DLL_PROCESS_ATTACH && !result)
        _CRT_INIT(instance, DLL_PROCESS_DETACH, reserved);
    if (reason == DLL_PROCESS_DETACH || reason == DLL_THREAD_DETACH) {
        if (!_CRT_INIT(instance, reason, reserved))
            result = FALSE;
        if (result && dll_raw_main != NULL)
            result = dll_raw_main(instance, reason, reserved);
    }
    return result;
}
