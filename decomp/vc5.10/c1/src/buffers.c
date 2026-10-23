#include "compiler-state.h"
#include <recon.h>

int __cdecl initialize_output_buffer_123ae(void)
{
    c1_bss.output_buffers[0].good = 1;
    c1_bss.output_buffers[0].data = NULL;
    c1_bss.output_buffers[0].cursor = NULL;
    c1_bss.output_buffers[0].capacity = 0;
    c1_bss.output_buffers[0].relocated = NULL;
    c1_bss.output_buffers[0].relocation_context = NULL;
    c1_bss.output_buffers[0].stream = NULL;
    c1_bss.output_buffers[0].file_descriptor = -1;
    c1_bss.output_buffers[0].mode = 0;
    return 0;
}

void __cdecl destroy_output_buffer_23940(void)
{
    __asm { int 3 }
}

int __cdecl initialize_output_buffer_12423(void)
{
    c1_bss.output_buffers[1].good = 1;
    c1_bss.output_buffers[1].data = NULL;
    c1_bss.output_buffers[1].cursor = NULL;
    c1_bss.output_buffers[1].capacity = 0;
    c1_bss.output_buffers[1].relocated = NULL;
    c1_bss.output_buffers[1].relocation_context = NULL;
    c1_bss.output_buffers[1].stream = NULL;
    c1_bss.output_buffers[1].file_descriptor = -1;
    c1_bss.output_buffers[1].mode = 0;
    return 0;
}


void __cdecl destroy_output_buffer_23939(void)
{
    __asm { int 3 }
}
