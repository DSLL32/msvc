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
#include <recon.h>

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

IrToken *__stdcall make_destructor_call(CppSymbol *tag, IrToken *object,
    int kind, int direct, int flags);
IrToken *__stdcall make_array_destructor_call(IrToken *object);

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
