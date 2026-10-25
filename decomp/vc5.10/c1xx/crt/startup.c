/* Retained binary-specific CRT variant, predating the current CRT boundary policy.
 * The two existing startup vertices preserve this DLL's attach/detach graph,
 * stdcall ABI, initializer ordering and onexit lifetime in both builds. They
 * remain separately classified CRT support (451 bytes), with zero App credit;
 * public malloc/free/_initterm still use MSVCRT or the shared native shims.
 * This layout move authorizes no additional CRT reconstruction. */
#include "startup.h"
#include <recon.h>

int process_attached;
RawDllEntry raw_dll_entry;
int dll_adjust_fdiv;
Initializer crt_initializers[2] = { NULL, NULL };
Initializer *onexit_begin, *onexit_end;

/* The original CRT entry remains a separate target vertex. */
BOOL WINAPI _DllMainCRTStartup(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    BOOL result = TRUE;
    if (reason == DLL_PROCESS_DETACH && !process_attached)
        return FALSE;
    if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH) {
        if (raw_dll_entry)
            result = raw_dll_entry(module, reason, reserved);
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
        if (result && raw_dll_entry)
            result = raw_dll_entry(module, reason, reserved);
    }
    return result;
}

BOOL WINAPI _CRT_INIT(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    Initializer *cursor;
    if (reason == DLL_PROCESS_DETACH) {
        if (process_attached <= 0)
            return FALSE;
        --process_attached;
    }
    dll_adjust_fdiv = _adjust_fdiv;
    if (reason == DLL_PROCESS_ATTACH) {
        onexit_begin = (Initializer *)malloc(128);
        if (!onexit_begin)
            return FALSE;
        *onexit_begin = NULL;
        onexit_end = onexit_begin;
        _initterm(crt_initializers, crt_initializers + 1);
        ++process_attached;
        return TRUE;
    }
    if (reason == DLL_PROCESS_DETACH && onexit_begin) {
        cursor = onexit_end - 1;
        while (cursor >= onexit_begin) {
            if (*cursor)
                (*cursor)();
            --cursor;
        }
        free(onexit_begin);
        onexit_begin = NULL;
    }
    return TRUE;
}
