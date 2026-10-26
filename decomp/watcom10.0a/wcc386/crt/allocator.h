#ifndef WCC_CRT_ALLOCATOR_H
#define WCC_CRT_ALLOCATOR_H
#include "memory.h"
#include <recon.h>
void *malloc_segment_wrapper(unsigned amount);
void free_segment_wrapper(void *memory);
void *wcc_calloc(unsigned count,unsigned element_size);
/* Explicit lexical opt-in for the proved old OSI calloc callback variant.
 * No host/global malloc or calloc symbol is replaced. */
#ifdef WCC_CALLBACK_CALLOC
#undef calloc
#define calloc wcc_calloc
#endif
#ifdef WCC_CALLBACK_MALLOC
#undef malloc
#define malloc malloc_segment_wrapper
#endif
#ifdef WCC_CALLBACK_FREE
#undef free
#define free free_segment_wrapper
#endif
#endif
