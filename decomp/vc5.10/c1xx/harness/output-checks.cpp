/* Narrow native unit for the newly reconstructed buffer-growth state changes. */
#include <new.h>
#include <string.h>
#include "output.h"
#include <recon.h>

struct BufferRelocationCheck {
    OutputBuffer *buffer;
    BYTE *previous;
    unsigned used, expected_capacity;
    int calls, valid;
};
static void __cdecl inspect_relocation(void *context, void *old_data, void *new_data)
{
    BufferRelocationCheck *check = (BufferRelocationCheck *)context;
    ++check->calls;
    check->valid &= old_data == check->previous && new_data == check->buffer->data &&
        check->buffer->cursor == check->buffer->data + check->used &&
        check->buffer->capacity == check->expected_capacity;
}
extern "C" int c1xx_test_output_growth(void)
{
    OutputBuffer buffer;
    BufferRelocationCheck check;
    unsigned i;
    memset(&buffer, 0, sizeof(buffer));
    check.buffer = &buffer;
    check.previous = NULL;
    check.used = 0;
    check.expected_capacity = 8192;
    check.calls = 0;
    check.valid = 1;
    buffer.relocation = inspect_relocation;
    buffer.relocation_context = &check;
    if (buffer.Ensure(-1) || !buffer.Grow(2) || !check.valid || check.calls != 1)
        return 0;
    for (i = 0; i < buffer.capacity; ++i)
        if (buffer.data[i]) return 0;
    buffer.data[0] = 0x5b;
    buffer.data[1] = 0x80;
    buffer.data[2] = 0xa7;
    buffer.cursor += 3;
    check.previous = buffer.data;
    check.used = 3;
    check.expected_capacity = 24576;
    if (!buffer.Grow(9000) || !check.valid || check.calls != 2 ||
        buffer.data[0] != 0x5b || buffer.data[1] != 0x80 || buffer.data[2] != 0xa7)
        return 0;
    for (i = 3; i < buffer.capacity; ++i)
        if (buffer.data[i]) return 0;
    ::operator delete(buffer.data);
    return 1;
}
