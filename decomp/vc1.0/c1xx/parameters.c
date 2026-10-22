/* C++ function signatures combine interned declaration types with defaults. */
#include "parameters.h"

int declaration_depth; /* original 0x8cc18; parser checks depth < 60 */
DefaultArgument *default_arguments[60]; /* original 0x911f8 */
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
