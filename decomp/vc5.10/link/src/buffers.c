#include <windows.h>
#include <string.h>
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

#pragma intrinsic(memcpy,memset)
unsigned __stdcall append_bytes(ByteBuffer *buffer,const void *bytes,unsigned count)
{
    if (buffer->used+count>buffer->capacity)
        reserve_byte_buffer(buffer,buffer->used+count);
    memcpy(buffer->data+buffer->used,bytes,count);
    buffer->used+=count;
    return buffer->used-count;
}

void __stdcall release_memory(void *);
void __stdcall clear_buffer(ByteBuffer *buffer)
{
    if(buffer->data) {
        release_memory(buffer->data);
        buffer->data=NULL;
        buffer->capacity=0;
        buffer->used=0;
    }
}

DWORD __stdcall append_zero_bytes(ByteBuffer *buffer,DWORD count)
{
    if(buffer->used+count>buffer->capacity)
        reserve_byte_buffer(buffer,buffer->used+count);
    memset(buffer->data+buffer->used,0,count);
    buffer->used+=count;
    return buffer->used-count;
}
