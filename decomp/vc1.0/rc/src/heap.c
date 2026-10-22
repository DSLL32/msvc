#include "driver.h"
#include <stdlib.h>
#include <recon.h>

/* Checked resource allocation, original 0x01007000..0x01007026. */
void * __stdcall resource_alloc(unsigned int bytes)
{
    void *block = calloc(1, bytes);
    if (!block)
        resource_finish("RC : fatal error RW1020: out of memory, needed %d bytes", bytes);
    return block;
}

/* Original 0x01007030..0x01007046. Returning NULL lets callers clear a
 * released resource pointer with the same wrapper. */
void * __stdcall resource_free(void *block)
{
    if (block)
        free(block);
    return 0;
}
