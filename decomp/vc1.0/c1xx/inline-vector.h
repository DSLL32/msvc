#ifndef INLINE_VECTOR_H
#define INLINE_VECTOR_H
/* Permanent vectors whose pointer array immediately follows a short header. */
typedef struct InlineVector {
    short count, capacity;
    void *items[1];
} InlineVector;
InlineVector *new_inline_vector(short capacity);
InlineVector *grow_inline_vector(InlineVector *vector);
unsigned inline_vector_bytes(short capacity);
extern InlineVector *empty_inline_vector;
#endif
