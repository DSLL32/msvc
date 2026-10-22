/* Load the case DLL and call its exported stdcall GUI entry point. */
#include <windows.h>
#include <string.h>

typedef int (__stdcall *HelloEntry)(HINSTANCE, HINSTANCE, LPSTR, int);

int main(void)
{
    char path[MAX_PATH], command_line[1];
    char *name;
    HMODULE library;
    HelloEntry entry;
    int result;
    if (!GetModuleFileName(NULL, path, sizeof(path))) return 10;
    name = strrchr(path, '\\');
    if (!name) return 11;
    strcpy(name + 1, "hello.dll");
    library = LoadLibrary(path);
    if (!library) return 12;
    entry = (HelloEntry)GetProcAddress(library, "_HelloMain@16");
    if (!entry) { FreeLibrary(library); return 13; }
    command_line[0] = 0;
    result = entry((HINSTANCE)library, NULL, command_line, SW_SHOWNORMAL);
    if (!UnregisterClass("HelloWindow", (HINSTANCE)library)) return 14;
    if (!FreeLibrary(library)) return 15;
    return result;
}
