#include "cvtres.h"
#include <stdlib.h>
#include <recon.h>

long __stdcall seek_file(FILE *file, long offset, int origin)
{
    if (fseek(file, offset, origin) == -1) {
        error(1105);
        exit(1);
    }
    return tell_file(file);
}

long __stdcall tell_file(FILE *file)
{
    long position = ftell(file);
    if (position == -1) {
        error(1104);
        exit(1);
    }
    return position;
}

int __stdcall read_bytes(FILE *file, void *buffer, unsigned long size)
{
    if (fread(buffer, 1, size, file) != size) {
        error(1103);
        exit(1);
    }
    return 0;
}

int __stdcall write_bytes(FILE *file, const void *buffer, unsigned long size)
{
    if (fwrite(buffer, 1, size, file) != size) {
        error(1106);
        exit(1);
    }
    return 0;
}

static unsigned char copy_buffer[1024];

unsigned long __stdcall copy_bytes(FILE *input, FILE *output, unsigned long size)
{
    unsigned long copied = 0, count;
    while (size) {
        count = size > sizeof(copy_buffer) ? sizeof(copy_buffer) : size;
        size -= count;
        if (read_bytes(input, copy_buffer, count)) break;
        copied += count;
        write_bytes(output, copy_buffer, count);
    }
    return copied;
}
