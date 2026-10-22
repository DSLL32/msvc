/* Pending graph vertices. Bodies trap until their own paired frontier. */
#include "driver.h"
#include <stdlib.h>

BOOL WINAPI interrupt_resource(DWORD control)
{
    __asm int 3
    return 0;
}
void __cdecl resource_finish(char *format, ...)
{
    __asm int 3
}
