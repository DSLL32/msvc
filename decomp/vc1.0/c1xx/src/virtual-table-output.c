/* C++ function/base virtual table emission. */
#include "virtual-table-output.h"
#include "unary-expressions.h"
#include "parameters.h"
#include "constructor-initializers.h"
#include "cpp-context.h"
#include <recon.h>

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
