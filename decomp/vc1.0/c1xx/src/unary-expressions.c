/* Unary C++ operator dispatch and builtin fallback. */
#include "lexer.h"
#include "unary-expressions.h"
#include <recon.h>

/* Original 0x5f070..0x5f0e5, stdcall4. Walk the value-producing side
   of an lvalue expression until its pointer-model source can be inspected. */
IrToken *__stdcall find_expression_type_base(IrToken *expression)
{
    IrToken *right;
    for (;;) {
        switch (expression->kind) {
        case 2: case 0x6e:
            return expression;
        case OP_ARROW:
            return expression->value.pointer;
        case 0x3f:
            right = expression->tail.right;
            expression = expression->type == right->type ? right : expression->value.pointer;
            break;
        case OP_SUBSCRIPT: case 0x59: case OP_CALL:
            expression = expression->value.pointer;
            break;
        case 0x67:
            expression = expression->value.pointer;
            if (expression->kind == OP_DEREFERENCE)
                expression = expression->value.pointer;
            break;
        default:
            return 0;
        }
    }
}

/* Original [0x5d8b0,0x5d91e). Taking the address of an adjusted this
   subtraction needs a temporary in the special member-displacement mode. */
int __stdcall requires_address_temporary(IrToken *expression)
{
    IrToken *operand, *reference;
    CppSymbol *symbol;
    if (expression->kind != OP_BIT_AND || (expression->flags & 7) != 1) return 0;
    operand = expression->value.pointer;
    if (operand->kind != OP_SUBTRACT) return 0;
    operand = operand->value.pointer;
    if (operand->kind != OP_DEREFERENCE) return 0;
    reference = operand->value.pointer;
    if (reference->kind != 2) return 0;
    symbol = reference->value.pointer;
    if (symbol->name != name_this || !function_symbol ||
        (function_symbol->kind & 31) != 7) return 0;
    return (function_symbol->cpp.parts.member_flags & 1) != 0 && function_symbol->field28 != 0;
}

/* Original 0x5f0f0..0x5f206. Try class operator lookup while temporarily
   permitting its evaluation context, diagnose a failed conversion when no
   builtin form is allowed, then dispatch the builtin unary operation. */
IrToken *__stdcall unary_expression(unsigned char operation, IrToken *expression)
{
    IrToken *result;
    unsigned failure, classification;
    int saved_context;
    if (!expression) return 0;
    if (expression->kind == 0xa3) {
        report_syntax_error(0x113, format_symbol_name(expression->value.pointer));
        return 0;
    }
    if (is_overload_operand(expression)) {
        saved_context = (cpp_parser_flags & 0x10) != 0;
        if (saved_context) cpp_parser_flags &= ~0x10UL;
        failure = 0;
        result = resolve_operator_expression(operation, expression, 0, 1, &failure);
        if (result) {
            if (saved_context) cpp_parser_flags |= 0x10;
            return result;
        }
        if (failure) {
            classification = expression ? (unsigned short)is_unqualified_class_type(expression->type) : 0;
            if (!operator_has_builtin_fallback(operation, 1, classification)) {
                report_operator_resolution_failure(operation, 1, failure, expression->type, 0);
                if (saved_context) cpp_parser_flags |= 0x10;
                return 0;
            }
        }
        if (saved_context) cpp_parser_flags |= 0x10;
    }
    return builtin_unary_expression(operation, expression);
}

/* Original 0x3b8a0..0x3b8bb. Expression presence precedes type lookup. */
int __stdcall is_overload_operand(IrToken *expression)
{
    if (!expression) return 0;
    return is_class_name_type(expression->type);
}

/* Original 0x5f210..0x5fd47. Builtin unary semantics, including unresolved
   member/function designators, constant folding and pointer-model recovery. */
IrToken *__stdcall builtin_unary_expression(unsigned char operation, IrToken *operand)
{
    unsigned char original_operation = operation;
    unsigned short shape_flags, kind, attributes, region;
    unsigned value;
    int slot, index;
    IrToken *result, *lvalue, *base, *right, *member;
    CppSymbol *symbol, *target, *function;
    TypeShape *shape, *array;
    Extended number;
    if (!operand) return 0;
    if (operation == OP_LOGICAL_AND) operation = OP_BIT_AND;
    if (!operand->type) {
        if (operation == OP_CALL) return operand;
        if (operand->kind == OP_BIT_AND) {
            report_syntax_error(0xab, token_spellings[operation]);
            return operand;
        }
        if (operand->kind == 0x59 || operand->kind == OP_ARROW) {
            member = operand->tail.right;
            if (operation != OP_BIT_AND || !member || member->kind != 2 ||
                !member->value.pointer) goto invalid_member;
            target = member->value.pointer;
            if ((target->kind & 31) == 18)
                target = (CppSymbol *)((QualifiedDeclarator *)target)->resolution->binding->target;
            if (!has_single_function(target) ||
                (((OverloadSet *)target)->functions->storage[0] & 7) != 3)
                goto invalid_member;
            if (cpp_parser_flags & 0x10) return operand;
            function = ((OverloadSet *)target)->functions;
            member->value.pointer = function;
            member->type = function->type;
            operand->type = member->type;
        } else {
            symbol = operand->value.pointer;
            target = symbol;
            if ((target->kind & 31) == 18)
                target = (CppSymbol *)((QualifiedDeclarator *)target)->resolution->binding->target;
            kind = target->kind & 31;
            if (kind == 3) {
                if (operation == OP_BIT_AND)
                    return member_pointer_expression((QualifiedDeclarator *)symbol);
                report_syntax_error(0xab, token_spellings[operation]);
                return 0;
            }
            if (kind == 2) {
                report_syntax_error(0x87, target->name);
                return 0;
            }
            if (target->declaration_flags & 0x3000) {
                report_syntax_error(0x115, format_symbol_name(target));
                return 0;
            }
            if (!has_single_function(target)) {
                if (operation == OP_BIT_AND)
                    return new_typed_unary_expression(OP_BIT_AND, 0, operand);
                report_syntax_error(0xf4, target->name);
                return 0;
            }
            function = ((OverloadSet *)target)->functions;
            if (kind == 17 && operation == OP_BIT_AND) {
                ((QualifiedDeclarator *)symbol)->resolution->binding->target = (ClassReference *)function;
                return member_pointer_expression((QualifiedDeclarator *)symbol);
            }
            operand->value.pointer = function;
            operand->type = strip_based_shape(function->type);
        }
    }
    shape_flags = operand->type->related ? operand->type->related->flags : 0;
    result = new_typed_unary_expression(operation, operand->type, operand);
    if (!check_expression_operands(operation, 1, 0, operand)) return 0;
    switch (operation) {
    case OP_BIT_AND:
    case 0x89:
        if ((operand->flags & 7) == 3) {
            report_syntax_error(0x65, token_spellings[operation]);
            return operand;
        }
        kind = shape_flags & 0x77;
        if (kind == 4) {
            operand->type = function_designator_type(operand->type);
            operand->type = change_function_reference_kind(operand->type, 2);
            return operand;
        }
        if (shape_flags & 8) {
            operand->type = change_function_reference_kind(operand->type, 2);
            return operand;
        }
        if (requires_address_temporary(result)) return make_address_temporary(result);
        lvalue = modifiable_lvalue(operand, 0);
        if (!lvalue) {
            if (kind == 2) {
                lvalue = operand;
                if ((operand->flags & 7) == 5) return operand;
            } else if (kind == 3 || kind == 0x44) {
                if (shape_flags & 0x400) report_syntax_error(0x66, token_spellings[operation]);
                return operand;
            } else {
                if (original_operation == OP_LOGICAL_AND) return operand;
                report_syntax_error(0x66, token_spellings[operation]);
                return 0;
            }
        }
        attributes = operand->type->attributes;
        if ((lvalue->flags & 7) == 4 && lvalue->kind != 0xa7) {
            symbol = lvalue->value.pointer;
            if (symbol->attributes & 2) {
                pointer_base_symbol = symbol->type->related->data.based_symbol;
                attributes = symbol->type->related->attributes;
            }
            if (normalized_Ol && options.configuration.option_qc) {
                inline_scopes[inline_scope_depth].flags |= 2;
                slot = find_inline_parameter_slot(inline_parameter_slots, inline_slot_count, symbol->index);
                if (slot != -1) {
                    inline_parameter_slots[(unsigned char)slot] = 0xffff;
                    region = (unsigned short)(declaration_type_index - 1);
                    while (inline_base_region < region) {
                        for (index = 0; index < inline_scope_depth; ++index)
                            if (inline_scopes[inline_scope_depth-index].region == region) break;
                        if (index >= inline_scope_depth) {
                            if (declaration_type_stack[region].left_slot == (unsigned char)slot)
                                declaration_type_stack[region].left_slot = 0xff;
                            if (declaration_type_stack[region].right_slot == (unsigned char)slot)
                                declaration_type_stack[region].right_slot = 0xff;
                        }
                        --region;
                    }
                }
            }
            symbol->attributes |= 0x800;
            if ((symbol->storage[0] & 7) >= 5 && (symbol->storage[0] & 7) <= 6)
                symbol->storage[0] = (symbol->storage[0] & 0xfc) | 4;
        } else {
            if (lvalue->kind == 0x3e && operation == OP_BIT_AND)
                report_syntax_error(0x68, token_spellings[operation]);
            if (lvalue->kind == OP_ARROW &&
                (((IrToken *)lvalue->value.pointer)->flags & 7) == 3) {
                right = lvalue->tail.right;
                right->value.integer += ((IrToken *)lvalue->value.pointer)->value.integer;
                lvalue = right;
            } else {
                symbol = find_expression_symbol(lvalue);
                if (symbol) {
                    symbol->attributes |= 0x800;
                    if (!(attributes & 15)) attributes |= symbol->type->attributes & 15;
                    if (attributes & 4)
                        pointer_base_symbol = symbol->type->related->data.based_symbol;
                } else {
                    base = find_expression_type_base(lvalue);
                    if (base) {
                        shape = base->type->related;
                        if (shape->attributes & 4) pointer_base_symbol = shape->data.based_symbol;
                        attributes = shape->attributes;
                    } else if ((cpp_parser_flags & 0x10) && lvalue->kind == 0x7a &&
                        (!lvalue->type || !lvalue->type->related ||
                         !(lvalue->type->related->flags & 0x41))) {
                        report_syntax_error(0x66, token_spellings[operation]);
                        return 0;
                    }
                }
            }
        }
        if (!(attributes & 15)) attributes = normalize_type_attributes(1, attributes);
        lvalue->type = make_qualified_type(operand->type, 1, (unsigned short)(attributes & 15), &pointer_base_symbol);
        if (cpp_parser_flags & 0x10) lvalue->flags |= 0x20;
        return lvalue;

    case OP_NOT:
        if (!(operand->type->flags & 0x340) && !(shape_flags & 3)) {
            report_syntax_error(0xab, token_spellings[operation]);
            return result;
        }
        result->type = type_0064;
        operand = fold_symbol_constant(operand);
        if ((operand->flags & 7) != 3) return result;
        if (!(operand->type->flags & 0x240) && !(shape_flags & 1))
            return builtin_binary_expression(OP_EQUAL, operand, make_integer_constant(0));
        value = operand->value.integer == 0;
        break;

    case OP_NEGATE:
        if (shape_flags || !(operand->type->flags & 0x340)) {
            report_syntax_error(0xab, token_spellings[operation]);
            return result;
        }
        operand = promote_arithmetic_expression(operand);
        result->type = operand->type;
        if ((operand->type->flags & 0x4000) ||
            (options.configuration.option_J && (operand->type->flags & 0x2fdf) == 0x41 &&
             !(operand->type->flags & 0x8000))) type_warning(2, 0x92, operand->type);
        operand = fold_symbol_constant(operand);
        if ((operand->flags & 7) != 3) return result;
        if (operand->type->flags & 0x100) {
            if (subtract_extended(&extended_zero, operand->value.pointer, &number)) compiler_warning(2, 0x38);
            operand->value.pointer = save_extended_constant(&number);
            return operand;
        }
        value = 0U - operand->value.integer;
        break;

    case OP_COMPLEMENT:
        if (!(operand->type->flags & 0x240) || shape_flags) {
            report_syntax_error(0xab, token_spellings[operation]);
            return result;
        }
        operand = promote_arithmetic_expression(operand);
        result->type = operand->type;
        operand = fold_symbol_constant(operand);
        if ((operand->flags & 7) != 3) return result;
        value = ~operand->value.integer;
        break;

    case OP_UNARY_PLUS:
        if (!(shape_flags & 1) && (shape_flags || !(operand->type->flags & 0x340)))
            report_syntax_error(0xab, token_spellings[operation]);
        operand = promote_arithmetic_expression(operand);
        result->type = operand->type;
        operand = fold_symbol_constant(operand);
        if ((operand->flags & 7) != 3)
            return operand->type == result->type ? operand : result;
        if (!shape_flags && (operand->type->flags & 0x100)) return operand;
        value = operand->value.integer;
        break;

    case OP_CALL:
        if (!(result->type->flags & 0x100)) return operand;
        operand = fold_symbol_constant(operand);
        if ((operand->flags & 7) == 3 || shape_flags) return operand;
        return result;

    case OP_DEREFERENCE:
    case 0xb1:
        if (reject_special_indirection(operation, (unsigned char)shape_flags)) return result->value.pointer;
        kind = shape_flags & 0x77;
        if (kind == 3 || (shape_flags & 0x44) == 0x44) {
            if (shape_flags & 8) return operand;
            operand->type = change_function_reference_kind(operand->type, 1);
            return operand;
        }
        if (kind != 1 && !(shape_flags & 0x40)) {
            if (kind == 2) return builtin_binary_expression(OP_SUBSCRIPT, operand, make_integer_constant(0));
            if (kind == 4) return operand;
            report_syntax_error(100);
            return result->value.pointer;
        }
        result->type = dereference_pointer_type(operand->type);
        if (!result->type->related) {
            if ((result->type->flags & 0x2fdf) == 0x80) {
                report_syntax_error(100);
                result = result->value.pointer;
            }
            return result;
        }
        if ((result->type->related->flags & 0x77) != 2) {
            if (token_allocation_flags & 0x800) return qualify_based_storage(result);
            return result;
        }
        operand->flags |= 0x20;
        array = result->type->related;
        attributes = (operand->type->related->attributes & 0xf77) | (array->attributes & 0x3000);
        operand->type = replace_first_type_shape(result->type, 2, attributes, &array->data.word);
        return operand;
    }
    /* As in the original, other internal operation codes reach the local
       folded-value slot without assigning it. Valid arithmetic cases above
       assign that slot before reusing the operand node. */
    operand->type = result->type;
    operand->value.integer = value;
    return operand;

invalid_member:
    report_syntax_error(0x114, token_spellings[operation]);
    return operand;
}

/* Original 0x615e0..0x6168c. Bare class operands require a builtin form
   explicitly permitted for this arity; other operands proceed directly. */
int __stdcall check_expression_operands(unsigned char operation, unsigned char arity,
    IrToken *left, IrToken *right)
{
    unsigned left_class = 0, right_class = 0;
    CType *type;
    if (left) left_class = (unsigned short)is_unqualified_class_type(left->type);
    if (right) right_class = (unsigned short)is_unqualified_class_type(right->type);
    if (!left_class && !right_class) return 1;
    if (!operator_has_builtin_fallback(operation, arity, left_class)) {
        type = left_class ? left->type : right->type;
        report_syntax_error(0x58, token_spellings[operation], type_kind_name(type->flags));
        return 0;
    }
    return 1;
}

/* Original 0x5fd50..0x5fd7e. Member-pointer shape flags disallow the
   ordinary indirection form, retaining the operation spelling diagnostic. */
int __stdcall reject_special_indirection(unsigned char operation, unsigned char shape_flags)
{
    if (shape_flags & 0x30) {
        report_syntax_error(0xab, token_spellings[operation]);
        return 1;
    }
    return 0;
}

/* Original 0x3b380..0x3b3c5. Follow transparent object wrappers and the
   type-preserving side of a conditional pair to its direct symbol, if any. */
CppSymbol *__stdcall find_expression_symbol(IrToken *expression)
{
    for (;;) {
        switch (expression->kind) {
        case 2:
            return expression->value.pointer;
        case 0x3f:
            if (expression->tail.right->type == expression->type)
                expression = expression->tail.right;
            else expression = expression->value.pointer;
            break;
        case 0x4a: case 0x59: case 0x67:
            expression = expression->value.pointer;
            break;
        default:
            return 0;
        }
    }
}

/* Original 0x5ccf0..0x5cd2c. Apply the integer promotion selected for an
   expression's type; absent expressions and unresolved types propagate null. */
IrToken *__stdcall promote_arithmetic_expression(IrToken *expression)
{
    CType *type;
    if (!expression || !expression->type) return 0;
    type = arithmetic_promotion_type(expression->type);
    if (!type) return expression;
    return convert_initializer(0x6e, type, expression);
}

/* Original 0x5cca0..0x5ccea. Enum and narrow integral values promote to
   the canonical int type; pointer/array/function shapes do not promote. */
CType *__stdcall arithmetic_promotion_type(CType *type)
{
    TypeShape *shape;
    if (!(type->flags & 0x240)) return 0;
    shape = type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    if (shape) return 0;
    if (type->flags & 0x200) return type_0064;
    if ((type->flags & 15) < 4) return type_0064;
    return 0;
}

/* Original 0x30ca0..0x30d4c. Probe the first two ordinary function
   declarations in a possibly wrapped overload list. NULL input leaves the
   original iterator unwritten; preserve that Win32 bug and reject that exact
   native path before its stale stack slot could be consumed. */
int __stdcall has_single_function(CppSymbol *symbol)
{
    CppSymbol *cursor;
    unsigned char kind;
    if (symbol) {
        kind = symbol->kind & 31;
        cursor = kind == 16 || kind == 17 ? ((OverloadSet *)symbol)->functions : symbol;
    }
#ifdef RECON_NATIVE_TEST
    else { __asm { int 3 } }
#endif
    if (!cursor) return 0;
    kind = cursor->kind & 31;
    if (kind != 6 && kind != 7) return 0;
    cursor = cursor->next;
    if (!cursor) return 1;
    kind = cursor->kind & 31;
    return kind != 6 && kind != 7;
}
