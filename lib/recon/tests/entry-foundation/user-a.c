#include <recon.h>
/* Separate App/DLL-style callers resolve against the same process shim. */
HMODULE entry_user_a(const char *name)
{
    HMODULE module=LoadLibraryA(name);
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(recon_fmode!=0)abort();
#endif
    int (__cdecl *function)(int)=(int (__cdecl *)(int))GetProcAddress(module,"AddOne");
    if(!function || function(41)!=42)abort();
    return module;
}
