/* Interned function parameter descriptions. */
#include "types.h"
#include "parser.h"

int declaration_context; /* original 0x5ffc0 */

/* Original 0x40820..0x40ad5. */
ParameterInfo *__stdcall build_parameter_info(void *parameters, unsigned short *flags)
{
    ParameterList *source = parameters;
    ParameterDeclaration *head, *first, *parameter;
    ParameterInfo *temporary;
    ParameterEntry **bucket, *entry, *previous;
    ArenaPool *pool;
    unsigned count = 0, bytes, i, hash;
    if (!source) {
        *flags |= 0x800;
        return empty_parameter_info();
    }
    head = source->head;
    if (declaration_context == 5 ||
        (declaration_context == 4 && (!head || head->type == type_0080))) {
        *flags |= 0x4000;
        if (head)
            head->flags |= 2;
        return void_parameter_info();
    }
    first = 0;
    if (head) {
        while (head && (head->kind & 0x1f) != 10 && (head->kind & 0x1f) != 11)
            head = head->next;
        if (head)
            first = head->circular_next;
    }
    parameter = first;
    if (head) {
        for (;;) {
            ++count;
            if (!parameter->type) {
                *flags |= 0x8000;
                break;
            }
            if (!parameter)
                break;
            parameter = parameter->circular_next;
            if (parameter == first)
                break;
        }
    }
    if (!count) {
        *flags |= 0x800;
        return empty_parameter_info();
    }
    bytes = (count * 4 + 11) & ~3;
    pool = &compiler_arena.pools[2];
    if (pool->remaining < (int)bytes)
        refill_arena(&compiler_arena, pool);
    temporary = (ParameterInfo *)pool->next;
    pool->remaining -= bytes;
    pool->next += bytes;
    temporary->count = count;
    head = source->head;
    first = 0;
    if (head) {
        while (head && (head->kind & 0x1f) != 10 && (head->kind & 0x1f) != 11)
            head = head->next;
        if (head)
            first = head->circular_next;
    }
    parameter = first;
    if (head) {
        i = 0;
        for (;;) {
            temporary->types[i++] = parameter->type;
            if (!parameter)
                break;
            parameter = parameter->circular_next;
            if (parameter == first)
                break;
        }
    }
    hash = hash_parameters(temporary);
    bucket = &parameter_buckets[hash & 0x1ff];
    previous = 0;
    for (entry = *bucket; entry; entry = entry->next) {
        if (entry->info.count > count)
            break;
        if (entry->info.count == count) {
            for (i = 0; i < count && entry->info.types[i] == temporary->types[i]; ++i)
                ;
            if (i == count)
                return &entry->info;
        }
        previous = entry;
    }
    entry = allocate_permanent((count * 4 + 15) & ~3);
    entry->next = previous ? previous->next : *bucket;
    if (previous)
        previous->next = entry;
    else
        *bucket = entry;
    entry->info.count = temporary->count;
    entry->info.auxiliary = temporary->auxiliary;
    for (i = 0; i < count; ++i)
        entry->info.types[i] = temporary->types[i];
    return &entry->info;
}

static ParameterInfo *empty_parameters; /* original 0x6b798 */

/* Original 0x407e0..0x40813. */
ParameterInfo *empty_parameter_info(void)
{
    if (!empty_parameters) {
        empty_parameters = allocate_permanent(12);
        empty_parameters->count = 0;
        empty_parameters->auxiliary &= 0xffff0000;
    }
    return empty_parameters;
}

/* Original 0x40740..0x40791. Interned type identities contribute their
   flags and page offsets. A null type stops advancing the cursor, matching
   the original handling of an incomplete parameter description. */
unsigned __stdcall hash_parameters(ParameterInfo *parameters)
{
    unsigned remaining = parameters->count, hash = remaining;
    CType **cursor = parameters->types;
    CType *type;
    while (remaining--) {
        type = *cursor;
        if (type) {
            hash += ((unsigned)type->related & 0xffc) +
                    ((type->field08 & 0xffc) >> 2) + type->flags;
            ++cursor;
        }
    }
    return hash;
}

static ParameterInfo *void_parameters; /* original 0x6b794 */

/* Original 0x407a0..0x407d6. Canonical descriptor for an explicit (void)
   prototype, distinct from the zero-count unspecified-parameter descriptor. */
ParameterInfo *void_parameter_info(void)
{
    if (!void_parameters) {
        void_parameters = allocate_permanent(sizeof(ParameterInfo));
        void_parameters->count = 1;
        void_parameters->types[0] = type_0080;
    }
    return void_parameters;
}

/* Original 0x2a920..0x2a9df. Locate the declaration's parameter marker,
   then count its circular parameter list in four-byte stack slots. */
unsigned __stdcall parameter_list_bytes(FrontendSymbol *parameters)
{
    FrontendSymbol *first, *parameter;
    unsigned bytes = 0;
    while (parameters && (parameters->kind & 0x1f) != 10 && (parameters->kind & 0x1f) != 11)
        parameters = parameters->next;
    if (!parameters) return 0;
    first = parameter = parameters->detail.parameter_next;
    if (parameter) do {
        if (parameter->type) bytes += (parameter_type_size(parameter->type, 0) + 3) & ~3UL;
        parameter = parameter->detail.parameter_next;
    } while (parameter && parameter != first);
    return bytes;
}
