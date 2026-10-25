/* RC's earlier CRT lacks the newer termination-done guard. Recover the whole
 * original startup/exit group so its private doexit/initterm vertices remain
 * shared rather than calling newer private neighbors in CRT0DAT.OBJ. */
#include "crt-runtime.h"
#include <stdlib.h>
#include <recon.h>

/* Original 0x0100e1b0..0x0100e1e0. */
void __cdecl _cinit(void)
{
    if (_FPinit)
        (*_FPinit)();
    _initterm(__xi_a, __xi_z);
    _initterm(__xc_a, __xc_z);
}
/* Original 0x0100e1e0..0x0100e1f2. */
void __cdecl exit(int status)
{
    doexit(status, 0, 0);
}
/* Original 0x0100e200..0x0100e212. */
void __cdecl _exit(int status)
{
    doexit(status, 1, 0);
}
/* Original 0x0100e220..0x0100e22f. */
void __cdecl _cexit(void)
{
    doexit(0, 0, 1);
}
/* Original 0x0100e230..0x0100e23f. */
void __cdecl _c_exit(void)
{
    doexit(0, 1, 1);
}

/* Original 0x0100e240..0x0100e2b8. Callback traversal stays in reverse
 * registration order; only the lower boundary is reread after each callback. */
void __cdecl doexit(int status, int quick, int return_to_caller)
{
    CRTCallback *next;
    _exitflag = (char)return_to_caller;
    if (!quick) {
        if (__onexitbegin) {
            next = __onexitend;
            while (next > __onexitbegin) {
                --next;
                if (*next)
                    (**next)();
            }
        }
        _initterm(__xp_a, __xp_z);
    }
    _initterm(__xt_a, __xt_z);
    if (!return_to_caller)
        ExitProcess(status);
}

/* Original 0x0100e2c0..0x0100e2e2. */
void __cdecl _initterm(CRTCallback *begin, CRTCallback *end)
{
    while (begin < end) {
        if (*begin)
            (**begin)();
        ++begin;
    }
}
