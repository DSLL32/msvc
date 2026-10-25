/* Original-only typed DLL host; shared recon-tool owns debugger/capture work. */
#include <windows.h>
#include <recon.h>

#ifndef RECON_NATIVE_TEST
typedef int (WINAPI *CompilerPass)(int, char **, BOOL);

int main(int argc, char **argv)
{
    char path[MAX_PATH];
    DWORD length;
    HMODULE module;
    CompilerPass invoke;
    FARPROC abort_pass;
    int result;

    length = GetEnvironmentVariableA("RECON_ORIGINAL_DLL", path, sizeof(path));
    if (!length || length >= sizeof(path))
        return 126;
    module = LoadLibraryA(path);
    if (!module)
        return 126;
    invoke = (CompilerPass)GetProcAddress(module, "_InvokeCompilerPass@12");
    abort_pass = GetProcAddress(module, "_AbortCompilerPass@4");
    if (!invoke || !abort_pass ||
        (FARPROC)invoke != GetProcAddress(module, (LPCSTR)2) ||
        abort_pass != GetProcAddress(module, (LPCSTR)1)) {
        FreeLibrary(module);
        return 126;
    }
    result = (*invoke)(argc, argv, TRUE);
    if (!FreeLibrary(module))
        return 126;
    return result;
}
#endif
