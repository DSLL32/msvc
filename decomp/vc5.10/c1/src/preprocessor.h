#ifndef C1_PREPROCESSOR_H
#define C1_PREPROCESSOR_H
#include <stddef.h>
#include "c1.h"
#include <recon.h>

/* Six intervening bytes retain storage only; conditional arithmetic consumes
 * the WORD type flags and QWORD value in the authentic16-byte arena record. */
typedef struct CompilerConditionValue {
    WORD flags;
    BYTE unmodeled_02[6];
    __int64 value;
} CompilerConditionValue;
typedef char CompilerConditionValue_payload[offsetof(CompilerConditionValue, value) == 8 ? 1 : -1];
typedef char CompilerConditionValue_extent[sizeof(CompilerConditionValue) == 16 ? 1 : -1];

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
/* Rescan stack records have an authentic24-byte stride and256-entry bound.
 * The current projection exposes only the first three encountered records. */
typedef struct CompilerMacroExpansionFrame {
    BYTE *source, *arguments, *replacement;
    CompilerMacro *macro;
    signed char argument_count;
    signed char argument_index; /* Number of argument streams already started. */
    BYTE unmodeled_12[2];
    unsigned active;
} CompilerMacroExpansionFrame;
typedef char CompilerMacroExpansionFrame_size[sizeof(CompilerMacroExpansionFrame) == 24 ? 1 : -1];
typedef char CompilerMacro_size[sizeof(CompilerMacro) == 8 ? 1 : -1];
typedef char CompilerMacroBody_kind[offsetof(CompilerMacroBody, kind) == 4 ? 1 : -1];
typedef char CompilerMacroBody_payload[offsetof(CompilerMacroBody, payload) == 5 ? 1 : -1];
/* Input-stack frame allocated as48 bytes. Only the low four flag bits are
 * interpreted here; the remaining bits are retained representation only. */
struct CompilerSourceFile;
typedef struct CompilerSourceFrame {
    int descriptor;
    void *saved_context;
    unsigned saved_context_value;
    struct CompilerSourceFile *file;
    BYTE *saved_cursor;
    BYTE *buffer;
    int available_bytes;
    int consumed_bytes;
    unsigned packing_alignment;
    int conditional_depth;
    unsigned flags;
    struct CompilerSourceFrame *next;
} CompilerSourceFrame;
typedef char CompilerSourceFrame_size[sizeof(CompilerSourceFrame) == 48 ? 1 : -1];
#endif
