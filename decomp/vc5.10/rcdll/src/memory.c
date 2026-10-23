#include "command.h"
#include <recon.h>

void *__stdcall rc_allocate(int bytes)
{
    char *block=HeapAlloc(rc_heap,9,bytes+8);
    if (!block) {
        rc_format(rc_error_text,2048,rc_message(1120),bytes);
        rc_fatal(rc_error_text);
        return NULL;
    }
    return block+8;
}
