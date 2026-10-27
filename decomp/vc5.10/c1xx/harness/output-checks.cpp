/* Narrow native unit for the newly reconstructed buffer-growth state changes. */
#include <new.h>
#include <string.h>
#include "output.h"
#include "frontend.h"
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

extern "C" int c1xx_test_reference_index_encoding(void)
{
    static const int il_values[] = { 0, -1, 0x7fff, 0x8000, 0x400000 };
    static const BYTE il_expected[] = {
        0,0, 0xff,0xff, 0xff,0x7f, 0,0x80,1,0, 0,0x80,0x80,0
    };
    static const int compact_values[] = { 0, 0x3fff, 0x4000, 0x3fff7f, 0x3fff80, -1 };
    static const BYTE compact_expected[] = {
        0x80,0, 0xbf,0xff, 0xc0,0,1, 0xff,0xfd,0xff, 0, 0x7f
    };
    OutputBuffer buffer;
    BYTE storage[64];
    unsigned i;
    int valid, saved_il = frontend_data.option_c9il;
    memset(storage, 0xa7, sizeof(storage));
    buffer.data = buffer.cursor = storage;
    buffer.capacity = sizeof(storage);
    buffer.ok = 1;
    frontend_data.option_c9il = 1;
    for (i = 0; i < sizeof(il_values) / sizeof(il_values[0]); ++i)
        buffer.WriteReferenceIndex(il_values[i]);
    valid = buffer.ok && buffer.cursor == storage + sizeof(il_expected) &&
        !memcmp(storage, il_expected, sizeof(il_expected)) && storage[sizeof(il_expected)] == 0xa7;
    memset(storage, 0xa7, sizeof(storage));
    buffer.cursor = storage;
    buffer.ok = 1;
    frontend_data.option_c9il = 0;
    for (i = 0; i < sizeof(compact_values) / sizeof(compact_values[0]); ++i)
        buffer.WriteReferenceIndex(compact_values[i]);
    valid = valid && buffer.ok && buffer.cursor == storage + sizeof(compact_expected) &&
        !memcmp(storage, compact_expected, sizeof(compact_expected)) && storage[sizeof(compact_expected)] == 0xa7;
    frontend_data.option_c9il = saved_il;
    return valid;
}
