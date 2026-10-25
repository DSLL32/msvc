/* C++ function/base virtual table emission. */
#include "virtual-table-output.h"
#include "unary-expressions.h"
#include "parameters.h"
#include "constructor-initializers.h"
#include "cpp-context.h"
#include "function-finalization.h"
#include "source-input.h"
#include "parameter-scopes.h"
#include <recon.h>

CppSymbol *__stdcall select_purecall_symbol(int model);

/* Original 0x7b6e0..0x7b70d, stdcall4. Pure virtual slots use
   the cached runtime handler while retaining this member's callable type. */
IrToken *__stdcall pure_virtual_table_expression(CppSymbol *function)
{
    IrToken *value = builtin_unary_expression(0x3a,
        symbol_reference(select_purecall_symbol(0)));
    value->type = function_designator_type(function->type);
    return value;
}

/* Original 0x7b5a0..0x7b6de, stdcall8. Reserve the initializer's
   symbol before testing whether this table definition has already appeared. */
void __stdcall emit_class_virtual_table(VirtualTable *table, CppSymbol *tag)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    CppSymbol *element, *function;
    VirtualEntry *entry;
    IrToken *value;
    int index = 0;
    if (pool->remaining < (int)sizeof(CppSymbol))
        refill_arena(&compiler_arena, pool);
    element = (CppSymbol *)pool->next;
    pool->remaining -= sizeof(CppSymbol);
    pool->next += sizeof(CppSymbol);
    if ((table->symbol->storage[2] & 4) && !(table->symbol->storage[2] & 8))
        return;
    if (!table->kind || table->kind == 2)
        prepare_virtual_table_functions(table, tag);
    table->symbol->attributes |= 4;
    table->symbol->storage[0] = (table->symbol->storage[0] & 0xfa) | 2;
    write_literal_initializer(table->symbol, 0, 0);
    element->kind = 9;
    element->name = 0;
    while (index < table->entry_count) {
        entry = table->entries;
        if (entry) entry += index;
        ++index;
        if (!entry) break;
        switch (table->kind) {
        case 0:
        case 2:
            function = resolve_virtual_table_function(&entry->dispatch, tag, table);
            if (entry->dispatch.function->cpp.parts.member_flags & 4)
                value = pure_virtual_table_expression(entry->dispatch.function);
            else
                value = builtin_unary_expression(0x3a, symbol_reference(function));
            break;
        case 1:
        case 3:
            value = make_integer_constant(virtual_base_table_entry_offset(
                &entry->base, tag, table));
            break;
        /* Producers establish kinds0..3. The original has no fallback
           assignment for other values; retain its previous/unwritten value. */
        }
        element->type = value->type;
        write_initializer_value(element, value, 0);
    }
    flush_initializer_records();
}

/* Original 0x7b540..0x7b597, stdcall8. Resolve every function slot
   before beginning the table's initializer stream. */
void __stdcall prepare_virtual_table_functions(VirtualTable *table, CppSymbol *tag)
{
    VirtualEntry *entry;
    int index = 0;
    while (index < table->entry_count) {
        entry = table->entries;
        if (entry) entry += index;
        ++index;
        if (!entry) break;
        resolve_virtual_table_function(&entry->dispatch, tag, table);
    }
}

/* Original 0x78cd0..0x78d49, stdcall12. Destructor slots point to their
   vector deleting form. Ordinary this adjustments and vtordisp slots may
   require a cached thunk with the original signed displacement words. */
CppSymbol *__stdcall resolve_virtual_table_function(VirtualCallEntry *entry,
    CppSymbol *tag, VirtualTable *table)
{
    CppSymbol *function = entry->function;
    int displacement = virtual_entry_this_displacement(entry, tag, table);
    int virtual_displacement;
    TypeShape *shape;
    unsigned kind;
    if (function->declaration_flags & 0x2000) {
        shape = function->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        function = lookup_vector_delete_destructor(shape->data.parameters->associated_type);
    }
    if (displacement || (entry->flags & 8)) {
        kind = (entry->flags & 8) ? 4 : 1;
        virtual_displacement = (int)(0U -
            (table->object_offset + virtual_base_entry_width()));
        function = get_virtual_adjustment_thunk(kind, function,
            displacement, virtual_displacement);
    }
    return function;
}

/* Original 0x78ca0..0x78cc6, stdcall12. The table's complete object
   position minus the function's required subobject gives its this adjustment. */
int __stdcall virtual_entry_this_displacement(VirtualCallEntry *entry,
    CppSymbol *tag, VirtualTable *table)
{
    unsigned table_offset = virtual_table_object_offset(table);
    unsigned function_offset = virtual_adjustment_offset(&entry->this_adjustment, tag);
    return (int)(table_offset - function_offset);
}

/* Original 0x788b0..0x788e9, stdcall8. A nonzero signed class index
   adds that virtual base's complete-object offset to the fixed displacement. */
unsigned __stdcall virtual_adjustment_offset(VirtualAdjustment *adjustment,
    CppSymbol *tag)
{
    unsigned virtual_offset = 0;
    if (adjustment->virtual_base_class)
        virtual_offset = virtual_base_subobject_offset(cpp_context_stack->items[
            (int)adjustment->virtual_base_class], tag);
    return adjustment->displacement + virtual_offset;
}

HashTable *virtual_adjustment_thunks; /* original 0x9f330 */

/* Original 0x7bb40..0x7bbaf, stdcall16. Temporary lookup descriptors
   use arena2; a generated thunk retains its own permanent copied key. */
CppSymbol *__stdcall get_virtual_adjustment_thunk(unsigned kind,
    CppSymbol *function, int displacement, int virtual_displacement)
{
    VirtualThunkKey *key, *stored;
    CppSymbol *thunk;
    key = new_virtual_thunk_key(kind, function, displacement,
        virtual_displacement, 2);
    if (!virtual_adjustment_thunks)
        virtual_adjustment_thunks = new_hash_table(32, 0, 4, 3);
    thunk = find_hash_value(virtual_adjustment_thunks, key);
    if (!thunk) {
        thunk = emit_virtual_adjustment_thunk(key);
        stored = copy_virtual_thunk_key(key, 0);
        set_hash_value(virtual_adjustment_thunks, stored, thunk);
    }
    return thunk;
}

/* Original 0x7ba30..0x7ba8d, stdcall20. Arena0 requests permanent
   storage; -1 selects the compiler's arena0 rather than permanent storage. */
VirtualThunkKey *__stdcall new_virtual_thunk_key(unsigned kind,
    CppSymbol *function, int displacement, int virtual_displacement, int arena)
{
    VirtualThunkKey *key;
    ArenaPool *pool;
    if (!arena)
        key = allocate_permanent(sizeof(VirtualThunkKey));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(VirtualThunkKey))
            refill_arena(&compiler_arena, pool);
        key = (VirtualThunkKey *)pool->next;
        pool->remaining -= sizeof(VirtualThunkKey);
        pool->next += sizeof(VirtualThunkKey);
    }
    key->kind = kind;
    key->function = function;
    key->displacement = displacement;
    key->virtual_displacement = virtual_displacement;
    return key;
}

/* Original 0x7baf0..0x7bb0c, cdecl callback. Pointer contribution
   uses the aligned low-page bits; all additions wrap as unsigned words. */
unsigned __cdecl hash_virtual_thunk_key(void *value)
{
    VirtualThunkKey *key = value;
    return (((unsigned)key->function & 0xffc) >> 2) +
        (unsigned)key->displacement + (unsigned)key->virtual_displacement + key->kind;
}

/* Original 0x7bbf0..0x7bdd7, stdcall4. Clone the function declaration
   into a generated this/vtordisp thunk, emit its dedicated IR operation,
   and register its completed function directory entry. */
CppSymbol *__stdcall emit_virtual_adjustment_thunk(VirtualThunkKey *key)
{
    CppSymbol *thunk;
    ArenaPool *pool;
    IrToken *arguments;
    int arena = declaration_mode != 0, final_offset;
    unsigned char storage_kind;
    if (!arena)
        thunk = allocate_permanent(sizeof(CppSymbol));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(CppSymbol))
            refill_arena(&compiler_arena, pool);
        thunk = (CppSymbol *)pool->next;
        pool->remaining -= sizeof(CppSymbol);
        pool->next += sizeof(CppSymbol);
    }
    *thunk = *key->function;
    thunk->next = 0;
    thunk->this_displacement = key->displacement;
    thunk->detail.thunk_virtual_displacement = key->virtual_displacement;
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    thunk->index = next_symbol_index++;
    thunk->cpp.parts.member_qualifiers =
        (thunk->cpp.parts.member_qualifiers & 0xf9) | (key->kind == 1 ? 2 : 4);
    thunk->flags |= 4;
    thunk->cpp.parts.member_qualifiers &= 0xfe;
    thunk->storage[0] = (thunk->storage[0] & 0xf9) | 1;
    storage_kind = thunk->storage[0] & 7;
    thunk->storage[0] = declaration_storage[0];
    thunk->storage[1] = declaration_storage[1];
    thunk->storage[2] = declaration_storage[2];
    thunk->storage[0] = (thunk->storage[0] & 0xf8) | storage_kind;
    thunk->storage[0] |= 0x40;
    thunk->storage[1] |= 2;
    begin_function_output(1);
    active_parameter_symbols = 0;
    write_ir_record(0x53, 0);
    emit_expression_tree(new_left_typed_expression(0x76,
        symbol_reference(thunk), 0));
    if (key->kind == 4) {
        arguments = append_expression(0,
            make_integer_constant(key->virtual_displacement), 2);
        arguments = append_expression(arguments,
            make_integer_constant(key->displacement), 2);
        arguments = append_expression(arguments, symbol_reference(key->function), 2);
        final_offset = virtual_thunk_reserved_offset(key);
    } else {
        arguments = append_expression(0, symbol_reference(key->function), 2);
        final_offset = key->displacement;
    }
    arguments = append_expression(arguments, make_integer_constant(final_offset), 2);
    emit_expression_tree(new_left_typed_expression(0xa8,
        make_integer_constant(key->kind), arguments));
    write_ir_record(0x47, 0);
    write_ir_record(0x54, make_integer_constant(0));
    record_function_definition(thunk);
    return thunk;
}

/* Original 0x7bbe0..0x7bbe4, stdcall4. This target retains a separate
   reserved-offset query entry even though every descriptor returns zero. */
int __stdcall virtual_thunk_reserved_offset(VirtualThunkKey *key)
{
    return 0;
}

/* Original 0x7ba90..0x7baec, stdcall8. Preserve a descriptor in the
   selected arena without retaining its temporary lookup allocation. */
VirtualThunkKey *__stdcall copy_virtual_thunk_key(VirtualThunkKey *key, int arena)
{
    VirtualThunkKey *copy;
    ArenaPool *pool;
    if (!arena)
        copy = allocate_permanent(sizeof(VirtualThunkKey));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(VirtualThunkKey))
            refill_arena(&compiler_arena, pool);
        copy = (VirtualThunkKey *)pool->next;
        pool->remaining -= sizeof(VirtualThunkKey);
        pool->next += sizeof(VirtualThunkKey);
    }
    *copy = *key;
    return copy;
}

/* Original 0x7bb10..0x7bb3e, cdecl callback. The function pointer and
   both signed adjustment words participate independently of the thunk kind. */
int __cdecl equal_virtual_thunk_keys(void *first, void *second)
{
    VirtualThunkKey *left = first, *right = second;
    return left->kind == right->kind && left->function == right->function &&
        left->displacement == right->displacement &&
        left->virtual_displacement == right->virtual_displacement;
}

/* Original 0x78d90..0x78dbb, stdcall12. Virtual-base slots are
   relative to the vbptr; the leading null-base entry negates its offset. */
int __stdcall virtual_base_table_entry_offset(VirtualBaseEntry *entry,
    CppSymbol *tag, VirtualTable *table)
{
    if (!entry->base) return (int)(0U - entry->pointer_offset);
    return (int)(virtual_base_subobject_offset(entry->base, tag) -
        table->virtual_base_offset - entry->pointer_offset);
}
