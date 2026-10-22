/* C++ function signatures combine interned declaration types with defaults. */
#include "parameters.h"

DeclarationControl declaration_control; /* original 0x8cc18..0x8cc58 */
/* The parser also owns the entry immediately before declaration depth zero. */
DefaultArgument *default_argument_storage[61]; /* original 0x911f4..0x912e8 */
CType *parameter_associated_type; /* original 0xa1e38 */
static DeclarationParameterInfo *void_parameters; /* original 0x9f3c0 */

/* Original 0x62e40..0x62e81. The sole type is the canonical void type. */
DeclarationParameterInfo *void_parameter_list(void)
{
    if (!void_parameters) {
        void_parameters = allocate_permanent(sizeof(DeclarationParameterInfo));
        void_parameters->count = 1;
        void_parameters->flags = 0;
        void_parameters->types[0] = type_0080;
    }
    return void_parameters;
}

/* Original 0x63170..0x6322a. Buckets are ordered by declaration count;
   within a count, all sixteen signature bytes participate in equality. */
FunctionParameters *__stdcall intern_function_parameters(FunctionParameters *parameters)
{
    unsigned hash, i;
    int count = parameters->declarations->count;
    ParameterEntry **bucket, *entry, *previous = 0;
    unsigned char *a, *b;
    hash = (((unsigned)parameters->declarations & 0xffc) >> 2) ^
           ((unsigned)parameters->associated_type & 0xffc);
    bucket = &parameter_buckets[hash & 0x1ff];
    for (entry = *bucket; entry; entry = entry->next) {
        if (entry->parameters.declarations->count > count)
            break;
        if (entry->parameters.declarations->count == count) {
            a = (unsigned char *)&entry->parameters;
            b = (unsigned char *)parameters;
            for (i = 0; i < sizeof(FunctionParameters) && a[i] == b[i]; ++i)
                ;
            if (i == sizeof(FunctionParameters))
                return &entry->parameters;
        }
        previous = entry;
    }
    entry = allocate_permanent(sizeof(ParameterEntry));
    entry->parameters = *parameters;
    entry->next = previous ? previous->next : *bucket;
    if (previous)
        previous->next = entry;
    else
        *bucket = entry;
    return &entry->parameters;
}

/* Original 0x63230..0x633b6. */
FunctionParameters *__stdcall build_parameter_info(void *declarations,
                                                   unsigned short *kind)
{
    DeclarationList *source = declarations;
    DeclarationParameterInfo *info;
    CppSymbol *head, *first, *parameter;
    FunctionParameters *result;
    ArenaPool *pool;
    unsigned count = 0;
    unsigned char default_count;
    if (!source || !source->head) {
        *kind |= 0x4000;
        info = void_parameter_list();
    } else if (source->head->type == type_0080) {
        *kind |= 0x4000;
        source->head->flags |= 8;
        info = void_parameter_list();
    } else {
        head = source->head;
        while (head && (head->kind & 0x1f) != 10 &&
                       (head->kind & 0x1f) != 11)
            head = head->next;
        if (head) {
            first = parameter = head->detail.parameter_next;
            for (;;) {
                ++count;
                if (!parameter->type) {
                    *kind |= 0x8000;
                    break;
                }
                if (!parameter)
                    break;
                parameter = parameter->detail.parameter_next;
                if (parameter == first)
                    break;
            }
        }
        info = intern_declaration_parameters(count, source->head);
    }
    pool = &compiler_arena.pools[2];
    if (pool->remaining < (int)sizeof(FunctionParameters))
        refill_arena(&compiler_arena, pool);
    result = (FunctionParameters *)pool->next;
    pool->remaining -= sizeof(FunctionParameters);
    pool->next += sizeof(FunctionParameters);
    result->declarations = info;
    if (default_arguments[declaration_depth]) {
        result->defaults = default_arguments[declaration_depth];
        default_count = 0;
        while (default_arguments[declaration_depth]) {
            ++default_count;
            default_arguments[declaration_depth] =
                default_arguments[declaration_depth]->next;
        }
        result->default_count = default_count;
    }
    result->associated_type = parameter_associated_type;
    return intern_function_parameters(result);
}

/* Original 0x62e90..0x6303e. Parameter-count ordering stops searches early;
   declaration type identity, rather than default arguments, is the key. */
DeclarationParameterInfo *intern_declaration_parameters(int count, CppSymbol *head)
{
    unsigned bytes = count * sizeof(CType *) + 8, i, n = 0, hash;
    ArenaPool *pool = &compiler_arena.pools[2];
    DeclarationParameterInfo *candidate;
    DeclarationParameterEntry **bucket, *entry, *previous = 0, *created;
    CppSymbol *first, *parameter;
    if (pool->remaining < (int)bytes)
        refill_arena(&compiler_arena, pool);
    candidate = (DeclarationParameterInfo *)pool->next;
    pool->remaining -= bytes;
    pool->next += bytes;
    candidate->count = count;
    while (head && (head->kind & 31) != 10 && (head->kind & 31) != 11)
        head = head->next;
    if (head) {
        first = parameter = head->detail.parameter_next;
        do {
            candidate->types[n++] = parameter->type;
            parameter = parameter->detail.parameter_next;
        } while (parameter && parameter != first);
    }
    hash = hash_declaration_parameters(candidate);
    bucket = &declaration_parameter_buckets[hash & 0x1ff];
    for (entry = *bucket; entry; entry = entry->next) {
        if (count < entry->parameters.count)
            break;
        if (count == entry->parameters.count) {
            for (i = 0; i < (unsigned)count &&
                        entry->parameters.types[i] == candidate->types[i]; ++i)
                ;
            if (i == (unsigned)count)
                return &entry->parameters;
        }
        previous = entry;
    }
    created = allocate_permanent(bytes + 4);
    created->parameters.count = candidate->count;
    created->parameters.flags = candidate->flags;
    created->parameters.field06 = candidate->field06;
    for (i = 0; i < (unsigned)count; ++i)
        created->parameters.types[i] = candidate->types[i];
    created->next = entry;
    if (previous)
        previous->next = created;
    else
        *bucket = created;
    return &created->parameters;
}

/* Original 0x62df0..0x62e37. A null variadic marker consumes the remaining
   count without advancing the pointer, as in the original loop. */
unsigned __stdcall hash_declaration_parameters(DeclarationParameterInfo *parameters)
{
    int remaining = parameters->count;
    unsigned hash = remaining;
    CType **next = parameters->types;
    CType *type;
    while (remaining--) {
        type = *next;
        if (type) {
            hash += ((unsigned)type->related & 0xffc) + type->field08 + type->flags;
            ++next;
        }
    }
    return hash;
}

void __stdcall validate_function_default_arguments(CppSymbol *function);
void __stdcall report_parameter_error(unsigned code, CppSymbol *symbol, unsigned parameter);
void __stdcall inherit_function_defaults(CppSymbol *declaration, DefaultArgument *defaults,
    FunctionParameters *previous, FunctionParameters *current, DefaultArgument *following);

/* Original 0x3a5a0..0x3a6da. Merge declarations' trailing defaults, retaining
   the deferred-expression replacement path and both native validation calls
   after an overlapping-default diagnostic. */
void __stdcall merge_function_defaults(FunctionParameters *current, FunctionParameters *previous,
    CppSymbol *declaration, int inherit)
{
    DefaultArgument *old_default = previous->defaults, *new_default = current->defaults;
    DefaultArgument *following;
    unsigned remaining = current->default_count, first_parameter;
    if ((cpp_parser_flags & 0x04000000UL) || !remaining) return;
    if (inherit) {
        if (!old_default) {
            inherit_function_defaults(declaration, new_default, previous, current, 0);
            validate_function_default_arguments(declaration);
            current->default_count = 0;
            current->defaults = 0;
            return;
        }
        if (*(unsigned char *)old_default->expression == 0xc3) {
            previous->defaults = new_default;
            previous->default_count = (unsigned char)remaining;
        } else {
            if (inherit == 2 && ((declaration->declaration_flags & 0x400) ||
                ((token_allocation_flags & 0x400) && (declaration->storage[1] & 4))))
                return;
            following = new_default->following;
            first_parameter = current->declarations->count - remaining;
            for (;;) {
                if (new_default->expression) {
                    report_parameter_error(572, declaration, first_parameter + remaining);
                    new_default->expression = 0;
                }
                new_default = new_default->next;
                if (!new_default) break;
                old_default = old_default->next;
                if (!old_default) {
                    inherit_function_defaults(declaration, new_default, previous,
                        current, following);
                    break;
                }
                --remaining;
            }
            validate_function_default_arguments(declaration);
            current->default_count = 0;
            current->defaults = 0;
        }
    }
    validate_function_default_arguments(declaration);
}
