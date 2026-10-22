/* Completed class layout, synthesized members and member-name indexing. */
#include "class-finalization.h"
#include "class-type-sets.h"
#include "class-members.h"

/* Original 0x2dfc0..0x2e0e7. Retain the temporary class type while running
   completion passes, then leave the wrapper scope and build its name index. */
CType *end_class_specifier(void)
{
    ClassContext *context;
    CppSymbol *tag;
    CType *type;
    unsigned short alignment;
    class_contexts[current_class_context].flags |= 3;
    context = &class_contexts[current_class_context];
    tag = context->symbol;
    type = context->type;
    if (!(tag->flags & 0x80) && browser_filename && (!declaration_mode || options.option_FR))
        browser_end_tag();
    if (!suppress_implicit_member) {
        build_conversion_type_sets(tag);
        token_allocation_flags |= 0x08000000UL;
        prepare_class_virtual_tables(tag);
        declare_implicit_special_members(tag);
        finalize_class_virtual_tables(tag);
        declare_default_constructor_closures(tag);
        declare_deleting_destructors(tag);
        token_allocation_flags &= ~0x08000000UL;
    }
    if (class_contexts[current_class_context].flags & 8) emit_deferred_class_terminator();
    tag->detail.class_info->alignment_flags =
        (tag->detail.class_info->alignment_flags & ~0x3f) |
        (class_contexts[current_class_context].alignment & 0x3f);
    tag->flags |= 4;
    tag->type->debug_index = 0;
    alignment = class_contexts[current_class_context].alignment;
    --current_class_context;
    if (current_class_context >= 0 && class_contexts[current_class_context].alignment <= alignment)
        class_contexts[current_class_context].alignment = alignment;
    tag_declaration_seen = 1;
    if ((unsigned char)current_scope->flags >= 250 && current_scope->buckets[0] == tag) {
        retire_scope_directory_entry();
        pop_symbol_scope();
    }
    build_class_member_index(tag);
    return type;
}

/* Original 0x66720..0x668b3. Gather inherited conversion targets, recording
   repeated paths separately. Locally declared conversions remove that ambiguity.
   The final sharing pass may reuse an identical inherited set. */
void __stdcall build_conversion_type_sets(CppSymbol *tag)
{
    TypeSet *types = new_type_set(), *ambiguous = new_type_set();
    BaseClassLink *base;
    CppSymbol *base_tag;
    TypeSetIterator iterator;
    ConversionIterator members;
    for (base = (BaseClassLink *)tag->auxiliary.scope_members; base;
            base = (BaseClassLink *)base->next) {
        if ((base->kind & 31) != 15 || (base->flags & 0x80)) continue;
        base_tag = cpp_context_stack->items[base->context_index];
        ambiguous = union_type_sets(ambiguous, base_tag->detail.class_info->ambiguous_conversion_types);
    }
    for (base = (BaseClassLink *)tag->auxiliary.scope_members; base;
            base = (BaseClassLink *)base->next) {
        if ((base->kind & 31) != 15 || (base->flags & 0x80)) continue;
        base_tag = cpp_context_stack->items[base->context_index];
        begin_type_set(&iterator, base_tag->detail.class_info->conversion_types);
        while (next_type_set(&iterator)) {
            if (!type_set_contains(types, iterator.type)) types = add_type_set(types, iterator.type);
            else ambiguous = add_type_set(ambiguous, iterator.type);
        }
    }
    begin_conversion_functions(&members, tag);
    while (next_conversion_function(&members)) {
        types = add_type_set(types, members.type);
        ambiguous = remove_type_set(ambiguous, members.type);
    }
    types = share_inherited_type_set(types, tag);
    ambiguous = share_inherited_type_set(ambiguous, tag);
    tag->detail.class_info->conversion_types = types;
    tag->detail.class_info->ambiguous_conversion_types = ambiguous;
}

/* Original 0x79150..0x79164. Preparation and construction are distinct passes. */
void __stdcall prepare_class_virtual_tables(CppSymbol *tag)
{
    initialize_class_virtual_tables(tag);
    build_class_virtual_tables(tag);
}

/* Original 0x79190..0x791d7. Clear inherited layout flags before allocating
   distinct vftable, vbtable and secondary-table vectors from arena two. */
void __stdcall initialize_class_virtual_tables(CppSymbol *tag)
{
    Vector *vector;
    int i;
    tag->detail.class_info->alignment_flags &= ~0x80;
    tag->detail.class_info->alignment_flags &= ~0x40;
    for (i = 0; i < 2; ++i) {
        vector = new_empty_vector(2);
        if (!i) tag->detail.class_info->virtual_tables = vector;
        else tag->detail.class_info->virtual_base_tables = vector;
    }
    tag->detail.class_info->field2c = 0;
    tag->detail.class_info->virtual_table_groups = new_empty_vector(2);
}

/* Original 0x791e0..0x792f4. Inherit base dispatch tables, create missing
   vfptr/vbptr members where needed, then close the nonvirtual layout. */
void __stdcall build_class_virtual_tables(CppSymbol *tag)
{
    BaseClassLink *base;
    CppSymbol *vfptr, *vbptr;
    VirtualTable *table;
    int virtual_base = 0, made_vfptr = 0, made_vbptr = 0;
    for (base = (BaseClassLink *)tag->auxiliary.scope_members; base;
            base = (BaseClassLink *)base->next) {
        if ((base->kind & 31) != 15 || (base->flags & 0x80)) continue;
        inherit_base_virtual_tables(tag, base, &virtual_base);
    }
    if (!(tag->detail.class_info->alignment_flags & 0x80) && virtual_base) {
        vbptr = create_virtual_pointer(tag, 1);
        made_vbptr = 1;
    }
    if (!(tag->detail.class_info->alignment_flags & 0x40) && has_new_virtual_members(tag)) {
        vfptr = create_virtual_pointer(tag, 0);
        made_vfptr = 1;
    }
    if (made_vfptr) {
        table = new_virtual_table(0, tag, 0, vfptr);
        append_vector(tag->detail.class_info->virtual_tables, table);
    }
    if (made_vbptr) {
        table = build_virtual_base_table(tag, vbptr);
        append_vector(tag->detail.class_info->virtual_base_tables, table);
    }
    finish_nonvirtual_class_size(tag);
}

/* Original 0x795f0..0x79692. A virtual member that does not override an
   inherited slot requires this class to introduce a vftable. */
int __stdcall has_new_virtual_members(CppSymbol *tag)
{
    CppSymbol *member, *next_member, *function, *next_function;
    for (member = tag->auxiliary.scope_members; member; member = next_member) {
        next_member = member->next;
        if ((member->kind & 31) != 17 || (member->flags & 0x80)) continue;
        for (function = ((OverloadSet *)member)->functions; function; function = next_function) {
            next_function = function->next;
            if ((function->storage[1] & 2) && !overrides_virtual_member(function, tag, 0)) return 1;
        }
    }
    return 0;
}

/* Original 0x79cb0..0x79d00. Finish a pending bit-field unit before the
   class alignment pass, then retain the nonvirtual size separately. */
void __stdcall finish_nonvirtual_class_size(CppSymbol *tag)
{
    ClassContext *context = &class_contexts[current_class_context];
    if (context->bit_field_type && context->bit_field_bits)
        context->size += context->bit_field_type & 15;
    align_class_member(context->alignment);
    tag->detail.class_info->nonvirtual_size = class_contexts[current_class_context].size;
}
