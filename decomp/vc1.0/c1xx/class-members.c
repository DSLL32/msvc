/* Class member declarations and their storage, access and layout dispatch. */
#include "class-members.h"

#define CLASS_CONTEXT (class_contexts[current_class_context])
#define CLASS_TAG (CLASS_CONTEXT.symbol)

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
