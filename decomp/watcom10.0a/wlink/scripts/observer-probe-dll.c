/* Authored debugger probe DLL only. Not reconstructed Watcom code. */
#include <windows.h>
#include <recon.h>
typedef DWORD (__cdecl *ProbeCallback)(DWORD);
__declspec(dllexport) volatile DWORD fixture_dll_entries;
__declspec(dllexport) volatile DWORD fixture_bad_return_entries;
__declspec(dllexport) DWORD __stdcall ProbeValue(DWORD value, ProbeCallback callback)
{
    ++fixture_dll_entries;
    if(callback) return callback(value);
    return value+7;
}

/* Deliberately wrong cleanup for a two-argument stdcall function. The harness
 * must reject its actual RET4 before the caller can write its completion file. */
#pragma warning(disable:4035) /* Authored naked body returns through its RET4. */
__declspec(dllexport) __declspec(naked) DWORD __stdcall ProbeBadReturn(DWORD a, DWORD b)
{
    __asm {
        inc dword ptr fixture_bad_return_entries
        mov eax,[esp+4]
        add eax,[esp+8]
        ret 4
    }
}
#pragma warning(default:4035)
