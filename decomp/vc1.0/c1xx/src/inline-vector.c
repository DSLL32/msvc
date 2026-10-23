/* Permanent vectors with an inline pointer array. */
#include <string.h>
#include "options.h"
#include "inline-vector.h"
#include <recon.h>

InlineVector *empty_inline_vector; /* original 0x9f3cc */

/* Original 0x4e380..0x4e3c8. */
InlineVector *new_inline_vector(short capacity)
{
    InlineVector *vector = allocate_permanent(inline_vector_bytes(capacity));
    vector->count = 0;
    vector->capacity = capacity;
    if (!empty_inline_vector) {
        /* The nonzero sentinel prevents recursion while making the empty one. */
        empty_inline_vector = (InlineVector *)1;
        empty_inline_vector = new_inline_vector(0);
    }
    return vector;
}

/* Original 0x4e360..0x4e36d. The four-byte header precedes capacity pointers. */
unsigned inline_vector_bytes(short capacity)
{
    return (int)capacity * 4 + 4;
}

/* Original 0x4e430..0x4e43f. Only AX is the short result. */
short inline_vector_count(InlineVector *vector)
{
    return vector ? vector->count : 0;
}

/* Original 0x4e410..0x4e42a. The permanent allocator retains a separate
   release entry even though this allocator's release implementation is empty. */
void release_inline_vector(InlineVector *vector)
{
    release_permanent(vector, inline_vector_bytes(vector->capacity));
}

int locate_inline_pointer(InlineVector *vector, void *pointer, int *position);

/* Original 0x4e440..0x4e4c2. Keep sorted unique pointers, growing the
   permanent vector only when full. Move the occupied suffix before insertion;
   failure to grow returns NULL without performing the insertion. */
InlineVector *add_inline_pointer(InlineVector *vector, void *pointer)
{
    int position;
    if (locate_inline_pointer(vector, pointer, &position)) return vector;
    if (vector->capacity == vector->count) {
        vector = grow_inline_vector(vector);
        if (!vector) return 0;
    }
    if (position < vector->count)
        memmove(&vector->items[position + 1], &vector->items[position],
            (vector->count - position) * 4);
    vector->items[position] = pointer;
    ++vector->count;
    return vector;
}

/* Original 0x4e5f0..0x4e686. Order pointers by their low twelve address
   bits, then scan the equal-key group for full identity. A new pointer uses
   the group's lower bound; a found pointer reports its actual slot. */
int locate_inline_pointer(InlineVector *vector, void *pointer, int *position)
{
    int count = vector->count, first = 0, last = count, middle, index;
    unsigned key = (unsigned)pointer & 0xfff;
    while (first < last) {
        middle = (first + last) >> 1;
        if (((unsigned)vector->items[middle] & 0xfff) < key) first = middle + 1;
        else last = middle;
    }
    for (index = first; index < count; ++index) {
        if (((unsigned)vector->items[index] & 0xfff) != key) break;
        if (vector->items[index] == pointer) {
            first = index;
            break;
        }
    }
    if (position) *position = first;
    /* This reload follows the optional position write in the original. */
    if (first < vector->count && vector->items[first] == pointer) return 1;
    return 0;
}

void *__stdcall resize_permanent(void *block, unsigned old_bytes, unsigned new_bytes);

/* Original 0x4e3d0..0x4e405. Increase signed-short capacity by a
   quarter plus one, storing it before resizing. The size getter's new/old
   query order and the resize helper's pointer result are preserved. */
InlineVector *grow_inline_vector(InlineVector *vector)
{
    short old_capacity = vector->capacity;
    unsigned new_bytes, old_bytes;
    vector->capacity = (short)(old_capacity + 1 + (old_capacity >> 2));
    new_bytes = inline_vector_bytes(vector->capacity);
    old_bytes = inline_vector_bytes(old_capacity);
    return resize_permanent(vector, old_bytes, new_bytes);
}
