#include <windows.h>
static int attached;
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
        ++attached;
    else if (reason == DLL_PROCESS_DETACH)
        --attached;
    return TRUE;
}
int WINAPI ProbeSum(int *values, int count)
{
    int index, total = 0;
    if (attached != 1)
        return -1;
    for (index = 0; index < count; ++index)
        total += values[index];
    values[0] = total;
    return total;
}
