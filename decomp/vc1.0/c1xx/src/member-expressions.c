/* Object and qualified-member selection for . and -> expressions. */
#include "member-expressions.h"
#include "lexer.h"
#include "constructor-initializers.h"
#include "operators.h"
#include <recon.h>

IrToken *__stdcall enumerator_expression(CppSymbol *symbol);
IrToken *__stdcall finish_symbol_expression(CppSymbol *symbol, IrToken *expression);
IrToken *__stdcall resolve_symbol_expression(CppSymbol *symbol);

/* Original 0x321f0..0x32670. Retain bound overloads separately from data
   member address adjustment, and propagate object qualifiers and temporaries. */
IrToken *__stdcall member_expression(unsigned char operation, IrToken *object, IrToken *member)
{
    IrToken *original_object, *result, *designator, *width, *offset;
    CppSymbol *reference, *target, *represented, *object_class;
    TypeShape *shape;
    CType *object_type, *member_type;
    TypeDescription *description;
    ArenaPool *pool;
    unsigned shape_parameter;
    unsigned short qualifiers, attributes;
    int temporary = 0, reference_object = 0;

    token_allocation_flags &= ~0x80U;
    class_lookup_error = 0;
    class_lookup_operator = 1;
    class_lookup_context = 0;
    if (!member || !object) return 0;
    original_object = object;
    object_type = object->type;
    if (member->kind == 0x46) {
        designator = member->value.pointer;
        represented = designator->value.pointer;
        if ((represented->kind & 0x1f) == 15) {
            object_class = cpp_context_stack->items[object_type->field08];
            report_syntax_error(39, represented->name, object_class->name);
        }
        return 0;
    }
    if (member->kind != 2) {
        report_syntax_error(operation == 0x5c ? 273 : 274, token_spellings[member->kind]);
        return 0;
    }
    reference = member->value.pointer;
    target = reference;
    if ((target->kind & 0x1f) == 18)
        target = (CppSymbol *)((QualifiedDeclarator *)target)->resolution->binding->target;
    if (is_static_data_member(target)) {
        check_qualified_member_access(reference);
        return resolve_symbol_expression(static_member_alias(target));
    }
    if ((target->kind & 0x1f) == 5) {
        check_qualified_member_access(reference);
        return enumerator_expression(target);
    }
    shape = object_type->related;
    if (operation == 0x5c) {
        if (!shape) {
            report_syntax_error(232, type_kind_name(object_type->flags));
            return 0;
        }
        if (shape->related || (!(shape->flags & 3) && !(shape->flags & 0x40))) {
            report_syntax_error(227, target->name);
            return 0;
        }
        if ((shape->attributes & 7) == 4) {
            report_syntax_error(214);
            return 0;
        }
    } else if (!shape) {
        object = materialize_member_object(object);
        temporary = object->flags & 0x20;
        if (object->kind == 0x3d && object->type && !object->type->related) temporary = 1;
    } else if (!(shape->flags & 0x40)) {
        if (!(shape->flags & 3) || shape->related) report_syntax_error(228, target->name);
        else report_syntax_error(231, type_kind_name(object_type->flags));
    } else reference_object = 1;

    if (!(object->flags & 7) && object->kind == 0xa3) {
        report_syntax_error(275, format_symbol_name(object->value.pointer));
        return 0;
    }
    if ((target->kind & 0x1f) == 17) {
        if (token_allocation_flags & 0x20000020U)
            result = allocate_permanent(sizeof(IrToken));
        else {
            pool = &compiler_arena.pools[2];
            if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
            result = (IrToken *)pool->next;
            pool->remaining -= sizeof(IrToken);
            pool->next += sizeof(IrToken);
        }
        result->flags = 2;
        if (operation == 0x59) {
            operation = 0x5c;
            if (!reference_object) object = builtin_unary_expression(0x3a, original_object);
        }
        result->kind = operation;
        result->value.pointer = object;
        member->value.pointer = reference;
        result->tail.right = member;
        return result;
    }
    qualifiers = object_type->attributes & 0x3000;
    result = adjust_qualified_member_object(object, (QualifiedDeclarator *)reference,
        0, 1, object->type);
    attributes = operation == 0x5c ? object_type->related->attributes : object_type->attributes;
    result->type = member_type = replace_type_pointer_kind(target->type, attributes & 7);
    if (qualifiers) {
        shape = member_type->related;
        if (shape) {
            shape_parameter = shape->data.word;
            if ((shape->attributes & 0x3000) != qualifiers)
                result->type = replace_first_type_shape(member_type, shape->flags,
                    shape->attributes | qualifiers, &shape_parameter);
        } else if ((member_type->attributes & 0x3000) != qualifiers) {
            description = copy_type_description(member_type);
            description->attributes |= qualifiers;
            result->type = intern_type(description);
            --type_description_depth;
        }
    }
    if (target->attributes & 0x400) return result;
    if ((target->kind & 0x1f) == 2) {
        /* Bit-field offset/width share the auxiliary word used by other
           symbol kinds for label-reference and declaring-class metadata. */
        width = make_integer_constant((target->auxiliary.value >> 8) & 0xff);
        offset = make_integer_constant(target->auxiliary.value & 0xff);
        width = new_left_typed_expression(0x3f, offset, width);
        result = new_left_typed_expression(0x3e, result, width);
        if (qualifiers) {
            description = copy_type_description(result->type);
            description->attributes |= qualifiers;
            result->type = intern_type(description);
            --type_description_depth;
        }
    }
    result = finish_symbol_expression(target, result);
    if (temporary) result->flags |= 0x20;
    return result;
}

/* Original 0x313c0..0x31432. Ordinary member access diagnoses its access
   failure directly; pointer conversions use a conversion diagnostic instead. */
IrToken *__stdcall adjust_qualified_member_object(IrToken *object,
    QualifiedDeclarator *member, int pointer_conversion, int derived_to_base,
    CType *object_type)
{
    QualifierBinding *binding = member->resolution->binding;
    if (!pointer_conversion) check_qualified_member_access(member);
    else if (!qualified_member_is_accessible(member))
        report_type_conversion(243, (cpp_parser_flags & 2) ? 3 : 0, 0x6e,
            object->type, object_type);
    return adjust_member_binding_object(object, binding, pointer_conversion,
        derived_to_base, object_type);
}

/* Original 0x320a0..0x320ba. Access checking returns no status to ordinary
   expression/declaration callers; the shared denial record supplies diagnostics. */
void __stdcall check_qualified_member_access(void *reference)
{
    if (reference && !qualified_member_is_accessible(reference))
        report_denied_member_access();
}

/* Original 0x31440..0x314e9. Accumulate the data-member and nonvirtual
   base offsets, retaining the separate virtual-base and pointer paths. */
IrToken *__stdcall adjust_member_binding_object(IrToken *object, QualifierBinding *binding,
    int pointer_conversion, int derived_to_base, CType *object_type)
{
    CppSymbol *target = (CppSymbol *)binding->target;
    BaseClassLink *base;
    unsigned kind = target->kind & 0x1f, displacement = 0;
    if (kind == 3 || kind == 2) {
        displacement = target->detail.value;
        binding = binding->next;
    } else if (kind == 17 || kind == 4 || kind == 7) binding = binding->next;
    while (binding && (binding->target->kind & 0x1f) == 15) {
        base = (BaseClassLink *)binding->target;
        if (base->state & 1)
            return adjust_virtual_member_object(object, binding, object_type,
                displacement, pointer_conversion);
        displacement += base->displacement;
        binding = binding->next;
    }
    if (!pointer_conversion) return make_member_at_offset(object, displacement, object_type);
    if (!derived_to_base) displacement = 0U - displacement;
    return adjust_pointer_by_offset(object, displacement, object_type);
}

/* Original 0x31710..0x3172f. Address-style member node with its explicit
   displacement constant; the caller replaces its type with the member type. */
IrToken *__stdcall make_member_at_offset(IrToken *object, unsigned displacement, CType *type)
{
    return new_ir_expression(0x5c, object, make_integer_constant(displacement), type);
}

/* Original 0x33560..0x33634. Establish the class-only lookup context,
   recursively applying overloaded operator-> when ordinary lookup permits it. */
IrToken *__stdcall begin_member_lookup(unsigned char operation, IrToken *object, int mode)
{
    CType *type;
    CppSymbol *tag;
    IrToken *converted, *result;
    if (!object) return 0;
    class_lookup_operator = operation;
    class_lookup_error = 0;
    type = object->type;
    if (!type || (type->flags & 0x410) != 0x410) return 0;
    tag = cpp_context_stack->items[type->field08];
    if (tag->flags & 4) {
        if (!mode && operation == 0x5c && object && is_unqualified_class_type(object->type)) {
            converted = resolve_operator_expression(0x5c, object, 0, 1, 0);
            if (converted) {
                result = begin_member_lookup(0x5c, converted, 0);
                if (result) return result;
            }
        }
        token_allocation_flags |= 0x80;
        class_lookup_context = tag;
        return object;
    }
    if (!options.prototypes_only) report_syntax_error(27, tag->name);
    return 0;
}
