#include <windows.h>
__declspec(dllexport) int answer(void) { return 42; }
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) { return TRUE; }
