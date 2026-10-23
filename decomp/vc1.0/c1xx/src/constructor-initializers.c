/* Constructor base/member initialization, original 0x35f90..0x363c3. */
#include "constructor-initializers.h"
#include "statement-scopes.h"
#include "class-members.h"
#include "class-type-sets.h"
#include "member-expressions.h"
#include "cpp-context.h"
#include "inline-vector.h"
#include "class-lookup.h"
#include "special-members.h"
#include "expression-preparation.h"
#include "operators.h"
#include "unary-expressions.h"
#include "call-expressions.h"
#include "object-initializers.h"
#include "macros.h"
#include "type-sizes.h"
#include "special-member-lookup.h"
#include "expression-records.h"
#include <recon.h>

CType *__stdcall runtime_callback_type(unsigned short pointer_attributes,
    unsigned short function_attributes);

/* Original 0x37f20..0x37f4b, stdcall 4. A zero-valued object carrying
   the constructor's associated pointer type exercises ordinary access checks
   without emitting an object construction. */
IrToken *__stdcall make_constructor_access_probe(CType *object_type)
{
    IrToken *object = new_integer_constant(0);
    CppSymbol *tag;
    object->type = object_type;
    tag = cpp_context_stack->items[object_type->field08];
    return make_constructor_call(tag, object, 0, 0, 1);
}

/* Original 0x386b0..0x38716, stdcall 4. Referenced vector callbacks
   retain the destructor declaration and use the runtime's pointer signature. */
IrToken *__stdcall make_array_destructor_address(CType *type)
{
    CppSymbol *destructor = lookup_complete_object_destructor(type);
    TypeShape *shape;
    IrToken *reference;
    unsigned short pointer_attributes, function_attributes;
    ++destructor->references;
    defer_generated_member(destructor);
    shape = destructor->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    pointer_attributes = shape->data.parameters->associated_type->related->attributes & 7;
    function_attributes = (destructor->type->related->attributes & 7) | 0x70;
    reference = symbol_reference(destructor);
    reference->type = runtime_callback_type(pointer_attributes, function_attributes);
    return reference;
}

/* Original 0x38680..0x386ad, stdcall 16. Retain the runtime helper
   lookup before constructing the destructor callback's typed address. */
IrToken *__stdcall make_vector_destructor_call(IrToken *object, unsigned element_bytes,
    IrToken *element_count, CType *element_type)
{
    IrToken *helper = identifier_expression(name_vector_dtor);
    IrToken *destructor = make_array_destructor_address(element_type);
    return make_vector_helper_call(object, element_bytes, element_count, destructor, helper);
}

/* Original 0x385f0..0x3867b, stdcall 4. Flatten nested array bounds
   using target unsigned multiplication, then pass the innermost element
   width and full count to the reverse-destruction helper. */
IrToken *__stdcall make_array_destructor_call(IrToken *object)
{
    TypeShape *shape = object->type->related, *following;
    CType *element;
    unsigned count = 1, element_bytes;
    IrToken *count_expression;
    while (shape && (following = shape->related) != 0 &&
        (following->flags & 0x77) == 2) {
        object = binary_expression(0x4a, object, make_integer_constant(0));
        count *= (unsigned)shape->data.array_bound;
        shape = following;
    }
    count *= (unsigned)shape->data.array_bound;
    element = drop_first_type_shape(object->type);
    element_bytes = inner_type_size(element->flags,
        cpp_context_stack->items[element->field08], element->related);
    count_expression = make_integer_constant(count);
    return make_vector_destructor_call(object, element_bytes, count_expression, element);
}

/* Original 0x37e80..0x37f11, stdcall 4. A constructor with default
   arguments needs a generated no-argument closure before it can serve as a
   vector callback. Access is probed through a typed null object first. */
IrToken *__stdcall make_array_constructor_address(CType *type)
{
    CppSymbol *constructor = find_default_constructor(type);
    TypeShape *shape;
    IrToken *reference;
    unsigned short pointer_attributes, function_attributes;
    if (!is_void_parameter_constructor(constructor))
        constructor = require_default_constructor_closure(constructor);
    defer_generated_member(constructor);
    shape = constructor->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    make_constructor_access_probe(shape->data.parameters->associated_type);
    shape = constructor->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    pointer_attributes = shape->data.parameters->associated_type->related->attributes & 7;
    function_attributes = (constructor->type->related->attributes & 7) | 0x70;
    reference = symbol_reference(constructor);
    reference->type = runtime_callback_type(pointer_attributes, function_attributes);
    return reference;
}

/* Original 0x37e00..0x37e73, stdcall 12. Array construction calls
   the vector helper with a callable constructor address. Virtual-base
   construction selects the helper carrying the extra most-derived argument. */
IrToken *__stdcall make_array_constructor_call(IrToken *object, int element_bytes,
    IrToken *element_count)
{
    CType *type = object->type;
    CppSymbol *tag = cpp_context_stack->items[type->field08];
    IrToken *constructor = make_array_constructor_address(type), *helper;
    int virtual_bases = 0;
    if (tag->detail.class_info->flags & 0x10)
        virtual_bases = is_void_parameter_constructor(find_default_constructor(type)) != 0;
    helper = identifier_expression(virtual_bases ? name_vector_ctor_vb : name_vector_ctor);
    return make_vector_helper_call(object, element_bytes, element_count,
        constructor, helper);
}

/* Original 0x64660..0x646eb, stdcall12. Reference binding carries
   normalized pointer distance, with a minimum nonexact rank of two. */
unsigned __stdcall rank_reference_attributes(unsigned rank, CType *target, CType *source)
{
    TypeShape *shape = target->related;
    unsigned short target_attributes = shape->attributes;
    unsigned short source_attributes, source_flags;
    if (!(target_attributes & 15))
        target_attributes = normalize_type_attributes(shape->flags, target_attributes);
    shape = source->related;
    if (shape && (shape->flags & 0x10)) return rank;
    source_attributes = source->attributes;
    if (!(source_attributes & 15)) {
        source_flags = 1;
        if (shape && ((shape->flags & 0x77) == 4 ||
            ((shape->flags & 0x77) == 3 && (shape->flags & 8))))
            source_flags = shape->flags;
        source_attributes = normalize_type_attributes(source_flags, source_attributes);
    }
    rank = rank_pointer_attributes(rank, (unsigned char)target_attributes,
        (unsigned char)source_attributes, 0);
    return rank < 3 ? 2 : rank;
}

typedef char InitializerClassTrivialOffsetMustBe54[
    (offsetof(ClassInfo, trivial_special_members) == 54) ? 1 : -1];
typedef char InitializerSymbolDeclarationFlagsOffsetMustBe2[
    (offsetof(CppSymbol, declaration_flags) == 2) ? 1 : -1];
typedef char ConversionParameterDeclarationsOffsetMustBe12[
    (offsetof(FunctionParameters, declarations) == 12) ? 1 : -1];
typedef char ConversionParameterAssociatedTypeOffsetMustBe0[
    (offsetof(FunctionParameters, associated_type) == 0) ? 1 : -1];

int __stdcall emit_constructor_initializers(CppSymbol *tag, IrToken *initializers)
{
    CppSymbol *member, *subobject_class, *target;
    BaseClassLink *base;
    IrToken *arguments, *expression, *initializer, *callee;
    TypeShape *shape;
    unsigned short attributes;
    unsigned displacement;
    int saved_access, member_subobject, copying, required;
    char *name;

    /* The native helper consumes this formal argument even though its body
       uses the global function_symbol. Keep its original stdcall 4 ABI. */
    attributes = constructor_pointer_attributes(function_symbol);
    prepare_class_initialization(tag);
    if (tag->detail.class_info->flags & 0x10) {
        saved_access = get_base_initialization_access();
        if (!saved_access) set_base_initialization_access(1);
        emit_virtual_base_initializers(tag, &initializers, attributes);
        if (!saved_access) set_base_initialization_access(0);
    }
    for (member = tag->auxiliary.scope_members; member; member = member->next) {
        if ((member->kind & 0x1f) == 17 || member->name == name_alignment ||
            is_static_data_member(member) || (member->flags & 0x10) ||
            (member->kind & 0x1f) == 5)
            continue;
        if ((member->kind & 0x1f) == 15) {
            base = (BaseClassLink *)member;
            if (base->state & 1) continue;
            subobject_class = cpp_context_stack->items[base->context_index];
            displacement = base->displacement;
            member_subobject = 0;
        } else {
            subobject_class = class_type_of_member(member);
            displacement = member->detail.value;
            member_subobject = subobject_class != 0;
        }
        arguments = take_member_initializer(&initializers, member->name);
        if (!arguments) {
            if (subobject_class) {
                if (!has_class_initialization(make_class_pointer_type(subobject_class, attributes))) {
                    report_syntax_error(512, subobject_class->name);
                    return 0;
                }
                if (member->flags & 0x40)
                    emit_class_array_initialization(member, subobject_class, 0,
                        displacement, 0, attributes);
                else {
                    if (!member_subobject) set_base_initialization_access(1);
                    make_constructor_call(subobject_class,
                        make_subobject_pointer(subobject_class, attributes, displacement),
                        0, 1, member_subobject);
                    set_base_initialization_access(0);
                }
                continue;
            }
            shape = member->type->related;
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            if (!shape) required = (member->type->attributes & 0x1000) != 0;
            else required = ((shape->attributes & 0x1000) && !(shape->flags & 3)) ||
                            (shape->flags & 0x40);
            if (required) report_syntax_error(758, member->name);
            continue;
        }
        if ((member->kind & 0x1f) == 3 && member->type->related) {
            shape = member->type->related;
            if (!(shape->flags & 0x77)) shape = shape->related;
            if (shape && (shape->flags & 0x77) == 2 &&
                (!subobject_class || (subobject_class->declaration_flags & 0x1000))) {
                copying = (function_symbol->cpp.parts.member_flags & 0x20) &&
                    (tag->detail.class_info->implicit_member_requirements & 2);
                if (copying) {
                    if (subobject_class && (member->flags & 0x40))
                        emit_class_array_initialization(member, subobject_class, arguments,
                            displacement, 1, attributes);
                    else
                        emit_array_copy_initialization(member, displacement, arguments, attributes);
                } else report_syntax_error(536, tag->name, member->name);
                continue;
            }
        }
        if (!subobject_class) {
            expression = prepare_expression(arguments, 1);
            mark_initialized_symbol(member);
            member_initializer_active = 1;
            if (!initialize_member_expression(displacement, member->type, expression, member))
                report_syntax_error(439, member->name);
            member_initializer_active = 0;
            initializing_symbol = 0;
            initializing_reference = 0;
        } else {
            if (!(subobject_class->declaration_flags & 0x1000)) {
                report_syntax_error(542, member->name);
                continue;
            }
            if (!member_subobject) set_base_initialization_access(1);
            expression = make_constructor_call(subobject_class,
                make_subobject_pointer(subobject_class, attributes, displacement),
                arguments, 0, member_subobject);
            expression = prepare_expression(expression, 0);
            if (expression) emit_expression_statement(expression);
            set_base_initialization_access(0);
        }
    }
    /* Entries left after consuming real subobjects are invalid or duplicate
       member initializers. Their designator retains the symbol's name. */
    for (initializer = initializers; initializer; initializer = initializer->tail.right) {
        expression = initializer->value.pointer;
        callee = expression->value.pointer;
        target = callee->value.pointer;
        name = target->name;
        for (member = tag->auxiliary.scope_members; member; member = member->next)
            if (member->name == name) break;
        if (!member) report_syntax_error(614, tag->name, name);
        else if ((member->kind & 0x1f) == 17) report_syntax_error(436, name);
        else if (is_static_data_member(member)) report_syntax_error(438, name);
        else report_syntax_error(437, name);
    }
    required = !(((function_symbol->cpp.parts.member_flags & 0x20) &&
                  (tag->detail.class_info->implicit_member_requirements & 2)) ||
                 ((function_symbol->cpp.parts.member_flags & 0x10) &&
                  (tag->detail.class_info->implicit_member_requirements & 1)));
    emit_class_table_initializers(tag, required);
    return 1;
}

/* Original 0x363d0..0x363fa. The unused formal is part of the original
   stdcall ABI; the selected function and implicit-this type are global. */
unsigned short __stdcall constructor_pointer_attributes(CppSymbol *unused_function)
{
    TypeShape *function = function_symbol->type->related;
    if (function && !(function->flags & 0x77)) function = function->related;
    return function->data.parameters->associated_type->related->attributes & 7;
}

InlineVector *classes_pending_table_output; /* original 0x9f3d0 */

/* Original 0x7b3f0..0x7b439. Queue classes with dispatch tables and make
   their deleting-destructor definitions available for initialization. */
void __stdcall prepare_class_initialization(CppSymbol *tag)
{
    if (tag->detail.class_info->alignment_flags & 0xc0) {
        if (!classes_pending_table_output)
            classes_pending_table_output = new_inline_vector(1);
        classes_pending_table_output = add_inline_pointer(classes_pending_table_output, tag);
    }
    require_deleting_destructor(tag);
}

/* Original 0x38bb0..0x38bc3. The deleting-destructor entry retains its
   own identity while sharing overload-definition processing by name. */
void __stdcall require_deleting_destructor(CppSymbol *tag)
{
    require_class_member_definitions(tag, name_delete_dtor);
}

/* Original 0x38b20..0x38ba6. Resolve a special-member overload set and
   defer each generated definition while retaining the original traversal. */
void __stdcall require_class_member_definitions(CppSymbol *tag, char *name)
{
    CppSymbol *member = find_class_special_member(tag, name), *current;
    if (member && ((member->kind & 0x1f) == 16 || (member->kind & 0x1f) == 17))
        member = ((OverloadSet *)member)->functions;
    while (member && ((member->kind & 0x1f) == 6 || (member->kind & 0x1f) == 7)) {
        current = member;
        member = member->next;
        if (current->storage[1] & 2) defer_generated_member(current);
    }
}

/* Original 0x364c0..0x3653c. Consume one matching initializer while
   restoring any symbol records detached with that expression. */
IrToken *__stdcall take_member_initializer(IrToken **initializers, char *name)
{
    IrToken **link, *item, *expression, *callee, *arguments;
    ExpressionSymbolList *symbols;
    CppSymbol *target;
    for (link = initializers; *link; link = &item->tail.right) {
        item = *link;
        symbols = 0;
        expression = item->value.pointer;
        if (expression && expression->kind == 0xc8) {
            symbols = (ExpressionSymbolList *)expression->tail.right;
            expression = expression->value.pointer;
        }
        callee = expression->value.pointer;
        target = callee->value.pointer;
        if (target->name == name) {
            *link = item->tail.right;
            arguments = expression->tail.right;
            if (!options.prototypes_only && symbols) restore_expression_symbols(symbols);
            return arguments;
        }
    }
    return 0;
}

/* Original 0x36540..0x365d2. Address a member relative to the adjusted
   this expression, preserving optional bit-field position/width metadata. */
int __stdcall initialize_member_expression(unsigned displacement, CType *type,
    IrToken *value, CppSymbol *member)
{
    IrToken *object, *offset, *target, *position, *width, *range;
    CType *member_type;
    object = adjusted_this_expression(0);
    member_type = replace_type_pointer_kind(type, (unsigned short)(object->type->related->attributes & 7));
    offset = make_integer_constant(displacement);
    target = new_ir_expression(0x5c, object, offset, member_type);
    if (member && (member->kind & 0x1f) == 2) {
        width = make_integer_constant((member->auxiliary.value >> 8) & 0xff);
        position = make_integer_constant(member->auxiliary.value & 0xff);
        range = new_left_typed_expression(0x3f, position, width);
        target = new_left_typed_expression(0x3e, target, range);
    }
    if (value->kind == 0x6a && !value->tail.right) value = value->value.pointer;
    return initialize_lvalue(target, value);
}

/* Original 0x347b0..0x347ee. An adjusted member function subtracts its
   recorded this displacement and marks the resulting address expression. */
IrToken *__stdcall adjusted_this_expression(CppSymbol *parameter)
{
    IrToken *object = this_pointer_expression(parameter), *offset;
    if (!object) return 0;
    if (function_symbol->field28) {
        offset = make_integer_constant(function_symbol->field28);
        object = new_left_typed_expression(0x4f, object, offset);
        object->flags |= 0x20;
    }
    return object;
}

/* Original 0x63680..0x636b6. Replace only the low pointer-kind bits in
   a copied type description, then release that description-stack entry. */
CType *__stdcall replace_type_pointer_kind(CType *type, unsigned short kind)
{
    TypeDescription *description = copy_type_description(type);
    CType *result;
    description->attributes &= 0xfff8;
    description->attributes |= kind & 7;
    result = intern_type(description);
    --type_description_depth;
    return result;
}

/* Original 0x46040..0x46124: initialize an already selected lvalue. */
int __stdcall initialize_lvalue(IrToken *target, IrToken *value)
{
    CType *target_type, *value_type;
    unsigned rank;
    if (!target) return 0;
    target_type = target->type;
    if (!value) return 0;
    value_type = value->type;
    rank = classify_initializer_conversion(target_type, value);
    if (rank < 0x400) {
        value = convert_initializer(0x7e, target_type, value);
        value = prepare_reference_initializer(target_type, value);
        if (!value) return 0;
        target = indirect_storage_expression(target);
        value = builtin_binary_expression(0x7e, target, value);
        if (initializer_list_started || initializing_symbol)
            value = prepare_expression(value, 2);
        emit_expression_statement(value);
        return 1;
    }
    if (report_operator_resolution_failure(0x7e, 2, rank, target_type, value_type))
        return 0;
    if (target_type && value_type) {
        report_type_conversion(440, 0, 0x7e, value_type, target_type);
        return 0;
    }
    report_type_error(592, target_type);
    return 0;
}

/* Original 0x65ca0..0x65cb6: ordinary initialization uses no extra
   conversion flags and no user-conversion search context. */
unsigned __stdcall classify_initializer_conversion(CType *type, IrToken *value)
{
    return classify_expression_conversion(type, value, 0, 0);
}

/* Original 0x65310..0x653d3. An untyped overload expression needs a
   target-directed lookup; typed expressions use ordinary type ranking. */
unsigned __stdcall classify_expression_conversion(CType *type, IrToken *value,
    unsigned flags, OverloadCandidate *context)
{
    CType *source;
    TypeShape *shape;
    if (!value) return 0x2000;
    if (!type) return 0x400;
    source = value->type;
    shape = type->related;
    if (source) {
        if ((value->flags & 0x80) && shape && !(shape->flags & 0x40))
            return 0x20;
        if (shape && (shape->flags & 0x77) == 1 && source->related &&
            (source->related->flags & 0x77) == 2)
            source = parameter_reference_type(value);
        return classify_type_conversion(type, source, flags, context);
    }
    if (shape && ((shape->flags & 0x77) == 3 || (shape->flags & 0x20))) {
        if (resolve_overloaded_initializer(value, type, 0)) return 1;
        return 0x200b;
    }
    return 0x2000;
}

/* Original 0x64f20..0x65270. Rank standard scalar conversions after
   reference/based-type adjustment, then permit user conversions. */
unsigned __stdcall classify_type_conversion(CType *target, CType *source,
    unsigned flags, OverloadCandidate *context)
{
    unsigned rank;
    unsigned short destination_flags, source_flags;
    target = strip_based_shape(target);
    source = strip_based_shape(source);
    if (source == target) return 1;
    rank = classify_special_type_conversion(target, source, flags, context);
    if (rank) return rank;
    rank = classify_derived_type_conversion(target, source, flags, context);
    if (rank) return rank;
    destination_flags = target->flags;
    source_flags = source->flags;
    if (((destination_flags & 0x9c0) || target->related) &&
        ((source_flags & 0x9c0) || source->related)) {
        if (source_flags == destination_flags) return 1;
        if (source_flags & 0x80) return 0x2000;
        if ((source_flags & 0x40) && (destination_flags & 0x40)) {
            if (!((source_flags ^ destination_flags) & 0xefff)) return 1;
            if (((source_flags & 0x2fdf) == 0x41 ||
                 (source_flags & 0x2fdf) == 0x42) &&
                (destination_flags & 0x20) && !(destination_flags & 0x4000) &&
                (!options.configuration.option_J ||
                 ((destination_flags & 0x2fdf) == 0x41 &&
                  (destination_flags & 0x8000))))
                return 0x10;
            return 0x20;
        }
        if ((source_flags & 0x100) && (destination_flags & 0x100)) {
            if (!((source_flags ^ destination_flags) & 0xf) &&
                !!(source_flags & 0x20) == !!(destination_flags & 0x20))
                return 1;
            if ((source_flags & 0xf) < (destination_flags & 0xf) &&
                (destination_flags & 0xf) < 9)
                return 0x10;
            return 0x20;
        }
        if (destination_flags & 0x800)
            return (source_flags & 0x800) ? 1 : 0x2000;
        return 0x20;
    }
    if ((destination_flags | source_flags) & 0x200) {
        if (destination_flags & 0x200) {
            if (source_flags & 0x200)
                return target->field08 == source->field08 ? 1 : 0x2000;
        } else if ((source_flags & 0x200) && (destination_flags & 0x140)) {
            if ((destination_flags & 0x20) && (destination_flags & 0x40))
                return 0x10;
            return 0x20;
        }
    }
    if (!((source->flags ^ target->flags) & 0xefff) &&
        target->field08 == source->field08) {
        if (!(flags & 4)) return 1;
        if ((!(source->attributes & 0x1000) || (target->attributes & 0x1000)) &&
            (!(source->attributes & 0x2000) || (target->attributes & 0x2000)))
            return 1;
    }
    if (classify_user_conversion((unsigned char)flags, source, target, context, &rank))
        return rank;
    return 0x2000;
}

/* Original 0x64070..0x64111: special destination forms precede the
   ordinary derived-type and arithmetic ranking paths. */
unsigned __stdcall classify_special_type_conversion(CType *target, CType *source,
    unsigned flags, OverloadCandidate *context)
{
    unsigned short kind;
    unsigned rank;
    kind = target->flags & 0x2fdf;
    if (kind == 0x40 || kind == 0x140)
        return classify_special_scalar_conversion(target, source, (unsigned char)flags, context);
    if (kind == 0x84) {
        rank = classify_derived_type_conversion(type_indirect_0080_2, source, flags, context);
        if ((int)rank < 0x400) return 4;
        if ((flags & 1) && has_pointer_or_member_pointer_shape(source)) return 4;
        return 0x2000;
    }
    return 0;
}

/* Original 0x646f0..0x64d9f: derived declarator shapes, including
   references, pointers, arrays, member pointers and function signatures. */
unsigned __stdcall classify_derived_type_conversion(CType *target, CType *source,
    unsigned flags, OverloadCandidate *context)
{
    TypeShape *destination_shape, *source_shape, *destination_function, *source_function;
    CType *destination_referent, *source_referent;
    unsigned short destination_flags, source_flags, destination_kind;
    unsigned short destination_shape_flags, source_shape_flags, source_attributes;
    unsigned rank, result;
    destination_shape = target->related;
    source_shape = source->related;
    if (!destination_shape && !source_shape) return 0;
    destination_flags = target->flags;
    source_flags = source->flags;
    rank = 1;
    if (!destination_shape) {
        if (source_shape->flags & 0x40) {
            source_referent = dereference_pointer_type(source);
            rank = classify_type_conversion(target, source_referent, flags, context);
            if (rank < 0x400) return rank > 2 ? rank : 2;
        }
        if (!(destination_flags & 0x400)) return 0x2000;
        if (classify_user_conversion((unsigned char)flags, source, target, context, &rank))
            return rank;
        return 0x2000;
    }
    destination_shape_flags = destination_shape->flags;
    if ((destination_flags & 0x80) && (destination_shape_flags & 0x77) != 4)
        rank = classify_null_pointer_shape(1, destination_shape, source_shape);
    destination_kind = destination_shape_flags & 0x77;
    if (destination_kind == 0x40 && (!source_shape || !(source_shape->flags & 0x40))) {
        destination_referent = dereference_pointer_type(target);
        rank = classify_class_reference_conversion(destination_referent, source);
        if (rank < 0x400) {
            rank = rank_conversion_qualifiers(rank, target->attributes, source->attributes);
            if (rank < 0x400) return rank_reference_attributes(rank, target, source);
        }
        if (classify_source_class_conversion((unsigned char)flags, source, target, context, &rank)
            && rank < 0x400)
            return rank;
        flags |= 4;
        rank = classify_type_conversion(destination_referent, source, flags, context);
        if (!(rank & 0x200)) {
            if (rank < 0x400) {
                rank = rank_conversion_qualifiers(rank, target->attributes, source->attributes);
                if (rank < 0x400) return rank_reference_attributes(rank, target, source);
            }
        } else if ((target->attributes & 0x1000) || (flags & 2)) {
            return rank;
        }
        if (destination_flags & 0x400) return 0x2000;
    }
    if (!source_shape) {
        if (classify_source_class_conversion((unsigned char)flags, source, target, context, &rank))
            return rank;
        return 0x2000;
    }
    source_shape_flags = source_shape->flags;
    if ((source_shape_flags & 0x40) && !(destination_shape_flags & 0x40)) {
        source_referent = dereference_pointer_type(source);
        rank = classify_class_reference_conversion(target, source_referent);
        if (rank & 0x2000)
            rank = classify_type_conversion(target, source_referent, flags, context);
        if (rank < 0x400) {
            if (!(flags & 2)) {
                rank = rank_conversion_qualifiers(rank, target->attributes, source->attributes);
                if (rank & 0x2000) return rank;
            }
            return rank_reference_attributes(rank, target, source);
        }
    }
    if ((destination_shape_flags & 0x30) && (source_shape_flags & 0x30)) {
        rank = classify_member_pointer_conversion(source, target, 0);
        if (rank & 0x2000) return rank;
    } else if (rank != 0x100 && destination_shape->related != source_shape->related) {
        rank = classify_nested_shape_conversion(destination_shape->related, source_shape->related);
        if (rank & 0x2000) return 0x2000;
    }
    if (!destination_shape->related) {
        if (rank == 0x100 && source->related && source->related->related)
            source_attributes = source->related->related->attributes;
        else
            source_attributes = source->attributes;
        rank = rank_conversion_qualifiers(rank, target->attributes, source_attributes);
        if (rank & 0x2000) {
            if (!(cpp_parser_flags & 2) || (flags & 1) ||
                ((rank & 0x1fff) != 5 && (rank & 0x1fff) != 6))
                return 0x2000;
            rank = 1;
        }
    } else if ((source->attributes ^ target->attributes) & 0x3000) {
        return 0x2000;
    }
    rank = rank_pointer_attributes(rank, (unsigned char)destination_shape->attributes,
        (unsigned char)source_shape->attributes, source_shape);
    if (rank & 0x2000) return 0x2000;
    if (!((destination_shape_flags ^ source_shape_flags) & 0x77)) {
        if (destination_kind == 2 && destination_shape->data.word &&
            source_shape->data.word != destination_shape->data.word)
            return 0x2000;
    } else if ((destination_kind == 1 && (source_shape_flags & 0x77) == 2) ||
               (destination_kind == 2 && (source_shape_flags & 0x77) == 1)) {
        if (rank == 1) rank = 2;
        else rank |= 2;
    } else if (!(destination_kind == 1 && (source_shape_flags & 0x77) == 3 && (rank & 0x100)) &&
               !((destination_shape_flags & 0x44) == 0x44 && (source_shape_flags & 8))) {
        return 0x2000;
    }
    if ((destination_kind == 3 || destination_kind == 4) &&
        ((source_shape_flags & 0x77) == 3 || (source_shape_flags & 0x77) == 4)) {
        destination_function = destination_kind == 3 ? destination_shape->related : destination_shape;
        source_function = (source_shape_flags & 0x77) == 3 ? source_shape->related : source_shape;
        if (source_function->data.parameters->declarations !=
            destination_function->data.parameters->declarations)
            return 0x2000;
        if ((source_function->attributes ^ destination_function->attributes) & 0xf0ff)
            return 0x2000;
        if ((source_function->attributes ^ destination_function->attributes) & 0x100)
            return 0x2000;
    }
    if ((destination_shape_flags & 0x2000) && (source_shape_flags & 0x2000) &&
        !type_has_base_relation(destination_shape->data.parameters->associated_type,
                                source_shape->data.parameters->associated_type))
        return 0x2000;
    if (!((target->flags ^ source->flags) & 0xefff) && source->field08 == target->field08)
        return rank;
    if (rank & 0x100) return rank;
    if ((destination_flags & 0x9c0) || (source_flags & 0x9c0)) return 0x2000;
    if (destination_kind != 1 || (source_shape_flags & 0x77) != 1) return 0x2000;
    source_referent = dereference_pointer_type(source);
    destination_referent = dereference_pointer_type(target);
    result = classify_class_reference_conversion(destination_referent, source_referent);
    if (result & 0x2000) return result;
    return rank | result;
}

/* Original 0x665a0..0x66717. Retain source qualifiers, class-copy
   construction and declarator-shape conversion before scalar conversion. */
IrToken *__stdcall convert_initializer(unsigned char operation, CType *type, IrToken *expression)
{
    CType *source, *destination;
    CppSymbol *tag, *constructor;
    TypeDescription *description;
    if (!expression) return 0;
    source = strip_based_shape(expression->type);
    if (!source) return resolve_assignment_operand(type, expression);
    destination = strip_based_shape(type);
    if (destination->related || source->related) {
        if (source == destination) return expression;
        return convert_derived_initializer(destination, expression);
    }
    if (same_unqualified_class_type(destination, source)) {
        tag = cpp_context_stack->items[destination->field08];
        if (!(tag->flags & 4)) {
            if (!options.prototypes_only) report_syntax_error(27, tag->name);
            tag->declaration_flags |= 2;
            return 0;
        }
        if (tag->detail.class_info->trivial_special_members & 2) return expression;
        constructor = find_copy_constructor(destination);
        if (!constructor) {
            report_syntax_error(558, tag->name);
            return expression;
        }
        return construct_copy_initializer(constructor, destination, expression);
    }
    if (source == destination) {
        expression->flags &= 0x7f;
        return expression;
    }
    if (!((source->flags ^ destination->flags) & 0xefff) &&
        source->field08 == destination->field08) {
        if (((source->attributes & 0x3000) | (destination->attributes & 0x3000)) !=
            (destination->attributes & 0x3000)) {
            description = copy_type_description(destination);
            description->attributes |= source->attributes & 0x3000;
            destination = intern_type(description);
            --type_description_depth;
        }
        expression->type = destination;
        expression->flags &= 0x7f;
        return expression;
    }
    return convert_scalar_initializer(operation, destination, expression);
}

/* Original 0x66510..0x6655a: descriptor identity ignoring flag 0x1000
   still requires the same class index and an unqualified class type. */
int __stdcall same_unqualified_class_type(CType *left, CType *right)
{
    if (left && right && !left->related && !right->related &&
        !((right->flags ^ left->flags) & 0xefff) && left->field08 == right->field08 &&
        is_unqualified_class_type(left))
        return 1;
    return 0;
}

/* Original 0x46130..0x4615f. This helper is shared by ordinary and default
   initializers; a reference destination requires an address expression. */
IrToken *__stdcall prepare_reference_initializer(CType *type, IrToken *expression)
{
    TypeShape *shape;
    shape = type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    if (expression && shape && (shape->flags & 0x40))
        return builtin_unary_expression(0x3a, expression);
    return expression;
}

/* Original 0x7b710..0x7b72e: initialize function-table pointers before
   the class's construction/destruction displacement slots. */
void __stdcall emit_class_table_initializers(CppSymbol *tag, int initialize)
{
    emit_vtable_pointer_initializers(tag, 0, 0);
    emit_vtordisp_initializers(tag, initialize);
}

/* Original 0x7b8f0..0x7b9e0. The two native callers select function
   tables (0) or virtual-base tables (1); both include inherited groups. */
void __stdcall emit_vtable_pointer_initializers(CppSymbol *tag, int table_kind, int use_member_address)
{
    Vector *tables, *groups;
    VirtualTableGroup *group;
    VirtualTable *table;
    QualifierBinding *path;
    IrToken *reference;
    short table_index, group_index;
    tables = table_kind ? tag->detail.class_info->virtual_base_tables :
                          tag->detail.class_info->virtual_tables;
    groups = tag->detail.class_info->virtual_table_groups;
    table_index = group_index = 0;
    for (;;) {
        while (table_index < tables->count) {
            table = tables->items[table_index++];
            reference = symbol_reference(table->symbol);
            if (!use_member_address) {
                path = make_table_qualifier_path(table, tag);
                initialize_qualified_member(path, reference);
            } else {
                initialize_member_expression(virtual_table_object_offset(table), reference->type, reference, 0);
            }
        }
        if (group_index >= groups->count) break;
        group = groups->items[group_index++];
        tables = table_kind ? group->base_tables : group->tables;
        table_index = 0;
    }
}

/* Original 0x7b750..0x7b7c2. Construction computes the displacement;
   teardown writes zero to each group with a vtordisp slot. */
void __stdcall emit_vtordisp_initializers(CppSymbol *tag, int initialize)
{
    Vector *groups;
    VirtualTableGroup *group;
    QualifierBinding *path;
    IrToken *value;
    short index;
    groups = tag->detail.class_info->virtual_table_groups;
    index = 0;
    while (index < groups->count) {
        group = groups->items[index++];
        if (group->has_vtordisp) {
            if (!initialize) value = make_integer_constant(0);
            else value = make_vtordisp_value(tag, group);
            path = make_vtordisp_qualifier_path(tag, group);
            initialize_qualified_member(path, value);
        }
    }
}

/* Original 0x65cc0..0x65cdf. Ordinary initialization succeeds only for
   a standard/user conversion rank below the native failure threshold. */
int __stdcall can_initialize(CType *type, IrToken *expression)
{
    return classify_initializer_conversion(type, expression) < 0x400;
}

/* Original 0x3afb0..0x3b032. Bind the supplied object and resolve the
   constructor through the normal member and call-expression machinery. */
IrToken *__stdcall make_constructor_call(CppSymbol *tag, IrToken *object,
    IrToken *arguments, int emit, int member_subobject)
{
    IrToken *result, *bound;
    if (!arguments && (tag->detail.class_info->trivial_special_members & 1))
        result = make_integer_constant(0);
    else {
        bound = begin_member_lookup(0x5c, object, 1);
        result = member_expression(0x5c, bound, identifier_expression(name_ctor));
        result = build_call_expression(result, arguments);
        if (!result) return 0;
        if (!member_subobject && (tag->detail.class_info->flags & 0x10))
            result = clear_most_derived_argument(result);
    }
    if (emit) {
        emit_expression_statement(result);
        result = 0;
    }
    return result;
}

/* Original 0x64410..0x6446a. Adding cv qualification weakens a standard
   conversion; discarding const and volatile has distinct failure codes. */
unsigned __stdcall rank_conversion_qualifiers(unsigned rank, unsigned short target, unsigned short source)
{
    if (target == source) return rank;
    if (!(target & 0x1000)) {
        if (source & 0x1000) return 0x2005;
    } else if (!(source & 0x1000)) rank |= 8;
    if (!(target & 0x2000)) {
        if (source & 0x2000) return 0x2006;
    } else if (!(source & 0x2000)) rank |= 8;
    return rank;
}

/* Original 0x64320..0x64408. Pointer-distance and based-pointer changes
   contribute distinct standard-conversion ranks. */
unsigned __stdcall rank_pointer_attributes(unsigned rank, unsigned char target,
    unsigned char source, TypeShape *shape)
{
    TypeShape *base_shape;
    CppSymbol *base;
    if (((target & 12) != 0) != ((source & 12) != 0)) {
        if ((target & 4) || (source & 7) == 4) return 0x2000;
        if ((source & 12) && based_pointer_has_indirect_base(shape)) {
            base = shape->data.based_symbol;
            base_shape = base->type->related;
            if (base_shape && !(base_shape->flags & 0x77)) base_shape = base_shape->related;
            if ((base_shape->attributes & 7) == 1)
                return rank | ((target & 7) == 1 ? 0x10 : 0x80);
        }
        if ((target & 7) == 1) return 0x2000;
        return rank | 0x10;
    }
    if (!((source ^ target) & 7)) return rank;
    if ((target & 7) == 1) return 0x2000;
    return rank | 0x10;
}

/* Original 0x63f40..0x63f8c. Bare class objects permit exact-class binding
   or a derived-to-base standard conversion. */
unsigned __stdcall classify_class_reference_conversion(CType *target, CType *source)
{
    if ((target->flags & 0x400) && !target->related &&
        (source->flags & 0x400) && !source->related) {
        if (target->field08 == source->field08) return 1;
        if (type_has_base_relation(source, target)) return 0x40;
    }
    return 0x2000;
}

/* Original 0x652b0..0x65300. Source-class conversion operators are disabled
   by flags bit zero; an optional candidate records the selected conversion. */
int __stdcall classify_source_class_conversion(unsigned char flags, CType *source,
    CType *target, OverloadCandidate *context, unsigned *rank)
{
    CppSymbol *conversion;
    if ((source->flags & 0x400) && !(flags & 1)) {
        conversion = find_source_conversion(cpp_context_stack->items[source->field08], target, rank);
        record_user_conversion(context, conversion, target, *rank);
        return 1;
    }
    return 0;
}

/* Original 0x65270..0x652ac. Class construction and conversion operators
   participate only when user-defined conversions have not been disabled. */
int __stdcall classify_user_conversion(unsigned char flags, CType *source, CType *target,
    OverloadCandidate *context, unsigned *rank)
{
    CppSymbol *conversion;
    if (flags & 1) return 0;
    conversion = find_user_conversion(target, source, rank);
    record_user_conversion(context, conversion, target, *rank);
    return 1;
}

/* Original 0x63e60..0x63f31. Consider a destination constructor and a source
   conversion operator; simultaneous successes are ambiguous. */
CppSymbol *__stdcall find_user_conversion(CType *target, CType *source, unsigned *rank)
{
    CppSymbol *constructor, *conversion;
    unsigned constructor_rank, conversion_rank;
    *rank = 0;
    if ((target->flags & 0x400) && !target->related) {
        constructor = find_class_conversion(target, source, &constructor_rank);
        if (!is_class_or_reference_type(source)) {
            *rank = constructor_rank;
            return constructor;
        }
        conversion = find_source_conversion(cpp_context_stack->items[source->field08],
            target, &conversion_rank);
        if (constructor) {
            if (!conversion) {
                *rank = constructor_rank;
                return constructor;
            }
            *rank = 0x3000;
            return 0;
        }
        if (conversion) {
            *rank = conversion_rank;
            return conversion;
        }
    } else if ((source->flags & 0x400) && !source->related)
        return find_source_conversion(cpp_context_stack->items[source->field08], target, rank);
    *rank = 0x2000;
    return 0;
}

/* Original 0x63d70..0x63e54. Rank a converting constructor with synthetic
   type-only operands; their self-links form one-element circular lists. */
CppSymbol *__stdcall find_class_conversion(CType *target, CType *source, unsigned *rank)
{
    CppSymbol *constructors, *selected;
    IrToken *object, *argument;
    ArenaPool *pool = &compiler_arena.pools[2];
    constructors = find_constructor_overloads(cpp_context_stack->items[target->field08]);
    if (!constructors) {
        *rank = 0x2000;
        return 0;
    }
    if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
    object = (IrToken *)pool->next;
    pool->remaining -= sizeof(IrToken);
    pool->next += sizeof(IrToken);
    object->flags = 2;
    object->type = make_pointer_to_type(0, target);
    object->linkage.list_head = object;
    object->tail.right = object;
    object->value.pointer = object;
    if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
    argument = (IrToken *)pool->next;
    pool->remaining -= sizeof(IrToken);
    pool->next += sizeof(IrToken);
    argument->flags = 2;
    argument->type = source;
    argument->linkage.list_head = argument;
    argument->tail.right = argument;
    argument->value.pointer = argument;
    selected = resolve_function_overload(constructors, object, argument, 1, 4);
    if (!selected) {
        *rank = 0x3000;
        return 0;
    }
    *rank = 0x200;
    return selected;
}

/* Original 0x30a80..0x30ae2. Locate the visible constructor overload set
   in source-order class members, skipping hidden declarations. */
CppSymbol *__stdcall find_constructor_overloads(CppSymbol *tag)
{
    CppSymbol *member;
    for (member = tag->auxiliary.scope_members; member; member = member->next)
        if ((member->kind & 31) == 17 && !(member->flags & 0x80) &&
            (member->declaration_flags & 0x1000)) return member;
    return 0;
}

/* Original 0x66a40..0x66a7c. A bare class or a single reference to it. */
int __stdcall is_class_or_reference_type(CType *type)
{
    TypeShape *shape;
    if (!(type->flags & 0x400)) return 0;
    shape = type->related;
    if (!shape) return 1;
    if (!(shape->flags & 0x40)) return 0;
    return shape->related == 0;
}

/* Original 0x7b9e0..0x7ba29. Build an arena-two qualification chain
   from the current class back through each inherited table's direct path.
   Every hop owns a new resolution whose parent is the preceding binding. */
QualifierBinding *__stdcall make_table_qualifier_path(VirtualTable *table, CppSymbol *tag)
{
    QualifierResolution *resolution;
    begin_qualification_allocation(2);
    resolution = append_qualifier_reference(0, 0, tag);
    while (table->inheritance_path) {
        resolution = append_qualifier_reference(0, resolution->binding,
            (CppSymbol *)table->inheritance_path);
        table = table->source_table;
    }
    end_qualification_allocation();
    return resolution->binding;
}

/* Original 0x365e0..0x36609. Capture the value type before obtaining
   adjusted this, select the qualified subobject and initialize that lvalue. */
void __stdcall initialize_qualified_member(QualifierBinding *path, IrToken *value)
{
    CType *type = value->type;
    IrToken *object = adjusted_this_expression(0);
    IrToken *target = adjust_member_binding_object(object, path, 0, 1, type);
    initialize_lvalue(target, value);
}

/* Original 0x37fe0..0x37fff. An explicitly supplied destructor resets
   this class's table pointers and vtordisp values before its body executes. */
void __stdcall begin_destructor_body(CppSymbol *tag)
{
    if (!(tag->detail.class_info->implicit_member_requirements & 8)) {
        prepare_class_initialization(tag);
        emit_class_table_initializers(tag, 1);
    }
}


/* Original 0x38000..0x38188. Collect nontrivial direct-base and member
   destruction calls in declaration order, then emit them in reverse. Virtual
   bases are handled separately; hidden alignment members carry no lifetime. */
void __stdcall emit_member_and_base_destructors(CppSymbol *tag)
{
    InlineVector *calls;
    IrToken *object, *call, *member_object, *member_name;
    CppSymbol *member, *member_class;
    BaseClassLink *base;
    CType *pointer_type;
    unsigned kind;
    short index;
    if (!tag) return;
    calls = new_type_set();
    object = adjusted_this_expression(0);
    for (member = tag->auxiliary.scope_members; member; member = member->next) {
        kind = member->kind & 31;
        if (kind == 15) {
            base = (BaseClassLink *)member;
            if (base->state & 1) goto finish_member;
            member_class = cpp_context_stack->items[base->context_index];
            set_base_initialization_access(1);
        } else if (kind == 3 && member->name != name_alignment &&
                !is_static_data_member(member)) {
            member_class = class_type_of_member(member);
            if (!member_class) goto finish_member;
        } else goto finish_member;
        if (!(member_class->detail.class_info->trivial_special_members & 8)) {
            if (kind == 15) {
                pointer_type = make_class_pointer_type(member_class,
                    (unsigned short)(object->type->related->attributes & 7));
                member_object = convert_initializer(1, pointer_type, object);
                call = make_destructor_call(member_class, member_object, 1, 1, 0);
            } else {
                member_object = begin_member_lookup(0x5c, object, 1);
                member_name = identifier_expression(member->name);
                member_object = member_expression(0x5c, member_object, member_name);
                if (member->type->related && (member->type->related->flags & 0x77) == 2)
                    call = make_array_destructor_call(member_object);
                else {
                    member_object = builtin_unary_expression(0x3a, member_object);
                    call = make_destructor_call(member_class, member_object, 3, 1, 0);
                }
            }
            if (call) {
                if (calls->capacity == calls->count) calls = grow_inline_vector(calls);
                calls->items[calls->count] = call;
                ++calls->count;
            }
        }
finish_member:
        set_base_initialization_access(0);
    }
    index = -1;
    if (calls) index = (short)(calls->count - 1);
    for (; index >= 0; --index) emit_expression_statement(calls->items[index]);
    release_inline_vector(calls);
}

void __stdcall emit_virtual_base_table_initializers(CppSymbol *tag);

/* Original 0x36400..0x364bd. The hidden $initVBases argument guards
   both vbptr setup and virtual-base construction. Consume named initializers
   for each canonical virtual base, then emit its prepared constructor call. */
void __stdcall emit_virtual_base_initializers(CppSymbol *tag, IrToken **initializers,
    unsigned short attributes)
{
    IrToken *end_label = new_branch_label(0x1e);
    IrToken *condition = identifier_expression(name_init_vbases);
    IrToken *arguments, *object, *call;
    VirtualBaseSearch bases;
    condition = builtin_binary_expression(0x44, condition, make_integer_constant(0));
    emit_condition_to_label(condition, end_label, 1);
    emit_virtual_base_table_initializers(tag);
    begin_virtual_base_search(&bases, tag);
    while (next_virtual_base_search(&bases)) {
        arguments = take_member_initializer(initializers, bases.tag->name);
        object = make_subobject_pointer(bases.tag, attributes, bases.field14);
        call = make_constructor_call(bases.tag, object, arguments, 0, 0);
        call = prepare_expression(call, 0);
        if (call) emit_expression_statement(call);
    }
    emit_label(end_label);
}

/* Original 0x7b730..0x7b740. Virtual-base pointers use member addresses
   rather than the ordinary function-table qualification path. */
void __stdcall emit_virtual_base_table_initializers(CppSymbol *tag)
{
    emit_vtable_pointer_initializers(tag, 1, 1);
}

/* Original 0x3af80..0x3afad. Preserve evaluation order: class pointer
   type, byte displacement constant, adjusted this, then the addition node. */
IrToken *__stdcall make_subobject_pointer(CppSymbol *tag, unsigned short attributes,
    unsigned displacement)
{
    CType *type = make_class_pointer_type(tag, attributes);
    IrToken *offset = make_integer_constant(displacement);
    IrToken *object = adjusted_this_expression(0);
    return new_ir_expression(0x5c, object, offset, type);
}

/* Original 0x3b040..0x3b05c. A base-subobject constructor must not
   initialize virtual bases again. Replace the existing hidden argument value
   with zero, retaining its argument node and the enclosing call expression. */
IrToken *__stdcall clear_most_derived_argument(IrToken *call)
{
    IrToken *callee = call->value.pointer;
    IrToken *argument = callee->tail.right;
    argument->value.pointer = make_integer_constant(0);
    return call;
}

IrToken *__stdcall make_virtual_base_displacement(IrToken *object,
    unsigned pointer_offset, unsigned table_offset, CType *pointer_type);
unsigned __stdcall virtual_base_subobject_offset(CppSymbol *base, CppSymbol *derived);

/* Original 0x7b7d0..0x7b838. The construction displacement is the
   runtime vbtable entry minus the fixed base-to-vbptr displacement. */
IrToken *__stdcall make_vtordisp_value(CppSymbol *tag, VirtualTableGroup *group)
{
    unsigned pointer_offset, table_offset, fixed_offset;
    CType *pointer_type;
    IrToken *object, *value, *constant;
    query_virtual_base_pointer(group->tag, tag, &pointer_offset, &table_offset, &pointer_type);
    object = adjusted_this_expression(0);
    value = make_virtual_base_displacement(object, pointer_offset, table_offset, pointer_type);
    fixed_offset = virtual_base_subobject_offset(group->tag, tag);
    constant = make_integer_constant(fixed_offset - pointer_offset);
    return new_left_typed_expression(0x4f, value, constant);
}

VirtualTableGroup *__stdcall find_virtual_base_group(CppSymbol *tag, Vector *groups);

/* Original 0x7bde0..0x7bdfa. The caller has an established virtual-base
   group; retain the original unchecked metadata lookup. */
unsigned __stdcall virtual_base_subobject_offset(CppSymbol *base, CppSymbol *derived)
{
    VirtualTableGroup *group = find_virtual_base_group(base,
        derived->detail.class_info->virtual_table_groups);
    return group->object_offset;
}

unsigned virtual_base_entry_width(void);

/* Original 0x7b840..0x7b8ed. Follow the group's first dispatch table to
   its originating pointer member. A copied symbol at minus one integer slot
   denotes the construction displacement stored immediately before that member. */
QualifierBinding *__stdcall make_vtordisp_qualifier_path(CppSymbol *tag,
    VirtualTableGroup *group)
{
    VirtualTable *table = first_vector_item(group->tables);
    QualifierResolution *resolution;
    ArenaPool *pool;
    CppSymbol *member;
    begin_qualification_allocation(2);
    resolution = append_qualifier_reference(0, 0, tag);
    while (table->inheritance_path) {
        resolution = append_qualifier_reference(0, resolution->binding,
            (CppSymbol *)table->inheritance_path);
        table = table->source_table;
    }
    pool = &compiler_arena.pools[2];
    if (pool->remaining < (int)sizeof(CppSymbol)) refill_arena(&compiler_arena, pool);
    member = (CppSymbol *)pool->next;
    pool->remaining -= sizeof(CppSymbol);
    pool->next += sizeof(CppSymbol);
    *member = *table->pointer;
    member->detail.value = 0U - virtual_base_entry_width();
    member->type = builtin_integer_type();
    resolution = append_qualifier_reference(0, resolution->binding, member);
    end_qualification_allocation();
    return resolution->binding;
}

int __stdcall tag_has_base_relation(CppSymbol *derived, CppSymbol *base);

/* Original 0x67460..0x6748d. Decode the two signed class indices
   independently, then retain the distinct tag-relation lookup entry. */
int __stdcall type_has_base_relation(CType *derived, CType *base)
{
    CppSymbol *base_tag = cpp_context_stack->items[base->field08];
    CppSymbol *derived_tag = cpp_context_stack->items[derived->field08];
    return tag_has_base_relation(derived_tag, base_tag);
}

/* Original 0x67490..0x674ba. Identical class tags succeed directly;
   otherwise class-name lookup searches base categories without a diagnostic. */
int __stdcall tag_has_base_relation(CppSymbol *derived, CppSymbol *base)
{
    if (base == derived) return 1;
    return lookup_class_name(derived, base->name, 1, 0) ? 1 : 0;
}

char *__stdcall destructor_name(int kind);
IrToken *__stdcall make_destructor_arguments(int kind, int flags);

/* Original 0x3ae00..0x3af2c. Destructor variants share ordinary member
   lookup/call binding. Deleting forms preserve a repeatable object or capture
   it once before lookup, then guard the resulting call when required. The
   already-bound mode is bit 0x100 of the high parser-flag word (0x9f796). */
IrToken *__stdcall make_destructor_call(CppSymbol *tag, IrToken *object,
    int kind, int direct, int flags)
{
    IrToken *saved = 0, *designator, *arguments, *result;
    unsigned char operation;
    int lookup_object = !(cpp_parser_flags & 0x01000000UL);
    cpp_parser_flags &= ~0x01000000UL;
    if (kind == 1 && !(tag->detail.class_info->flags & 0x20)) kind = 3;
    else if (kind == 3 && (tag->detail.class_info->flags & 0x20)) kind = 2;
    if (lookup_object) {
        if (kind == 4 || kind == 5) {
            if (expression_can_be_repeated(object)) saved = clone_expression(object, 0);
            else capture_allocation_pointer(&saved, &object);
        }
        object = begin_member_lookup(0x5c, object, 1);
    }
    designator = identifier_expression(destructor_name(kind));
    if (direct)
        ((CppSymbol *)designator->value.pointer)->declaration_flags |= 0x800;
    if (lookup_object) {
        arguments = make_destructor_arguments(kind, flags);
        operation = 0x5c;
    } else {
        arguments = make_destructor_arguments(kind, 0);
        operation = class_lookup_operator;
    }
    result = member_expression(operation, object, designator);
    result = build_call_expression(result, arguments);
    if (lookup_object && (kind == 4 || kind == 5) && result && object != result)
        result = guard_allocation_result(saved, result);
    return result;
}

/* Original 0x385a0..0x385e7. These interned identifiers are shared
   with member declaration and mangling, so preserve their pointer identities. */
char *__stdcall destructor_name(int kind)
{
    switch (kind) {
    case 1: case 3: return name_dtor;
    case 2: return name_vbase_dtor;
    case 4: return name_delete_dtor;
    case 5: return name_vector_delete_dtor;
    }
    return 0;
}

/* Original 0x3af30..0x3af7c. Only deleting variants receive a flags
   argument. The original allocates an unsigned-typed constant, then separately
   allocates the actual integer argument; retain both allocations and that first
   type assignment rather than folding away their arena effects. */
IrToken *__stdcall make_destructor_arguments(int kind, int flags)
{
    unsigned value = (kind == 5 ? 2U : 0U) | (flags != 0);
    IrToken *unused, *argument;
    if (kind != 4 && kind != 5) return 0;
    unused = make_integer_constant(value);
    unused->type = type_4064_first;
    argument = make_integer_constant(value);
    return append_expression(0, argument, 2);
}

/* Original 0x382d0..0x382f4, stdcall4. A scalar deleting destructor
   performs its cleanup and returns the same adjusted object pointer. */
void __stdcall emit_deleting_destructor_body(CppSymbol *tag)
{
    IrToken *object = adjusted_this_expression(0);
    emit_scalar_deleting_cleanup(tag);
    begin_return_expression();
    emit_return_statement(object, 2);
}

/* Original 0x38300..0x3838c, stdcall4. Always destroy the object;
   bit zero of __flags controls class-specific or global deallocation. */
void __stdcall emit_scalar_deleting_cleanup(CppSymbol *tag)
{
    IrToken *object = adjusted_this_expression(0);
    IrToken *flags = identifier_expression(name_flags);
    IrToken *end_label = new_branch_label(0x1e);
    IrToken *call;
    QualifiedDeclarator *deallocation;
    emit_expression_statement(make_destructor_call(tag, object, 3, 1, 0));
    emit_condition_to_label(builtin_binary_expression(OP_BIT_AND, flags,
        make_integer_constant(1)), end_label, 0);
    deallocation = lookup_class_name(tag, name_delete, 2, 0x181);
    if (deallocation)
        call = make_class_delete_call(deallocation, object, tag);
    else
        call = make_global_delete_call(object);
    emit_expression_statement(call);
    emit_label(end_label);
}

/* Original 0x38400..0x3847b, stdcall4. Global delete accepts the object
   address, with an explicit segment argument for a based pointer. The
   original changes that shared pointer shape from attribute5 to4 in place. */
IrToken *__stdcall make_global_delete_call(IrToken *object)
{
    CppSymbol *deallocator = find_scope_symbol(global_scope, name_delete, 0);
    IrToken *arguments = 0, *segment;
    TypeShape *shape;
    if (object->type && (shape = object->type->related) != 0 &&
        shape->attributes == 5) {
        segment = symbol_reference((CppSymbol *)shape->data.based_symbol);
        segment->type = type_0802;
        arguments = append_expression(0, segment, 2);
        object->type->related->attributes = 4;
    }
    arguments = append_expression(arguments, object, 2);
    return build_call_expression(symbol_reference(deallocator), arguments);
}

/* Original 0x642e0..0x64316, stdcall12. A source shape converted to a
   single pointer shape receives this distinct conversion rank. */
unsigned __stdcall classify_null_pointer_shape(unsigned rank,
    TypeShape *target, TypeShape *source)
{
    if (source && !target->related && (target->flags & 0x77) == 1)
        return 0x100;
    return rank;
}

/* Original 0x38190..0x381cb, stdcall4. The virtual-base wrapper first
   destroys the ordinary subobject, then emits the virtual-base cleanup. */
void __stdcall emit_vbase_destructor_epilogue(CppSymbol *tag)
{
    IrToken *object = adjusted_this_expression(0);
    emit_expression_statement(make_destructor_call(tag, object, 1, 1, 0));
    set_base_initialization_access(1);
    emit_virtual_base_destructors(tag, object);
    set_base_initialization_access(0);
}

/* Original 0x381d0..0x382c5, stdcall8. Build each nontrivial virtual
   base's destruction using its object displacement, then emit in reverse
   virtual-base order and return the temporary vector to its shared pool. */
void __stdcall emit_virtual_base_destructors(CppSymbol *tag, IrToken *object)
{
    InlineVector *calls = new_type_set();
    VirtualBaseSearch bases;
    CType *pointer_type;
    IrToken *offset, *base_object, *call;
    short index;
    begin_virtual_base_search(&bases, tag);
    while (next_virtual_base_search(&bases)) {
        if (!(bases.tag->detail.class_info->trivial_special_members & 8)) {
            pointer_type = make_class_pointer_type(bases.tag,
                (unsigned short)(object->type->related->attributes & 7));
            offset = make_integer_constant(bases.field14);
            base_object = clone_expression(object, 0);
            base_object = new_ir_expression(OP_ADD, base_object, offset, pointer_type);
            call = make_destructor_call(bases.tag, base_object, 1, 1, 0);
            if (call) {
                if (calls->capacity == calls->count) calls = grow_inline_vector(calls);
                calls->items[calls->count] = call;
                ++calls->count;
            }
        }
    }
    index = -1;
    if (calls) index = (short)(calls->count - 1);
    for (; index >= 0; --index) emit_expression_statement(calls->items[index]);
    release_inline_vector(calls);
}

/* Original 0x63cb0..0x63d61, stdcall 12. The original 16-byte
   search seed at 0xa1e40 is {0x2000, 0, NULL, 0}. */
CppSymbol *__stdcall find_source_conversion(CppSymbol *tag, CType *target, unsigned *rank)
{
    SourceConversionSearch search;
    search.rank = 0x2000;
    search.count = 0;
    search.type = 0;
    search.ambiguous = 0;
    rank_source_conversion_functions(tag, target, &search);
    if (search.rank & 0x2000) {
        *rank = 0x2000;
        return 0;
    }
    if (search.count != 1 || search.ambiguous) {
        *rank = 0x3000;
        return 0;
    }
    *rank = search.rank | 0x200;
    return find_source_conversion_function(search.type, tag);
}

/* Original 0x63aa0..0x63b56, stdcall 12. Prefer exact and promotion
   ranks before general conversions; equal-ranked alternatives remain ambiguous. */
void __stdcall rank_source_conversion_functions(CppSymbol *tag, CType *target,
    SourceConversionSearch *search)
{
    ConversionTypeIterator iterator;
    unsigned rank, best;
    begin_conversion_types(&iterator, tag);
    while (next_conversion_type(&iterator)) {
        rank = classify_type_conversion(target, strip_based_shape(iterator.type), 1, 0);
        best = search->rank;
        if ((rank == 1 && best != 1) || (rank < 3 && best > 2) ||
            (rank < 0x400 && (best & 0x2000))) {
            search->rank = rank;
            search->count = 1;
            search->type = iterator.type;
            search->ambiguous = iterator.ambiguous;
        } else if ((best != 1 || rank == 1) && (best > 2 || rank < 3) &&
                   (best >= 0x400 || !(rank & 0x2000))) {
            ++search->count;
        }
    }
}

/* Original 0x63990..0x639c0, stdcall 8. A selected conversion type
   must resolve back to a declaration in the source class hierarchy. */
CppSymbol *__stdcall find_source_conversion_function(CType *type, CppSymbol *tag)
{
    CppSymbol *function = lookup_source_conversion_function(type, tag);
    if (!function) fatal_error(1, pch_diagnostic_name, 159);
    return function;
}

typedef char ConversionBaseIndexOffsetMustBe12[
    (offsetof(BaseClassLink, context_index) == 12) ? 1 : -1];
/* Original 0x639d0..0x63a9a, stdcall 8. Search local conversion
   declarations first, then visible base-class links in source order. */
CppSymbol *__stdcall lookup_source_conversion_function(CType *type, CppSymbol *tag)
{
    ConversionIterator iterator;
    CppSymbol *member, *function;
    BaseClassLink *base;
    if (!type_set_contains(tag->detail.class_info->conversion_types, type)) return 0;
    begin_conversion_functions(&iterator, tag);
    while (next_conversion_function(&iterator))
        if (iterator.type == type) return iterator.target;
    for (member = tag->auxiliary.scope_members; member; member = member->next) {
        if ((member->kind & 31) != 15 || (member->flags & 0x80)) continue;
        base = (BaseClassLink *)member;
        function = lookup_source_conversion_function(type,
            cpp_context_stack->items[base->context_index]);
        if (function) return function;
    }
    return 0;
}

/* Original 0x38760..0x38775. Ordinary destruction uses the interned
   __dtor identifier without an additional candidate predicate. */
CppSymbol *__stdcall lookup_destructor_member(CType *type)
{
    return lookup_special_member(type, name_dtor, 0);
}

CppSymbol *__stdcall single_qualified_function(QualifiedDeclarator *declaration);

/* Original 0x38390..0x383f5. A class deallocator can receive the
   object size as its second argument. Preserve the normal qualified designator
   and call-building path for both signatures. */
IrToken *__stdcall make_class_delete_call(QualifiedDeclarator *declaration,
    IrToken *object, CppSymbol *tag)
{
    IrToken *arguments = append_expression(0, object, 2);
    CppSymbol *function = single_qualified_function(declaration);
    TypeShape *shape = function->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    if (shape->data.parameters->declarations->count == 2)
        arguments = append_expression(arguments,
            make_integer_constant(tag->detail.class_info->size), 2);
    return build_call_expression(symbol_reference((CppSymbol *)declaration), arguments);
}

/* Original 0x38570..0x3859f. Only a single unambiguous qualification
   path containing exactly one function supplies a direct declaration. */
CppSymbol *__stdcall single_qualified_function(QualifiedDeclarator *declaration)
{
    QualifierResolution *resolution = declaration->resolution;
    CppSymbol *functions;
    if (resolution->binding != resolution->tail) return 0;
    functions = (CppSymbol *)resolution->binding->target;
    if (!has_single_function(functions)) return 0;
    return ((OverloadSet *)functions)->functions;
}

/* Original 0x38480..0x38563. Bit one selects array destruction using
   the preceding count cookie; the scalar branch uses ordinary deleting
   cleanup. Both forms return the original object pointer. */
void __stdcall emit_vector_deleting_destructor_body(CppSymbol *tag)
{
    IrToken *object = adjusted_this_expression(0);
    IrToken *flags = identifier_expression(name_flags);
    IrToken *scalar_label = new_branch_label(0x1d);
    IrToken *end_label = new_branch_label(0x1e);
    IrToken *constant, *cookie, *allocation, *count, *call;
    emit_condition_to_label(builtin_binary_expression(OP_BIT_AND, flags,
        make_integer_constant(2)), scalar_label, 0);
    constant = make_integer_constant(complete_type_size(type_0064));
    constant->type = type_0064;
    cookie = new_left_typed_expression(OP_SUBTRACT, object, constant);
    allocation = clone_expression(cookie, 0);
    count = new_typed_unary_expression(OP_DEREFERENCE, type_0064, cookie);
    call = make_vector_destructor_call(object, tag->detail.class_info->size,
        count, object->type);
    emit_expression_statement(call);
    emit_expression_statement(make_global_delete_call(allocation));
    write_ir_record(0x3a, end_label);
    emit_label(scalar_label);
    emit_scalar_deleting_cleanup(tag);
    emit_label(end_label);
    begin_return_expression();
    emit_return_statement(object, 2);
}
