/* Completed class layout, synthesized members and member-name indexing. */
#include <stdlib.h>
#include "class-finalization.h"
#include "class-type-sets.h"
#include "class-members.h"
#include "global-ir.h"
#include "cpp-context.h"
#include "constructor-initializers.h"
#include <recon.h>

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
            if (!type_set_contains(types, iterator.type)) types = add_inline_pointer(types, iterator.type);
            else ambiguous = add_inline_pointer(ambiguous, iterator.type);
        }
    }
    begin_conversion_functions(&members, tag);
    while (next_conversion_function(&members)) {
        types = add_inline_pointer(types, members.type);
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

/* Original 0x79170..0x79185. Complete dispatch layout before declaring
   table symbols; these are separate native passes. */
void __stdcall finalize_class_virtual_tables(CppSymbol *tag)
{
    complete_class_virtual_layout(tag);
    declare_class_virtual_tables(tag);
}

/* Original 0x79d60..0x79db4. Preserve the nine ordered native passes;
   only the last query supplies the abstract-class bit in the class flags. */
void __stdcall complete_class_virtual_layout(CppSymbol *tag)
{
    int abstract;
    reset_previous_base_class();
    complete_base_class_layout(tag);
    finish_class_size(tag);
    assign_virtual_base_group_offsets(tag);
    resolve_virtual_overrides(tag);
    diagnose_virtual_ambiguities(tag);
    coalesce_virtual_table_paths(tag);
    persist_class_virtual_tables(tag);
    abstract = class_has_pure_virtual(tag);
    tag->detail.class_info->flags = (tag->detail.class_info->flags & 0x7f) |
        ((abstract & 1) << 7);
}

CppSymbol *previous_base_class; /* original 0xa8134 */

/* Original 0x7a130..0x7a13b. Reset the previous-base comparison context
   before laying out the direct base classes. */
void reset_previous_base_class(void)
{
    previous_base_class = 0;
}

void __stdcall complete_direct_base_layout(BaseClassLink *base, CppSymbol *tag);

/* Original 0x79dc0..0x79e31. Finalize every visible direct base before the
   derived size and virtual overrides are resolved. Save the next link first. */
void __stdcall complete_base_class_layout(CppSymbol *tag)
{
    BaseClassLink *base = (BaseClassLink *)tag->auxiliary.scope_members, *next;
    while (base) {
        next = (BaseClassLink *)base->next;
        if ((base->kind & 31) == 15 && !(base->flags & 0x80))
            complete_direct_base_layout(base, tag);
        base = next;
    }
}


/* Original 0x79d10..0x79d5b. Empty class objects still occupy one byte;
   both the active layout context and the canonical class information agree. */
void __stdcall finish_class_size(CppSymbol *tag)
{
    ClassInfo *info = tag->detail.class_info;
    info->size = class_contexts[current_class_context].size;
    if (!info->size) {
        class_contexts[current_class_context].size = 1;
        info->size = 1;
        info->flags |= 8;
        info->flags |= 4;
    }
}

int __stdcall find_virtual_base_group_offsets(VirtualTableGroup *group, CppSymbol *tag);
void __stdcall create_virtual_base_group_offsets(VirtualTableGroup *group, CppSymbol *tag);

/* Original 0x7a970..0x7a9cf. Reuse a group's existing virtual-base entry
   where present, otherwise add the entry and record its offsets. */
void __stdcall assign_virtual_base_group_offsets(CppSymbol *tag)
{
    Vector *groups = tag->detail.class_info->virtual_table_groups;
    VirtualTableGroup *group;
    short index = 0;
    while (index < groups->count) {
        group = groups->items[index++];
        if (!find_virtual_base_group_offsets(group, tag))
            create_virtual_base_group_offsets(group, tag);
    }
}

void __stdcall resolve_function_virtual_overrides(CppSymbol *member, CppSymbol *tag);

/* Original 0x7a5f0..0x7a679. Only visible function-overload wrappers
   participate; each function is visited with its next link saved first. */
void __stdcall resolve_virtual_overrides(CppSymbol *tag)
{
    CppSymbol *member = tag->auxiliary.scope_members, *next, *function, *following;
    while (member) {
        next = member->next;
        if ((member->kind & 31) == 17 && !(member->flags & 0x80)) {
            function = ((OverloadSet *)member)->functions;
            while (function) {
                following = function->next;
                resolve_function_virtual_overrides(function, tag);
                function = following;
            }
        }
        member = next;
    }
}

void __stdcall resolve_table_override(CppSymbol *member, VirtualTable *table,
    CppSymbol *tag, char virtual_path);
VirtualTable *__stdcall primary_virtual_table(CppSymbol *tag);
void __stdcall initialize_virtual_entry(VirtualEntry *entry, CppSymbol *member);
void __stdcall append_virtual_entry(VirtualTable *table, VirtualEntry *entry);
unsigned __stdcall reserve_virtual_table_offset(VirtualTable *table, unsigned size);

/* Original 0x7a680..0x7a7ac. Resolve this function against inherited
   primary/group tables before allocating any newly introduced virtual slot. */
void __stdcall resolve_function_virtual_overrides(CppSymbol *member, CppSymbol *tag)
{
    Vector *tables = tag->detail.class_info->virtual_tables;
    Vector *groups = tag->detail.class_info->virtual_table_groups;
    VirtualTableGroup *group = 0;
    VirtualTable *table;
    VirtualEntry entry;
    short table_index = 0, group_index = 0;
    for (;;) {
        while (table_index < tables->count) {
            table = tables->items[table_index++];
            resolve_table_override(member, table, tag, (char)(group && group->field20));
        }
        if (group_index >= groups->count) break;
        group = groups->items[group_index++];
        tables = group->tables;
        table_index = 0;
    }
    if (member->cpp.parts.member_flags & 2) {
        member->storage[1] |= 2;
        ((unsigned char *)member->name)[-1] |= 0x10;
        return;
    }
    if (member->storage[1] & 2) {
        table = primary_virtual_table(tag);
        initialize_virtual_entry(&entry, member);
        append_virtual_entry(table, &entry);
        member->detail.virtual_table_offset = reserve_virtual_table_offset(table, type_size(member->type));
        member->auxiliary.virtual_function.table_class = tag->detail.class_info->type_index;
    }
}

void __stdcall collapse_virtual_conflicts(VirtualEntry *entry);

/* Original 0x7a460..0x7a5e1. Diagnose every conflicting final overrider
   before collapsing its alternative list; retain the warning-only flag too. */
void __stdcall diagnose_virtual_ambiguities(CppSymbol *tag)
{
    Vector *groups = tag->detail.class_info->virtual_table_groups;
    Vector *tables = tag->detail.class_info->virtual_tables, *conflicts;
    VirtualTableGroup *group;
    VirtualTable *table;
    VirtualEntry *entry;
    CppSymbol *function, *owner;
    short group_index = 0, table_index = 0, conflict_index;
    int index;
    for (;;) {
        while (table_index < tables->count) {
            table = tables->items[table_index++];
            if (table->kind != 2) continue;
            for (index = 0; index < table->entry_count; ++index) {
                entry = &table->entries[index];
                if (entry->field0c & 4) {
                    conflicts = (Vector *)entry->function;
                    for (conflict_index = 0; conflict_index < conflicts->count; ++conflict_index) {
                        function = conflicts->items[conflict_index];
                        owner = cpp_context_stack->items[function->auxiliary.declaring_class];
                        report_syntax_error(250, tag->name, owner->name, function->name);
                    }
                    collapse_virtual_conflicts(entry);
                } else if (entry->field0c & 2) {
                    function = entry->function;
                    owner = cpp_context_stack->items[function->auxiliary.declaring_class];
                    compiler_warning(2, 250, tag->name, owner->name, function->name);
                }
            }
        }
        if (group_index >= groups->count) break;
        group = groups->items[group_index++];
        tables = group->tables;
        table_index = 0;
    }
}

/* Original 0x7aab0..0x7ac6d. Tables with equal owner paths share their
   owners explicitly. Scan both vfptr/vbptr families and every virtual group. */
void __stdcall coalesce_virtual_table_paths(CppSymbol *tag)
{
    Vector *groups, *tables, *other_groups, *other_tables;
    VirtualTableGroup *group;
    VirtualTable *table, *other;
    short group_index, table_index, other_group_index, other_table_index;
    int kind, matched;
    for (kind = 0; kind < 2; ++kind) {
        tables = kind ? tag->detail.class_info->virtual_base_tables : tag->detail.class_info->virtual_tables;
        groups = tag->detail.class_info->virtual_table_groups;
        group_index = table_index = 0;
        for (;;) {
            while (table_index < tables->count) {
                table = tables->items[table_index++];
                if (!table->unshared_owner) continue;
                matched = 0;
                other_tables = kind ? tag->detail.class_info->virtual_base_tables : tag->detail.class_info->virtual_tables;
                other_groups = tag->detail.class_info->virtual_table_groups;
                other_group_index = other_table_index = 0;
                for (;;) {
                    while (other_table_index < other_tables->count) {
                        other = other_tables->items[other_table_index++];
                        if (other == table || !other->unshared_owner) continue;
                        if (equal_vectors(table->shared_owners, other->shared_owners)) {
                            matched = 1;
                            append_vector(other->shared_owners, other->unshared_owner);
                            other->unshared_owner = 0;
                        }
                    }
                    if (other_group_index >= other_groups->count) break;
                    group = other_groups->items[other_group_index++];
                    other_tables = kind ? group->base_tables : group->tables;
                    other_table_index = 0;
                }
                if (matched) {
                    append_vector(table->shared_owners, table->unshared_owner);
                    table->unshared_owner = 0;
                }
            }
            if (group_index >= groups->count) break;
            group = groups->items[group_index++];
            tables = kind ? group->base_tables : group->tables;
            table_index = 0;
        }
    }
}

/* Both copy callbacks retain two stack slots (RET 8). The table callback
   does not read its arena slot in the native body. */
void *__stdcall clone_class_virtual_table(void *item, int arena);
void *__stdcall clone_class_virtual_group(void *item, int arena);

/* Original 0x7ac70..0x7acb6. Move the completed tables and their grouped
   collections into permanent arena zero through native copy callbacks. */
void __stdcall persist_class_virtual_tables(CppSymbol *tag)
{
    ClassInfo *info = tag->detail.class_info;
    info->virtual_tables = copy_vector_elements(info->virtual_tables, 0, clone_class_virtual_table);
    info->virtual_base_tables = copy_vector_elements(info->virtual_base_tables, 0, clone_class_virtual_table);
    info->virtual_table_groups = copy_vector_elements(info->virtual_table_groups, 0, clone_class_virtual_group);
}

/* Original 0x7acc0..0x7adb2. Every primary or grouped vftable contributes
   its final-overrider flags to the abstract-class classification. */
int __stdcall class_has_pure_virtual(CppSymbol *tag)
{
    Vector *groups = tag->detail.class_info->virtual_table_groups;
    Vector *tables = tag->detail.class_info->virtual_tables;
    VirtualTableGroup *group;
    VirtualTable *table;
    short group_index = 0, table_index = 0;
    int index;
    for (;;) {
        while (table_index < tables->count) {
            table = tables->items[table_index++];
            for (index = 0; index < table->entry_count; ++index)
                if (table->entries[index].function->cpp.parts.member_flags & 4) return 1;
        }
        if (group_index >= groups->count) break;
        group = groups->items[group_index++];
        tables = group->tables;
        table_index = 0;
    }
    return 0;
}

CppSymbol *__stdcall declare_virtual_table_symbol(int kind, VirtualTable *table, CppSymbol *tag);

/* Original 0x7adc0..0x7ae7c. Declare every primary and grouped vfptr/vbptr
   table after permanence, storing its resulting declaration symbol at +4. */
void __stdcall declare_class_virtual_tables(CppSymbol *tag)
{
    Vector *groups, *tables;
    VirtualTableGroup *group;
    VirtualTable *table;
    short group_index, table_index;
    int kind;
    for (kind = 0; kind < 2; ++kind) {
        tables = kind ? tag->detail.class_info->virtual_base_tables : tag->detail.class_info->virtual_tables;
        groups = tag->detail.class_info->virtual_table_groups;
        group_index = table_index = 0;
        for (;;) {
            while (table_index < tables->count) {
                table = tables->items[table_index++];
                table->symbol = declare_virtual_table_symbol(kind, table, tag);
            }
            if (group_index >= groups->count) break;
            group = groups->items[group_index++];
            tables = kind ? group->base_tables : group->tables;
            table_index = 0;
        }
    }
}

int class_member_hash_bits(unsigned count);

typedef struct ClassMemberIndexLink {
    CppSymbol *symbol;
    struct ClassMemberIndexLink *next;
} ClassMemberIndexLink;
typedef struct ClassMemberIndexBucket {
    ClassMemberIndexLink *first, *last;
    unsigned count;
} ClassMemberIndexBucket;
typedef char ClassMemberIndexLinkSizeMustBe8[(sizeof(ClassMemberIndexLink) == 8) ? 1 : -1];
typedef char ClassMemberIndexBucketSizeMustBe12[(sizeof(ClassMemberIndexBucket) == 12) ? 1 : -1];

/* Original 0x33660..0x337ed. Preserve declaration order within each
   interned-name bucket, then pack the persistent member-pointer arrays. */
void __stdcall build_class_member_index(CppSymbol *tag)
{
    unsigned members = 0, bucket_count = 1, mask, index, bucket;
    CppSymbol *member;
    ClassMemberIndexBucket *temporary;
    ClassMemberBucket *result;
    ClassMemberIndexLink *link;
    CppSymbol **destination;
    ArenaPool *pool = &compiler_arena.pools[2];
    for (member = tag->auxiliary.scope_members; member; member = member->next) ++members;
    if (members) bucket_count = 1U << (class_member_hash_bits(members) & 31);
    mask = bucket_count - 1;
    temporary = malloc(bucket_count * sizeof(ClassMemberIndexBucket));
    if (!temporary) fatal_error(60);
    for (index = 0; index < bucket_count; ++index) {
        temporary[index].first = temporary[index].last = 0;
        temporary[index].count = 0;
    }
    for (member = tag->auxiliary.scope_members; member; member = member->next) {
        bucket = (unsigned)member->name & mask;
        if (pool->remaining < (int)sizeof(ClassMemberIndexLink)) refill_arena(&compiler_arena, pool);
        link = (ClassMemberIndexLink *)pool->next;
        pool->remaining -= sizeof(ClassMemberIndexLink);
        pool->next += sizeof(ClassMemberIndexLink);
        link->symbol = member;
        /* The native body retains the arena's zero-filled next slot. */
        if (!temporary[bucket].first) {
            temporary[bucket].first = temporary[bucket].last = link;
            temporary[bucket].count = 1;
        } else {
            temporary[bucket].last->next = link;
            temporary[bucket].last = link;
            ++temporary[bucket].count;
        }
    }
    result = allocate_permanent(bucket_count * sizeof(ClassMemberBucket));
    if (!result) fatal_error(60);
    for (index = 0; index < bucket_count; ++index) {
        result[index].symbols = 0;
        result[index].count = 0;
    }
    for (index = 0; index < bucket_count; ++index) {
        if (!temporary[index].count) continue;
        result[index].count = temporary[index].count;
        destination = allocate_permanent(temporary[index].count * sizeof(CppSymbol *));
        result[index].symbols = destination;
        for (link = temporary[index].first; link; link = link->next) *destination++ = link->symbol;
    }
    free(temporary);
    tag->detail.class_info->member_index = result;
    tag->detail.class_info->member_index_mask = mask;
}


/* Original 0x33640..0x33654. The table-size exponent is floor(log2(count)). */
int class_member_hash_bits(unsigned count)
{
    int bits = 0;
    count >>= 1;
    while (count) {
        ++bits;
        count >>= 1;
    }
    return bits;
}

/* Original 0x7b440..0x7b537. Emit each queued class's virtual dispatch
   and virtual-base tables, including the tables retained in its base groups. */
void finish_inline_functions(void)
{
    TypeSetIterator iterator;
    CppSymbol *tag;
    VirtualTableGroup *group;
    Vector *tables, *groups;
    short table_index, group_index;
    int kind;
    if (!classes_pending_table_output) return;
    begin_type_set(&iterator, classes_pending_table_output);
    while (next_type_set(&iterator)) {
        tag = (CppSymbol *)iterator.type;
        for (kind = 0; kind < 2; ++kind) {
            tables = kind ? tag->detail.class_info->virtual_base_tables : tag->detail.class_info->virtual_tables;
            table_index = 0;
            groups = tag->detail.class_info->virtual_table_groups;
            group_index = 0;
            for (;;) {
                while (table_index < tables->count)
                    emit_class_virtual_table(tables->items[table_index++], tag);
                if (group_index >= groups->count) break;
                group = groups->items[group_index++];
                tables = kind ? group->base_tables : group->tables;
                table_index = 0;
            }
        }
    }
}

/* Original 0x79960..0x79b69. Insert a synthetic vfptr/vbptr member and
   reserve its aligned prefix by moving existing member/base displacements. */
CppSymbol *__stdcall create_virtual_pointer(CppSymbol *tag, int kind)
{
    CppSymbol *pointer;
    unsigned bytes, base, padding, span, alignment, limit;
    pointer = new_declarator(kind == 0 || kind == 2 ? name_vfptr : name_vbptr);
    pointer->flags |= 0x80;
    pointer->kind = 3;
    if (kind == 0) {
        attach_type_shape(pointer, new_type_shape(1, tag->detail.class_info->pointer_attributes, 0));
        attach_type_shape(pointer, new_type_shape(2, 0, 0));
        attach_type_shape(pointer, new_type_shape(1, 0, 0));
        add_function_shape(pointer, 0, 0);
        normalize_symbol_type(type_0080, pointer);
        tag->detail.class_info->virtual_dispatch_pointer = pointer;
    } else if (kind == 1) {
        attach_type_shape(pointer, new_type_shape(1, tag->detail.class_info->pointer_attributes, 0));
        attach_type_shape(pointer, new_type_shape(2, 0, 0));
        normalize_symbol_type(builtin_integer_type(), pointer);
    }
    prepend_class_member(pointer);
    bytes = type_size(pointer->type);
    base = kind == 0 ? 0 : nonvirtual_base_end(tag);
    padding = alignment_padding_at(bytes, base);
    alignment = class_contexts[current_class_context].alignment;
    span = ((bytes - 1 + padding + alignment) / alignment) * alignment;
    shift_class_member_offsets(tag, span, base);
    limit = (unsigned)top_packing(packing_stack)->alignment < bytes ?
        top_packing(packing_stack)->alignment : bytes;
    if (class_contexts[current_class_context].alignment <= (unsigned short)limit) {
        if ((unsigned)top_packing(packing_stack)->alignment < bytes)
            bytes = top_packing(packing_stack)->alignment;
        class_contexts[current_class_context].alignment = (unsigned short)bytes;
    }
    pointer->detail.value = base + padding;
    if (kind == 0) {
        tag->detail.class_info->field2c += span;
        tag->detail.class_info->alignment_flags |= 0x40;
    } else {
        tag->detail.class_info->field2c = base + padding;
        tag->detail.class_info->alignment_flags |= 0x80;
    }
    return pointer;
}

/* Original 0x79bf0..0x79ca6. Reserve a class prefix without moving static
   data or virtual bases. Ordinary fields, bit-fields and nonvirtual bases at
   or beyond the insertion point receive the same displacement. */
void __stdcall shift_class_member_offsets(CppSymbol *tag, unsigned bytes, unsigned offset)
{
    ClassMemberIterator iterator;
    CppSymbol *member;
    BaseClassLink *base;
    unsigned kind;
    begin_class_members(&iterator, tag);
    while (next_class_member(&iterator)) {
        member = iterator.member;
        kind = member->kind & 0x1f;
        if (kind == 3) {
            if (member->attributes & 0x40) continue;
            if (member->detail.value >= offset) member->detail.value += bytes;
        } else if (kind == 2) {
            if (member->detail.value >= offset) member->detail.value += bytes;
        } else if (kind == 15) {
            base = (BaseClassLink *)member;
            if (!(base->state & 1) && base->displacement >= offset)
                base->displacement += bytes;
        }
    }
    class_contexts[current_class_context].size += bytes;
}
