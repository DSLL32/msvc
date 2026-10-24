#include <windows.h>
typedef int(WINAPI *Probe)(int *, int);
void WINAPI start(void)
{
    char path[1024];
    static const char message[] = "sum=7\n";
    static const BYTE bytes[] = {0, 255};
    HMODULE module;
    Probe probe;
    HANDLE file;
    DWORD count;
    int values[2];
    if (!GetEnvironmentVariableA("RECON_ORIGINAL_DLL", path, sizeof(path)))
        ExitProcess(10);
    module = LoadLibraryA(path);
    if (!module)
        ExitProcess(11);
    probe = (Probe)GetProcAddress(module, "ProbeSum");
    values[0] = 2;
    values[1] = 5;
    if (!probe || probe(values, 2) != 7)
        ExitProcess(12);
    file = CreateFileA("out.bin", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (file == INVALID_HANDLE_VALUE ||
        !WriteFile(file, bytes, sizeof(bytes), &count, NULL) || !CloseHandle(file))
        ExitProcess(13);
    if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), message, sizeof(message) - 1,
                   &count, NULL) ||
        !FreeLibrary(module))
        ExitProcess(14);
    ExitProcess(0);
}
