/* Object construction, allocation functions and array-construction records. */
#include "object-initializers.h"
#include "binary-expressions.h"
#include "class-members.h"
#include "global-ir.h"
#include "special-members.h"
#include "type-sizes.h"
#include "initialization.h"
#include "function-definitions.h"
#include "expression-preparation.h"
#include "return-statements.h"
#include <recon.h>

CppSymbol *allocation_base_symbol; /* original 0xa0d18: based allocation context */

/* Original 0x50030..0x5017f, stdcall 20. A heap array may store a
   count cookie and guard a captured allocation pointer; an existing stack
   array only requires its construction expression. */
IrToken *__stdcall initialize_allocated_array(IrToken *object, int allocated,
    unsigned prefix_bytes, IrToken *element_count, int element_bytes)
{
    IrToken *saved, *construction = 0, *size;
    if (!object || !element_count) return object;
    if (has_trivial_class_constructor(object->type) &&
        (!allocated || !prefix_bytes)) return object;
    if (allocated) capture_allocation_pointer(&saved, &object);
    if (prefix_bytes) {
        type_size(type_4064_first);
        type_size(type_4044);
        element_count = new_right_typed_expression(0x3d,
            clone_expression(object, 0), element_count);
        size = make_size_constant(prefix_bytes);
        size->type = type_4064_first;
        object = new_left_typed_expression(0x5a, object, size);
    }
    element_count = convert_initializer(0x6e, type_0064, element_count);
    if (!has_trivial_class_constructor(object->type))
        construction = make_array_constructor_call(clone_expression(object, 0),
            element_bytes, element_count);
    if (!allocated) return construction;
    object = new_right_typed_expression(0x40,
        construction ? construction : element_count, object);
    return guard_allocation_result(saved, object);
}

/* Original [0x4f8f0,0x4fef4), including all allocation and supplied-object
   branches. Argument lists retain their circular head links while synthetic
   size and based-pointer operands are spliced after placement arguments. */
IrToken *__stdcall make_object_constructor_call(unsigned char mode, unsigned short attributes,
    IrToken *placement, CppSymbol *symbol, IrToken *arguments, IrToken *object, IrToken *array_bound)
{
    int is_array = 0, unshaped_element = 0, destructor_cleanup = 0;
    int needs_constructor = 0, element_bytes = 0;
    unsigned element_count = 1, prefix_bytes = 0;
    CppSymbol *allocation_function = 0, *tag, *temporary, *destructor, *base;
    CType *pointer_type;
    TypeShape *shape;
    IrToken *variable_count = 0, *count, *result, *value, *right, *entry;
    unsigned short pointer_attributes;

    if (!symbol || (placement && !placement->value.pointer)) return 0;
    shape = symbol->type->related;
    if (shape) {
        if (!(shape->flags & 0x77)) shape = shape->related;
        if (shape) {
            if (shape->flags & 0x40) {
                report_type_error(464, symbol->type);
                return 0;
            }
            do {
                if ((shape->flags & 0x77) != 2) break;
                is_array = 1;
                if (!(shape->flags & 0x1000)) {
                    element_count *= (unsigned)shape->data.array_bound;
                    if (!shape->data.array_bound) report_syntax_error(466);
                } else variable_count = array_bound;
                shape = shape->related;
            } while (shape);
            unshaped_element = shape == 0;
            if (is_array) {
                mode = 2;
                if (arguments) {
                    report_syntax_error(538);
                    arguments = 0;
                }
            }
        }
    }
    if (is_array) {
        pointer_attributes = normalize_type_attributes(1, 0);
        pointer_type = replace_first_type_shape(symbol->type, 1, pointer_attributes, 0);
    } else if (!(attributes & 7))
        pointer_type = make_pointer_to_type(0, symbol->type);
    else if (!(attributes & 4))
        pointer_type = make_qualified_type(symbol->type, 1, attributes, 0);
    else {
        base = allocation_base_symbol;
        pointer_type = make_qualified_type(symbol->type, 1, 5, &base);
    }
    tag = class_type_of_member(symbol);
    if (!tag) {
        if (!(attributes & 7)) attributes = pointer_type->related->attributes & 7;
    } else {
        if (!(attributes & 7)) {
            if (object && object->type->related) attributes = object->type->related->attributes;
            if (!(attributes & 7)) attributes = tag->detail.class_info->pointer_attributes;
        }
        if (!object) {
            if (unshaped_element && !(tag->detail.class_info->trivial_special_members & 8) &&
                has_destructor_member_model(pointer_type)) {
                destructor_cleanup = 1;
                lookup_destructor_member(pointer_type);
            }
            if (mode != 2)
                allocation_function = (CppSymbol *)lookup_class_name(tag, name_new, 2, 0x181);
        }
        if (!has_class_initialization(pointer_type)) {
            if (!arguments) {
                report_syntax_error(512, tag->name);
                return 0;
            }
            if (!(tag->declaration_flags & 0x1000)) {
                report_syntax_error(514, tag->name);
                return 0;
            }
            if (!(tag->detail.class_info->flags2 & 1) && unshaped_element) {
                report_syntax_error(539, tag->name);
                return 0;
            }
        } else needs_constructor = !(tag->detail.class_info->trivial_special_members & 1);
    }
    if (!object && !allocation_function)
        allocation_function = find_scope_symbol(global_scope, name_new, 0);
    if (is_array || !object) element_bytes = symbol_element_size(symbol);
    result = object;
    if (!variable_count) {
        count = make_size_constant(element_count);
        /* The original also allocates this unused size token. Its arena and
           constant-allocation effects remain observable to later expressions. */
        make_size_constant(element_bytes);
    } else {
        if (needs_constructor) {
            temporary = create_generated_object(4, type_4064_first);
            count = resolve_symbol_expression(temporary);
            right = make_size_constant(element_count);
            value = builtin_binary_expression(OP_MULTIPLY, variable_count, right);
            value = builtin_binary_expression(OP_ASSIGN, count, value);
            emit_expression_statement(value);
        } else {
            right = make_size_constant(element_count);
            count = builtin_binary_expression(OP_MULTIPLY, variable_count, right);
        }
        if (!allocation_function) goto have_object;
        right = make_size_constant(element_bytes);
        builtin_binary_expression(OP_MULTIPLY, count, right);
    }
    if (allocation_function) {
        if (destructor_cleanup) prefix_bytes = type_size(type_4064_first);
        right = make_size_constant(element_bytes);
        right->type = type_4064_first;
        value = builtin_binary_expression(OP_MULTIPLY, count, right);
        value->type = type_4064_first;
        if (prefix_bytes) {
            right = make_size_constant(prefix_bytes);
            value = builtin_binary_expression(OP_ADD, value, right);
            value->type = type_4064_second;
        }
        entry = append_expression(0, value, 2);
        if (placement) {
            entry->linkage.list_head = placement->linkage.list_head;
            placement->linkage.list_head->tail.right = entry;
            placement->linkage.list_head = entry;
        } else placement = entry;
        if (attributes & 4) {
            value = symbol_reference(allocation_base_symbol);
            value->type = type_0802;
            entry = append_expression(0, value, 2);
            entry->linkage.list_head = placement->linkage.list_head;
            placement->linkage.list_head->tail.right = entry;
            placement->linkage.list_head = entry;
        }
        value = symbol_reference(allocation_function);
        result = build_call_expression(value, placement);
        if (!result) return 0;
        result = finish_call_expression(result, attributes);
    }
have_object:
    if (!result) return 0;
    result->type = pointer_type;
    if (!tag) {
        if (arguments) return initialize_allocated_scalar(result, arguments);
        return result;
    }
    if (!needs_constructor && !arguments && (!is_array || !prefix_bytes)) return result;
    if ((tag->detail.class_info->flags & 0x80) && !(tag->declaration_flags & 2)) {
        report_syntax_error(259, tag->name);
        tag->declaration_flags |= 2;
    }
    if (is_array) {
        if (destructor_cleanup) {
            destructor = lookup_vector_delete_destructor(pointer_type);
            if (destructor) {
                defer_generated_member(destructor);
                destructor->storage[1] |= 0x80;
            }
        }
        result = initialize_allocated_array(result, object == 0, prefix_bytes, count, element_bytes);
    } else result = initialize_allocated_class(object == 0, tag, result, arguments);
    if (!result) return 0;
    if (result->type->attributes & 0x1000)
        result->type = remove_type_qualifiers(result->type, 0x1000);
    return result;
}

/* Original 0x4ffb0..0x50021. Newly allocated objects preserve the allocation
   value for the final null guard; supplied objects call the constructor directly. */
IrToken *__stdcall initialize_allocated_class(int allocated, CppSymbol *tag,
    IrToken *object, IrToken *arguments)
{
    IrToken *saved;
    if (allocated) capture_allocation_pointer(&saved, &object);
    object = make_constructor_call(tag, object, arguments, 0, 1);
    if (!object) return 0;
    object = modifiable_lvalue(object, 0);
    if (allocated) object = guard_allocation_result(saved, object);
    return object;
}

/* Original 0x4f860..0x4f8e1. A based allocation temporarily supplies
   its base symbol while constructor-call formation builds the expression. */
IrToken *__stdcall new_expression(unsigned char operation, TypeShape *modifier,
    IrToken *placement, CppSymbol *type, IrToken *initializer, unsigned bound)
{
    IrToken *result;
    if (modifier && (modifier->attributes & 4)) {
        if (modifier->attributes != 5 || modifier->flags || !modifier->data.based_symbol)
            return 0;
        allocation_base_symbol = modifier->data.based_symbol;
        result = make_object_constructor_call(2, 5, placement, type, initializer,
            0, (IrToken *)bound);
        allocation_base_symbol = 0;
        return result;
    }
    return make_object_constructor_call(operation, modifier->attributes,
        placement, type, initializer, 0, (IrToken *)bound);
}
