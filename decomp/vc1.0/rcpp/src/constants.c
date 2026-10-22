/* Typed numeric tokens retained from the original compiler front end. */
#include "numbers.h"
#include <stddef.h>
typedef char ConstantSizeMustBe20[sizeof(RcConstant) == 20 ? 1 : -1];
typedef char ConstantPayloadMustBe12[offsetof(RcConstant, value) == 12 ? 1 : -1];
typedef char TypeSizeMustBe20[sizeof(RcType) == 20 ? 1 : -1];

/* Original [0100d250,0100d38e). Unused token/type fields are left untouched,
 * as in the target; this is not a zero-initialized compiler node allocator. */
RcConstant *__stdcall rc_make_constant(unsigned char kind, const void *value)
{
    RcConstant *constant = rc_malloc(sizeof(*constant));
    RcType *type;
    unsigned short flags;
    if (!constant) {
        rc_message_format = rc_message(1002);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_error(1002);
        return NULL;
    }
    constant->kind = kind;
    constant->flags = 1;
    switch (kind) {
    case TK_FLOAT:
        flags = 6;
        constant->value.pointer = *(void * const *)value;
        break;
    case TK_DOUBLE:
        flags = 7;
        constant->value.pointer = *(void * const *)value;
        break;
    case TK_LONG_DOUBLE:
        flags = 8;
        constant->value.pointer = *(void * const *)value;
        break;
    case TK_INT:
    case TK_LONG:
    case TK_UINT:
    case TK_ULONG:
        flags = kind == TK_INT || kind == TK_UINT ? 3 : 4;
        if (kind == TK_UINT || kind == TK_ULONG) flags |= 0x10;
        constant->value.integer = *(long const *)value;
        if (!constant->value.integer) constant->flags |= 0x40;
        break;
    /* Other kinds leave the original local type flags and payload undefined;
     * all callers provide one of the numeric token kinds above. */
    }
    type = rc_malloc(sizeof(*type));
    if (!type) {
        rc_message_format = rc_message(1002);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_error(1002);
        return NULL;
    }
    type->flags = flags | 0x1000;
    constant->type = rc_intern_type(type);
    return constant;
}

typedef struct RcTypeEntry {
    struct RcTypeEntry *next;
    RcType type;
} RcTypeEntry;
static RcTypeEntry *type_buckets[256];
/* Original [0100d180,0100d220). Canonical types share the three fields used
 * in equality; other compiler metadata is copied on first insertion. */
RcType *__stdcall rc_intern_type(RcType *type)
{
    RcTypeEntry **bucket, *entry;
    unsigned hash = ((unsigned)type->related + type->field08 + type->flags) & 255;
    bucket = type_buckets + hash;
    for (entry = *bucket; entry; entry = entry->next)
        if (rc_equal_type(type, &entry->type)) return &entry->type;
    entry = rc_malloc(sizeof(*entry));
    if (!entry) {
        rc_message_format = rc_message(1002);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_error(1002);
        return NULL;
    }
    entry->next = *bucket;
    *bucket = entry;
    entry->type = *type;
    entry->type.references = 0;
    return &entry->type;
}

/* Original [0100d220,0100d24d). Attributes/reference counters do not enter
 * this old preprocessor's canonical type key. */
int __stdcall rc_equal_type(RcType *left, RcType *right)
{
    return left->flags == right->flags && left->field08 == right->field08 &&
           left->related == right->related;
}
