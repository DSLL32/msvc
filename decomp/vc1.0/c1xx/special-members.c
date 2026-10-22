/* C++ special-member synthesis after class member and virtual layout. */
#include "special-members.h"
#include "class-members.h"

/* Original 0x36790..0x36967. Classify bases, fields and declared functions,
   then decide each implicit member independently with its original diagnostics. */
void __stdcall declare_implicit_special_members(CppSymbol *tag)
{
    SpecialMemberState states[4];
    int model, constructors, assignments, i, j;
    model = special_member_model(tag->detail.class_info->pointer_attributes);
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 7; ++j) states[i].words[j] = 0;
    for (i = 0; i < 4; ++i) {
        states[i].fields.can_generate = 1;
        states[i].fields.nonprivate = 1;
        states[i].fields.qualifiers = 1;
    }
    inherit_base_special_members(tag, states, model);
    inherit_field_special_members(tag, states, model);
    inspect_declared_special_members(tag, states, &constructors, &assignments);
    if (tag->detail.class_info->alignment_flags & 0xc0) {
        states[0].fields.nontrivial = 1;
        states[1].fields.nontrivial = 1;
        states[2].fields.nontrivial = 1;
        tag->detail.class_info->flags2 &= ~1;
    }
    if (constructors > 0 || states[0].fields.nontrivial || states[1].fields.nontrivial ||
            (class_contexts[current_class_context].flags & 4))
        tag->detail.class_info->flags2 &= ~1;
    complete_special_member(tag, states, 0, model, 510, constructors == 0, 0);
    complete_special_member(tag, states, 1, model, 511, 1, constructors);
    complete_special_member(tag, states, 2, model, 512, 1, assignments);
    complete_special_member(tag, states, 3, model, 513, 1, 0);
    if (!constructors && !(tag->detail.class_info->trivial_special_members & 1) &&
            !(tag->detail.class_info->implicit_member_requirements & 1)) {
        if (options.configuration.extensions_enabled)
            report_syntax_error(610, type_kind_name(tag->type->flags), tag->name);
        else compiler_warning(1, 610, type_kind_name(tag->type->flags), tag->name);
        return;
    }
    if (find_special_member_model(tag, 3, 0) == -1)
        compiler_warning(3, 527, type_kind_name(tag->type->flags), tag->name);
}

/* Original 0x37040..0x37064. This flat-model build maps every supported
   pointer kind to slot zero, retaining the default-kind lookup for zero. */
int __stdcall special_member_model(unsigned short pointer_kind)
{
    if (!pointer_kind) pointer_kind = (unsigned short)data_pointer_kind;
    if (pointer_kind != 1) return 0;
    return 0;
}

/* Original 0x36d40..0x36db9. Every visible direct base contributes its
   special-member requirements and makes the derived class non-POD. */
void __stdcall inherit_base_special_members(CppSymbol *tag, SpecialMemberState *states, int model)
{
    BaseClassLink *base, *next;
    CppSymbol *subobject;
    for (base = (BaseClassLink *)tag->auxiliary.scope_members; base; base = next) {
        next = (BaseClassLink *)base->next;
        if ((base->kind & 31) != 15 || (base->flags & 0x80)) continue;
        subobject = cpp_context_stack->items[base->context_index];
        merge_subobject_special_members(tag, subobject, states, model);
        tag->detail.class_info->flags2 &= ~1;
    }
}

/* Original 0x36dc0..0x36e90. Only visible nonstatic data members contribute.
   Retain the native per-field replacement of the class flag's bit three. */
void __stdcall inherit_field_special_members(CppSymbol *tag, SpecialMemberState *states, int model)
{
    CppSymbol *member, *next, *subobject;
    for (member = tag->auxiliary.scope_members; member; member = next) {
        next = member->next;
        if ((member->kind & 31) != 3 || (member->flags & 0x80)) continue;
        subobject = class_type_of_member(member);
        if (is_static_data_member(member)) continue;
        if (subobject) {
            merge_subobject_special_members(tag, subobject, states, model);
            tag->detail.class_info->flags2 &= subobject->detail.class_info->flags2 | 0xfe;
            tag->detail.class_info->flags = (tag->detail.class_info->flags & ~8) |
                (subobject->detail.class_info->flags & 8);
        }
        if (is_const_or_reference_member(member)) {
            states[0].fields.can_generate = 0;
            states[0].fields.nontrivial = 1;
            states[2].fields.can_generate = 0;
            states[2].fields.nontrivial = 1;
        }
    }
}

/* Original 0x36ea0..0x36ed0. Only const qualification or a direct reference
   prevents the synthesized default construction/assignment of this member. */
int __stdcall is_const_or_reference_member(CppSymbol *member)
{
    TypeShape *shape;
    if (type_qualifiers(member->type) & 0x1000) return 1;
    shape = member->type->related;
    return shape && (shape->flags & 0x40) ? 1 : 0;
}

/* Original 0x36ae0..0x36cc0. Inspect member overload sets in source order.
   A declaration with no associated this type retains the native model -1,
   whose cv-byte access aliases the preceding class ambiguity byte. */
void __stdcall inspect_declared_special_members(CppSymbol *tag, SpecialMemberState *states,
    int *constructors, int *assignments)
{
    CppSymbol *member, *next_member, *function, *next_function;
    TypeShape *shape;
    FunctionParameters *parameters;
    unsigned nonprivate;
    unsigned char *qualifiers;
    int model;
    *assignments = 0;
    *constructors = 0;
    for (member = tag->auxiliary.scope_members; member; member = next_member) {
        next_member = member->next;
        if ((member->kind & 31) != 17 || (member->flags & 0x80)) continue;
        for (function = ((OverloadSet *)member)->functions; function; function = next_function) {
            next_function = function->next;
            model = -1;
            nonprivate = (function->flags & 3) != 1;
            *constructors += (function->declaration_flags & 0x1000) != 0;
            *assignments += function->name == name_assignment;
            shape = function->type->related;
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            parameters = shape->data.parameters;
            if (parameters->associated_type) {
                shape = function->type->related;
                if (shape && !(shape->flags & 0x77)) shape = shape->related;
                model = special_member_model(shape->data.parameters->associated_type->related->attributes & 7);
            }
            if (function->cpp.parts.member_flags & 0x10)
                record_special_member(tag, states, 0, model, nonprivate, 520);
            if (function->cpp.parts.member_flags & 0x20) {
                record_special_member(tag, states, 1, model, nonprivate, 521);
                qualifiers = &tag->detail.class_info->special_members.bytes[3 + model];
                *qualifiers = (*qualifiers & ~3) | ((function->cpp.parts.member_qualifiers >> 6) & 3);
            } else if (function->cpp.parts.member_flags & 0x40) {
                record_special_member(tag, states, 2, model, nonprivate, 522);
                qualifiers = &tag->detail.class_info->special_members.bytes[3 + model];
                *qualifiers = (*qualifiers & ~12) | ((function->cpp.parts.member_qualifiers >> 4) & 12);
            } else if (function->declaration_flags & 0x2000)
                record_special_member(tag, states, 3, model, nonprivate, 523);
        }
    }
}

/* The complete ClassInfo byte view admits the original model-prefix aliases
   while retaining named field offsets inside its final trait word. */
#define CLASS_SPECIAL_BYTE(info_, field_, model_) \
    (((unsigned char *)(info_))[offsetof(ClassInfo, special_members) + \
        offsetof(ClassSpecialFields, field_) + (model_)])

/* Original 0x36970..0x36adb. Seven stack slots are retained even though
   unused_count is not read by this build. One flat pointer model is supported. */
void __stdcall complete_special_member(CppSymbol *tag, SpecialMemberState *states,
    int kind, int model, unsigned warning, int enabled, int unused_count)
{
    SpecialMemberState *state = &states[kind];
    unsigned shift = (unsigned)kind & 31;
    unsigned char bit = (unsigned char)(1U << shift), *qualifiers;
    if (state->fields.declared) {
        tag->detail.class_info->special_members.parts.available |=
            (unsigned)state->fields.available << shift;
        tag->detail.class_info->special_members.parts.nonprivate |=
            (unsigned)state->fields.nonprivate << shift;
        tag->detail.class_info->special_members.parts.ambiguous |=
            (unsigned)state->fields.ambiguous << shift;
        return;
    }
    if (!enabled) return;
    if (!state->fields.nontrivial) {
        tag->detail.class_info->trivial_special_members |= bit;
        tag->detail.class_info->special_members.parts.nonprivate |= bit;
        if (kind == 1) {
            qualifiers = &tag->detail.class_info->special_members.parts.qualifiers;
            *qualifiers = (*qualifiers & ~2) | 1;
        } else if (kind == 2) {
            qualifiers = &tag->detail.class_info->special_members.parts.qualifiers;
            *qualifiers = (*qualifiers & ~8) | 4;
        }
        create_implicit_special_member(tag, kind, model, 0, kind == 2);
        return;
    }
    if (!state->fields.can_generate) {
        compiler_warning(4, warning, tag->name);
        return;
    }
    tag->detail.class_info->implicit_member_requirements |= bit;
    CLASS_SPECIAL_BYTE(tag->detail.class_info, available, model) |= bit;
    CLASS_SPECIAL_BYTE(tag->detail.class_info, nonprivate, model) |= bit;
    if (kind == 1) {
        qualifiers = &CLASS_SPECIAL_BYTE(tag->detail.class_info, qualifiers, model);
        *qualifiers = (*qualifiers & ~3) | (state->fields.qualifiers & 3);
    } else if (kind == 2) {
        qualifiers = &CLASS_SPECIAL_BYTE(tag->detail.class_info, qualifiers, model);
        *qualifiers = (*qualifiers & ~12) | ((state->fields.qualifiers << 2) & 12);
    }
    create_implicit_special_member(tag, kind, model, 0, 1);
}
