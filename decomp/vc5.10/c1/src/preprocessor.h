#ifndef C1_PREPROCESSOR_H
#define C1_PREPROCESSOR_H
#include <stddef.h>
#include "c1.h"
#include <recon.h>

/* One partial 32-byte prototype object. Only these two bytes are defined by
 * this initializer. The intervening layout bytes and allocation tail remain
 * UNMODELED: no initialization, copies or semantic reads are permitted. */
typedef struct CompilerTokenPrototype {
    BYTE kind;
    BYTE unmodeled_header[7];
    BYTE tag;
} CompilerTokenPrototype;
typedef char CompilerTokenPrototype_tag_offset[
    offsetof(CompilerTokenPrototype, tag) == 8 ? 1 : -1];
#endif
