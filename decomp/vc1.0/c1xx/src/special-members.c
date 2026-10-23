/* C++ special-member synthesis after class member and virtual layout. */
#include "special-members.h"
#include "class-members.h"
#include "inline-capture.h"
#include "class-type-sets.h"
#include <recon.h>

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
                model = special_member_model((unsigned short)(shape->data.parameters->associated_type->related->attributes & 7));
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

/* Original code 0x374d0..0x37729, seven-entry switch table 0x3772c..0x37747.
   Build the implicit declarator through the same parameter/type/member paths
   as a user declaration, preserving its permanent symbol and arena type. */
CppSymbol *__stdcall create_implicit_special_member(CppSymbol *tag, int kind, int model,
    unsigned short attributes, int generated)
{
    CppSymbol *member, *parameters = 0;
    CType *return_type;
    ArenaPool *pool;
    DeclarationType declaration;
    unsigned char traits;
    unsigned short pointer_kind, qualifiers;
    pointer_kind = generated_member_pointer_kind(model);
    member = allocate_permanent(sizeof(CppSymbol));
    member->flags |= 0x80;
    member->cpp.parts.token = 1;
    member->cpp.parts.member_flags |= 1;
    member->flags |= 3;
    member->cpp.parts.member_flags = (member->cpp.parts.member_flags & ~8) |
        (((unsigned)generated << 3) & 8);
    member->cpp.parts.low_flags = (unsigned char)kind;
    member->name = generated_member_name(kind);
    switch (kind) {
    case 0:
        member->name = tag->name;
        member->cpp.parts.member_flags |= 0x10;
        member->declaration_flags |= 0x1000;
        break;
    case 1:
        member->name = tag->name;
        member->cpp.parts.member_flags |= 0x20;
        member->declaration_flags |= 0x1000;
        traits = CLASS_SPECIAL_BYTE(tag->detail.class_info, qualifiers, model);
        qualifiers = 0;
        if (traits & 1) qualifiers |= 0x1000;
        if (traits & 2) qualifiers |= 0x2000;
        parameters = make_copy_parameter(pointer_kind, qualifiers);
        break;
    case 2:
        member->cpp.parts.token = 0x3d;
        member->cpp.parts.member_flags |= 0x40;
        traits = CLASS_SPECIAL_BYTE(tag->detail.class_info, qualifiers, model);
        qualifiers = 0;
        if (traits & 4) qualifiers |= 0x1000;
        if (traits & 8) qualifiers |= 0x2000;
        parameters = make_copy_parameter(pointer_kind, qualifiers);
        break;
    case 3:
        member->name = tag->name;
        member->declaration_flags |= 0x2000;
        break;
    case 5: case 6:
        add_declaration_parameter(1, &parameters, name_flags, type_4064_first);
        break;
    }
    if (!parameters) parameters = make_ellipsis_parameter();
    if (!(token_allocation_flags & 0x20)) {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(CType)) refill_arena(&compiler_arena, pool);
        member->type = (CType *)pool->next;
        pool->remaining -= sizeof(CType);
        pool->next += sizeof(CType);
    } else member->type = allocate_permanent(sizeof(CType));
    pending_function_qualifiers = pointer_kind;
    member = attach_function_parameters(member, parameters);
    member->kind = 3;
    if (kind == 2) {
        attach_type_shape(member, new_type_shape(0x40, pointer_kind, 0));
        return_type = class_contexts[current_class_context].type;
    } else {
        return_type = type_0080;
        if (kind > 4 && kind < 7) {
            attach_type_shape(member, new_type_shape(1, pointer_kind, 0));
            return_type = type_0080;
        }
    }
    declaration.storage[0] = (declaration_storage[0] & ~7) | 0x10;
    declaration.storage[1] = declaration_storage[1];
    declaration.storage[2] = declaration_storage[2];
    declaration.type = return_type;
    declaration_spec_flags[declaration_depth] |= 1;
    member->type->attributes |= attributes;
    declare_class_member(&declaration, member);
    if ((member->storage[2] & 12) == 12 && generated) defer_generated_member(member);
    return member;
}

/* Original 0x37020..0x37032. Both special-member models use the near
   pointer kind in this flat-model build; the result ABI is unsigned short. */
unsigned short __stdcall generated_member_pointer_kind(int context)
{
    if (context) return 1;
    return 1;
}

/* Original 0x37460..0x374af, eight-entry switch table 0x374b0..0x374cf.
   Return the existing interned identities shared by mangling and lookup. */
char *__stdcall generated_member_name(int kind)
{
    switch (kind) {
    case 0: case 1: return name_ctor;
    case 2: return name_assignment;
    case 3: return name_dtor;
    case 4: return name_vbase_dtor;
    case 5: return name_delete_dtor;
    case 6: return name_vector_delete_dtor;
    case 7: return name_default_ctor_closure;
    default: return 0;
    }
}

/* Original 0x37820..0x3797e. Build the internal reference parameter and
   its circular parameter link, applying cv qualification to the class type. */
CppSymbol *__stdcall make_copy_parameter(unsigned short pointer_kind, unsigned short qualifiers)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    CppSymbol *parameter;
    TypeDescription *description;
    CType *type;
    if (pool->remaining < (int)sizeof(CppSymbol)) refill_arena(&compiler_arena, pool);
    parameter = (CppSymbol *)pool->next;
    pool->remaining -= sizeof(CppSymbol);
    pool->next += sizeof(CppSymbol);
    parameter->kind = 10;
    parameter->name = name_that;
    if (!(token_allocation_flags & 0x20)) {
        if (pool->remaining < (int)sizeof(CType)) refill_arena(&compiler_arena, pool);
        parameter->type = (CType *)pool->next;
        pool->remaining -= sizeof(CType);
        pool->next += sizeof(CType);
    } else parameter->type = allocate_permanent(sizeof(CType));
    if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_symbol_rollover();
    parameter->index = next_symbol_index++;
    attach_type_shape(parameter, new_type_shape(0x40, pointer_kind, 0));
    parameter->storage[0] = (declaration_storage[0] & ~7) | 5;
    parameter->storage[1] = declaration_storage[1];
    parameter->storage[2] = declaration_storage[2];
    if (qualifiers) {
        description = copy_type_description(class_contexts[current_class_context].type);
        description->attributes = qualifiers;
        type = intern_type(description);
        --type_description_depth;
        normalize_symbol_type(type, parameter);
    } else normalize_symbol_type(class_contexts[current_class_context].type, parameter);
    parameter->detail.parameter_next = parameter;
    return parameter;
}

/* Original 0x37c10..0x37c52. A trivial member is usable in the requested
   model directly; other members must be present in the flat-model trait slot. */
int __stdcall find_special_member_model(CppSymbol *tag, unsigned char kind, int first)
{
    unsigned mask = 1U << (kind & 31);
    if (tag->detail.class_info->trivial_special_members & mask) return first;
    while (first < 1) {
        if (CLASS_SPECIAL_BYTE(tag->detail.class_info, available, first) & mask) return first;
        ++first;
    }
    return -1;
}

CppSymbol *__stdcall find_class_special_member(CppSymbol *tag, char *name);
void __stdcall make_default_constructor_closure(CppSymbol *tag, CppSymbol *constructor, int model);
void __stdcall make_deleting_destructors(CppSymbol *tag, CppSymbol *destructor, int model);

/* Original 0x37070..0x37170. Create at most one closure for each pointer
   model when a nontrivial default constructor takes explicit default arguments. */
void __stdcall declare_default_constructor_closures(CppSymbol *tag)
{
    CppSymbol *member, *next;
    TypeShape *shape;
    unsigned created = 0, mask;
    int model;
    if (tag->detail.class_info->trivial_special_members & 1) return;
    member = find_class_special_member(tag, name_ctor);
    if (!member) return;
    if ((member->kind & 31) == 16 || (member->kind & 31) == 17)
        member = ((OverloadSet *)member)->functions;
    while (member && ((member->kind & 31) == 6 || (member->kind & 31) == 7)) {
        next = member->next;
        shape = member->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        model = special_member_model((unsigned short)(shape->data.parameters->associated_type->related->attributes & 7));
        if (member->cpp.parts.member_flags & 0x10) {
            shape = member->type->related;
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            if (shape->data.parameters->default_count) {
                mask = 1U << (model & 31);
                if (!(created & mask)) {
                    make_default_constructor_closure(tag, member, model);
                    created |= mask;
                }
            }
        }
        member = next;
    }
}


/* Original 0x37270..0x37344. A nontrivial destructor needs its deleting
   forms once per pointer model, independent of overload-chain order. */
void __stdcall declare_deleting_destructors(CppSymbol *tag)
{
    CppSymbol *member, *next;
    TypeShape *shape;
    unsigned created = 0, mask;
    int model;
    if (tag->detail.class_info->trivial_special_members & 8) return;
    member = find_class_special_member(tag, name_dtor);
    if (!member) return;
    if ((member->kind & 31) == 16 || (member->kind & 31) == 17)
        member = ((OverloadSet *)member)->functions;
    while (member && ((member->kind & 31) == 6 || (member->kind & 31) == 7)) {
        next = member->next;
        shape = member->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        model = special_member_model((unsigned short)(shape->data.parameters->associated_type->related->attributes & 7));
        mask = 1U << (model & 31);
        if (!(created & mask)) {
            make_deleting_destructors(tag, member, model);
            created |= mask;
        }
        member = next;
    }
}

/* Original 0x37b30..0x37b3e. Constructor availability in the type's model. */
int __stdcall has_class_initialization(CType *type)
{
    return has_special_member_model(type, 0);
}

/* Original 0x37bd0..0x37c08. Resolve the class and its effective pointer
   model before testing one of the four special-member categories. */
int __stdcall has_special_member_model(CType *type, unsigned char kind)
{
    CppSymbol *tag = cpp_context_stack->items[type->field08];
    int model = special_member_model(type_pointer_model(type));
    return find_special_member_model(tag, kind, model) != -1;
}

/* Original 0x37b60..0x37bce, cdecl with a sixteen-bit result. A leading
   based modifier is ignored when choosing the effective pointer model. */
unsigned short type_pointer_model(CType *type)
{
    TypeShape *shape = type->related;
    CppSymbol *tag;
    unsigned short model;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    model = (shape ? shape->attributes : type->attributes) & 7;
    if (!model) {
        tag = cpp_context_stack->items[type->field08];
        if (tag && (tag->kind & 31) == 4)
            return tag->detail.class_info->pointer_attributes;
        model = (unsigned short)data_pointer_kind;
    }
    return model;
}

InlineVector *generated_member_queue; /* original 0x9f1c0 */

/* Original 0x379a0..0x379f3. Queue a synthesized member once, clearing
   its pending bit. The empty-vector factory is shared with class type sets. */
void __stdcall defer_generated_member(CppSymbol *member)
{
    InlineVector *queue;
    if ((member->kind & 31) == 7 && (member->cpp.parts.member_flags & 8)) {
        if (!generated_member_queue) generated_member_queue = new_type_set();
        queue = generated_member_queue;
        if (queue->count == queue->capacity) queue = grow_inline_vector(queue);
        queue->items[queue->count++] = member;
        generated_member_queue = queue;
        member->cpp.parts.member_flags &= ~8;
    }
}

/* Original 0x37a00..0x37afe. Generate queued special-member text under
   the shared generated-member flag, then release the exhausted pointer list. */
int emit_pending_runtime_functions(void)
{
    TypeSetIterator iterator;
    CppSymbol *member, *owner;
    int kind, model;
    unsigned short pointer_kind;
    if (error_count || !generated_member_queue) return 0;
    token_allocation_flags |= 0x08000000UL;
    begin_type_set(&iterator, generated_member_queue);
    while (next_type_set(&iterator)) {
        /* This shared inline-vector cursor also carries function symbols. */
        member = (CppSymbol *)iterator.type;
        kind = (signed char)member->cpp.parts.low_flags;
        if (kind == -1) continue;
        if (kind < 8 || kind > 10) {
            owner = cpp_context_stack->items[member->auxiliary.declaring_class];
            pointer_kind = member->type->related->attributes & 7;
            if (!pointer_kind) pointer_kind = normalize_type_attributes(4, 0) & 7;
            model = generated_member_model(member);
            kind = (signed char)member->cpp.parts.low_flags;
        } else {
            owner = member;
            pointer_kind = 0;
            model = -1;
        }
        capture_inline_member_declaration(owner, kind, model, pointer_kind);
    }
    token_allocation_flags &= ~0x08000000UL;
    release_inline_vector(generated_member_queue);
    generated_member_queue = 0;
    return 1;
}
