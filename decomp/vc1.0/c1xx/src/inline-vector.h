#ifndef INLINE_VECTOR_H
#define INLINE_VECTOR_H
#include <recon.h>
/* Permanent vectors whose pointer array immediately follows a short header. */
typedef struct InlineVector {
    short count, capacity;
    void *items[1];
} InlineVector;
InlineVector *new_inline_vector(short capacity);
InlineVector *grow_inline_vector(InlineVector *vector);
InlineVector *add_inline_pointer(InlineVector *vector, void *pointer);
int locate_inline_pointer(InlineVector *vector, void *pointer, int *position);
unsigned inline_vector_bytes(short capacity);
short inline_vector_count(InlineVector *vector);
int equal_inline_vectors(InlineVector *left, InlineVector *right);
void release_inline_vector(InlineVector *vector);
extern InlineVector *empty_inline_vector;
#endif
