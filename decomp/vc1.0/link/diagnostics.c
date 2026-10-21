/* Not yet reached: variadic warning reporting, original 00021830. */
#include "linker-types.h"
void warning_message(const char *file,unsigned message,...)
{
    __asm { int 3 }
}

/* Not yet reached: fatal diagnostic, original 000218d0. */
void fatal_message(const char *file,int message,...)
{
    __asm { int 3 }
}

void out_of_memory(void) { __asm { int 3 } }
