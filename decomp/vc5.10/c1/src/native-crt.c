#include "c1.h"
#include <recon.h>

/* Linked CRT state: original RVAs 0x5c090 and 0x5c09c. */
int c1_crt_attached;
C1DllEntry c1_raw_dllmain;

/* Original linked CRT DLL entry, RVA 0x1202a; cold code at 0x302dc.
 * This is a trial body, not yet accepted. Keep CRT_INIT and DllMain trapping. */
BOOL WINAPI c1_crt_entry(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    BOOL result = TRUE;

    if (reason == DLL_PROCESS_DETACH && !c1_crt_attached)
        return FALSE;

    if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH) {
        if (c1_raw_dllmain)
            result = (*c1_raw_dllmain)(module, reason, reserved);
        if (!result)
            return FALSE;
        if (!c1_crt_init(module, reason, reserved))
            return FALSE;
    }

    result = DllMain(module, reason, reserved);
    if (reason == DLL_PROCESS_ATTACH && !result)
        c1_crt_init(module, DLL_PROCESS_DETACH, reserved);

    if (reason == DLL_PROCESS_DETACH || reason == DLL_THREAD_DETACH) {
        if (!c1_crt_init(module, reason, reserved))
            result = FALSE;
        if (result && c1_raw_dllmain)
            result = (*c1_raw_dllmain)(module, reason, reserved);
    }
    return result;
}

/* Original CRT_INIT, RVA 0x120ce, remains the next named frontier. */
BOOL WINAPI c1_crt_init(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    __asm { int 3 }
    return FALSE;
}
