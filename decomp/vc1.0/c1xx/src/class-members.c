/* Class member declarations and their storage, access and layout dispatch. */
#include "class-members.h"
#include "parameters.h"
#include "inline-capture.h"
#include "member-access.h"
#include "symbol-bindings.h"
#include <recon.h>

/* Original 0x4e130..0x4e181. Qualified member-pointer names must
   resolve to a class tag, following a base-list link when necessary. */
void __stdcall resolve_member_pointer_class(CppSymbol **result, QualifiedDeclarator *name)
{
    CppSymbol *symbol = (CppSymbol *)name->resolution->binding->target;
    if ((symbol->kind & 31) == 15)
        symbol = cpp_context_stack->items[((BaseClassLink *)symbol)->context_index];
    if ((symbol->kind & 31) == 4) *result = symbol;
    else {
        report_syntax_error(653, symbol->name);
        *result = 0;
    }
}

/* Original0x30bb0..0x30c91. Count overloads that survive omission of
   the class's trivial special members. Preserve the original NULL cursor. */
unsigned __stdcall count_emitted_overloads(OverloadSet *overloads)
{
    CppSymbol *function, *next, *owner;
    unsigned count = 0;
#ifdef RECON_NATIVE_TEST
    /* NULL bypasses the original entryESP-8 cursor initialization. */
    if (!overloads) { __asm { int 3 } }
#endif
    if (overloads) {
        function = overloads->functions;
        owner = cpp_context_stack->items[function->auxiliary.declaring_class];
        if ((overloads->name == name_ctor &&
             (owner->detail.class_info->trivial_special_members & 3) == 3) ||
            (overloads->name == name_assignment &&
             (owner->detail.class_info->trivial_special_members & 4)) ||
            (overloads->name == name_dtor &&
             (owner->detail.class_info->trivial_special_members & 8))) next = 0;
        else next = function;
    }
    while (next) {
        function = next;
        next = function->next;
        if (function->name == name_ctor) {
            if (function->cpp.parts.member_flags & 0x20) {
                owner = cpp_context_stack->items[function->auxiliary.declaring_class];
                if (owner->detail.class_info->trivial_special_members & 2) continue;
            }
            if (function->cpp.parts.member_flags & 0x10) {
                owner = cpp_context_stack->items[function->auxiliary.declaring_class];
                if (owner->detail.class_info->trivial_special_members & 1) continue;
            }
        }
        ++count;
    }
    return count;
}

void __stdcall check_missing_defaults(DefaultArgument *defaults, unsigned count,
    FunctionParameters *parameters, unsigned first_parameter, CppSymbol *symbol);

#define CLASS_CONTEXT (class_contexts[current_class_context])
#define CLASS_TAG (CLASS_CONTEXT.symbol)

/* Original 0x31f20..0x31f33. The key points at the owner-index word, which
   is also the first word of a stored friendship record. */
FriendRecord *__stdcall find_friend_record(unsigned *owner_index)
{
    return (FriendRecord *)find_hash_value(declaration_table, owner_index);
}

/* Original 0x31f40..0x31f77. Preserve insertion order within one owner's
   list, keeping the first record as the hash-table value. */
void __stdcall append_friend_record(FriendRecord *record)
{
    FriendRecord *last = find_friend_record(&record->owner_index);
    if (!last) {
        set_hash_value(declaration_table, record, record);
        return;
    }
    while (last->next) last = last->next;
    last->next = record;
}

/* Original 0x31f80..0x31fea. Friendship records share their owner's symbol
   index as the hash key and retain either a class or a function symbol. */
void __stdcall record_friend(CppSymbol *owner, DeclarationType *type, CppSymbol *function)
{
    FriendRecord *record;
    owner->detail.class_info->flags |= 0x40;
    record = allocate_permanent(sizeof(FriendRecord));
    record->owner_index = owner->index;
    record->next = 0;
    if (!function) {
        record->symbol = cpp_context_stack->items[type->type->field08];
        append_friend_record(record);
        return;
    }
    record->symbol = function;
    capture_inline_member_declaration(function, -1, -1, 0);
    append_friend_record(record);
}

/* Original 0x2fc10..0x2fd8b. The by-value storage argument is present in
   the ABI but unused here; qualified declarations take storage from context. */
void __stdcall declare_friend_member(DeclarationType *context, CppSymbol *member,
    QualifiedDeclarator *qualified, CppSymbol *owner, StorageSpec storage, int pure)
{
    QualifiedDeclarator *resolved;
    short declaring_class;
    int diagnostic;

    if (!context) return;
    if (pure) report_syntax_error(254, member->name);
    if (qualified) {
        member->storage[0] = context->storage[0];
        member->storage[1] = context->storage[1];
        member->storage[2] = context->storage[2];
        normalize_symbol_type(context->type, member);
        if ((member->kind & 31) != 7) {
            report_syntax_error(63, format_symbol_name(
                (CppSymbol *)qualified->resolution->binding->target));
            unwind_qualified_scopes(1);
            return;
        }
        resolved = (QualifiedDeclarator *)resolve_declaration_alias(
            (CppSymbol *)qualified, member);
        member = resolved ? (CppSymbol *)resolved->resolution->binding->target : 0;
        if (!member || (member->kind & 31) != 7) {
            report_syntax_error(245, format_symbol_name(
                (CppSymbol *)qualified->resolution->binding->target));
            unwind_qualified_scopes(1);
            return;
        }
    } else {
        member->kind = 1;
        member = apply_declaration_type(context, member);
        if (!member || (member->declaration_flags & 2)) return;
    }
    if (member->declaration_flags & 0x3000) {
        declaring_class = member->auxiliary.declaring_class;
        if (!declaring_class || cpp_context_stack->items[declaring_class] == owner) {
            if (!declaring_class)
                member->auxiliary.declaring_class = owner->detail.class_info->type_index;
            diagnostic = (member->declaration_flags & 0x1000) ? 633 : 256;
            report_syntax_error(diagnostic, format_symbol_name(member));
        } else record_friend(owner, context, member);
    } else record_friend(owner, context, member);
    if (qualified) unwind_qualified_scopes(1);
    if (browser_filename) browser_friend_reference(member);
}

/* Original 0x2f230..0x2fa55. Pure-specifier markers use kind 19 until this
   routine restores the ordinary declarator kind. Qualifiers remain separate
   from the member being resolved and from any existing overload set. */
void __stdcall declare_class_member(DeclarationType *context, CppSymbol *member)
{
    CType *base;
    CppSymbol *tag, *existing, *merged = 0, *aggregate, *copy;
    QualifiedDeclarator *qualified;
    TypeShape *last, *shape;
    StorageSpec storage;
    int pure, apply_dll;
    char *name, *keyword, *owner_name, *owner_kind, *aggregate_name, *aggregate_kind;

    if (current_class_context < 0) {
        report_syntax_error(28);
        return;
    }
    base = context->type;
    if (!member) {
        if (context->storage[1] & 4) {
            record_friend(CLASS_TAG, context, 0);
            if (browser_filename)
                browser_friend_reference(cpp_context_stack->items[context->type->field08]);
            return;
        }
        if ((base->flags & 0x400) &&
            (((CppSymbol *)cpp_context_stack->items[base->field08])->flags & 0x80)) {
            if ((base->flags & 0x404) != 0x404) {
                if (options.configuration.extensions_enabled) compiler_warning(4, 201);
                else {
                    keyword = (base->flags & 0x402) == 0x402 ? token_spellings[32] :
                        ((base->flags & 0x408) == 0x408 ? token_spellings[30] : token_spellings[31]);
                    report_syntax_error(467, keyword);
                }
            }
            tag = cpp_context_stack->items[base->field08];
            if (!(tag->flags & 4)) {
                if (!options.prototypes_only) report_syntax_error(27, tag->name);
                return;
            }
            validate_anonymous_aggregate(context);
            declare_anonymous_aggregate(base);
            return;
        }
        if (!base->field08 || (context->storage[0] & 0x80)) report_type_error(208, base);
        return;
    }
    pure = (member->kind & 31) == 19;
    if (pure) member->kind = 3;
    context->storage[0] &= 0x7f;
    if (declaration_mode && !(token_allocation_flags & 0x80000)) {
        copy = allocate_permanent(sizeof(CppSymbol));
        *copy = *member;
        member = copy;
    }
    qualified = (QualifiedDeclarator *)member->next;
    if (!qualified || (qualified->resolution && !qualified->resolution->binding->next)) qualified = 0;
    if (member->declaration_flags & 0x2000)
        select_destructor_type(member, context, &base, &storage);
    else {
        base = context->type;
        storage.flags = context->storage[0] | (context->storage[1] << 8);
        storage.extended = context->storage[2];
    }
    if (is_current_class_name(member->name)) {
        if (!(member->attributes & 0x80)) {
            report_syntax_error(580, member->name);
            member->declaration_flags |= 2;
            return;
        }
        member->name = name_ctor;
        member->declaration_flags |= 0x1000;
        member->auxiliary.declaring_class = CLASS_TAG->detail.class_info->type_index;
    }
    if (storage.flags & 0x400) {
        declare_friend_member(context, member, qualified, CLASS_TAG, storage, pure);
        return;
    }
    if (qualified) {
        if ((context->storage[0] & 0x30) == 0x30 && !member->type->related) {
            unwind_qualified_scopes(1);
            apply_member_access_declaration(CLASS_TAG, qualified);
            return;
        }
        if ((CppSymbol *)qualified->resolution->binding->next->target != CLASS_TAG) {
            report_syntax_error(838);
            unwind_qualified_scopes(1);
            return;
        }
    }
    if (CLASS_TAG->detail.class_info->flags2 & 4) {
        last = last_type_shape(member->type->related);
        apply_dll = (storage.flags & 7) == 3 || (last && (last->flags & 0x77) == 4);
        if (!apply_dll && !last) {
            shape = context->type->related;
            if (shape) {
                if (!(shape->flags & 0x77)) shape = shape->related;
                apply_dll = (shape->flags & 0x77) == 4;
            }
        }
        if (apply_dll) inherit_class_dll_storage(context, member);
    }
    if ((member->kind & 31) != 2)
        member->auxiliary.declaring_class = CLASS_TAG->detail.class_info->type_index;
    if (qualified && !qualified->resolution) member->next = 0;
    member = resolve_declaration(context, member);
    if (!member) return;
    storage.flags = member->storage[0] | (member->storage[1] << 8);
    storage.extended = member->storage[2];
    name = member->name;
    member->next = 0;
    if ((member->kind & 31) == 7) {
        if ((member->declaration_flags & 0x2000) && !(member->type->related->flags & 0x4000)) {
            report_syntax_error(524, CLASS_TAG->name);
            return;
        }
        member->declaration_flags |= 0x400;
    } else {
        if (member->declaration_flags & 0x2000) {
            report_syntax_error(524, CLASS_TAG->name);
            return;
        }
        if (member->declaration_flags & 0x1000) {
            report_syntax_error(461, CLASS_TAG->name);
            return;
        }
        if ((context->storage[0] & 0x30) == 0x30) return;
        if (pure) {
            report_syntax_error(252, member->name);
            return;
        }
        if (!(member->flags & 0x80) && member->name)
            capture_inline_member_declaration(member, -1, -1, 0);
    }
    member->flags = (member->flags & ~3) | (CLASS_CONTEXT.flags & 3);
    if ((CLASS_CONTEXT.flags & 3) < 3) CLASS_CONTEXT.flags |= 4;
    existing = find_class_member_name(name);
    if (existing) {
        merged = reconcile_declaration(existing, member);
        if (!merged) {
            if (member->declaration_flags & 2) return;
        } else {
            if ((merged->declaration_flags & 2) || (member->declaration_flags & 2)) return;
            if ((member->kind & 31) == 7 && (existing->kind & 31) != 17)
                existing = find_tag_declaration(existing);
        }
    }
    if ((storage.flags & 7) == 7) {
        member->kind = 0x29;
        if (!merged) insert_class_member(member);
    } else {
        if ((member->kind & 31) != 7 && (storage.flags & 7) != 3 &&
            (type_qualifiers(member->type) & 0x1000)) CLASS_TAG->detail.class_info->flags2 |= 0x40;
        if ((member->kind & 31) == 2)
            layout_bit_field(member, name, member->type->flags, merged == 0);
        else if ((member->kind & 31) == 7) {
            if (!existing) existing = make_overload_symbol(member);
            if (member->declaration_flags & 0x1000) {
                CLASS_TAG->declaration_flags |= 0x1000;
                existing->declaration_flags |= 0x1000;
            } else if (member->declaration_flags & 0x2000) {
                CLASS_TAG->declaration_flags |= 0x2000;
                existing->declaration_flags |= 0x2000;
            }
            configure_member_function(storage, member, pure);
            if (!merged) insert_class_member(existing);
        } else {
            if ((CLASS_TAG->type->flags & 0x404) == 0x404 && !validate_union_member(CLASS_TAG, member)) return;
            if (CLASS_TAG->detail.class_info->flags2 & 4) {
                aggregate = class_type_of_member(member);
                if (aggregate && !(aggregate->detail.class_info->flags2 & 5)) {
                    owner_name = CLASS_TAG->name;
                    owner_kind = type_kind_name(CLASS_TAG->type->flags);
                    aggregate_name = aggregate->name;
                    aggregate_kind = type_kind_name(aggregate->type->flags);
                    compiler_warning(2, 251, member->name, aggregate_kind, aggregate_name, owner_kind, owner_name);
                }
            }
            if (member->name != name_alignment) {
                if (!is_static_data_member(member)) member->detail.value = layout_data_member(member);
                else {
                    member->detail.value = 0;
                    if (declaration_mode) report_syntax_error(246, member->name);
                }
            }
            if ((storage.flags & 7) == 3) create_static_member_alias(member);
            if (!merged) insert_class_member(member);
            if ((storage.flags & 7) != 3) {
                CLASS_TAG->attributes = (CLASS_TAG->attributes & ~0x100U) | (member->attributes & 0x100);
                CLASS_CONTEXT.bit_field_type = 0;
            }
        }
    }
    if (browser_filename && (!declaration_mode || options.option_FR) && !(CLASS_TAG->flags & 0x80))
        browser_declaration(member);
}

/* Original 0x2ef90..0x2efe0. The name's interner flag avoids scanning a
   class list when this spelling has never been inserted as a member. */
CppSymbol *__stdcall find_class_member_name(char *name)
{
    CppSymbol *member;
    if (!name || name == name_alignment || !(((unsigned char *)name)[-1] & 4)) return 0;
    for (member = CLASS_TAG->auxiliary.scope_members; member; member = member->next)
        if (member->name == name && (member->kind & 31) != 15) return member;
    return 0;
}

/* Original 0x2e920..0x2e94d. Only data/bit-field variants can carry the
   static-member storage class; qualified functions and tags do not. */
int __stdcall is_static_data_member(CppSymbol *symbol)
{
    unsigned kind;
    if (!symbol) return 0;
    kind = symbol->kind & 31;
    return (kind == 2 || kind == 3) && (symbol->storage[0] & 7) == 3;
}

/* Original 0x2e950..0x2ec82. Structs advance the current size; unions retain
   the largest member. Incomplete-array recovery and aggregate properties
   remain separate from the offset returned for this member. */
unsigned __stdcall layout_data_member(CppSymbol *member)
{
    unsigned size, alignment, offset;
    CType *type;
    CppSymbol *tag;
    char *name, *kind;
    if ((CLASS_CONTEXT.flags & 0x10) && (CLASS_TAG->attributes & 0x100)) {
        tag = CLASS_TAG;
        name = tag->name;
        kind = type_kind_name(tag->type->flags);
        report_syntax_error(229, kind, name);
        CLASS_TAG->attributes &= ~0x100U;
    }
    if ((member->kind & 31) == 4) size = member->detail.class_info->nonvirtual_size;
    else {
        size = type_size(member->type);
        if (!size) {
            tag = cpp_context_stack->items[member->type->field08];
            if (tag && !(tag->flags & 4)) {
                report_syntax_error(79, member->name, tag->name);
                return 0;
            }
            if (!options.configuration.extensions_enabled) {
                report_syntax_error(133, member->name);
                return 0;
            }
            compiler_warning(2, 200);
            size = type_size(drop_first_type_shape(member->type));
            member->attributes |= 0x100;
        }
    }
    if ((CLASS_CONTEXT.flags & 0x10) && CLASS_CONTEXT.bit_field_type) {
        CLASS_CONTEXT.size += CLASS_CONTEXT.bit_field_type & 15;
        CLASS_CONTEXT.bit_field_bits = 0;
    }
    type = member->type;
    while (type->related && (type->related->flags & 0x77) == 2)
        type = drop_first_type_shape(type);
    if (!type->related) {
        if (type->flags & 0x140) {
            alignment = type->flags & 15;
            if (top_packing(packing_stack)->alignment > alignment) goto align;
        }
        if ((type->flags & 0x410) == 0x410) {
            tag = cpp_context_stack->items[type->field08];
            alignment = tag->detail.class_info->alignment_flags & 0x3f;
        } else if (type->flags & 0x400)
            alignment = member->detail.class_info->alignment_flags & 0x3f;
        else alignment = type_size(type);
    } else {
        if (!(type->related->flags & 1)) goto place;
        alignment = type_size(type);
    }
align:
    align_class_member(alignment);
place:
    if (CLASS_CONTEXT.flags & 0x10) {
        offset = CLASS_CONTEXT.size;
        if (!(member->attributes & 0x100)) {
            if (size && addition_overflows(offset, size - 1, 1, 4)) {
                tag = CLASS_TAG;
                kind = type_kind_name(tag->type->flags);
                report_syntax_error(89, tag->name, kind);
                return 0;
            }
            CLASS_CONTEXT.size += size;
        }
    } else {
        offset = 0;
        if (CLASS_CONTEXT.size < size) CLASS_CONTEXT.size = size;
    }
    type = member->type;
    if (!type->related && (type->flags & 0x400)) {
        if ((member->kind & 31) == 4) return add_member_offset(offset, 0);
        tag = cpp_context_stack->items[type->field08];
        member->attributes = (member->attributes & ~0x100U) | (tag->attributes & 0x100);
        CLASS_TAG->attributes |= ((CppSymbol *)cpp_context_stack->items[member->type->field08])->attributes & 0x200;
        if ((member->type->flags & 0x404) == 0x404 ||
            (((CppSymbol *)cpp_context_stack->items[member->type->field08])->detail.class_info->flags & 1))
            CLASS_TAG->detail.class_info->flags |= 1;
    }
    return add_member_offset(offset, 0);
}

/* Original 0x32ca0..0x32dda. Packing is consulted at the same points as in
   the original, including its separate maximum-alignment update. Nonzero
   padding is represented by an unnamed hidden data member in the class list. */
void __stdcall align_class_member(unsigned alignment)
{
    unsigned effective = alignment, padding;
    CppSymbol *member;
    ArenaPool *pool;
    if (top_packing(packing_stack)->alignment < alignment)
        effective = top_packing(packing_stack)->alignment;
    if (CLASS_CONTEXT.alignment <= (unsigned short)effective) {
        effective = alignment;
        if (top_packing(packing_stack)->alignment < alignment)
            effective = top_packing(packing_stack)->alignment;
        CLASS_CONTEXT.alignment = (unsigned short)effective;
    }
    padding = class_alignment_padding(alignment);
    if (padding) {
        effective = alignment;
        if (top_packing(packing_stack)->alignment < alignment)
            effective = top_packing(packing_stack)->alignment;
        if (alignment != effective) compiler_warning(4, 121, CLASS_TAG->name);
        if (!declaration_mode) member = allocate_permanent(sizeof(CppSymbol));
        else {
            pool = &compiler_arena.pools[1];
            if (pool->remaining < (int)sizeof(CppSymbol)) refill_arena(&compiler_arena, pool);
            member = (CppSymbol *)pool->next;
            pool->remaining -= sizeof(CppSymbol);
            pool->next += sizeof(CppSymbol);
        }
        member->kind = 3;
        member->flags |= 0x80;
        member->name = name_alignment;
        member->detail.value = padding;
        insert_class_member(member);
        CLASS_CONTEXT.size += padding;
    }
}

/* Original 0x32c10..0x32c2c. */
unsigned __stdcall class_alignment_padding(unsigned alignment)
{
    return alignment_padding_at(alignment, CLASS_CONTEXT.size);
}

/* Original 0x32c30..0x32c99. Re-read the packing entry for the nonzero-
   remainder subtraction, as the original macro-expanded expression does. */
unsigned __stdcall alignment_padding_at(unsigned alignment, unsigned offset)
{
    unsigned effective = alignment, remainder;
    if (top_packing(packing_stack)->alignment < alignment)
        effective = top_packing(packing_stack)->alignment;
    remainder = offset % effective;
    if (remainder) {
        if (top_packing(packing_stack)->alignment < alignment)
            alignment = top_packing(packing_stack)->alignment;
        remainder = alignment - remainder;
    }
    return remainder;
}

/* Original 0x2fd90..0x2fd9b. Both words are offset quantities and wrap
   exactly as the original 32-bit ADD. */
unsigned __stdcall add_member_offset(unsigned offset, unsigned displacement)
{
    return offset + displacement;
}

/* Original 0x32e40..0x32e4e. The general list operation also accepts
   chains; this wrapper inserts exactly one member. */
void __stdcall insert_class_member(CppSymbol *symbol)
{
    append_class_members(symbol, symbol);
}

/* Original 0x32de0..0x32e33. Mark every interned spelling in the inclusive
   chain, then splice it after the current tail without changing last->next. */
void __stdcall append_class_members(CppSymbol *first, CppSymbol *last)
{
    CppSymbol *member = first;
    for (;;) {
        if (member->name) ((unsigned char *)member->name)[-1] |= 4;
        if (member == last) break;
        member = member->next;
    }
    if (CLASS_CONTEXT.last_member) CLASS_CONTEXT.last_member->next = first;
    else CLASS_TAG->auxiliary.scope_members = first;
    CLASS_CONTEXT.last_member = last;
}

/* Original 0x9ec68. These shared descriptions are indexed by the four
   compiler-generated special-member requirements at ClassInfo +0x37. */
char *implicit_member_descriptions[4] = {
    "default constructor", "copy constructor", "assignment", "destructor"
};

/* Original 0x2db20..0x2dc23. An anonymous class must not introduce an
   ordinary visible member function or require an implicit special member. */
void __stdcall check_class_specifier(DeclarationType *type)
{
    CppSymbol *tag = cpp_context_stack->items[type->type->field08];
    CppSymbol *member, *function;
    unsigned requirement;
    if (!tag || (tag->type->flags & 0x200) ||
        (token_allocation_flags & 0x400) || !(tag->flags & 0x80)) return;
    for (member = tag->auxiliary.scope_members; member; member = member->next) {
        if ((member->kind & 31) != 17 || (member->flags & 0x80)) continue;
        for (function = ((OverloadSet *)member)->functions; function; function = function->next) {
            if (!(function->flags & 0x80)) {
                report_syntax_error(627);
                return;
            }
        }
    }
    for (requirement = 0; requirement < 4; ++requirement) {
        if (tag->detail.class_info->implicit_member_requirements & (1 << requirement)) {
            report_syntax_error(639, implicit_member_descriptions[requirement]);
            return;
        }
    }
}

/* Original 0x33540..0x3355e. Access occupies the low two context bits. */
void __stdcall set_member_access(unsigned char access)
{
    CLASS_CONTEXT.flags = (CLASS_CONTEXT.flags & ~3) | (access & 3);
}

/* Original 0x32e50..0x32e72. Constructor syntax requires an implicit return
   type and excludes the two conflicting storage bits. Preserve the supplied
   class spelling as the diagnostic's variadic argument. */
void __stdcall check_constructor_storage(StorageSpec storage, char *class_name)
{
    if ((storage.flags & 0x600) || !(storage.flags & 0x20))
        report_syntax_error(633, class_name);
}

/* Original 0x33040..0x3308e. A constructor for the active class uses the
   interned internal spelling; a mismatched class keeps its diagnostic name. */
CppSymbol *__stdcall declare_constructor(DeclarationType *type, CppSymbol *class_symbol,
    CppSymbol *parameters)
{
    char *name = is_current_class(class_symbol) ? name_ctor : class_symbol->name;
    CppSymbol *member = new_declarator(name);
    member->kind = 3;
    member = attach_function_parameters(member, parameters);
    if (name == name_ctor) member->declaration_flags |= 0x1000;
    declare_class_member(type, member);
    return member;
}

/* Original 0x2fef0..0x3022c. Configure access and implicit-this status,
   classify special members, then apply virtual/default-argument/inline work.
   All original diagnostics retain their variadic names and qualifier strings. */
void __stdcall configure_member_function(StorageSpec storage, CppSymbol *member, int pure)
{
    TypeShape *shape;
    ClassContext *context;
    StorageSpec member_storage;
    char *formatted;
    member->flags = (member->flags & ~3) | (CLASS_CONTEXT.flags & 3);
    if (!member->index) {
        if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
        if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_symbol_rollover();
        member->index = next_symbol_index++;
    }
    member->declaration_flags |= 0x400;
    member->auxiliary.declaring_class = CLASS_TAG->detail.class_info->type_index;
    if (member->name == name_new || member->name == name_delete)
        member->cpp.parts.member_flags &= ~1;
    else if ((storage.flags & 7) == 3) {
        shape = member->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        if (has_qualified_this_type(shape->data.parameters->associated_type))
            report_syntax_error(272, member->name);
    } else member->cpp.parts.member_flags |= 1;
    if (!(member->cpp.parts.member_flags & 1))
        member->type = transform_function_parameters(member->type, mark_static_parameters);
    if (member->declaration_flags & 0x1000) {
        classify_constructor(member);
        shape = member->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        if (shape->data.parameters->associated_type->attributes & 0x1000)
            report_syntax_error(583, format_symbol_name(member), token_spellings[TOK_CONST]);
        shape = member->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        if (shape->data.parameters->associated_type->attributes & 0x2000)
            report_syntax_error(583, format_symbol_name(member), token_spellings[TOK_VOLATILE]);
    } else if (member->declaration_flags & 0x2000) {
        shape = member->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        if (shape->data.parameters->associated_type->attributes & 0x1000)
            report_syntax_error(583, format_symbol_name(member), token_spellings[TOK_CONST]);
        shape = member->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        if (shape->data.parameters->associated_type->attributes & 0x2000)
            report_syntax_error(583, format_symbol_name(member), token_spellings[TOK_VOLATILE]);
        if ((storage.flags & 0x200) && !(CLASS_CONTEXT.flags & 0x10)) {
            context = &CLASS_CONTEXT;
            formatted = format_symbol_name(member);
            report_syntax_error(571, context->symbol->name, formatted);
        }
    } else if (member->name == name_assignment) {
        if ((storage.flags & 7) == 3) report_syntax_error(581, CLASS_TAG->name);
        else classify_copy_assignment(member);
    } else if ((storage.flags & 0x200) && !(CLASS_CONTEXT.flags & 0x10)) {
        report_syntax_error(571, CLASS_TAG->name, member->name);
        return;
    }
    if (member->storage[1] & 2) ((unsigned char *)member->name)[-1] |= 0x10;
    if ((member->cpp.parts.member_flags & 1) && overrides_virtual_member(member, CLASS_TAG, 1)) {
        member->storage[1] |= 2;
        ((unsigned char *)member->name)[-1] |= 0x10;
    }
    if (pure) {
        member_storage.flags = member->storage[0] | (member->storage[1] << 8);
        member_storage.extended = member->storage[2];
        set_pure_virtual_member(member, CLASS_TAG, member_storage);
    }
    validate_special_function(member, 0);
    validate_function_default_arguments(member);
    capture_inline_member_declaration(member, -1, -1, 0);
}

/* Original 0x30230..0x3035e. Constructor classification recognizes default
   arguments and the single reference-to-self form, retaining its cv bits. */
void __stdcall classify_constructor(CppSymbol *member)
{
    TypeShape *outer = member->type->related, *function, *shape;
    FunctionParameters *parameters;
    DeclarationParameterInfo *declarations;
    CType *type;
    CppSymbol *tag;
    unsigned required, qualifiers;
    if (outer->flags & 0x4000) {
        member->cpp.parts.member_flags |= 0x10;
        return;
    }
    function = outer;
    if (function && !(function->flags & 0x77)) function = function->related;
    parameters = function->data.parameters;
    declarations = parameters->declarations;
    if ((outer->flags & 0x8000) && declarations->count == 1) {
        member->cpp.parts.member_flags |= 0x30;
        return;
    }
    required = (unsigned)declarations->count - parameters->default_count;
    if (!required) member->cpp.parts.member_flags |= 0x10;
    else if (required != 1) return;
    type = declarations->types[0];
    if (!(type->flags & 0x400) || type->field08 != CLASS_CONTEXT.type->field08) return;
    shape = type->related;
    if (!shape || (!(shape->flags & 0x77) && !shape->related)) {
        tag = cpp_context_stack->items[type->field08];
        report_syntax_error(652, tag->name, tag->name);
        member->declaration_flags |= 2;
        member->cpp.parts.member_flags |= 0x20;
        return;
    }
    if ((shape->flags & 0x40) && !shape->related) {
        member->cpp.parts.member_flags |= 0x20;
        qualifiers = 0;
        if (type->attributes & 0x1000) qualifiers |= 1;
        if (type->attributes & 0x2000) qualifiers |= 2;
        member->cpp.parts.member_qualifiers =
            (member->cpp.parts.member_qualifiers & 0x3f) | (qualifiers << 6);
    }
}

/* Original 0x796a0..0x798ef. Name identity and canonical parameter lists
   select overloads before comparing return types. With diagnostics enabled,
   continue across all inherited slots so each conflicting return is reported. */
int __stdcall overrides_virtual_member(CppSymbol *member, CppSymbol *owner, int diagnose)
{
    BaseClassLink *base;
    CppSymbol *base_tag, *inherited, *inherited_owner;
    Vector *tables, *groups;
    VirtualTableGroup *group;
    VirtualTable *table;
    TypeShape *member_shape, *inherited_shape;
    FunctionParameters *member_parameters, *inherited_parameters;
    CType *member_this, *inherited_this;
    short table_index, group_index;
    int entry_index, found = 0;
    if (!(((unsigned char *)member->name)[-1] & 0x10)) return 0;
    for (base = (BaseClassLink *)owner->auxiliary.scope_members; base;
            base = (BaseClassLink *)base->next) {
        if ((base->kind & 31) != 15 || (base->flags & 0x80)) continue;
        base_tag = cpp_context_stack->items[base->context_index];
        tables = base_tag->detail.class_info->virtual_tables;
        groups = base_tag->detail.class_info->virtual_table_groups;
        table_index = group_index = 0;
        for (;;) {
            if (table_index >= tables->count) {
                if (group_index >= groups->count) break;
                group = groups->items[group_index++];
                tables = group->tables;
                table_index = 0;
                continue;
            }
            table = tables->items[table_index++];
            for (entry_index = 0; entry_index < table->entry_count; ++entry_index) {
                inherited = table->entries[entry_index].dispatch.function;
                if (member->name != inherited->name) continue;
                member_shape = member->type->related;
                if (member_shape && !(member_shape->flags & 0x77)) member_shape = member_shape->related;
                inherited_shape = inherited->type->related;
                if (inherited_shape && !(inherited_shape->flags & 0x77)) inherited_shape = inherited_shape->related;
                member_parameters = member_shape->data.parameters;
                inherited_parameters = inherited_shape->data.parameters;
                if (member_parameters->declarations != inherited_parameters->declarations) continue;
                member_this = member_parameters->associated_type;
                inherited_this = inherited_parameters->associated_type;
                if ((member->type->attributes ^ inherited->type->attributes) & 7) continue;
                if ((member_this->related->attributes ^ inherited_this->related->attributes) & 7) continue;
                if ((member_this->attributes ^ inherited_this->attributes) & 0x3000) continue;
                if (!compatible_override_return(member->type, inherited->type)) {
                    if (diagnose) {
                        inherited_owner = cpp_context_stack->items[inherited->auxiliary.declaring_class];
                        report_syntax_error(555, owner->name, member->name,
                            inherited_owner->name, inherited->name);
                    }
                    continue;
                }
                if (!diagnose) return 1;
                found = 1;
            }
        }
    }
    return found;
}

/* Original 0x3a550..0x3a596. Validate defaults on a function declaration,
   stripping the optional leading type modifier before accessing its list. */
void __stdcall validate_function_default_arguments(CppSymbol *function)
{
    TypeShape *shape;
    FunctionParameters *parameters;
    unsigned count;
    if (!function || (function->kind & 31) != 7 || !function->type) return;
    shape = function->type->related;
    if (!shape) return;
    if (!(shape->flags & 0x77)) shape = shape->related;
    parameters = shape->data.parameters;
    count = parameters->default_count;
    check_missing_defaults(parameters->defaults, count, parameters,
        parameters->declarations->count - count, function);
}

/* Original 0x2e0f0..0x2e134. Class-valued members contribute their class
   context; ordinary indirections do not, after an optional type modifier. */
CppSymbol *__stdcall class_type_of_member(CppSymbol *member)
{
    CType *type = member->type;
    TypeShape *shape;
    if (!type || !(type->flags & 0x400)) return 0;
    if (!(member->flags & 0x40) && type->related) {
        shape = type->related;
        if (!(shape->flags & 0x77)) shape = shape->related;
        if (shape) return 0;
    }
    return cpp_context_stack->items[member->type->field08];
}

TypeShape *__stdcall skip_reference_shape(TypeShape *shape);

/* Original 0x30360..0x3040c. A one-argument assignment from this class,
   after erasing its outer reference, supplies the special-member cv flags. */
void __stdcall classify_copy_assignment(CppSymbol *member)
{
    TypeShape *outer, *function;
    FunctionParameters *parameters;
    CType *type;
    unsigned qualifiers = 0;
    if (member->name != name_assignment) return;
    outer = member->type->related;
    function = outer;
    if (function && !(function->flags & 0x77)) function = function->related;
    parameters = function->data.parameters;
    if ((outer->flags & 0x4000) || parameters->declarations->count != 1 ||
        parameters->default_count) return;
    type = parameters->declarations->types[0];
    if (!(type->flags & 0x400) || type->field08 != CLASS_CONTEXT.type->field08 ||
        skip_reference_shape(type->related)) return;
    member->cpp.parts.member_flags |= 0x40;
    if (type->attributes & 0x1000) qualifiers |= 1;
    if (type->attributes & 0x2000) qualifiers |= 2;
    member->cpp.parts.member_qualifiers =
        (member->cpp.parts.member_qualifiers & 0x3f) | (qualifiers << 6);
}

/* Original 0x33090..0x330d0. Destructor declarators use the shared internal
   int return type and interned destructor name. A missing context changes only
   the low storage byte, preserving its other caller-supplied storage fields. */
void __stdcall select_destructor_type(CppSymbol *member, DeclarationType *context,
    CType **base, StorageSpec *storage)
{
    if (context) {
        storage->flags = context->storage[0] | (context->storage[1] << 8);
        storage->extended = context->storage[2];
    } else storage->flags = (storage->flags & ~7) | 0x10;
    *base = type_0064;
    member->name = name_dtor;
}

/* Original 0x32fc0..0x32ff6. Synthetic dispatch members precede the
   existing chain; the source-order tail remains where parsing left it. */
void __stdcall prepend_class_member(CppSymbol *member)
{
    if (member->name) ((unsigned char *)member->name)[-1] |= 4;
    member->next = CLASS_TAG->auxiliary.scope_members;
    CLASS_TAG->auxiliary.scope_members = member;
}

/* Original 0x309a0..0x309af. Selection is filled by next_class_member. */
void __stdcall begin_class_members(ClassMemberIterator *iterator, CppSymbol *tag)
{
    iterator->next = tag->auxiliary.scope_members;
}

/* Original 0x309b0..0x309ce. Exhaustion also clears the selected member. */
int __stdcall next_class_member(ClassMemberIterator *iterator)
{
    iterator->member = iterator->next;
    if (!iterator->member) return 0;
    iterator->next = iterator->member->next;
    return 1;
}

void __stdcall add_base_class(unsigned access, int is_virtual, CppSymbol *tag);

/* Original 0x305e0..0x30669. Microsoft extensions admit an unqualified
   typedef naming a class as a base, with both spellings in warning 97. */
void __stdcall add_named_base(unsigned access, int is_virtual, char *name)
{
    CppSymbol *tag = lookup_tag_name(name), *symbol, *alias;
    if (options.configuration.extensions_enabled && !tag) {
        symbol = find_outer_name(name);
        if (symbol && !symbol->type->related) {
            alias = cpp_context_stack->items[symbol->type->field08];
            if (alias && (alias->kind & 31) == 4) {
                compiler_warning(3, 97, name, alias->name);
                tag = alias;
            }
        }
    }
    if (!tag) {
        report_syntax_error(505, name);
        return;
    }
    add_base_class(access, is_virtual, tag);
}

int __stdcall can_add_base_class(int is_virtual, CppSymbol *base, CppSymbol *derived);
void __stdcall browser_base_class(BaseClassLink *base);

/* Original 0x306a0..0x30951. Validate the class before allocating the
   24-byte base-list record. Access occupies a full original stack word: a
   zero word selects the class/struct default, then only its low bits are stored. */
void __stdcall add_base_class(unsigned access, int is_virtual, CppSymbol *tag)
{
    CppSymbol *alias;
    BaseClassLink *base;
    ArenaPool *pool;
    unsigned short flags;
    unsigned arena;
    char *base_kind, *base_name, *owner_kind, *owner_name;
    if (options.configuration.extensions_enabled && !(tag->type->flags & 0x400) &&
        !tag->type->related) {
        alias = cpp_context_stack->items[tag->type->field08];
        if (alias && (alias->kind & 31) == 4) {
            compiler_warning(3, 97, tag->name, alias->name);
            tag = alias;
        }
    }
    if (!(tag->type->flags & 0x400)) {
        report_syntax_error(505, tag->name);
        return;
    }
    if (!(tag->flags & 4)) {
        report_syntax_error(504, tag->name);
        return;
    }
    if (!(CLASS_CONTEXT.flags & 0x10)) {
        report_syntax_error(570, CLASS_TAG->name);
        return;
    }
    if (is_current_class(tag)) {
        report_syntax_error(730, tag->name);
        return;
    }
    flags = tag->type->flags;
    if ((flags & 0x404) == 0x404) {
        report_syntax_error(569, tag->name);
        return;
    }
    if (tag->attributes & 0x100) {
        report_syntax_error(503, tag->name);
        return;
    }
    if ((CLASS_TAG->detail.class_info->flags2 & 4) &&
        !(tag->detail.class_info->flags2 & 5)) {
        owner_name = CLASS_TAG->name;
        owner_kind = type_kind_name(CLASS_TAG->type->flags);
        base_name = tag->name;
        base_kind = type_kind_name(flags);
        compiler_warning(2, 275, base_kind, base_name, owner_kind, owner_name);
    }
    if (!can_add_base_class(is_virtual, tag, CLASS_TAG)) return;
    if (!access) access = (CLASS_TAG->type->flags & 0x408) == 0x408 ? 1 : 3;
    arena = declaration_mode != 0;
    if (!arena) base = allocate_permanent(sizeof(BaseClassLink));
    else {
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(BaseClassLink)) refill_arena(&compiler_arena, pool);
        base = (BaseClassLink *)pool->next;
        pool->remaining -= sizeof(BaseClassLink);
        pool->next += sizeof(BaseClassLink);
    }
    base->kind = 15;
    base->name = tag->name;
    base->flags = (base->flags & ~3) | (access & 3);
    base->state = (base->state & ~1) | ((unsigned)is_virtual & 1);
    base->context_index = tag->detail.class_info->type_index;
    if (is_virtual || (tag->detail.class_info->flags & 0x10))
        CLASS_TAG->detail.class_info->flags |= 0x10;
    base->displacement = 0;
    insert_class_member((CppSymbol *)base);
    if (browser_filename && (!declaration_mode || options.option_FR) &&
        !(CLASS_TAG->flags & 0x80)) browser_base_class(base);
}

int __stdcall classify_base_repetition(int is_virtual, CppSymbol *base,
    CppSymbol *derived, int direct, CppSymbol **conflict);

/* Original 0x30510..0x305d3. A duplicate direct base stops insertion;
   the two ambiguous paths diagnose in different name order but still insert
   the record so the compiler can continue recovering the class declaration. */
int __stdcall can_add_base_class(int is_virtual, CppSymbol *base, CppSymbol *derived)
{
    CppSymbol *conflict;
    int status = classify_base_repetition(is_virtual, base, derived, 1, &conflict);
    switch (status) {
    case 0:
        return 1;
    case 1:
        report_syntax_error(500, derived->name, base->name);
        return 0;
    case 2:
        report_syntax_error(584, derived->name, base->name, conflict->name);
        return 1;
    case 3:
        report_syntax_error(584, derived->name, conflict->name, base->name);
        return 1;
    }
    return 0;
}

/* Original 0x30410..0x3050e. Compare visible base chains in both
   directions for a direct insertion. Virtual matches collapse only below
   the initial level; the two recursive findings retain distinct statuses. */
int __stdcall classify_base_repetition(int is_virtual, CppSymbol *base,
    CppSymbol *derived, int direct, CppSymbol **conflict)
{
    BaseClassLink *link = (BaseClassLink *)derived->auxiliary.scope_members, *next;
    CppSymbol *inherited;
    unsigned char state;
    while (link) {
        next = (BaseClassLink *)link->next;
        if ((link->kind & 31) == 15 && !(link->flags & 0x80)) {
            inherited = cpp_context_stack->items[link->context_index];
            state = link->state;
            if (conflict) *conflict = inherited;
            if (inherited == base && (!is_virtual || direct || !(state & 1))) return 1;
            if (classify_base_repetition(is_virtual, base, inherited, 0, 0)) return 2;
            if (direct && classify_base_repetition(state & 1, inherited, base, 0, 0)) return 3;
        }
        link = next;
    }
    return 0;
}

/* Original 0x798f0..0x79959. Return compatibility excludes parameter
   declarations (checked by the caller) and ignores the native pointer-kind
   attribute bits. Optional leading qualifier shapes precede the function. */
int __stdcall compatible_override_return(CType *member, CType *inherited)
{
    TypeShape *shape, *inherited_shape;
    if (member->flags != inherited->flags) return 0;
    if ((member->attributes ^ inherited->attributes) & 0xfbf8) return 0;
    if (member->field08 != inherited->field08) return 0;
    shape = member->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    inherited_shape = inherited->related;
    if (inherited_shape && !(inherited_shape->flags & 0x77))
        inherited_shape = inherited_shape->related;
    return shape->related == inherited_shape->related;
}

/* Original 0x793c0..0x79507. Place a direct base's nonvirtual portion,
   honoring packing and keeping adjacent empty bases distinct. The repeated
   packing queries come from the original macro-expanded alignment tests. */
unsigned __stdcall place_base_subobject(CppSymbol *base, CppSymbol **previous_base,
    CppSymbol *derived)
{
    unsigned short alignment;
    unsigned padding, offset;
    unsigned char flags;
    if (top_packing(packing_stack)->alignment <
        (unsigned)(base->detail.class_info->alignment_flags & 0x3f))
        alignment = (unsigned short)top_packing(packing_stack)->alignment;
    else alignment = base->detail.class_info->alignment_flags & 0x3f;
    if (CLASS_CONTEXT.alignment <= alignment) {
        if (top_packing(packing_stack)->alignment <
            (unsigned)(base->detail.class_info->alignment_flags & 0x3f))
            alignment = (unsigned short)top_packing(packing_stack)->alignment;
        else alignment = base->detail.class_info->alignment_flags & 0x3f;
        CLASS_CONTEXT.alignment = alignment;
    }
    padding = class_alignment_padding(base->detail.class_info->alignment_flags & 0x3f);
    if (!padding && *previous_base &&
        ((*previous_base)->detail.class_info->flags & 8) &&
        (base->detail.class_info->flags & 4)) {
        ++CLASS_CONTEXT.size;
        padding = class_alignment_padding(base->detail.class_info->alignment_flags & 0x3f);
    }
    offset = CLASS_CONTEXT.size + padding;
    CLASS_CONTEXT.size = offset;
    CLASS_CONTEXT.size += base->detail.class_info->nonvirtual_size;
    if (derived) {
        if (!*previous_base) {
            flags = derived->detail.class_info->flags;
            derived->detail.class_info->flags = flags ^
                ((base->detail.class_info->flags ^ flags) & 4);
        }
        flags = derived->detail.class_info->flags;
        derived->detail.class_info->flags = flags ^
            ((base->detail.class_info->flags ^ flags) & 8);
    }
    *previous_base = base;
    return offset;
}

/* Original 0x2ec90..0x2ed8f. Reserve an unnamed object in the current
   class layout, flushing any pending bit-field unit and applying the current
   packing limit. Size-wrap recovery reports the class and retains its ABI. */
unsigned __stdcall layout_anonymous_data(CType *type)
{
    unsigned size = type_size(type), alignment, offset;
    CppSymbol *tag;
    char *kind, *name;
    if (CLASS_TAG->attributes & 0x100) {
        tag = CLASS_TAG;
        name = tag->name;
        kind = type_kind_name(tag->type->flags);
        report_syntax_error(229, kind, name);
        CLASS_TAG->attributes &= ~0x100U;
    }
    if (CLASS_CONTEXT.bit_field_type) {
        CLASS_CONTEXT.size += CLASS_CONTEXT.bit_field_type & 15;
        CLASS_CONTEXT.bit_field_bits = 0;
    }
    if (top_packing(packing_stack)->alignment < (unsigned)(type->flags & 15))
        alignment = top_packing(packing_stack)->alignment;
    else alignment = type->flags & 15;
    align_class_member(alignment);
    offset = CLASS_CONTEXT.size;
    CLASS_CONTEXT.size = offset + size;
    if (CLASS_CONTEXT.size < offset) {
        tag = CLASS_TAG;
        kind = type_kind_name(tag->type->flags);
        report_syntax_error(89, tag->name, kind);
        return 0;
    }
    return add_member_offset(offset, 0);
}

/* Original 0x785f0..0x7867d. Test canonical identity, then recursively
   inspect every visible direct base, including virtual paths. The successor
   is saved before recursion, and hidden/non-base declarations are skipped. */
int __stdcall class_derives_from(CppSymbol *derived, CppSymbol *base)
{
    BaseClassLink *link, *next;
    CppSymbol *direct_base;
    if (derived == base) return 1;
    for (link = (BaseClassLink *)derived->auxiliary.scope_members; link; link = next) {
        next = (BaseClassLink *)link->next;
        if ((link->kind & 31) != 15 || (link->flags & 0x80)) continue;
        direct_base = cpp_context_stack->items[link->context_index];
        if (class_derives_from(direct_base, base)) return 1;
    }
    return 0;
}

/* Original 0x2e680..0x2e911. Share a storage unit only for matching
   scalar widths and enough unused bits. Unnamed leading fields retain the
   separate copied declaration used by the original class layout list. */
void __stdcall layout_bit_field(CppSymbol *member, char *name,
    unsigned short flags, int new_member)
{
    CppSymbol *copy, *tag;
    unsigned width, bytes, bits;
    char *tag_name, *kind;
    if ((CLASS_CONTEXT.flags & 0x10) && (CLASS_TAG->attributes & 0x100)) {
        tag = CLASS_TAG;
        tag_name = tag->name;
        kind = type_kind_name(tag->type->flags);
        report_syntax_error(229, kind, tag_name);
        CLASS_TAG->attributes &= ~0x100U;
    }
    if (!(flags & 0x240)) {
        report_syntax_error(150, name);
        member->type = type_4064_first;
        flags = 0x4064;
    }
    if (member->type->related) report_syntax_error(33, name);
    width = member->auxiliary.bit_field.width;
    if (!width) {
        if (member->name != name_unnamed) report_syntax_error(149, name);
        if (!CLASS_CONTEXT.bit_field_type) return;
    }
    bytes = flags & 15;
    bits = bytes * 8;
    if (width > bits) {
        report_syntax_error(34, name);
        width = bits;
    }
    if (CLASS_CONTEXT.flags & 0x10) {
        if (((flags ^ CLASS_CONTEXT.bit_field_type) & 15) || !width ||
            CLASS_CONTEXT.bit_field_bits + width > bits) {
            if (CLASS_CONTEXT.bit_field_type)
                CLASS_CONTEXT.size += CLASS_CONTEXT.bit_field_type & 15;
            CLASS_CONTEXT.bit_field_bits = 0;
            align_class_member(bytes);
        }
    } else if ((int)CLASS_CONTEXT.size < (CLASS_CONTEXT.bit_field_type & 15))
        CLASS_CONTEXT.size = CLASS_CONTEXT.bit_field_type & 15;
    CLASS_CONTEXT.bit_field_type = width ? flags : 0;
    if (member->name == name_unnamed) {
        if ((CLASS_CONTEXT.flags & 0x10) && !CLASS_TAG->auxiliary.scope_members) {
            copy = allocate_permanent(sizeof(CppSymbol));
            *copy = *member;
            if (new_member) insert_class_member(copy);
            copy->detail.value = CLASS_CONTEXT.size;
            copy->auxiliary.bit_field.width = (unsigned char)width;
            copy->auxiliary.bit_field.offset = CLASS_CONTEXT.bit_field_bits;
        }
    } else {
        if (new_member) insert_class_member(member);
        member->auxiliary.bit_field.width = (unsigned char)width;
        member->auxiliary.bit_field.offset = CLASS_CONTEXT.bit_field_bits;
    }
    if (CLASS_CONTEXT.flags & 0x10) {
        CLASS_CONTEXT.bit_field_bits += (unsigned char)width;
        member->detail.value = CLASS_CONTEXT.size;
    } else member->detail.value = 0;
}

/* Original 0x2fda0..0x2fe88. A union may not contain a class that
   requires nontrivial construction, copying, assignment or destruction. The
   first failed property selects the diagnostic and makes the member invalid. */
int __stdcall validate_union_member(CppSymbol *owner, CppSymbol *member)
{
    CppSymbol *tag;
    unsigned char trivial;
    if ((member->storage[0] & 7) == 3)
        report_declaration_diagnostic(203, 619, CLASS_TAG->name, member->name);
    tag = class_type_of_member(member);
    if (!tag) return 1;
    trivial = tag->detail.class_info->trivial_special_members;
    if (!(trivial & 2)) {
        report_syntax_error(621, owner->name, member->name);
        return 0;
    }
    if (!(trivial & 1)) {
        report_syntax_error(620, owner->name, member->name);
        return 0;
    }
    if (!(trivial & 4)) {
        report_syntax_error(622, owner->name, member->name);
        return 0;
    }
    if (!(trivial & 8)) {
        report_syntax_error(623, owner->name, member->name);
        return 0;
    }
    return 1;
}

/* Original 0x2fab0..0x2fb9c. A static class member gets a global
   external declaration with its decorated name. Bind that declaration under
   C linkage, then associate the class declaration with its emitted symbol. */
void __stdcall create_static_member_alias(CppSymbol *member)
{
    CppSymbol *alias;
    DeclarationType context;
    SymbolScope *saved_scope;
    LanguageLinkage saved_linkage;
    if (options.prototypes_only) return;
    alias = new_declarator(make_anonymous_identifier());
    alias->declaration_flags &= ~0x300;
    alias->name = mangle_static_member_name(CLASS_TAG, member);
    alias->declaration_flags = (alias->declaration_flags & ~0x100) | 0x200;
    context.storage[0] = (member->storage[0] & ~7) | 1;
    context.storage[1] = member->storage[1];
    context.storage[2] = member->storage[2];
    if (member->type->attributes & 0x1000) context.storage[1] |= 0x10;
    context.type = member->type;
    saved_scope = current_scope;
    current_scope = global_scope;
    alias->attributes |= 0x40;
    saved_linkage = linkage_stack[linkage_depth];
    linkage_stack[linkage_depth] = LINKAGE_C;
    alias->flags |= 0x80;
    alias = apply_declaration_type(&context, alias);
    alias->flags &= ~0x80;
    linkage_stack[linkage_depth] = saved_linkage;
    alias->attributes &= ~0x40U;
    member->attributes |= 0x40;
    current_scope = saved_scope;
    member->index = alias->index;
    register_static_member_alias(member, alias);
}

/* Original 0x3b350..0x3b37e. A missing initializer is allowed; an
   explicit pure specifier must be the frontend's integral zero constant. */
void __stdcall validate_pure_specifier(IrToken *expression)
{
    if (!expression) return;
    if ((expression->flags & 7) != 3 || expression->kind != 7 ||
        !(expression->flags & 0x80)) report_syntax_error(258);
}

/* Original 0x2fba0..0x2fc0e. Only virtual declarations in a class
   (rather than a union) can mark the member and owner abstract. */
void __stdcall set_pure_virtual_member(CppSymbol *member, CppSymbol *owner, StorageSpec storage)
{
    if (!(storage.flags & 0x200)) {
        report_syntax_error(253, member->name);
        return;
    }
    if (CLASS_TAG->flags & 0x80) {
        report_syntax_error(283, member->name, type_kind_name(CLASS_TAG->type->flags));
        return;
    }
    owner->detail.class_info->flags |= 0x80;
    member->cpp.parts.member_flags |= 4;
}

/* Original 0x2e210..0x2e384, stdcall4. Anonymous aggregates inherit
   storage from their surrounding declaration and expose only public data. */
void __stdcall validate_anonymous_aggregate(DeclarationType *context)
{
    CppSymbol *tag = cpp_context_stack->items[context->type->field08];
    CppSymbol *member;
    ClassMemberIterator iterator;
    unsigned storage, access;
    int at_file_scope = !declaration_mode && current_class_context < 0;
    int found_data = 0;
    if (!(context->storage[0] & 7))
        context->storage[0] = (context->storage[0] & ~7) | (at_file_scope ? 2 : 4);
    storage = context->storage[0] & 7;
    if (storage != 3 && !(storage >= 4 && storage <= 6 && !at_file_scope)) {
        report_syntax_error(646);
        context->storage[0] = (context->storage[0] & ~4) | 3;
    }
    begin_class_members(&iterator, tag);
    while (next_class_member(&iterator)) {
        member = iterator.member;
        if ((member->flags & 0x80) ||
            ((member->kind & 31) != 2 && (member->kind & 31) != 3)) continue;
        found_data = 1;
        access = member->flags & 3;
        if (access < 3)
            report_syntax_error(626, member_access_keyword(access), member->name);
        if ((member->storage[0] & 7) == 3) {
            if (options.configuration.extensions_enabled)
                report_syntax_error(619, message_text(333), member->name);
            member->storage[0] = (member->storage[0] & ~7) | (at_file_scope ? 2 : 4);
        }
    }
    if (!found_data) report_syntax_error(625);
}

/* Original 0x2efe0..0x2f226, stdcall4. Preserve the anonymous
   aggregate's begin/end markers and copy its visible member declarations
   into the surrounding class, retaining relative offsets for a struct. */
void __stdcall declare_anonymous_aggregate(CType *type)
{
    CppSymbol *first, *last, *member, *copy, *tag;
    unsigned offset, kind;
    CLASS_TAG->attributes |= 0x200;
    first = allocate_permanent(sizeof(CppSymbol));
    first->flags |= 0x80;
    first->kind = 3;
    first->type = type;
    first->name = name_alignment;
    first->attributes |= 0x800;
    first->flags = (first->flags & ~3) | (CLASS_CONTEXT.flags & 3);
    offset = is_static_data_member(first) ? 0 : layout_data_member(first);
    first->detail.value = offset;
    tag = cpp_context_stack->items[type->field08];
    CLASS_TAG->attributes = (CLASS_TAG->attributes & ~0x100U) | (tag->attributes & 0x100);
    if ((tag->detail.class_info->flags & 1) || (type->flags & 0x404) == 0x404)
        CLASS_TAG->detail.class_info->flags |= 1;
    else CLASS_TAG->detail.class_info->flags &= ~1;
    last = first;
    for (member = tag->auxiliary.scope_members; member; member = member->next) {
        kind = member->kind & 31;
        if (kind == 15 || kind == 17 || kind == 4) continue;
        if (find_class_member_name(member->name)) continue;
        if (is_current_class_name(member->name)) {
            report_syntax_error(459, format_symbol_name(member));
            continue;
        }
        copy = allocate_permanent(sizeof(CppSymbol));
        *copy = *member;
        last->next = copy;
        last = copy;
        if (CLASS_CONTEXT.flags & 0x10) copy->detail.value += offset;
        copy->flags = (copy->flags & ~3) | (CLASS_CONTEXT.flags & 3);
    }
    member = allocate_permanent(sizeof(CppSymbol));
    member->flags |= 0x80;
    member->kind = 3;
    member->type = type;
    member->name = name_alignment;
    member->attributes |= 0x1000;
    member->flags = (member->flags & ~3) | (CLASS_CONTEXT.flags & 3);
    last->next = member;
    if (first) {
        member->next = 0;
        append_class_members(first, member);
    }
    CLASS_CONTEXT.bit_field_type = 0;
}

char *__stdcall member_protection_keyword(unsigned char protection);
void __stdcall record_member_access_declaration(CppSymbol *owner,
    BaseClassLink *base, CppSymbol *member);
unsigned char __stdcall qualified_member_protection(QualifierBinding *binding);
int __stdcall uniform_overload_protection(CppSymbol *overloads,
    QualifierBinding *binding, unsigned char *protection);

/* Original0x30d50..0x3104d, stdcall8. An access declaration must name
   a member through this class's direct base path and retain its protection.
   Record every function in a uniformly protected overload set separately. */
void __stdcall apply_member_access_declaration(CppSymbol *owner,
    QualifiedDeclarator *qualified)
{
    QualifierResolution *resolution = qualified->resolution;
    QualifierBinding *binding = resolution->binding, *path;
    CppSymbol *member, *node, *function, *following, *base_class;
    BaseClassLink *base = 0;
    unsigned kind;
    unsigned char protection;
    short first, second;
    char *first_text, *second_text, *old_access, *new_access;
    if (resolution->tail != binding) return;
    member = (CppSymbol *)binding->target;
    for (path = binding->next; path; path = path->next) {
        node = (CppSymbol *)path->target;
        kind = node->kind & 31;
        if (kind == 4 && node != owner) {
            begin_type_diagnostic();
            first = reserve_type_diagnostic();
            format_diagnostic_symbol(first,node);
            second = reserve_type_diagnostic();
            format_diagnostic_symbol(second,owner);
            second_text = type_diagnostic_text(second);
            first_text = type_diagnostic_text(first);
            report_syntax_error(602,first_text,member->name,second_text);
            end_type_diagnostic();
            return;
        }
        if (kind == 15) base = (BaseClassLink *)node;
    }
    if (binding->following->target != (ClassReference *)base) {
        begin_type_diagnostic();
        first = reserve_type_diagnostic();
        base_class = cpp_context_stack->items[binding->following->target->context_index];
        format_diagnostic_symbol(first,base_class);
        second = reserve_type_diagnostic();
        format_diagnostic_symbol(second,owner);
        second_text = type_diagnostic_text(second);
        first_text = type_diagnostic_text(first);
        report_syntax_error(603,first_text,second_text);
        end_type_diagnostic();
        return;
    }
    if ((CLASS_CONTEXT.flags & 3) == 1) {
        begin_type_diagnostic();
        first = reserve_type_diagnostic();
        base_class = cpp_context_stack->items[base->context_index];
        format_diagnostic_symbol(first,base_class);
        first_text = type_diagnostic_text(first);
        report_syntax_error(606,first_text,member->name);
        end_type_diagnostic();
        return;
    }
    kind = member->kind & 31;
    if (kind == 16 || kind == 17) {
        if (!uniform_overload_protection(member,binding,&protection)) {
            begin_type_diagnostic();
            first = reserve_type_diagnostic();
            base_class = cpp_context_stack->items[base->context_index];
            format_diagnostic_symbol(first,base_class);
            first_text = type_diagnostic_text(first);
            report_syntax_error(605,first_text,member->name);
            end_type_diagnostic();
            return;
        }
    } else protection = qualified_member_protection(binding);
    if ((CLASS_CONTEXT.flags & 3) != protection) {
        begin_type_diagnostic();
        first = reserve_type_diagnostic();
        base_class = cpp_context_stack->items[base->context_index];
        format_diagnostic_symbol(first,base_class);
        first_text = type_diagnostic_text(first);
        old_access = member_protection_keyword(protection);
        new_access = member_protection_keyword((unsigned char)(CLASS_CONTEXT.flags & 3));
        report_syntax_error(604,new_access,old_access,first_text,member->name);
        end_type_diagnostic();
        return;
    }
    kind = member->kind & 31;
    if (kind != 16 && kind != 17) {
        record_member_access_declaration(owner,base,member);
        return;
    }
    function = member;
    if (function && ((function->kind & 31) == 16 || (function->kind & 31) == 17))
        function = ((OverloadSet *)function)->functions;
    while (function && ((function->kind & 31) == 6 || (function->kind & 31) == 7)) {
        following = function->next;
        record_member_access_declaration(owner,base,function);
        function = following;
    }
}

/* Original0x31080..0x310e1, stdcall4. Follow the inherited qualification
   path, honoring an access declaration before combining each base protection. */
unsigned char __stdcall qualified_member_protection(QualifierBinding *binding)
{
    CppSymbol *member = (CppSymbol *)binding->target;
    QualifierBinding *path = binding->next;
    ClassReference *next;
    CppSymbol *owner;
    unsigned char protection = member->flags & 3;
    while (path->next) {
        next = path->next->target;
        if ((next->kind & 31) != 15) break;
        owner = cpp_context_stack->items[next->context_index];
        if (!has_member_access_declaration(owner,(BaseClassLink *)path->target,member))
            protection = combine_member_access(protection,
                (unsigned char)(((CppSymbol *)path->target)->flags & 3));
        path = path->next;
    }
    return protection;
}

void __stdcall insert_member_access_declaration(CppSymbol *owner,
    BaseClassLink *base, CppSymbol *member);

/* Original0x31050..0x31079, stdcall12. Duplicate declarations do not
   add another access record to the class. */
void __stdcall record_member_access_declaration(CppSymbol *owner,
    BaseClassLink *base, CppSymbol *member)
{
    if (!has_member_access_declaration(owner,base,member))
        insert_member_access_declaration(owner,base,member);
}

/* Original0x32bd0..0x32be6, cdecl0/RET0. A negative current context
   denotes namespace/global scope rather than a class table index. */
CppSymbol *get_current_class_symbol(void)
{
    if (current_class_context < 0) return 0;
    return class_contexts[current_class_context].symbol;
}

/* Original 0x2fa60..0x2fab0. Class DLL storage supplies the member's
   import/export direction while diagnosing an explicit duplicate modifier. */
void __stdcall inherit_class_dll_storage(DeclarationType *context, CppSymbol *member)
{
    if (context->storage[2] & 4)
        report_syntax_error(487, format_symbol_name(member));
    context->storage[2] |= 4;
    context->storage[2] = (context->storage[2] & ~8) |
        ((CLASS_TAG->detail.class_info->flags2 << 2) & 8);
}

CppSymbol *__stdcall new_anonymous_aggregate_object(DeclarationType *context);
void __stdcall bind_anonymous_aggregate_member(CppSymbol *object, CppSymbol *member,
    DeclarationType *context);
void __stdcall finish_anonymous_aggregate_object(CppSymbol *object);

/* Original 0x2e140..0x2e1c5. A standalone anonymous aggregate exposes
   its real data members through a separately declared backing object. */
void __stdcall declare_anonymous_aggregate_object(DeclarationType *context)
{
    CppSymbol *object, *member, *next, *tag;
    validate_anonymous_aggregate(context);
    object = new_anonymous_aggregate_object(context);
    if (object) {
        tag = cpp_context_stack->items[(short)object->type->field08];
        for (member = tag->auxiliary.scope_members; member; member = next) {
            next = member->next;
            if ((member->kind & 31) == 3 && !(member->flags & 0x80))
                bind_anonymous_aggregate_member(object, member, context);
        }
        finish_anonymous_aggregate_object(object);
    }
}

/* Original 0x2e390..0x2e3ad. The hidden anonymous object is completed
   through the same declaration path as an explicitly named object. */
CppSymbol *__stdcall new_anonymous_aggregate_object(DeclarationType *context)
{
    CppSymbol *object = new_declarator(name_unnamed);
    object->flags |= 0x80;
    return complete_declaration(context, object);
}

void __stdcall record_symbol_binding(int local, CppSymbol *alias,
    CppSymbol *object, CppSymbol *member);

/* Original 0x2e3b0..0x2e4e2. Expose a copied member in the surrounding
   scope while retaining its backing object/member binding separately. */
void __stdcall bind_anonymous_aggregate_member(CppSymbol *object, CppSymbol *member,
    DeclarationType *context)
{
    CppSymbol *copy, *existing;
    ArenaPool *pool;
    int local = declaration_mode != 0;
    unsigned i;
    if (!local)
        copy = allocate_permanent(sizeof(CppSymbol));
    else {
        pool = &compiler_arena.pools[1];
        if (pool->remaining < (int)sizeof(CppSymbol))
            refill_arena(&compiler_arena, pool);
        copy = (CppSymbol *)pool->next;
        pool->remaining -= sizeof(CppSymbol);
        pool->next += sizeof(CppSymbol);
    }
    *copy = *member;
    copy->kind = 9;
    if (next_symbol_index >= 0x7fffffffUL)
        fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    copy->index = next_symbol_index++;
    for (i = 0; i < 3; ++i)
        copy->storage[i] = context->storage[i];
    copy->attributes |= 0x1000;
    record_symbol_binding(local, copy, object, member);
    existing = lookup_current_scope_declaration(copy);
    if (existing)
        copy = existing;
    else if (is_current_class_name(copy->name)) {
        report_syntax_error(630, copy->name);
        return;
    } else
        insert_current_scope_symbol(copy);
    if (browser_filename && (!declaration_mode || options.option_FR))
        browser_declaration(copy);
}

/* Original 0x2e4f0..0x2e4f3 is an empty RET4 extension hook. The backing
   object argument is neither read nor forwarded; retain its real ABI/entry. */
void __stdcall finish_anonymous_aggregate_object(CppSymbol *object)
{
}
