#include "compiler-state.h"
#include <recon.h>

int __cdecl initialize_output_buffer_123ae(void)
{
    c1_bss.output_buffer.good = 1;
    c1_bss.output_buffer.data = NULL;
    c1_bss.output_buffer.cursor = NULL;
    c1_bss.output_buffer.capacity = 0;
    c1_bss.output_buffer.relocated = NULL;
    c1_bss.output_buffer.relocation_context = NULL;
    c1_bss.output_buffer.stream = NULL;
    c1_bss.output_buffer.file_descriptor = -1;
    c1_bss.output_buffer.mode = 0;
    return 0;
}

void __cdecl destroy_output_buffer_23940(void)
{
    __asm { int 3 }
}

int __cdecl initialize_output_buffer_12423(void)
{
    __asm { int 3 }
    return 0;
}

void __cdecl destroy_output_buffer_23939(void)
{
    __asm { int 3 }
}
