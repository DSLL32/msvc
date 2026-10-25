#include <new.h>
#include <string.h>
#include "output.h"
#pragma intrinsic(memcpy, memset)
#include <recon.h>

int RECON_THISCALL OutputBuffer::Grow(int bytes)
{
    int increment = (int)capacity / 2;
    unsigned new_capacity, used;
    BYTE *replacement, *new_cursor, *old_data;
    if (bytes + 4096 > increment)
        increment = bytes + 4096;
    new_capacity = (capacity + increment + 4095) & ~4095U;
    replacement = (BYTE *)::operator new(new_capacity);
    if (!replacement) return 0;
    capacity = new_capacity;
    used = (unsigned)(cursor - data);
    memcpy(replacement, data, used);
    new_cursor = replacement + used;
    memset(new_cursor, 0, capacity - used);
    ::operator delete(data);
    old_data = data;
    data = replacement;
    cursor = new_cursor;
    if (old_data != replacement && relocation)
        relocation(relocation_context, old_data, replacement);
    return 1;
}
