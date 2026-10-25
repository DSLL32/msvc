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
/* Macro descriptor from pool5. Its body is a separately allocated span:
 * four unrecovered bytes, one signed selector, then length-defined payload.
 * No value/default is assigned to the leading four bytes or allocation tail. */
typedef struct CompilerMacroBody {
    BYTE unmodeled_header[4];
    signed char kind;
    BYTE payload[1];
} CompilerMacroBody;
typedef struct CompilerMacro {
    struct CompilerNameEntry *identifier;
    CompilerMacroBody *body;
} CompilerMacro;
typedef char CompilerMacro_size[sizeof(CompilerMacro) == 8 ? 1 : -1];
typedef char CompilerMacroBody_kind[offsetof(CompilerMacroBody, kind) == 4 ? 1 : -1];
typedef char CompilerMacroBody_payload[offsetof(CompilerMacroBody, payload) == 5 ? 1 : -1];
#endif
