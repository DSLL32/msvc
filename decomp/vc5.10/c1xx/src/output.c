#include "output.h"
#include "frontend.h"
#include <recon.h>

void initialize_output_buffer_01(void)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[0];
    buffer->ok = 1;
    buffer->data = NULL;
    buffer->cursor = NULL;
    buffer->capacity = 0;
    buffer->relocation = NULL;
    buffer->relocation_context = NULL;
    buffer->file = NULL;
    buffer->descriptor = -1;
    buffer->mode = 0;
}

void destroy_output_buffer_01(void)
{
    __asm { int 3 }
}

void initialize_output_buffer_02(void)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[1];
    buffer->ok = 1;
    buffer->data = NULL;
    buffer->cursor = NULL;
    buffer->capacity = 0;
    buffer->relocation = NULL;
    buffer->relocation_context = NULL;
    buffer->file = NULL;
    buffer->descriptor = -1;
    buffer->mode = 0;
}

void destroy_output_buffer_02(void)
{
    __asm { int 3 }
}

void initialize_output_buffer_03(void)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[2];
    buffer->ok = 1;
    buffer->data = NULL;
    buffer->cursor = NULL;
    buffer->capacity = 0;
    buffer->relocation = NULL;
    buffer->relocation_context = NULL;
    buffer->file = NULL;
    buffer->descriptor = -1;
    buffer->mode = 0;
}

void destroy_output_buffer_03(void)
{
    __asm { int 3 }
}

void initialize_output_buffer_04(void)
{
    __asm { int 3 }
}

void destroy_output_buffer_04(void)
{
    __asm { int 3 }
}
