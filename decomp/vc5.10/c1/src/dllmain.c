#include "c1.h"
#include <recon.h>

/* Original application DllMain: RVA 0x1218e. Not yet reconstructed. */
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    __asm { int 3 }
    return FALSE;
}
