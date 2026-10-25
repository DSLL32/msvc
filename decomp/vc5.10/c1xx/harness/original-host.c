/* Original-only typed DLL host. The shared broker owns masking and observation. */
#include <windows.h>
#include <recon.h>

typedef int (WINAPI *InvokePass)(int, char **, int);

int main(int argc, char **argv)
{
    char path[MAX_PATH];
    DWORD length;
    HMODULE module;
    InvokePass invoke;
    int result;

    length = GetEnvironmentVariableA("RECON_ORIGINAL_DLL", path, sizeof(path));
    if (!length || length >= sizeof(path))
        return 126;
    module = LoadLibraryA(path);
    if (!module)
        return 126;
    invoke = (InvokePass)GetProcAddress(module, "_InvokeCompilerPass@12");
    if (!invoke) {
        FreeLibrary(module);
        return 126;
    }
    result = invoke(argc, argv, 1);
    if (!FreeLibrary(module))
        return 126;
    return result;
}
