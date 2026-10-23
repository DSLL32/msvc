/* Declaration lookup uses the first word of its declaration records as a key. */
#include "declarations.h"
#include "parser.h"
#include "global-ir.h"
#include "cpp-context.h"
#include "parameters.h"
#include "operators.h"
#include <recon.h>

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
/* Saved at function entry and cleared at function/external completion.
   No direct reader of this retained native state is identified yet. */
unsigned active_function_declaration_kind; /* original 0x9188c */

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

unsigned short pending_function_qualifiers; /* original 0x911f0 */

/* Original 0x62990..0x62b47. Bind a function's implicit class pointer to its
   parameter signature, then prepend the function shape to the declarator. */
void __stdcall add_function_shape(CppSymbol *declaration, unsigned short attributes,
                                   DeclarationList *parameters)
{
    unsigned short kind = 4, shape_attributes;
    QualifiedDeclarator *qualifier;
    QualifierResolution *resolution;
    QualifierBinding *binding;
    ClassReference *reference;
    TypeDescription *description;
    CType *member_type;
    TypeShape *shape;

    if (!is_member_declarator(declaration)) {
        if (pending_function_qualifiers)
            report_syntax_error((declaration->name == name_new || declaration->name == name_delete) ?
                                271 : 270, declaration->name);
        pending_function_qualifiers = 0;
        goto make_shape;
    }
    kind = 0x2004;
    shape = declaration->type->related;
    if (shape && (shape->flags & 0x10))
        member_type = make_class_pointer_type(shape->data.tag_symbol, 0x1000);
    else {
        qualifier = (QualifiedDeclarator *)declaration->next;
        if (qualifier) {
            resolution = qualifier->resolution;
            if (resolution && !resolution->binding->next)
                goto implicit_class;
            if (!resolution)
                reference = (ClassReference *)get_current_class_symbol();
            else {
                binding = resolution->binding->next;
                if (!binding) {
                    kind = 4;
                    parameter_associated_type = 0;
                    goto make_shape;
                }
                reference = binding->target;
            }
            if ((reference->kind & 31) != 4)
                reference = cpp_context_stack->items[reference->context_index];
            member_type = make_class_pointer_type((CppSymbol *)reference, 0x1000);
        } else {
implicit_class:
            /* The member-declarator predicate requires a class context here. */
            if (current_class_context >= 0)
                member_type = make_class_pointer_type(0, 0x1000);
        }
    }
    if (pending_function_qualifiers) {
        if (pending_function_qualifiers == 1 || pending_function_qualifiers == 2 ||
            (pending_function_qualifiers & 0x3000)) {
            description = copy_type_description(member_type);
            shape = description->related;
            shape_attributes = shape->attributes;
            if (pending_function_qualifiers & 0x3000)
                description->attributes |= pending_function_qualifiers & 0x3000;
            if (pending_function_qualifiers & 7)
                shape_attributes = (shape_attributes & ~7) | (pending_function_qualifiers & 7);
            member_type = replace_first_type_shape((CType *)description, shape->flags,
                                                   shape_attributes, &shape->data);
            --type_description_depth;
        }
        pending_function_qualifiers = 0;
    }
    parameter_associated_type = member_type;
make_shape:
    shape = new_type_shape(kind, attributes, parameters);
    attach_type_shape(declaration, shape);
    parameter_associated_type = 0;
}

int suppress_implicit_member; /* original 0x911ec */

/* Original 0x628e0..0x6298a. Allocation operators never receive implicit this. */
int __stdcall is_member_declarator(CppSymbol *declaration)
{
    TypeShape *shape;
    QualifiedDeclarator *qualifier;
    if (declaration->name == name_new || declaration->name == name_delete)
        return 0;
    shape = declaration->type->related;
    if (shape && !(shape->flags & 0x77))
        shape = shape->related;
    if (shape)
        return shape->flags & 0x10;
    qualifier = (QualifiedDeclarator *)declaration->next;
    if (qualifier && (!qualifier->resolution || qualifier->resolution->binding->next))
        return 1;
    if ((declaration->kind & 31) == 18)
        return 1;
    if (current_class_context >= 0) {
        if (suppress_implicit_member)
            return 0;
        if (!(global_declaration_type(0)->storage[1] & 4))
            return 1;
    }
    return 0;
}

/* Original 0x62b50..0x62c25. Apply a new outer shape while checking attributes. */
void __stdcall attach_type_shape(CppSymbol *declaration, TypeShape *shape)
{
    unsigned short attributes = shape->attributes, kind = shape->flags;
    CppSymbol *class_symbol, *based_symbol;
    if (attributes & 0x800) {
        compiler_warning(1, 147, format_symbol_name(declaration), token_spellings[48]);
        attributes &= ~0x800;
    }
    if (attributes & 0x200) {
        compiler_warning(1, 147, format_symbol_name(declaration), token_spellings[47]);
        attributes &= ~0x200;
    }
    if ((kind & 0x41) && !(shape->attributes & 7) &&
        is_unqualified_class_type(declaration->type)) {
        class_symbol = cpp_context_stack->items[declaration->type->field08];
        attributes |= class_symbol->detail.class_info->pointer_attributes;
    }
    shape->attributes = attributes;
    if ((attributes & 7) == 5) {
        based_symbol = shape->data.based_symbol;
        if ((based_symbol->kind & 31) == 13 && (based_symbol->auxiliary.value & 0xff) == 8)
            based_symbol->auxiliary.value = (based_symbol->auxiliary.value & ~0xffU) | 1;
    }
    shape->related = declaration->type->related;
    declaration->type->related = shape;
}

/* Original 0x36700..0x36741. Return the parsed parameter symbols before
   releasing the temporary, single-bucket parameter scope. */
CppSymbol *finish_parameter_scope(void)
{
    CppSymbol *parameters = 0;
    if ((current_scope->flags & 0xff) != 1 || current_scope->bucket_mask)
        return 0;
    if (current_scope->buckets)
        parameters = *current_scope->buckets;
    retire_scope_directory_entry();
    pop_symbol_scope();
    return parameters;
}

/* Original 0x4f790..0x4f7aa. */
void __stdcall restore_declaration_context(unsigned char mode, SymbolScope *scope)
{
    declaration_mode_cache = mode;
    declaration_mode = mode;
    current_scope = scope;
}

/* Original 0x3b930..0x3b955. Prepending preserves the reverse circular
   parameter link and places the old head in the source-order chain. */
void __stdcall prepend_declarator(CppSymbol *list, CppSymbol *declaration)
{
    if (!declaration) return;
    if (!list) {
        declaration->detail.parameter_next = declaration;
        return;
    }
    declaration->detail.parameter_next = list->detail.parameter_next;
    list->detail.parameter_next = declaration;
    declaration->next = list;
}

unsigned char symbol_scope_mode; /* original 0x8c138 */

/* Original 0x3bed0..0x3bfda. Scope mode one uses temporary allocation
   while retaining global insertion; other nonzero modes use current_scope. */
CppSymbol *__stdcall new_symbol(char *name, unsigned char scope)
{
    CppSymbol *symbol;
    ArenaPool *pool;
    unsigned kind;
    if (!scope) {
        symbol = allocate_permanent(sizeof(CppSymbol));
        kind = 1;
    } else {
        pool = &compiler_arena.pools[1];
        if (pool->remaining < (int)sizeof(CppSymbol)) refill_arena(&compiler_arena, pool);
        symbol = (CppSymbol *)pool->next;
        pool->remaining -= sizeof(CppSymbol);
        pool->next += sizeof(CppSymbol);
        kind = 4;
    }
    if (scope == 1) scope = 0;
    symbol->storage[0] = (declaration_storage[0] & ~7) | kind;
    symbol->storage[1] = declaration_storage[1];
    symbol->storage[2] = declaration_storage[2];
    symbol->kind = 9;
    symbol->name = name;
    if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_symbol_rollover();
    symbol->index = next_symbol_index++;
    symbol->references = 0;
    symbol->type = type_0064;
    symbol_scope_mode = scope;
    insert_scope_symbol(scope ? current_scope : global_scope, symbol);
    return symbol;
}

/* Original 0x3c1e0..0x3c2a1, stdcall4. Abstract declarators use the
   shared cast name and an arena selected by implicit-member suppression.
   Keep both original allocation dispatches, including permanent/-1 cases. */
CppSymbol *__stdcall new_abstract_declarator(TypeShape *shape)
{
    unsigned arena = (suppress_implicit_member <= 0) + 1;
    CppSymbol *declaration;
    ArenaPool *pool;
    if (!arena)
        declaration = allocate_permanent(sizeof(CppSymbol));
    else {
        if (arena == (unsigned)-1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(CppSymbol))
            refill_arena(&compiler_arena, pool);
        declaration = (CppSymbol *)pool->next;
        pool->remaining -= sizeof(CppSymbol);
        pool->next += sizeof(CppSymbol);
    }
    declaration->kind = 12;
    declaration->name = name_cast;
    arena = (suppress_implicit_member <= 0) + 1;
    if (!arena)
        declaration->type = allocate_permanent(sizeof(CType));
    else {
        if (arena == (unsigned)-1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(CType))
            refill_arena(&compiler_arena, pool);
        declaration->type = (CType *)pool->next;
        pool->remaining -= sizeof(CType);
        pool->next += sizeof(CType);
    }
    if (!shape) shape = new_type_shape(0, 0, 0);
    return apply_declarator_modifier(declaration, shape);
}

/* Original 0x3c150..0x3c1d6, stdcall8. Distance/based modifiers gain
   their own outer shape and copied mutable type; ordinary qualifiers update
   the existing type's attribute word directly. */
CppSymbol *__stdcall apply_declarator_modifier(CppSymbol *declaration,
    TypeShape *modifier)
{
    CType *type;
    ArenaPool *pool;
    if (modifier->attributes & 12) {
        declaration->attributes |= 2;
        if (token_allocation_flags & 0x20)
            type = allocate_permanent(sizeof(CType));
        else {
            pool = &compiler_arena.pools[2];
            if (pool->remaining < (int)sizeof(CType))
                refill_arena(&compiler_arena, pool);
            type = (CType *)pool->next;
            pool->remaining -= sizeof(CType);
            pool->next += sizeof(CType);
        }
        *type = *declaration->type;
        modifier->related = type->related;
        type->related = modifier;
        declaration->type = type;
    } else {
        declaration->type->attributes = modifier->attributes;
    }
    return declaration;
}
