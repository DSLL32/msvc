/* Pending graph vertices. Bodies trap until their own paired frontier. */
#include "driver.h"
#include <stdlib.h>

int __stdcall switch_character(char character)
{
    return character == '/' || character == '-';
}
BOOL WINAPI interrupt_resource(DWORD control)
{
    __asm int 3
    return 0;
}
void __cdecl resource_finish(char *format, ...)
{
    __asm int 3
}
void __stdcall initialize_lexer(FILE *input)
{
    __asm int 3
}
void __cdecl compile_resources(void)
{
    __asm int 3
}
long __stdcall seek_file(FILE *file, long offset, int origin)
{
    __asm int 3
    return 0;
}
