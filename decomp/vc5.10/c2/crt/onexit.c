/* This embedded CRT vertex calls the DLL API directly. The public stdlib.h
 * _onexit declaration selects an extra static wrapper and cannot be used here. */
#include <recon.h>

typedef void (__cdecl *C2Initializer)(void);
typedef int (__cdecl *C2OnexitCallback)(void);
__declspec(dllimport) C2OnexitCallback __cdecl _onexit(C2OnexitCallback);
C2OnexitCallback __cdecl __dllonexit(C2OnexitCallback,
                                    C2OnexitCallback **, C2OnexitCallback **);
extern C2Initializer *c2_onexit_begin, *c2_onexit_end;

C2Initializer c2_register_onexit(C2Initializer callback)
{
    if (c2_onexit_begin == (C2Initializer *)-1)
        return (C2Initializer)_onexit((C2OnexitCallback)callback);
    return (C2Initializer)__dllonexit((C2OnexitCallback)callback,
                    (C2OnexitCallback **)&c2_onexit_begin,
                    (C2OnexitCallback **)&c2_onexit_end);
}
