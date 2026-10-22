/* Declaration lookup uses the first word of its declaration records as a key. */
#include "declarations.h"
#include "global-ir.h"

HashTable *declaration_table; /* original 0x9f340 */

/* Original 0x31f00..0x31f13. */
void initialize_declaration_table(void)
{
    declaration_table = new_hash_table(16, 0, HASH_FIRST_WORD, EQUAL_FIRST_WORD);
}

unsigned char declaration_storage[3]; /* original 0x8c3fc */
unsigned char declaration_mode; /* original 0x8c13c */
unsigned declaration_kind; /* original 0x91550 */

/* Original 0x4f750..0x4f783. Temporarily declare a compiler helper globally. */
void __stdcall save_declaration_context(unsigned char *mode, SymbolScope **scope)
{
    *mode = declaration_mode;
    *scope = current_scope;
    current_scope = global_scope;
    declaration_mode_cache = 0;
    declaration_mode = 0;
}

/* Original 0x3bfe0..0x3c089. The declarator initially owns a mutable type;
   its twelve-byte layout agrees with the later interned CType descriptor. */
CppSymbol *__stdcall new_declarator(char *name)
{
    unsigned arena = declaration_mode != 0;
    CppSymbol *declaration;
    ArenaPool *pool;
    if (!arena)
        declaration = allocate_permanent(sizeof(CppSymbol));
    else {
        if (arena == (unsigned)-1)
            arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(CppSymbol))
            refill_arena(&compiler_arena, pool);
        declaration = (CppSymbol *)pool->next;
        pool->remaining -= sizeof(CppSymbol);
        pool->next += sizeof(CppSymbol);
    }
    declaration->kind = 1;
    declaration->name = name;
    if (token_allocation_flags & 0x20)
        declaration->type = allocate_permanent(sizeof(CType));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(CType))
            refill_arena(&compiler_arena, pool);
        declaration->type = (CType *)pool->next;
        pool->remaining -= sizeof(CType);
        pool->next += sizeof(CType);
    }
    declaration->cpp.parts.token = 1;
    return declaration;
}

/* Original 0x34000..0x340f3. Add a compiler-created parameter declarator. */
CppSymbol *__stdcall add_declaration_parameter(int prepend, CppSymbol **list,
                                               char *name, CType *type)
{
    ArenaPool *pool = &compiler_arena.pools[1];
    CppSymbol *parameter;
    if (pool->remaining < (int)sizeof(CppSymbol))
        refill_arena(&compiler_arena, pool);
    parameter = (CppSymbol *)pool->next;
    pool->remaining -= sizeof(CppSymbol);
    pool->next += sizeof(CppSymbol);
    parameter->kind = 10;
    parameter->storage[0] = (declaration_storage[0] & ~7) | 5;
    parameter->storage[1] = declaration_storage[1];
    parameter->storage[2] = declaration_storage[2];
    parameter->type = (CType *)copy_type_description(type);
    parameter->name = name;
    parameter->flags |= 0x80;
    if (next_symbol_index >= 0x7fffffffU)
        fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    parameter->index = next_symbol_index++;
    normalize_symbol_type(0, parameter);
    --type_description_depth;
    if (!prepend) {
        append_declarator(*list, parameter);
        if (!*list)
            *list = parameter;
        return *list;
    }
    prepend_declarator(*list, parameter);
    *list = parameter;
    return parameter;
}

/* Original 0x3b960..0x3b99b. Keep the source-order chain and the reverse
   circular parameter chain together when appending a declarator. */
void __stdcall append_declarator(CppSymbol *list, CppSymbol *declaration)
{
    if (!declaration)
        return;
    if (!list) {
        declaration->next = 0;
        declaration->detail.parameter_next = declaration;
        return;
    }
    declaration->detail.parameter_next = list->detail.parameter_next;
    list->detail.parameter_next = declaration;
    declaration->next = declaration->detail.parameter_next->next;
    declaration->detail.parameter_next->next = declaration;
}

/* Original 0x34e00..0x34e47. Choose the plain or nested declarator path. */
CppSymbol *__stdcall attach_function_parameters(CppSymbol *declaration, CppSymbol *parameters)
{
    TypeShape *shape = declaration->type->related;
    if (shape && !(shape->flags & 0x77))
        shape = shape->related;
    if (!shape && !(cpp_parser_flags & 0x100))
        return attach_plain_function_parameters(declaration, parameters);
    finish_parameter_scope();
    return build_function_declarator(declaration, parameters);
}

CppSymbol *parsed_parameter_symbols; /* original 0x91168 */
unsigned function_declaration_kind; /* original 0x91578 */

/* Original 0x34d90..0x34df6. Preserve parameter scope for the function body. */
CppSymbol *__stdcall attach_plain_function_parameters(CppSymbol *declaration, CppSymbol *parameters)
{
    CType *type;
    if (!parameters)
        return declaration;
    declaration = build_function_declarator(declaration, parameters);
    type = declaration->type;
    type->related->attributes |= type->attributes;
    declaration->attributes |= 0x80;
    parsed_parameter_symbols = finish_parameter_scope();
    type = declaration->type;
    if (type->related->flags & 0x4000)
        function_declaration_kind = 5;
    else
        function_declaration_kind = declaration_kind;
    return declaration;
}

/* Original 0x34e50..0x34e76. A function declarator consumes a list head. */
CppSymbol *__stdcall build_function_declarator(CppSymbol *declaration, CppSymbol *parameters)
{
    DeclarationList list;
    list.head = parameters;
    add_function_shape(declaration, 0, &list);
    return declaration;
}
