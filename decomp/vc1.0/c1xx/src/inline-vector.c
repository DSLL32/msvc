/* Permanent vectors with an inline pointer array. */
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
