#include <recon.h>
HMODULE entry_user_b(const char *name)
{
    HMODULE module=LoadLibraryA(name);
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(recon_fmode!=0)abort();
#endif
    int (WINAPI *function)(int)=(int (WINAPI *)(int))GetProcAddress(module,(const char *)2);
    if(!function || function(21)!=42)abort();
    return module;
}
