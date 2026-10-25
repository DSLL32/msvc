/* Typed native DLL host. Historical CRT support is outside the App trace. */
#include <stdio.h>
#include <float.h>
#include "c1xx.h"
#include "startup.h"
#include "cases.h"
#include <recon.h>

int c1xx_case_lifecycle(int argc, char **argv)
{
    ReconModuleSpec spec = { "C1XX.DLL", "R:\\toolchains\\5.10\\bin\\C1XX.DLL",
        "e27df3bb9a37058c85dad3fb7d1ed30b9e52d38fb6c0f66c8fdba0a11111d3d9", RECON_MODULE_NO_TLS };
    HINSTANCE module = recon_register_module(&spec);
    Initializer *allocated;
    int (WINAPI *pass)(int, char **, int) = InvokeCompilerPass;
    void (WINAPI *abort_pass)(DWORD) = AbortCompilerPass;
    SetLastError(203);
    _controlfp(_PC_53, _MCW_PC);
    if (!module || recon_module_thread_calls_disabled(module) ||
        !_DllMainCRTStartup(module, DLL_PROCESS_ATTACH, NULL)) return 3;
    allocated = onexit_begin;
    if (process_attached != 1 || dll_module != module || raw_dll_entry ||
        dll_adjust_fdiv || !allocated || onexit_end != allocated || *allocated ||
        !recon_module_thread_calls_disabled(module) || GetLastError() != 203 ||
        crt_initializers[0] || crt_initializers[1] || _controlfp(0, 0) != 589855) return 4;
    puts("attach=1");
    if (!pass || !abort_pass || (void *)pass == (void *)abort_pass) return 5;
    puts("exports=1");
    if (!_DllMainCRTStartup(module, DLL_PROCESS_DETACH, NULL) ||
        process_attached || onexit_begin || onexit_end != allocated ||
        dll_module != module || GetLastError() != 203 || _controlfp(0, 0) != 589855) return 6;
    puts("detach=1");
    return 0;
}
