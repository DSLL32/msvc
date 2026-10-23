#include <stdlib.h>
#include "c1.h"
#include <recon.h>

#if !defined(RECON_NATIVE_TEST)
__declspec(dllimport) extern int _adjust_fdiv;
#endif
extern void __cdecl _initterm(C1Initializer *, C1Initializer *);

/* Linked CRT state: original RVAs 0x5c090 and 0x5c09c. */
int c1_crt_attached;
C1DllEntry c1_raw_dllmain;
int c1_crt_adjust_fdiv;
C1Initializer *c1_onexit_begin, *c1_onexit_end;
C1Initializer c1_initializers[1] = { NULL };

/* Original linked CRT DLL entry, RVA 0x1202a; cold code at 0x302dc.
 * Accepted with the original hot and cold chunks as one logical function. */
BOOL WINAPI _DllMainCRTStartup(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    BOOL result = TRUE;

    if (reason == DLL_PROCESS_DETACH && !c1_crt_attached)
        return FALSE;

    if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH) {
        if (c1_raw_dllmain)
            result = (*c1_raw_dllmain)(module, reason, reserved);
        if (!result)
            return FALSE;
        if (!_CRT_INIT(module, reason, reserved))
            return FALSE;
    }

    result = DllMain(module, reason, reserved);
    if (reason == DLL_PROCESS_ATTACH && !result)
        _CRT_INIT(module, DLL_PROCESS_DETACH, reserved);

    if (reason == DLL_PROCESS_DETACH || reason == DLL_THREAD_DETACH) {
        if (!_CRT_INIT(module, reason, reserved))
            result = FALSE;
        if (result && c1_raw_dllmain)
            result = (*c1_raw_dllmain)(module, reason, reserved);
    }
    return result;
}

/* Original CRT_INIT, RVA 0x120ce; cold code at 0x30318. Trial body. */
BOOL WINAPI _CRT_INIT(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    C1Initializer *cursor;

    if (reason == DLL_PROCESS_DETACH) {
        if (c1_crt_attached <= 0)
            return FALSE;
        --c1_crt_attached;
    }
    c1_crt_adjust_fdiv = _adjust_fdiv;

    if (reason == DLL_PROCESS_ATTACH) {
        c1_onexit_begin = (C1Initializer *)malloc(128);
        if (!c1_onexit_begin)
            return FALSE;
        *c1_onexit_begin = NULL;
        c1_onexit_end = c1_onexit_begin;
        _initterm(c1_initializers, c1_initializers + 1);
        ++c1_crt_attached;
    }

    if (reason == DLL_PROCESS_DETACH && c1_onexit_begin) {
        cursor = (C1Initializer *)((DWORD)c1_onexit_end - sizeof(C1Initializer));
        while ((DWORD)cursor >= (DWORD)c1_onexit_begin) {
            if (*cursor)
                (*cursor)();
            cursor = (C1Initializer *)((DWORD)cursor - sizeof(C1Initializer));
        }
        free(c1_onexit_begin);
        c1_onexit_begin = NULL;
    }
    return TRUE;
}
