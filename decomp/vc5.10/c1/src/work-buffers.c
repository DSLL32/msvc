#include "compiler-state.h"
#include <recon.h>

/* Each committed work area leaves five bytes beyond its initial limit. */
void __cdecl initialize_work_buffers(void)
{
    c1_bss.work_buffers[0].data = reserve_work_buffer(0x500000, 0x4000);
    c1_bss.work_buffers[0].limit = c1_bss.work_buffers[0].data + 0x3ffb;
    c1_bss.work_buffers[1].data = reserve_work_buffer(0x500000, 0x4000);
    c1_bss.work_buffers[1].limit = c1_bss.work_buffers[1].data + 0x3ffb;
}

void *__cdecl reserve_work_buffer(unsigned reserve_bytes, unsigned commit_bytes)
{
    __asm { int 3 }
    return NULL;
}
