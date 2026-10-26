/* Genuine DLL input; compile explicitly with VC5 /TC /DLINK_FIXTURE_DLL_INPUT
 * /c /O2 /G5 /Zl. This is fixture source, never reconstruction logic. */
#ifdef LINK_FIXTURE_DLL_INPUT
#include <windows.h>
int shared_value=7;
static HINSTANCE dll_instance;
__declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved)
{
    if(reason==DLL_PROCESS_ATTACH)dll_instance=instance;
    return TRUE;
}
int WINAPI HelloWindow(HWND owner,int flags)
{
    return MessageBoxA(owner,"Hello, world!","Hello DLL",flags|MB_OK);
}
int WINAPI AddNumbers(int left,int right)
{
    return left+right+shared_value;
}
#endif
