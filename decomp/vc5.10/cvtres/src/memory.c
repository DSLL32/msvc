#include "cvtres.h"
#include <stdlib.h>
#include <recon.h>

void *__stdcall allocate_block(unsigned long size)
{
    void *block = calloc(1, size);
    if (!block) {
        error(1102, size);
        exit(1);
    }
    return block;
}
