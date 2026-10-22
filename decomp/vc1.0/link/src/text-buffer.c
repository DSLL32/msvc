/* Growable byte buffers. Length excludes any terminator supplied by a caller. */
#include "linker-types.h"
#include <recon.h>

void reserve_text(TextBuffer *buffer,unsigned long required)
{
    unsigned long capacity;
    if (required <= buffer->capacity) return;
    capacity=buffer->capacity*2;
    if (capacity <= required) capacity=required;
    if (capacity <= 1024) capacity=1024;
    buffer->data=realloc(buffer->data,capacity);
    if (!buffer->data) out_of_memory();
    buffer->capacity=capacity;
}

unsigned long append_text(TextBuffer *buffer,const void *bytes,unsigned long size)
{
    unsigned long offset=buffer->length, i;
    const char *source=bytes;
    if (buffer->capacity < offset+size) reserve_text(buffer,offset+size);
    for (i=0;i<size;++i) buffer->data[offset+i]=source[i];
    buffer->length+=size;
    return offset;
}

void release_text(TextBuffer *buffer)
{
    if (buffer->data) {
        free(buffer->data);
        buffer->data=NULL;
        buffer->capacity=buffer->length=0;
    }
}
