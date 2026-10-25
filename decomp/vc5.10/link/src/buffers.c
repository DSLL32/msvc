#include <windows.h>
#include "options.h"
#include <recon.h>

void *__stdcall reallocate_memory(void *,unsigned);
void __stdcall reserve_byte_buffer(ByteBuffer *buffer,unsigned bytes)
{
    unsigned doubled;
    if (buffer->capacity>=bytes) return;
    doubled=buffer->capacity*2;
    if (bytes<=doubled) bytes=doubled;
    if (bytes<=1024) bytes=1024;
    buffer->data=(BYTE *)reallocate_memory(buffer->data,bytes);
    buffer->capacity=bytes;
}
