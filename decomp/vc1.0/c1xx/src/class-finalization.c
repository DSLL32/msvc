/* Completed class layout, synthesized members and member-name indexing. */
#include <stdlib.h>
#include "class-finalization.h"
#include "class-type-sets.h"
#include "class-members.h"
#include "member-access.h"
#include "global-ir.h"
#include "cpp-context.h"
#include "constructor-initializers.h"
#include <recon.h>

/* Original 0x7c010..0x7c024. The current-member output is left
   untouched until the first successful advance. */
void __stdcall begin_virtual_entries(VirtualEntryIterator *iterator, VirtualTable *table)
{
    iterator->table = table;
    iterator->index = 0;
}

/* Original 0x7c030..0x7c05a. Only a successful advance publishes
   a member; the original signed count comparison is retained. */
int __stdcall next_virtual_entry(VirtualEntryIterator *iterator)
{
    if (iterator->index < iterator->table->entry_count) {
        iterator->member = iterator->table->entries[iterator->index].dispatch.function;
        ++iterator->index;
        return 1;
    }
    return 0;
}

/* Original 0x7c0e0..0x7c0f3. Keep the original signed-short vector count. */
short __stdcall class_virtual_table_group_count(CppSymbol *tag)
{
    return vector_count(tag->detail.class_info->virtual_table_groups);
}

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
    tag->detail.class_info->virtual_base_pointer_offset = 0;
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
            resolve_table_override(member, table, tag, (char)(group && group->has_vtordisp));
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
                if (entry->dispatch.flags & 4) {
                    conflicts = (Vector *)entry->dispatch.function;
                    for (conflict_index = 0; conflict_index < conflicts->count; ++conflict_index) {
                        function = conflicts->items[conflict_index];
                        owner = cpp_context_stack->items[function->auxiliary.declaring_class];
                        report_syntax_error(250, tag->name, owner->name, function->name);
                    }
                    collapse_virtual_conflicts(entry);
                } else if (entry->dispatch.flags & 2) {
                    function = entry->dispatch.function;
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
                if (table->entries[index].dispatch.function->cpp.parts.member_flags & 4) return 1;
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
        tag->detail.class_info->virtual_base_pointer_offset += span;
        tag->detail.class_info->alignment_flags |= 0x40;
    } else {
        tag->detail.class_info->virtual_base_pointer_offset = base + padding;
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

/* Original 0x78950..0x78991. The separately zeroed table owns a vector of
   classes sharing it; its original owner remains distinct from that vector. */
VirtualTable *__stdcall new_virtual_table(unsigned kind, CppSymbol *tag,
    unsigned offset, CppSymbol *pointer)
{
    VirtualTable *table = allocate_zeroed(sizeof(VirtualTable));
    table->kind = kind;
    table->shared_owners = new_vector(1, 0);
    table->unshared_owner = tag;
    table->object_offset = offset;
    table->pointer_attributes = tag->detail.class_info->pointer_attributes;
    table->pointer = pointer;
    return table;
}

/* The third match argument is retained by the original RET 12 even though
   this compiler build does not inspect its class context. */
int __stdcall matches_virtual_override(CppSymbol *member, VirtualEntry *entry, CppSymbol *tag);
void __stdcall validate_virtual_override(CppSymbol *member, VirtualEntry *entry);
void __stdcall initialize_virtual_adjustment(VirtualAdjustment *adjustment, unsigned displacement);
unsigned __stdcall virtual_table_object_offset(VirtualTable *table);

/* Original 0x7a7b0..0x7a879. Replace every matching inherited entry, carrying
   the original slot and table-class identity into the first override found. */
void __stdcall resolve_table_override(CppSymbol *member, VirtualTable *table,
    CppSymbol *tag, char virtual_path)
{
    int index = 0;
    VirtualEntry *entry;
    while (index < table->entry_count) {
        entry = &table->entries[index++];
        if (!entry) break;
        if (!matches_virtual_override(member, entry, tag)) continue;
        validate_virtual_override(member, entry);
        if (entry->dispatch.flags & 4) collapse_virtual_conflicts(entry);
        if (!(member->cpp.parts.member_flags & 2)) {
            member->cpp.parts.member_flags |= 2;
            member->field28 = virtual_table_object_offset(table);
            member->detail.virtual_table_offset = entry->dispatch.function->detail.virtual_table_offset;
            member->auxiliary.virtual_function.table_class =
                entry->dispatch.function->auxiliary.virtual_function.table_class;
        }
        entry->dispatch.function = member;
        initialize_virtual_adjustment(&entry->dispatch.this_adjustment, member->field28);
        entry->dispatch.flags = (entry->dispatch.flags & ~15U) | (((unsigned)virtual_path << 3) & 8);
    }
}

void *__stdcall first_vector_item(Vector *vector);

/* Original 0x78b20..0x78b32. The generic first-element helper retains its
   own entry between the class query and vector storage. */
VirtualTable *__stdcall primary_virtual_table(CppSymbol *tag)
{
    return first_vector_item(tag->detail.class_info->virtual_tables);
}

/* Original 0x78c60..0x78c90. The original clears only the low flag
   nibble, leaving the high nibble and three following bytes unwritten before
   copying the entry into PCH output. Consumers use only bits0..3. Zero this
   original output defect while retaining the meaningful flag initialization. */
void __stdcall initialize_virtual_entry(VirtualEntry *entry, CppSymbol *member)
{
    entry->dispatch.function = member;
    initialize_virtual_adjustment(&entry->dispatch.this_adjustment, 0);
    entry->dispatch.flags = 0;
    entry->dispatch.reserved_flags[0] = 0;
    entry->dispatch.reserved_flags[1] = 0;
    entry->dispatch.reserved_flags[2] = 0;
}

/* Original 0x78810..0x78822. A zero class index denotes a direct this
   displacement. The original leaves the reserved halfword unwritten, then
   copies it into PCH output. Zero that unused output padding. */
void __stdcall initialize_virtual_adjustment(VirtualAdjustment *adjustment,
    unsigned displacement)
{
    adjustment->displacement = displacement;
    adjustment->virtual_base_class = 0;
    adjustment->reserved = 0;
}

void __stdcall grow_virtual_entries(VirtualTable *table, int extra);

/* Original 0x78b60..0x78baa. Grow before selecting the destination; the
   complete 16-byte entry includes its adjustment and reserved bytes. */
void __stdcall append_virtual_entry(VirtualTable *table, VirtualEntry *entry)
{
    VirtualEntry *destination;
    if (table->entry_count == table->entry_capacity)
        grow_virtual_entries(table,
            (table->entry_capacity >> 2) + table->entry_capacity + 1);
    destination = &table->entries[table->entry_count++];
    *destination = *entry;
}

unsigned __stdcall virtual_entry_capacity_bytes(VirtualTable *table);

/* Original 0x78bb0..0x78bfb. The requested amount is an increment, not a
   replacement capacity. Existing heap entries are resized in place when able. */
void __stdcall grow_virtual_entries(VirtualTable *table, int extra)
{
    table->entry_capacity += extra;
    if (!table->entries)
        table->entries = malloc(virtual_entry_capacity_bytes(table));
    else table->entries = realloc(table->entries, virtual_entry_capacity_bytes(table));
    if (!table->entries) fatal_error(60);
}

/* Original 0x78930..0x7893c. The shift retains the native 32-bit size wrap. */
unsigned __stdcall virtual_entry_capacity_bytes(VirtualTable *table)
{
    return (unsigned)table->entry_capacity << 4;
}

/* Original 0x78c10..0x78c22. Return the old byte offset while reserving
   this slot's width; the table's count and storage capacity are independent. */
unsigned __stdcall reserve_virtual_table_offset(VirtualTable *table, unsigned size)
{
    unsigned offset = table->byte_size;
    table->byte_size += size;
    return offset;
}

/* Original 0x7a880..0x7a952. Resolve conflict vectors to their first
   function before comparing interned names, parameter identity and this type.
   The unused class argument is nevertheless popped by the native RET 12. */
int __stdcall matches_virtual_override(CppSymbol *member, VirtualEntry *entry,
    CppSymbol *tag)
{
    CppSymbol *inherited = entry->dispatch.function;
    CType *type, *inherited_type, *this_type, *inherited_this;
    TypeShape *shape, *inherited_shape;
    FunctionParameters *parameters, *inherited_parameters;
    if (entry->dispatch.flags & 4) inherited = first_vector_item((Vector *)inherited);
    if (inherited->name != member->name) return 0;
    type = member->type;
    inherited_type = inherited->type;
    shape = type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    inherited_shape = inherited_type->related;
    if (inherited_shape && !(inherited_shape->flags & 0x77))
        inherited_shape = inherited_shape->related;
    parameters = shape->data.parameters;
    inherited_parameters = inherited_shape->data.parameters;
    if (parameters->declarations != inherited_parameters->declarations) return 0;
    this_type = parameters->associated_type;
    inherited_this = inherited_parameters->associated_type;
    if ((type->attributes ^ inherited_type->attributes) & 7) return 0;
    if ((this_type->related->attributes ^ inherited_this->related->attributes) & 7)
        return 0;
    if ((this_type->attributes ^ inherited_this->attributes) & 0x3000) return 0;
    return compatible_override_return(type, inherited_type) ? 1 : 0;
}

Vector *__stdcall copy_vector(Vector *vector, int arena);
unsigned __stdcall virtual_entry_used_bytes(VirtualTable *table);
void __stdcall release_virtual_table(VirtualTable *table);

/* Original 0x78a40..0x78aad. Persist the table and its used entries, then
   release the temporary heap table. The callback arena slot is unused here;
   this pass always selects permanent arena zero for the owner vector too. */
void *__stdcall clone_class_virtual_table(void *item, int arena)
{
    VirtualTable *table = item;
    VirtualTable *copy = allocate_permanent(sizeof(VirtualTable));
    unsigned bytes, i;
    *copy = *table;
    copy->shared_owners = copy_vector(table->shared_owners, 0);
    if (copy->entry_count > 0) {
        bytes = virtual_entry_used_bytes(table);
        copy->entries = allocate_permanent((bytes + 3) & ~3U);
        bytes = virtual_entry_used_bytes(table);
        for (i = 0; i < bytes; ++i)
            ((unsigned char *)copy->entries)[i] = ((unsigned char *)table->entries)[i];
    }
    release_virtual_table(table);
    return copy;
}

/* Original 0x78940..0x7894c. Used size is independent of reserved capacity. */
unsigned __stdcall virtual_entry_used_bytes(VirtualTable *table)
{
    return (unsigned)table->entry_count << 4;
}

/* Original 0x789a0..0x789c3. The count, rather than capacity or pointer
   value, controls release of entries. Owners are held by permanent vectors. */
void __stdcall release_virtual_table(VirtualTable *table)
{
    if (table->entry_count > 0) free(table->entries);
    free(table);
}

char *__stdcall intern_generated_symbol_name(CppSymbol *symbol, CppSymbol *owner);

/* Original 0x7ae80..0x7afe4. Give each persistent table its decorated
   declaration, using a temporary const-int type solely while naming it.
   The caller supplies kind zero (vftable) or one (vbtable). */
CppSymbol *__stdcall declare_virtual_table_symbol(int kind, VirtualTable *table,
    CppSymbol *tag)
{
    CppSymbol *symbol;
    CType *saved_type;
    DeclarationType declaration;
    int count = table->entry_count;
    symbol = new_declarator(table->kind == 0 || table->kind == 2 ?
        name_vftable : name_vbtable);
    symbol->type->attributes = normalize_type_attributes(2, table->pointer_attributes);
    symbol->flags |= 0x80;
    symbol->auxiliary.virtual_table = table;
    saved_type = symbol->type;
    symbol->type = type_0064_q1000;
    symbol->name = intern_generated_symbol_name(symbol, tag);
    symbol->type = saved_type;
    symbol->declaration_flags = (symbol->declaration_flags & ~0x100) | 0x200;
    if (kind == 0) {
        attach_type_shape(symbol, new_type_shape(2, table->pointer_attributes & ~0x400, &count));
        attach_type_shape(symbol, new_type_shape(1, 0, 0));
        add_function_shape(symbol, 0, 0);
        declaration.type = type_0080;
    } else if (kind == 1) {
        attach_type_shape(symbol, new_type_shape(2, table->pointer_attributes & ~0x400, &count));
        declaration.type = builtin_integer_type();
    }
    declaration.storage[0] = (declaration_storage[0] & ~7) | 0x41;
    declaration.storage[1] = declaration_storage[1];
    declaration.storage[2] = declaration_storage[2];
    if (tag->detail.class_info->flags2 & 4) {
        declaration.storage[2] |= 4;
        declaration.storage[2] = (declaration.storage[2] & ~8) |
            ((tag->detail.class_info->flags2 << 2) & 8);
    }
    if (!(declaration.storage[2] & 4) || (declaration.storage[2] & 8))
        symbol->flags |= 4;
    return apply_declaration_type(&declaration, symbol);
}

/* Original 0x7c000..0x7c009. The owner-path vector has a distinct query
   entry used by symbol decoration, separate from the unshared owner slot. */
Vector *__stdcall virtual_table_owners(VirtualTable *table)
{
    return table->shared_owners;
}

void __stdcall inherit_virtual_pointer_layout(CppSymbol *tag, CppSymbol *base,
    unsigned displacement, int unused);

/* Original 0x79510..0x79548. Virtual bases require a derived vbptr;
   direct nonvirtual bases can instead provide the inherited pointer layout. */
void __stdcall inherit_base_virtual_tables(CppSymbol *tag, BaseClassLink *base,
    int *virtual_base)
{
    CppSymbol *base_tag;
    if (base->state & 1) {
        *virtual_base = 1;
        return;
    }
    base_tag = cpp_context_stack->items[base->context_index];
    inherit_virtual_pointer_layout(tag, base_tag, base->displacement, 0);
}

/* Original 0x79b70..0x79beb. Base-list order determines the last direct
   subobject end. Retain that replacement, rather than taking a maximum. */
unsigned __stdcall nonvirtual_base_end(CppSymbol *tag)
{
    BaseClassLink *base = (BaseClassLink *)tag->auxiliary.scope_members, *next;
    CppSymbol *base_tag;
    unsigned end = 0;
    while (base) {
        next = (BaseClassLink *)base->next;
        if ((base->kind & 31) == 15 && !(base->flags & 0x80)) {
            base_tag = cpp_context_stack->items[base->context_index];
            if (!(base->state & 1))
                end = base_tag->detail.class_info->nonvirtual_size + base->displacement;
        }
        base = next;
    }
    return end;
}

void __stdcall initialize_virtual_base_entry(VirtualEntry *entry, CppSymbol *owner,
    CppSymbol *base);
unsigned virtual_base_entry_width(void);

/* Original 0x78dc0..0x78e0c. The initial vbtable entry describes its own
   pointer displacement; later virtual bases append further offset entries. */
VirtualTable *__stdcall build_virtual_base_table(CppSymbol *tag, CppSymbol *pointer)
{
    VirtualTable *table = new_virtual_table(1, tag,
        tag->detail.class_info->virtual_base_pointer_offset, pointer);
    VirtualEntry entry;
    initialize_virtual_base_entry(&entry, tag, 0);
    append_virtual_entry(table, &entry);
    reserve_virtual_table_offset(table, virtual_base_entry_width());
    return table;
}

/* Original 0x78d70..0x78d89. The original initializes only this
   two-word prefix, then copies the unwritten generic-entry words into PCH
   output. They have no vbtable consumer; zero that original output defect. */
void __stdcall initialize_virtual_base_entry(VirtualEntry *entry, CppSymbol *owner,
    CppSymbol *base)
{
    entry->base.base = base;
    entry->base.pointer_offset = owner->detail.class_info->virtual_base_pointer_offset;
    entry->base.reserved[0] = entry->base.reserved[1] = 0;
}

/* Original 0x78c50..0x78c5b. The final full-word mask zero-extends the
   integer width even though the native load itself targets AX. */
unsigned virtual_base_entry_width(void)
{
    return builtin_integer_type()->flags & 15;
}

void __stdcall merge_virtual_table_groups(Vector *destination, Vector *source,
    CppSymbol *tag, BaseClassLink *base);
VirtualTable *__stdcall copy_virtual_table(VirtualTable *table, int extra);
void __stdcall prepare_inherited_virtual_table(VirtualTable *table,
    BaseClassLink *base, VirtualTable *original);
void __stdcall shift_inherited_virtual_table(VirtualTable *table,
    BaseClassLink *base, unsigned displacement);
VirtualTableGroup *__stdcall build_virtual_base_group(BaseClassLink *base,
    CppSymbol *tag, int arena);
void __stdcall merge_virtual_base_group(Vector *groups, VirtualTableGroup *group,
    CppSymbol *tag, BaseClassLink *base);

/* Original 0x79e40..0x79f1a. Propagate each direct base's virtual-base
   groups first. Nonvirtual bases contribute independent copies of both table
   families; a virtual base contributes a new group at the derived scope. */
void __stdcall complete_direct_base_layout(BaseClassLink *base, CppSymbol *tag)
{
    CppSymbol *base_tag = cpp_context_stack->items[base->context_index];
    Vector *tables, *destination;
    VirtualTable *original, *table;
    VirtualTableGroup *group;
    int family;
    short index;

    merge_virtual_table_groups(tag->detail.class_info->virtual_table_groups,
        base_tag->detail.class_info->virtual_table_groups, tag, base);
    if (!(base->state & 1)) {
        for (family = 0; family < 2; ++family) {
            tables = family ? base_tag->detail.class_info->virtual_base_tables :
                base_tag->detail.class_info->virtual_tables;
            index = 0;
            while (index < tables->count) {
                original = tables->items[index++];
                table = copy_virtual_table(original, 0);
                prepare_inherited_virtual_table(table, base, original);
                shift_inherited_virtual_table(table, base, base->displacement);
                destination = family ? tag->detail.class_info->virtual_base_tables :
                    tag->detail.class_info->virtual_tables;
                append_vector(destination, table);
            }
        }
    } else {
        group = build_virtual_base_group(base, tag, 2);
        merge_virtual_base_group(tag->detail.class_info->virtual_table_groups,
            group, tag, base);
    }
}

VirtualTableGroup *__stdcall copy_inherited_virtual_group(VirtualTableGroup *group,
    BaseClassLink *base, int arena);

/* Original 0x7a030..0x7a089. Preserve source order while making an
   inherited group for every virtual base already represented by this base. */
void __stdcall merge_virtual_table_groups(Vector *destination, Vector *source,
    CppSymbol *tag, BaseClassLink *base)
{
    short index = 0;
    VirtualTableGroup *group;
    while (index < source->count) {
        group = copy_inherited_virtual_group(source->items[index++], base, 2);
        merge_virtual_base_group(destination, group, tag, base);
    }
}

void __stdcall prepare_virtual_group_tables(Vector *destination, Vector *source,
    BaseClassLink *base);
void *__stdcall copy_virtual_table_element(void *item, int arena);

/* Original 0x78e30..0x78eee. The tag argument occupies its original stack
   slot but is unused. Only the tag, low flags byte and two table vectors are
   initialized here; the remaining group fields are filled by later passes. */
VirtualTableGroup *__stdcall build_virtual_base_group(BaseClassLink *base,
    CppSymbol *tag, int arena)
{
    VirtualTableGroup *group;
    ArenaPool *pool;
    CppSymbol *base_tag;
    Vector *source, *destination;
    int index = arena, family;
    if (!arena) group = allocate_permanent(sizeof(VirtualTableGroup));
    else {
        if (index == -1) index = 0;
        pool = &compiler_arena.pools[index];
        if (pool->remaining < (int)sizeof(VirtualTableGroup))
            refill_arena(&compiler_arena, pool);
        group = (VirtualTableGroup *)pool->next;
        pool->remaining -= sizeof(VirtualTableGroup);
        pool->next += sizeof(VirtualTableGroup);
    }
    group->tag = cpp_context_stack->items[base->context_index];
    group->flags = 3;
    for (family = 0; family < 2; ++family) {
        base_tag = cpp_context_stack->items[base->context_index];
        source = family ? base_tag->detail.class_info->virtual_base_tables :
            base_tag->detail.class_info->virtual_tables;
        destination = copy_vector_elements(source, arena, copy_virtual_table_element);
        if (family) group->base_tables = destination;
        else group->tables = destination;
        base_tag = cpp_context_stack->items[base->context_index];
        source = family ? base_tag->detail.class_info->virtual_base_tables :
            base_tag->detail.class_info->virtual_tables;
        prepare_virtual_group_tables(destination, source, base);
    }
    return group;
}

/* Original 0x78ab0..0x78b1b. Virtual-base group copies distinguish
   inherited dispatch/base tables by kinds 2/3. The callback arena is unused. */
void *__stdcall copy_virtual_table_element(void *item, int arena)
{
    VirtualTable *table = copy_virtual_table(item, 0);
    int index;
    if (table->kind == 0) {
        table->kind = 2;
        for (index = 0; index < table->entry_count; ++index)
            table->entries[index].dispatch.flags |= 1;
    } else if (table->kind == 1)
        table->kind = 3;
    return table;
}

Vector *__stdcall copy_vector_with_spare(Vector *vector, int arena);

/* Original 0x789d0..0x78a31. Clone the complete table into zeroed heap
   storage and leave one spare owner slot. Entries reserve full capacity but
   only used entries are copied. The second stack argument is unused. */
VirtualTable *__stdcall copy_virtual_table(VirtualTable *table, int extra)
{
    VirtualTable *copy = allocate_zeroed(sizeof(VirtualTable));
    unsigned bytes, index;
    *copy = *table;
    copy->shared_owners = copy_vector_with_spare(table->shared_owners, 0);
    if (copy->entry_count > 0) {
        copy->entries = allocate_zeroed(virtual_entry_capacity_bytes(table));
        bytes = virtual_entry_used_bytes(table);
        for (index = 0; index < bytes; ++index)
            ((unsigned char *)copy->entries)[index] = ((unsigned char *)table->entries)[index];
    }
    return copy;
}

/* Original 0x78ef0..0x78f66. Walk the copied and source vectors in
   lockstep, stopping at the end of either signed-short iterator. */
void __stdcall prepare_virtual_group_tables(Vector *destination, Vector *source,
    BaseClassLink *base)
{
    short destination_index = 0, source_index = 0;
    VirtualTable *table, *original;
    while (destination_index < destination->count) {
        table = destination->items[destination_index++];
        if (source_index >= source->count) return;
        original = source->items[source_index++];
        prepare_inherited_virtual_table(table, base, original);
    }
}

/* Original 0x79f20..0x79f45. Retain the direct path and source table
   separately: later override/offset passes use both identities. */
void __stdcall prepare_inherited_virtual_table(VirtualTable *table,
    BaseClassLink *base, VirtualTable *original)
{
    table->unshared_owner = cpp_context_stack->items[base->context_index];
    table->inheritance_path = base;
    table->source_table = original;
}

VirtualTableGroup *__stdcall find_virtual_base_group(CppSymbol *tag, Vector *groups);
void __stdcall shift_virtual_base_group(VirtualTableGroup *group,
    BaseClassLink *base, unsigned unused_offset);
void __stdcall merge_virtual_group_entries(CppSymbol *tag,
    VirtualTableGroup *destination, VirtualTableGroup *source);
void __stdcall release_virtual_group(VirtualTableGroup *group);
int __stdcall needs_virtual_base_vtordisp(CppSymbol *tag, CppSymbol *base_tag);
int __stdcall track_empty_base_layout(CppSymbol *base_tag, CppSymbol *tag);
unsigned __stdcall layout_anonymous_data(CType *type);

/* Original 0x7a090..0x7a122. A virtual base shared by two paths retains
   one group: adjust and merge the duplicate's entries, then release its
   temporary tables. A first occurrence allocates its object/vtordisp space. */
void __stdcall merge_virtual_base_group(Vector *groups, VirtualTableGroup *group,
    CppSymbol *tag, BaseClassLink *base)
{
    VirtualTableGroup *existing = find_virtual_base_group(group->tag, groups);
    unsigned offset;
    if (existing) {
        shift_virtual_base_group(group, base, existing->object_offset);
        merge_virtual_group_entries(tag, existing, group);
        release_virtual_group(group);
        return;
    }
    group->has_vtordisp = needs_virtual_base_vtordisp(tag, group->tag);
    if (track_empty_base_layout(group->tag, tag) || group->has_vtordisp)
        layout_anonymous_data(builtin_integer_type());
    offset = layout_data_member(group->tag);
    group->object_offset = offset;
    shift_virtual_base_group(group, base, offset);
    append_vector(groups, group);
}

/* Original 0x7a220..0x7a267. Virtual bases are shared by canonical tag
   identity. Return the first matching group in signed-short vector order. */
VirtualTableGroup *__stdcall find_virtual_base_group(CppSymbol *tag, Vector *groups)
{
    short index = 0;
    VirtualTableGroup *group;
    while (index < groups->count) {
        group = groups->items[index++];
        if (group->tag == tag) return group;
    }
    return 0;
}

int __stdcall inherits_virtual_base_vtordisp(CppSymbol *tag, CppSymbol *base_tag);
int __stdcall has_non_destructor_virtual_functions(CppSymbol *tag);
int __stdcall has_implicit_lifetime_members(CppSymbol *tag);
int __stdcall class_allows_vtordisp(CppSymbol *tag);
int __stdcall has_construction_virtual_override(CppSymbol *tag, CppSymbol *base_tag);

/* Original 0x7aff0..0x7b05e. An inherited slot remains required even
   with vtordisp generation suppressed. New slots depend on virtual overrides
   and the derived class's construction/destruction requirements. */
int __stdcall needs_virtual_base_vtordisp(CppSymbol *tag, CppSymbol *base_tag)
{
    if (inherits_virtual_base_vtordisp(tag, base_tag)) return 1;
    if (suppress_vtordisp) return 0;
    if (!has_non_destructor_virtual_functions(base_tag)) return 0;
    if (has_implicit_lifetime_members(tag)) return 0;
    if (!class_allows_vtordisp(tag)) return 0;
    return has_construction_virtual_override(tag, base_tag);
}

/* Original 0x7b160..0x7b1e6. Any visible direct base may already carry
   this virtual base's displacement slot, regardless of the current pragma. */
int __stdcall inherits_virtual_base_vtordisp(CppSymbol *tag, CppSymbol *base_tag)
{
    BaseClassLink *base = (BaseClassLink *)tag->auxiliary.scope_members, *next;
    CppSymbol *direct_tag;
    VirtualTableGroup *group;
    while (base) {
        next = (BaseClassLink *)base->next;
        if ((base->kind & 31) == 15 && !(base->flags & 0x80)) {
            direct_tag = cpp_context_stack->items[base->context_index];
            group = find_virtual_base_group(base_tag,
                direct_tag->detail.class_info->virtual_table_groups);
            if (group && group->has_vtordisp) return 1;
        }
        base = next;
    }
    return 0;
}

/* Original 0x7b060..0x7b151. Inspect ordinary vftables, then each
   virtual-base group's dispatch tables. The 0x2000 declaration flag denotes
   a destructor, which alone does not require a construction displacement. */
int __stdcall has_non_destructor_virtual_functions(CppSymbol *tag)
{
    Vector *tables = tag->detail.class_info->virtual_tables;
    Vector *groups = tag->detail.class_info->virtual_table_groups;
    VirtualTableGroup *group;
    VirtualTable *table;
    short table_index = 0, group_index = 0;
    int entry_index;
    for (;;) {
        while (table_index < tables->count) {
            table = tables->items[table_index++];
            for (entry_index = 0; entry_index < table->entry_count; ++entry_index)
                if (!(table->entries[entry_index].dispatch.function->declaration_flags & 0x2000))
                    return 1;
        }
        if (group_index >= groups->count) return 0;
        group = groups->items[group_index++];
        tables = group->tables;
        table_index = 0;
    }
}

/* Original 0x7b1f0..0x7b21c. Construction and destruction must each
   be trivial or supplied by the implicit-member requirements. */
int __stdcall has_implicit_lifetime_members(CppSymbol *tag)
{
    ClassInfo *info = tag->detail.class_info;
    if ((info->trivial_special_members & 1) ||
        (info->implicit_member_requirements & 1)) {
        if ((info->trivial_special_members & 8) ||
            (info->implicit_member_requirements & 8)) return 1;
    }
    return 0;
}

/* Original 0x7b220..0x7b237. Layout attribute0x100 excludes the tag
   from new vtordisp storage. Return a complete 32-bit Boolean value. */
int __stdcall class_allows_vtordisp(CppSymbol *tag)
{
    return (tag->attributes & 0x100) ? 0 : 1;
}

/* Original 0x7b240..0x7b3e5. Search the derived overload sets for a
   non-destructor method matching one of the virtual base's dispatch entries.
   The original inlines the parameter/this comparisons and calls only the
   separate return-type compatibility predicate. */
int __stdcall has_construction_virtual_override(CppSymbol *tag, CppSymbol *base_tag)
{
    CppSymbol *member, *next_member, *function, *next_function, *inherited;
    Vector *tables;
    VirtualTable *table;
    TypeShape *shape, *inherited_shape;
    FunctionParameters *parameters, *inherited_parameters;
    CType *type, *inherited_type, *this_type, *inherited_this;
    short table_index;
    int entry_index;
    for (member = tag->auxiliary.scope_members; member; member = next_member) {
        next_member = member->next;
        if ((member->kind & 31) != 17 || (member->flags & 0x80)) continue;
        for (function = ((OverloadSet *)member)->functions; function;
                function = next_function) {
            next_function = function->next;
            if ((function->declaration_flags & 0x2000) ||
                (function->cpp.parts.member_flags & 4)) continue;
            tables = base_tag->detail.class_info->virtual_tables;
            table_index = 0;
            while (table_index < tables->count) {
                table = tables->items[table_index++];
                for (entry_index = 0; entry_index < table->entry_count; ++entry_index) {
                    inherited = table->entries[entry_index].dispatch.function;
                    if (function->name != inherited->name) continue;
                    type = function->type;
                    inherited_type = inherited->type;
                    shape = type->related;
                    if (shape && !(shape->flags & 0x77)) shape = shape->related;
                    inherited_shape = inherited_type->related;
                    if (inherited_shape && !(inherited_shape->flags & 0x77))
                        inherited_shape = inherited_shape->related;
                    parameters = shape->data.parameters;
                    inherited_parameters = inherited_shape->data.parameters;
                    if (parameters->declarations != inherited_parameters->declarations) continue;
                    this_type = parameters->associated_type;
                    inherited_this = inherited_parameters->associated_type;
                    if ((inherited_type->attributes ^ type->attributes) & 7) continue;
                    if ((inherited_this->related->attributes ^ this_type->related->attributes) & 7)
                        continue;
                    if ((inherited_this->attributes ^ this_type->attributes) & 0x3000) continue;
                    if (compatible_override_return(type, inherited_type)) return 1;
                }
            }
        }
    }
    return 0;
}

/* Original 0x7a140..0x7a19b. Adjacent empty-base properties determine
   whether another storage unit is needed. Always retain the current base
   and copy only its trailing-empty flag into the derived class. */
int __stdcall track_empty_base_layout(CppSymbol *base_tag, CppSymbol *tag)
{
    int separate = 0;
    unsigned char flags;
    if (previous_base_class && (previous_base_class->detail.class_info->flags & 8) &&
        (base_tag->detail.class_info->flags & 4)) separate = 1;
    previous_base_class = base_tag;
    flags = tag->detail.class_info->flags;
    tag->detail.class_info->flags = flags ^
        ((base_tag->detail.class_info->flags ^ flags) & 8);
    return separate;
}

/* Original 0x7a1a0..0x7a217. Adjust both table families by the group's
   stored offset, then combine path access. The third stack slot is present
   in callers and RET12 but is never read by this original body. */
void __stdcall shift_virtual_base_group(VirtualTableGroup *group,
    BaseClassLink *base, unsigned unused_offset)
{
    Vector *tables;
    short index;
    int family;
    for (family = 0; family < 2; ++family) {
        tables = family ? group->base_tables : group->tables;
        index = 0;
        while (index < tables->count)
            shift_inherited_virtual_table(tables->items[index++], base, group->object_offset);
    }
    group->flags = combine_member_access(group->flags, (unsigned char)(base->flags & 3));
}

void __stdcall adjust_virtual_this(VirtualAdjustment *adjustment, BaseClassLink *base);

/* Original 0x79f50..0x7a02e. Dispatch entries adjust their this path;
   direct vbtable entries move only when they name a base. Grouped kinds2/3
   retain the group's displacement separately from the original object offset. */
void __stdcall shift_inherited_virtual_table(VirtualTable *table,
    BaseClassLink *base, unsigned displacement)
{
    int index;
    switch (table->kind) {
    case 0: case 2:
        for (index = 0; index < table->entry_count; ++index)
            adjust_virtual_this(&table->entries[index].dispatch.this_adjustment, base);
        if (table->kind == 0) table->object_offset += displacement;
        else table->virtual_base_offset = displacement;
        return;
    case 1:
        table->object_offset += displacement;
        for (index = 0; index < table->entry_count; ++index)
            if (table->entries[index].base.base)
                table->entries[index].base.pointer_offset += displacement;
        return;
    case 3:
        table->virtual_base_offset = displacement;
        return;
    }
}

void __stdcall add_direct_this_displacement(VirtualAdjustment *adjustment,
    unsigned displacement);
void __stdcall set_virtual_this_base(VirtualAdjustment *adjustment, CppSymbol *tag);

/* Original 0x78870..0x788a6. Select the virtual identity or fixed
   displacement helper from the direct inheritance path's virtual flag. */
void __stdcall adjust_virtual_this(VirtualAdjustment *adjustment, BaseClassLink *base)
{
    if (base->state & 1)
        set_virtual_this_base(adjustment, cpp_context_stack->items[base->context_index]);
    else
        add_direct_this_displacement(adjustment, base->displacement);
}

/* Original 0x78850..0x7886c. The first virtual base on the path wins;
   an existing virtual identity leaves the entire adjustment untouched. */
void __stdcall set_virtual_this_base(VirtualAdjustment *adjustment, CppSymbol *tag)
{
    if (!adjustment->virtual_base_class)
        adjustment->virtual_base_class = tag->detail.class_info->type_index;
}

VirtualTable *__stdcall primary_virtual_base_table(CppSymbol *tag);
unsigned __stdcall virtual_base_entry_byte_offset(VirtualEntry *entry, VirtualTable *table);

/* Original 0x7a9d0..0x7aa4c. Reuse the first primary-vbtable entry
   naming this canonical base. Record its pointer location, byte index and
   pointer symbol together; absence leaves all three group fields untouched. */
int __stdcall find_virtual_base_group_offsets(VirtualTableGroup *group, CppSymbol *tag)
{
    VirtualTable *table = primary_virtual_base_table(tag);
    VirtualEntry *entry;
    int index;
    for (index = 0; index < table->entry_count; ++index) {
        entry = &table->entries[index];
        if (entry->base.base != group->tag) continue;
        group->virtual_pointer_offset = virtual_table_object_offset(table);
        group->virtual_base_table_offset = virtual_base_entry_byte_offset(entry, table);
        group->virtual_base_pointer = table->pointer;
        return 1;
    }
    return 0;
}

/* Original 0x78b40..0x78b52. Keep this selector distinct from the
   vftable selector: both delegate to the same unchecked first-item getter. */
VirtualTable *__stdcall primary_virtual_base_table(CppSymbol *tag)
{
    return first_vector_item(tag->detail.class_info->virtual_base_tables);
}

/* Original 0x7aa50..0x7aaab. Missing virtual bases append one generic
   entry to the primary vbtable and reserve one integer-sized byte offset.
   Its unused entry suffix remains untouched, as in the native initializer. */
void __stdcall create_virtual_base_group_offsets(VirtualTableGroup *group, CppSymbol *tag)
{
    VirtualTable *table = first_vector_item(tag->detail.class_info->virtual_base_tables);
    VirtualEntry entry;
    initialize_virtual_base_entry(&entry, tag, group->tag);
    append_virtual_entry(table, &entry);
    group->virtual_pointer_offset = tag->detail.class_info->virtual_base_pointer_offset;
    group->virtual_base_table_offset = reserve_virtual_table_offset(table, virtual_base_entry_width());
    group->virtual_base_pointer = table->pointer;
}

/* Original 0x7a960..0x7a962 is exactly RET8 in this build. Keep its
   separate call and both stack slots despite the deliberately empty body. */
void __stdcall validate_virtual_override(CppSymbol *member, VirtualEntry *entry)
{
}

/* Original 0x78c00..0x78c0c. Grouped tables add their virtual-base
   displacement to the pointer's offset within its original object. */
unsigned __stdcall virtual_table_object_offset(VirtualTable *table)
{
    return table->virtual_base_offset + table->object_offset;
}

/* Original 0x79060..0x790f7. Preserve the complete group, then copy
   both vectors through the original table-persistence callback. Unlike that
   callback, the group itself uses the supplied arena (including -1). */
void *__stdcall clone_class_virtual_group(void *item, int arena)
{
    VirtualTableGroup *group = item, *copy;
    ArenaPool *pool;
    int index = arena, family;
    Vector *tables;
    if (!arena) copy = allocate_permanent(sizeof(VirtualTableGroup));
    else {
        if (index == -1) index = 0;
        pool = &compiler_arena.pools[index];
        if (pool->remaining < (int)sizeof(VirtualTableGroup))
            refill_arena(&compiler_arena, pool);
        copy = (VirtualTableGroup *)pool->next;
        pool->remaining -= sizeof(VirtualTableGroup);
        pool->next += sizeof(VirtualTableGroup);
    }
    *copy = *group;
    for (family = 0; family < 2; ++family) {
        tables = family ? group->base_tables : group->tables;
        tables = copy_vector_elements(tables, arena, clone_class_virtual_table);
        if (family) copy->base_tables = tables;
        else copy->tables = tables;
    }
    return copy;
}

/* Original 0x79550..0x795e2. The first base supplying each virtual
   pointer family owns that slot. A vbptr needs its base displacement added;
   a vfptr carries the original supplying declaration. Argument4 is unused. */
void __stdcall inherit_virtual_pointer_layout(CppSymbol *tag, CppSymbol *base,
    unsigned displacement, int unused)
{
    int family;
    unsigned char mask;
    for (family = 0; family < 2; ++family) {
        mask = (unsigned char)(0x40U << family);
        if ((tag->detail.class_info->alignment_flags & mask) ||
            !(base->detail.class_info->alignment_flags & mask)) continue;
        if (family == 1) {
            tag->detail.class_info->virtual_base_pointer_offset =
                base->detail.class_info->virtual_base_pointer_offset + displacement;
            tag->detail.class_info->alignment_flags |= 0x80;
        } else tag->detail.class_info->alignment_flags |= 0x40;
        if (family == 0)
            tag->detail.class_info->virtual_dispatch_pointer =
                base->detail.class_info->virtual_dispatch_pointer;
    }
}

VirtualTableGroup *__stdcall copy_virtual_group(VirtualTableGroup *group, int arena);

/* Original 0x79100..0x7914d. Copy the group, then retain corresponding
   source-table/direct-path identities in both copied table families. */
VirtualTableGroup *__stdcall copy_inherited_virtual_group(VirtualTableGroup *group,
    BaseClassLink *base, int arena)
{
    VirtualTableGroup *copy = copy_virtual_group(group, arena);
    Vector *destination, *source;
    int family;
    for (family = 0; family < 2; ++family) {
        destination = family ? copy->base_tables : copy->tables;
        source = family ? group->base_tables : group->tables;
        prepare_virtual_group_tables(destination, source, base);
    }
    return copy;
}

Vector *__stdcall copy_table_vector(Vector *tables, int arena);

/* Original 0x78f70..0x78ffd. A working copy preserves every group
   field and copies its two vectors through the table-copy wrapper. It uses
   the requested arena and leaves table-kind conversion to the earlier pass. */
VirtualTableGroup *__stdcall copy_virtual_group(VirtualTableGroup *group, int arena)
{
    VirtualTableGroup *copy;
    ArenaPool *pool;
    Vector *tables;
    int index = arena, family;
    if (!arena) copy = allocate_permanent(sizeof(VirtualTableGroup));
    else {
        if (index == -1) index = 0;
        pool = &compiler_arena.pools[index];
        if (pool->remaining < (int)sizeof(VirtualTableGroup))
            refill_arena(&compiler_arena, pool);
        copy = (VirtualTableGroup *)pool->next;
        pool->remaining -= sizeof(VirtualTableGroup);
        pool->next += sizeof(VirtualTableGroup);
    }
    *copy = *group;
    for (family = 0; family < 2; ++family) {
        tables = family ? group->base_tables : group->tables;
        tables = copy_table_vector(tables, arena);
        if (family) copy->base_tables = tables;
        else copy->tables = tables;
    }
    return copy;
}

/* Original 0x78e10..0x78e26. The table copier has the same two-slot pointer/
   integer stdcall ABI as the generic callback; only its pointer types differ. */
Vector *__stdcall copy_table_vector(Vector *tables, int arena)
{
    return copy_vector_elements(tables, arena, (VectorCopyElement)copy_virtual_table);
}

/* Original 0x78830..0x78843. Once a virtual identity is recorded, direct
   path offsets no longer contribute to the stored fixed this displacement. */
void __stdcall add_direct_this_displacement(VirtualAdjustment *adjustment,
    unsigned displacement)
{
    if (!adjustment->virtual_base_class) adjustment->displacement += displacement;
}

void __stdcall merge_virtual_entry(VirtualEntry *destination, VirtualEntry *source);

/* Original 0x7a270..0x7a367. Corresponding virtual-base dispatch tables
   merge entry by entry, then retain the least restrictive access. Argument1
   is unused. The destination iterator deliberately retains its selected
   table on exhaustion; matching group layouts provide corresponding tables. */
void __stdcall merge_virtual_group_entries(CppSymbol *tag,
    VirtualTableGroup *destination, VirtualTableGroup *source)
{
    Vector *destination_tables = destination->tables, *source_tables = source->tables;
    short destination_index = 0, source_index = 0;
    /* The original selected-table slot is unwritten until the first
       successful destination iteration, and is not cleared at its end. */
    VirtualTable *destination_table, *source_table;
    VirtualEntry *destination_entry, *source_entry;
    int destination_entry_index, source_entry_index;
    for (;;) {
        if (destination_index < destination_tables->count)
            destination_table = destination_tables->items[destination_index++];
        if (source_index >= source_tables->count) break;
        source_table = source_tables->items[source_index++];
        destination_entry_index = source_entry_index = 0;
        for (;;) {
            if (destination_entry_index < destination_table->entry_count)
                destination_entry = &destination_table->entries[destination_entry_index++];
            else destination_entry = 0;
            if (source_entry_index < source_table->entry_count)
                source_entry = &source_table->entries[source_entry_index++];
            else source_entry = 0;
            if (!source_entry) break;
            merge_virtual_entry(destination_entry, source_entry);
        }
    }
    if (destination->flags < source->flags) destination->flags = source->flags;
}

int __stdcall class_derives_from(CppSymbol *derived, CppSymbol *base);
int __stdcall vector_contains(Vector *vector, void *item);

/* Original 0x7a370..0x7a45a. Related declaring classes select the more
   derived override. Unrelated alternatives become an arena-backed conflict
   vector; inherited-only entries are not added to an existing conflict set. */
void __stdcall merge_virtual_entry(VirtualEntry *destination, VirtualEntry *source)
{
    CppSymbol *destination_owner, *source_owner;
    Vector *conflicts;
    if (!(destination->dispatch.flags & 4)) {
        if (source->dispatch.function == destination->dispatch.function) return;
        destination_owner = cpp_context_stack->items[
            destination->dispatch.function->auxiliary.virtual_function.declaring_class];
        source_owner = cpp_context_stack->items[
            source->dispatch.function->auxiliary.virtual_function.declaring_class];
        if (class_derives_from(destination_owner, source_owner)) {
            destination->dispatch.flags |= 2;
            destination->dispatch.flags &= ~1U;
            return;
        }
        if (class_derives_from(source_owner, destination_owner)) {
            destination->dispatch.function = source->dispatch.function;
            destination->dispatch.this_adjustment = source->dispatch.this_adjustment;
            destination->dispatch.flags |= 2;
            destination->dispatch.flags &= ~1U;
            return;
        }
        conflicts = new_vector(2, 2);
        append_vector(conflicts, destination->dispatch.function);
        append_vector(conflicts, source->dispatch.function);
        destination->dispatch.function = (CppSymbol *)conflicts;
        destination->dispatch.flags |= 4;
    } else if (!(source->dispatch.flags & 1)) {
        conflicts = (Vector *)destination->dispatch.function;
        if (!vector_contains(conflicts, source->dispatch.function))
            append_vector(conflicts, source->dispatch.function);
    }
}

/* Original 0x79000..0x79052. The vectors and group belong to arenas;
   release only the heap tables stored in each of their two families. */
void __stdcall release_virtual_group(VirtualTableGroup *group)
{
    Vector *tables;
    short index;
    int family;
    for (family = 0; family < 2; ++family) {
        tables = family ? group->base_tables : group->tables;
        index = 0;
        while (index < tables->count)
            release_virtual_table(tables->items[index++]);
    }
}

/* Original 0x78c30..0x78c4a. Generic entries occupy 16 bytes internally;
   the emitted vbtable uses integer-sized slots, queried by its own getter. */
unsigned __stdcall virtual_base_entry_byte_offset(VirtualEntry *entry, VirtualTable *table)
{
    int index = entry - table->entries;
    return virtual_base_entry_width() * index;
}

/* Original 0x78d50..0x78d66. Collapse an arena-owned conflict vector
   to its first function while preserving every flag except the vector tag. */
void __stdcall collapse_virtual_conflicts(VirtualEntry *entry)
{
    entry->dispatch.function = first_vector_item((Vector *)entry->dispatch.function);
    entry->dispatch.flags &= ~4U;
}

/* Original0x7c100..0x7c11a, stdcall4; AX is the defined result.
   Follow the real primary table's pointer declaration and its outer shape. */
unsigned short __stdcall primary_vtable_pointer_attributes(CppSymbol *tag)
{
    VirtualTable *table = primary_virtual_table(tag);
    return table->pointer->type->related->attributes;
}
