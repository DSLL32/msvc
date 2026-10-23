/* Class member declarations and their storage, access and layout dispatch. */
#include "class-members.h"
#include "parameters.h"
#include <recon.h>

void __stdcall check_missing_defaults(DefaultArgument *defaults, unsigned count,
    FunctionParameters *parameters, unsigned first_parameter, CppSymbol *symbol);

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
                inherited = table->entries[entry_index].function;
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
