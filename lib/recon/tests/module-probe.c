/* Explicit original/native API probe, not a fixture dependency. */
#include <windows.h>
#include <stdio.h>
#include <errno.h>
#include <recon.h>

typedef BOOL (WINAPI *DisableCalls)(HMODULE);

int main(void)
{
    HMODULE module;
    DisableCalls disable;
    BOOL result;
#if defined(RECON_NATIVE_TEST)
    ReconModuleSpec spec={"module-probe.exe", "R:\\module-probe.exe",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS};
    module=recon_register_module(&spec);
    disable=recon_DisableThreadLibraryCalls;
#else
    module=GetModuleHandleA(NULL);
    disable=(DisableCalls)GetProcAddress(GetModuleHandleA("kernel32.dll"),
        "DisableThreadLibraryCalls");
#endif
    if (!module || !disable) return 1;
    errno=123; SetLastError(0x1122);
    result=disable(module);
    printf("valid %d errno=%d error=%lu\n", result, errno, GetLastError());
    errno=123; SetLastError(0x1122);
    result=disable(module);
    printf("repeat %d errno=%d error=%lu\n", result, errno, GetLastError());
    errno=123; SetLastError(0x1122);
    result=disable(NULL);
    printf("null %d errno=%d error=%lu\n", result, errno, GetLastError());
    errno=123; SetLastError(0x1122);
    result=disable((HMODULE)0x12345000);
    printf("unknown %d errno=%d error=%lu\n", result, errno, GetLastError());
    return 0;
}
