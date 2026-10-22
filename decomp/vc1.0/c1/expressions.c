/* Semantic construction of C expressions. */
#include "expressions.h"
#include "global-ir.h"

Extended extended_zero = { 0, 0, 0, 4 }; /* original 0x6c0f0 */
int target_signed_int_maximum = 2147483647; /* original 0x6bfd0 */

/* Original 0x3c750..0x3d2db, operator dispatch 0x3d2dc..0x3d3f8.
   Pointer arithmetic is expressed in byte offsets in the intermediate tree;
   array indexing adds the original dereference after scaling the index. */
IrToken *__stdcall make_binary_expression(unsigned char operation,
    IrToken *left, IrToken *right)
{
    ExpressionNode *node, *converted;
    CType *type, *copy;
    TypeShape *shape;
    ArenaPool *pool = &compiler_arena.pools[2];
    unsigned short left_shape, right_shape, qualifiers, attributes;
    unsigned size, shift, parameter;
    int swapped;

again:
    if (!left || !right) return 0;
    switch (token_categories[operation]) {
    case TOK_LOGICAL_AND: case TOK_ASSIGN: case TOK_COMMA:
    case TOK_LOGICAL_OR: case TOK_POSTDECREMENT: case TOK_POSTINCREMENT:
    case TOK_INCREMENT_OPERATOR:
    case TOK_INITIALIZING: case TOK_MEMBER_OPERATOR:
        break;
    case TOK_LEFT_BRACKET: case TOK_ASSIGNMENT_OPERATOR:
        right = promote_integer_expression(right);
        break;
    default:
        left = promote_integer_expression(left);
        right = promote_integer_expression(right);
        break;
    }
    node = (ExpressionNode *)make_typed_ir_expression(operation, left, right, 0);
    if (!check_expression_operands(operation, 2, left, right)) return 0;
    switch (operation) {
    case TOK_MINUS:
        if (left->type && left->type->related && (left->type->related->flags & 7) == 2)
            left->type = decay_array_expression_type(left);
        if (right->type && right->type->related && (right->type->related->flags & 7) == 2)
            right->type = decay_array_expression_type(right);
        node->type = usual_arithmetic_type(operation, left->type, right->type);
        if (!node->type) return 0;
        left_shape = left->type->related ? left->type->related->flags : 0;
        right_shape = right->type->related ? right->type->related->flags : 0;
        if (left_shape && (left_shape & 7) <= 2) {
            size = complete_type_size(left->type);
            if (right_shape && (right_shape & 7) <= 2) {
                check_binary_type_conversion(TOK_MINUS, left, right);
                node->type = type_0064;
                left = fold_binary_constants((IrToken *)node, left, right);
                if (size == 1) return left;
                if (!(size & (size - 1))) {
                    size >>= 1;
                    shift = 1;
                    while (size >>= 1) ++shift;
                    right = make_integer_constant(shift);
                    operation = TOK_RIGHT_SHIFT;
                } else {
                    right = make_integer_constant(size);
                    operation = TOK_DIVIDE;
                }
                goto again;
            }
            if ((right_shape & 7) == 3) {
                report_function_operand(operation, 2);
                return 0;
            }
            if (right->type->flags & 0x240) {
                constant_one = size;
                node->value.children.right = make_binary_expression(TOK_MULTIPLY, right,
                    make_constant_token(TOK_LONG_CONSTANT, &constant_one));
            } else report_syntax_error(112);
            return (IrToken *)node;
        }
        if ((left_shape & 7) == 3) {
            report_function_operand(operation, 1);
            return 0;
        }
        if (right_shape & 3) {
            report_syntax_error(113);
            return 0;
        }
        if (((left->type->flags & 0x80) && !left_shape) ||
            ((right->type->flags & 0x80) && !right_shape))
            report_syntax_error(186, token_names[operation]);
        return fold_binary_constants((IrToken *)node, left, right);

    case TOK_BIT_AND: case TOK_DIVIDE: case TOK_REMAINDER:
    case TOK_MULTIPLY: case TOK_BIT_OR: case TOK_BIT_XOR:
        node->type = usual_arithmetic_type(operation, left->type, right->type);
        if (!node->type) return 0;
        return fold_binary_constants(convert_binary_operands((IrToken *)node), left, right);

    case TOK_ASSIGN: case TOK_AND_ASSIGN: case TOK_DIVIDE_ASSIGN:
    case TOK_LEFT_SHIFT_ASSIGN: case TOK_MINUS_ASSIGN: case TOK_REMAINDER_ASSIGN:
    case TOK_MULTIPLY_ASSIGN: case TOK_OR_ASSIGN: case TOK_PLUS_ASSIGN:
    case TOK_RIGHT_SHIFT_ASSIGN: case TOK_XOR_ASSIGN: case TOK_INITIALIZING:
        converted = (ExpressionNode *)convert_binary_operands((IrToken *)node);
        if (!converted) return 0;
        converted->type = left->type;
        converted->value.children.left = modifiable_lvalue(left, operation != TOK_INITIALIZING);
        if (!converted->value.children.left) {
            report_syntax_error(106, token_names[operation]);
            return (IrToken *)converted;
        }
        shape = converted->type->related;
        if ((scanner_flags & 0x800) && shape && (shape->attributes & 0xc)) {
            if (operation == TOK_INITIALIZING || operation == TOK_ASSIGN)
                converted->value.children.right = convert_based_pointer(converted->type,
                    converted->value.children.right);
            converted = (ExpressionNode *)normalize_based_expression((IrToken *)converted);
        }
        return (IrToken *)converted;

    case TOK_COLON:
        if (left->type->related && (left->type->related->flags & 7) == 2)
            left->type = decay_array_expression_type(left);
        if (right->type->related && (right->type->related->flags & 7) == 2)
            right->type = decay_array_expression_type(right);
        node->type = usual_arithmetic_type(operation, left->type, right->type);
        if (!node->type) return 0;
        attributes = left->type->attributes;
        if ((attributes ^ right->type->attributes) & 0x3000) {
            attributes |= (attributes | right->type->attributes) & 0x3000;
            if (pool->remaining < (int)sizeof(CType)) refill_arena(&compiler_arena, pool);
            copy = (CType *)pool->next;
            pool->remaining -= sizeof(CType);
            pool->next += sizeof(CType);
            *copy = *node->type;
            copy->attributes = attributes;
            node->type = copy;
        }
        check_binary_type_conversion(operation, left, right);
        shape = node->type->related;
        if (shape && (shape->flags & 1)) {
            if ((left->flags & 0x80) && (right->flags & 0x80)) node->flags |= 0x80;
            else if (left->flags & 0x80) node->type = right->type;
            else if (right->flags & 0x80) node->type = left->type;
        }
        return (IrToken *)node;

    case TOK_PREDECREMENT: case TOK_PREINCREMENT:
    case TOK_POSTDECREMENT: case TOK_POSTINCREMENT:
        node->type = usual_arithmetic_type(operation, left->type, 0);
        if (!node->type) return 0;
        node->value.children.left = modifiable_lvalue(left, 1);
        if (!node->value.children.left) {
            report_syntax_error(105, token_names[operation]);
            return (IrToken *)node;
        }
        left_shape = node->type->related ? node->type->related->flags : 0;
        if (!left_shape) {
            node->value.children.right = convert_expression(operation, node->type,
                node->value.children.right);
            return (IrToken *)node;
        }
        if ((left_shape & 7) == 1) {
            constant_one = complete_type_size(node->type);
            node->value.children.right = make_constant_token(
                (int)constant_one > target_signed_int_maximum ? TOK_LONG_CONSTANT : TOK_INT_CONSTANT,
                &constant_one);
        } else if ((left_shape & 7) == 3)
            report_syntax_error(171, token_names[operation]);
        if ((scanner_flags & 0x800) && (node->type->related->attributes & 0xc))
            return normalize_based_expression((IrToken *)node);
        return (IrToken *)node;

    case TOK_EQUAL: case TOK_GREATER: case TOK_GREATER_EQUAL:
    case TOK_LESS: case TOK_LESS_EQUAL: case TOK_NOT_EQUAL:
        if ((left->flags & 7) == 5 || (right->flags & 7) == 5)
            compiler_warning(4, 130, token_names[operation]);
        converted = (ExpressionNode *)convert_binary_operands((IrToken *)node);
        if (!converted) return 0;
        converted->type = type_0064;
        return fold_binary_constants((IrToken *)converted, left, right);

    case TOK_LOGICAL_AND: case TOK_LOGICAL_OR:
        converted = (ExpressionNode *)convert_binary_operands((IrToken *)node);
        if (!converted) return 0;
        converted->type = type_0064;
        return fold_binary_constants((IrToken *)converted, left, right);

    case TOK_LEFT_BRACKET:
        if (left->type && left->type->related && (left->type->related->flags & 7) == 2)
            left->type = decay_array_expression_type(left);
        if (right->type && right->type->related && (right->type->related->flags & 7) == 2)
            right->type = decay_array_expression_type(right);
        node->type = usual_arithmetic_type(operation, left->type, 0);
        if (!node->type) return 0;
        left_shape = left->type->related ? left->type->related->flags : 0;
        right_shape = right->type->related ? right->type->related->flags : 0;
        if (!(left_shape && (left_shape & 7) <= 2)) {
            if (!(right_shape && (right_shape & 7) <= 2)) {
                report_syntax_error(109);
                return (IrToken *)node;
            }
            node->value.children.left = right;
            node->value.children.right = left;
            right = left;
            left = node->value.children.left;
            node->type = left->type;
            right_shape = left_shape;
        }
        if ((left->type->related->attributes & 7) == 4) {
            report_syntax_error(214);
            return (IrToken *)node;
        }
        if (right_shape) {
            report_syntax_error(107);
            return (IrToken *)node;
        }
        if (!(right->type->flags & 0x240)) {
            report_syntax_error(108);
            return (IrToken *)node;
        }
        constant_one = complete_type_size(left->type);
        node->value.children.right = make_binary_expression(TOK_MULTIPLY, right,
            make_constant_token(TOK_LONG_CONSTANT, &constant_one));
        if ((node->value.children.right->flags & 7) == 3 && left->kind == TOK_LEFT_BRACKET &&
            (((ExpressionNode *)left)->value.children.right->flags & 7) == 3) {
            node->value.children.left = ((ExpressionNode *)left)->value.children.left;
            node->value.children.right->value.integer +=
                ((ExpressionNode *)left)->value.children.right->value.integer;
        }
        qualifiers = left->type->related->attributes & 0x3000;
        node->type = drop_first_type_shape(left->type);
        if ((left->type->related->flags & 7) == 2 && qualifiers) {
            shape = node->type->related;
            if (shape) {
                if ((shape->attributes & 0x3000) != qualifiers) {
                    parameter = shape->field08;
                    node->type = replace_outer_type_shape(node->type, shape->flags,
                        shape->attributes | qualifiers, &parameter);
                }
            } else node->type->attributes |= qualifiers;
        }
        if (!node->type->related || (node->type->related->flags & 7) != 2)
            node = (ExpressionNode *)make_indirection((IrToken *)node);
        return (IrToken *)node;

    case TOK_LEFT_SHIFT: case TOK_RIGHT_SHIFT:
        node->value.children.left = left;
        node->type = left->type;
        if (!node->type) return 0;
        return fold_binary_constants(convert_binary_operands((IrToken *)node), left, right);

    case TOK_PLUS:
        swapped = 0;
        if (left->type && left->type->related && (left->type->related->flags & 7) == 2)
            left->type = decay_array_expression_type(left);
        if (right->type && right->type->related && (right->type->related->flags & 7) == 2)
            right->type = decay_array_expression_type(right);
        node->type = usual_arithmetic_type(operation, left->type, right->type);
        if (!node->type) return 0;
        left_shape = left->type->related ? left->type->related->flags : 0;
        right_shape = right->type->related ? right->type->related->flags : 0;
        if (!(left_shape & 3)) {
            if (!(right_shape & 3)) {
                if (((left->type->flags & 0x80) && !left_shape) ||
                    ((right->type->flags & 0x80) && !right_shape))
                    report_syntax_error(186, token_names[operation]);
                return fold_binary_constants((IrToken *)node, left, right);
            }
            node->value.children.left = right;
            node->value.children.right = left;
            right = left;
            left = node->value.children.left;
            swapped = 1;
            node->type = left->type;
            size = left_shape;
            left_shape = right_shape;
            right_shape = (unsigned short)size;
        }
        if ((left_shape & 7) == 3) {
            report_function_operand(operation, swapped ? 2 : 1);
            return 0;
        }
        if (right_shape & 3) {
            report_syntax_error(110);
            return 0;
        }
        if (right->type->flags & 0x240) {
            constant_one = complete_type_size(left->type);
            node->value.children.right = make_binary_expression(TOK_MULTIPLY, right,
                make_constant_token(TOK_LONG_CONSTANT, &constant_one));
            return (IrToken *)node;
        }
        report_syntax_error(111);
        return 0;

    case TOK_QUESTION:
        if (!(left->type->flags & 0x340) &&
            !(left->type->related && (left->type->related->flags & 3))) {
            report_syntax_error(155);
            return 0;
        }
        if ((left->flags & 7) == 3)
            node = (ExpressionNode *)convert_scalar_expression(1, right->type,
                left->value.integer ? ((ExpressionNode *)right)->value.children.left :
                                      ((ExpressionNode *)right)->value.children.right);
        /* Fall through to the comma expression's result-value attributes. */
    case TOK_COMMA:
        node->flags |= 0x20;
        node->type = right->type;
        if (right->flags & 0x80) node->flags |= 0x80;
        if (node->type && node->type->related && (node->type->related->flags & 7) == 2)
            node->type = decay_array_expression_type((IrToken *)node);
        return (IrToken *)node;
    }
    return (IrToken *)node;
}

/* Original 0x3c140..0x3c17d. Apply the default integer promotion by
   constructing a cast only when the type helper requests one. */
IrToken *__stdcall promote_integer_expression(IrToken *expression)
{
    CType *promoted;
    if (!expression || !expression->type) return 0;
    promoted = default_argument_promotion(expression->type);
    if (!promoted) return expression;
    return convert_scalar_expression(TOK_CAST, promoted, expression);
}

/* Original 0x3f5c0..0x3f66d. Struct/union values have a restricted set
   of legal unary/binary operators; diagnostics name their base type. */
int __stdcall check_expression_operands(unsigned char operation,
    unsigned char arity, IrToken *left, IrToken *right)
{
    unsigned short left_record = left ? (unsigned short)is_record_type(left->type) : 0;
    unsigned short right_record = right ? (unsigned short)is_record_type(right->type) : 0;
    CType *type;
    if (!left_record && !right_record) return 1;
    if (allows_record_operands(operation, arity, left_record)) return 1;
    type = left_record ? left->type : right->type;
    report_syntax_error(88, token_names[operation], type_specifier_name(type->flags));
    return 0;
}

/* Original 0x3c620..0x3c6cb. The operation occupies its original ABI
   slot even though selection depends only on the two operand types. */
CType *__stdcall usual_arithmetic_type(unsigned char operation, CType *left, CType *right)
{
    unsigned short left_flags, right_flags;
    if (!left) return right;
    if (!right) return left;
    if (left->related) {
        if (right->related && (right->flags & 0x80)) return right;
        return left;
    }
    if (right->related) return right;
    left_flags = left->flags;
    right_flags = right->flags;
    if (left_flags & 0x100) {
        if ((right_flags & 0x100) && (right_flags & 15) > (left_flags & 15)) return right;
        return left;
    }
    if (right_flags & 0x100) return right;
    if ((left_flags & 0x240) && (right_flags & 0x240)) {
        if ((left_flags ^ right_flags) & 15)
            return (right_flags & 15) >= (left_flags & 15) ? right : left;
        return left_flags & 0x4000 ? left : right;
    }
    return left;
}

/* Original 0x27930..0x280da, operator dispatch 0x280dc..0x28188.
   The constant folder preserves the original int/long spelling separately
   from width and signedness, and retains pointer-valued constant nodes. */
IrToken *__stdcall fold_binary_constants(IrToken *expression, IrToken *left, IrToken *right)
{
    CType *result_type = 0;
    unsigned short left_flags = left->type->flags, right_flags = right->type->flags;
    unsigned short is_unsigned, bytes;
    unsigned char operation;
    unsigned value, left_value, right_value;
    int is_long = 0, truth;
    Extended left_float, right_float;
    if (!expression) return 0;
    operation = expression->kind;
    if ((left->flags & 7) != 3) return expression;
    if ((right->flags & 7) != 3) {
        if (operation != TOK_LOGICAL_AND && operation != TOK_LOGICAL_OR) return expression;
        if (left_flags & 0x100) {
            left_float = *(Extended *)left->value.pointer;
            truth = compare_extended(6, &left_float, &extended_zero);
            is_unsigned = 0;
        } else {
            truth = left->value.integer != 0;
            is_unsigned = (left_flags & 0x4000) ||
                (!options.extensions_enabled && options.option_J && (left_flags & 0x2fdf) == 0x41 &&
                 !(left_flags & 0x8000));
        }
        if ((operation == TOK_LOGICAL_AND) == truth) return expression;
        constant_one = truth;
        return make_constant_token(is_unsigned ? TOK_UINT_CONSTANT : TOK_INT_CONSTANT, &constant_one);
    }
    if ((left_flags & 0x100) || (right_flags & 0x100)) {
        if (left_flags & 0x100) left_float = *(Extended *)left->value.pointer;
        else {
            if (left_flags & 0x4000)
                truth = unsigned_to_extended(&left->value.integer, &left_float);
            else truth = integer_to_extended((int *)&left->value.integer, &left_float);
            if (truth) compiler_warning(2, 56);
        }
        if (right_flags & 0x100) right_float = *(Extended *)right->value.pointer;
        else {
            if (right_flags & 0x4000)
                truth = unsigned_to_extended(&right->value.integer, &right_float);
            else truth = integer_to_extended((int *)&right->value.integer, &right_float);
            if (truth) compiler_warning(2, 56);
        }
        return fold_floating_binary(expression, &left_float, &right_float);
    }
    if (!left->type->related && !right->type->related) {
        if (!((left_flags ^ right_flags) & 15)) {
            bytes = left_flags & 15;
            is_unsigned = bytes != 1 && ((left_flags | right_flags) & 0x4000);
            if (bytes <= 4) bytes = 4;
        } else if ((right_flags & 15) < (left_flags & 15)) {
            is_unsigned = left_flags & 0x4000;
            bytes = left_flags & 15;
        } else {
            is_unsigned = right_flags & 0x4000;
            bytes = right_flags & 15;
        }
    } else {
        is_unsigned = 1;
        bytes = expression->type->flags & 15;
    }
    if (bytes == 4)
        is_long = ((left_flags & 15) && !((left_flags & 0x20) && (left_flags & 0x40))) ||
                  ((right_flags & 15) && !((right_flags & 0x20) && (right_flags & 0x40)));
    left_value = left->value.integer;
    right_value = right->value.integer;
    if (operation != TOK_LOGICAL_AND && operation != TOK_LEFT_SHIFT &&
        operation != TOK_LOGICAL_OR && operation != TOK_RIGHT_SHIFT && is_unsigned &&
        (((!(left_flags & 0x4000) && (int)left_value < 0) ||
          (!(right_flags & 0x4000) && (int)right_value < 0))) &&
        operation != TOK_BIT_AND && operation != TOK_BIT_OR && operation != TOK_BIT_XOR)
        compiler_warning(2, 308);
    switch (operation) {
    case TOK_BIT_AND: value = left_value & right_value; break;
    case TOK_LOGICAL_AND:
        value = left_value && right_value;
        result_type = type_0064; is_unsigned = 0;
        break;
    case TOK_DIVIDE:
        if (!right_value) { right->flags = 0x40; return right; }
        value = is_unsigned ? left_value / right_value : (int)left_value / (int)right_value;
        break;
    case TOK_EQUAL:
        value = left_value == right_value;
        result_type = type_0064; is_unsigned = 0;
        break;
    case TOK_GREATER:
        value = is_unsigned ? left_value > right_value : (int)left_value > (int)right_value;
        result_type = type_0064; is_unsigned = 0;
        break;
    case TOK_GREATER_EQUAL:
        value = is_unsigned ? left_value >= right_value : (int)left_value >= (int)right_value;
        result_type = type_0064; is_unsigned = 0;
        break;
    case TOK_LEFT_SHIFT:
        value = left_value << (right_value & 31);
        result_type = expression->type;
        break;
    case TOK_LESS:
        value = is_unsigned ? left_value < right_value : (int)left_value < (int)right_value;
        result_type = type_0064; is_unsigned = 0;
        break;
    case TOK_LESS_EQUAL:
        value = is_unsigned ? left_value <= right_value : (int)left_value <= (int)right_value;
        result_type = type_0064; is_unsigned = 0;
        break;
    case TOK_MINUS:
        value = left_value - right_value;
        if (subtraction_overflows(left_value, right_value, is_unsigned, bytes))
            compiler_warning(2, 307, token_names[operation]);
        break;
    case TOK_REMAINDER:
        if (!right_value) { right->flags = 0x40; return right; }
        value = is_unsigned ? left_value % right_value : (int)left_value % (int)right_value;
        break;
    case TOK_MULTIPLY:
        value = left_value * right_value;
        if (multiplication_overflows(left_value, right_value, is_unsigned, bytes))
            compiler_warning(2, 307, token_names[operation]);
        break;
    case TOK_NOT_EQUAL:
        value = left_value != right_value;
        result_type = type_0064; is_unsigned = 0;
        break;
    case TOK_BIT_OR: value = left_value | right_value; break;
    case TOK_LOGICAL_OR:
        value = left_value || right_value;
        result_type = type_0064; is_unsigned = 0;
        break;
    case TOK_PLUS: case TOK_ARROW:
        value = left_value + right_value;
        if (addition_overflows(left_value, right_value, is_unsigned, bytes))
            compiler_warning(2, 307, token_names[operation]);
        break;
    case TOK_RIGHT_SHIFT:
        value = is_unsigned ? left_value >> (right_value & 31) : (int)left_value >> (right_value & 31);
        result_type = expression->type;
        break;
    case TOK_BIT_XOR: value = left_value ^ right_value; break;
    default: return expression;
    }
    if ((operation == TOK_PLUS || operation == TOK_MINUS) && expression->type->related &&
        (expression->type->related->flags & 1)) {
        left->value.integer = value;
        return left;
    }
    if (result_type) {
        bytes = result_type->flags;
        is_long = (bytes & 15) == 4 && !((bytes & 0x20) && (bytes & 0x40));
    }
    constant_one = value;
    return make_constant_token(is_long ? (is_unsigned ? TOK_ULONG_CONSTANT : TOK_LONG_CONSTANT) :
                                        (is_unsigned ? TOK_UINT_CONSTANT : TOK_INT_CONSTANT), &constant_one);
}

/* Original 0x2d150..0x2d17b. Missing names take the diagnostic/implicit
   declaration route before the common identifier-reference wrapper. */
IrToken *__stdcall make_identifier_expression(char *name)
{
    FrontendSymbol *symbol = find_identifier_symbol(name);
    if (!symbol) symbol = recover_undeclared_identifier(name);
    if (!symbol) symbol = make_implicit_declaration(name, 0);
    return reference_identifier_symbol(symbol);
}

/* Original 0x2d180..0x2d1ce. Browser records may be restricted to names
   found in the global scope, depending on the requested browser mode. */
IrToken *__stdcall reference_identifier_symbol(FrontendSymbol *symbol)
{
    if (browser_filename && (!found_scope_depth || options.option_FR))
        browser_symbol_reference(symbol);
    return make_symbol_value(symbol);
}

/* Original 0x2d2a0..0x2d349. Data references receive a dereference;
   function designators instead receive the callable expression shape. */
IrToken *__stdcall make_symbol_value(FrontendSymbol *symbol)
{
    IrToken *expression;
    CType *type;
    unsigned kind = symbol->kind & 0x1f;
    if (kind == 5) return make_enumerator_constant((EnumeratorSymbol *)symbol);
    if ((symbol->storage[0] & 7) == 5 && (symbol->attributes & 0x400) &&
        !unevaluated_expression_depth)
        report_syntax_error(103, token_names[TOK_BIT_AND]);
    expression = make_symbol_reference(symbol);
    if (kind != 6 && kind != 7) {
        if (symbol->attributes & 0x400) return expression;
        return make_indirection(expression);
    }
    type = remove_type_prefix(symbol->type);
    if (!type || !type->related) return 0;
    expression->type = prepend_expression_type_shape(type, 11,
        type->related->attributes & 0xf77, 0);
    return expression;
}

/* Original 0x3c6d0..0x3c6fe. Address-valued symbol/array expressions
   become unary data references, retaining based-pointer normalization. */
IrToken *__stdcall make_indirection(IrToken *expression)
{
    if (!expression) return 0;
    expression = make_typed_unary_expression(TOK_DEREFERENCE, expression->type, expression);
    if (scanner_flags & 0x800) expression = normalize_based_expression(expression);
    return expression;
}

/* Original 0x3c700..0x3c746. Unary expressions occupy one ordinary
   twenty-byte IR node; the unused right-child word is left untouched. */
IrToken *__stdcall make_typed_unary_expression(unsigned char operation,
    CType *type, IrToken *operand)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    ExpressionNode *expression;
    if (pool->remaining < (int)sizeof(ExpressionNode)) refill_arena(&compiler_arena, pool);
    expression = (ExpressionNode *)pool->next;
    pool->remaining -= sizeof(ExpressionNode);
    pool->next += sizeof(ExpressionNode);
    expression->flags = 1;
    expression->kind = operation;
    expression->value.children.left = operand;
    expression->type = type;
    return (IrToken *)expression;
}

/* Original 0x41c90..0x41eac. Attributes retain their original stack
   slot although this integer-only conversion does not consume them. */
#define INTEGER_CONVERSION_WARNING() do { \
    if (operation == TOK_CAST) compiler_warning(4, 310); \
    else compiler_warning(2, 309, token_names[operation]); \
} while (0)
unsigned __stdcall convert_integer_constant(unsigned char operation, IrToken *node,
    unsigned short shape_flags, unsigned short attributes, unsigned short type_flags)
{
    unsigned value = node->value.integer, before;
    unsigned short source_flags, target_unsigned;
    /* The private ABI supplies integer/enum types on these paths. The
       original leaves widths unspecified for other base-type classes. */
    int target_bytes, source_bytes;
    if (shape_flags & 1) return value;
    if (type_flags & 0x40) target_bytes = type_flags & 15;
    else if (type_flags & 0x200) target_bytes = 4;
    source_flags = node->type->flags;
    if (!node->type->related) {
        if (source_flags & 0x40) source_bytes = source_flags & 15;
        else if (source_flags & 0x200) source_bytes = 4;
        if (target_bytes > source_bytes) {
            before = value;
            if (source_bytes == 2)
                value = source_flags & 0x4000 ? value & 0xffff : (unsigned)(int)(short)value;
            else if (source_bytes != 4) {
                if ((source_flags & 0x4000) ||
                    (!options.extensions_enabled && options.option_J && !(source_flags & 0x8000)))
                    value &= 0xff;
                else value = (unsigned)(int)(signed char)value;
            }
            if (value != before) INTEGER_CONVERSION_WARNING();
        }
    }
    target_unsigned = type_flags & 0x4000;
    if (integer_conversion_overflow(type_flags & 15, target_unsigned,
        (source_flags & 0x4000) != 0, value)) INTEGER_CONVERSION_WARNING();
    switch (target_bytes) {
    case 4: return value;
    case 2: return target_unsigned ? value & 0xffff : (unsigned)(int)(short)value;
    default:
        if (target_unsigned || (!options.extensions_enabled && options.option_J && !(type_flags & 0x8000)))
            return value & 0xff;
        return (unsigned)(int)(signed char)value;
    }
}
#undef INTEGER_CONVERSION_WARNING

/* Original 0x2b7c0..0x2b8ea. Resolve a named call before argument
   parsing, supplying an implicit int-returning declaration if necessary. */
IrToken *__stdcall prepare_named_function_call(char *name)
{
    FrontendSymbol *symbol = find_identifier_symbol(name);
    TypeShape *shape;
    IrToken *expression;
    if (!symbol) {
        compiler_warning(3, 13, name);
        symbol = declare_implicit_function(name);
        symbol->attributes |= 0x800;
        if (browser_filename && (!found_scope_depth || options.option_FR))
            browser_symbol_definition(symbol);
    }
    if (browser_filename && (!found_scope_depth || options.option_FR))
        browser_symbol_reference(symbol);
    shape = symbol->type->related;
    if (shape && !(shape->flags & 7)) shape = shape->related;
    if (shape && ((shape->flags & 7) == 4 || (shape->flags & 7) == 3)) {
        expression = make_symbol_reference(symbol);
        if (symbol->attributes & 0x2000) compiler_warning(4, 217);
        if ((shape->flags & 7) == 3) expression = make_indirection(expression);
        else expression->type = prepend_expression_type_shape(expression->type,
            (shape->flags & ~7) | 3, shape->attributes & 0xf77, 0);
        return prepare_call_expression(expression);
    }
    report_syntax_error(63, name);
    return make_indirection(make_symbol_reference(declare_implicit_function(name)));
}

/* Original 0x3d580..0x3dde8. Unary constant folding reuses the operand;
   address-taking instead recovers its lvalue and pointer-model attributes. */
IrToken *__stdcall make_unary_expression(unsigned char operation, IrToken *operand)
{
    unsigned char original_operation = operation;
    unsigned short shape_flags, kind, attributes, region;
    unsigned value;
    int slot, index;
    IrToken *result, *lvalue, *base, *right;
    FrontendSymbol *symbol;
    TypeShape *shape, *array;
    Extended number;
    if (!operand) return 0;
    if (operation == TOK_LOGICAL_AND) operation = TOK_BIT_AND;
    shape_flags = operand->type->related ? operand->type->related->flags : 0;
    result = make_typed_unary_expression(operation, operand->type, operand);
    if (!check_expression_operands(operation, 1, 0, operand)) return 0;
    switch (operation) {
    case TOK_BIT_AND: case TOK_SELF:
        if ((operand->flags & 7) == 3) {
            report_syntax_error(101, token_names[operation]);
            return operand;
        }
        kind = shape_flags & 7;
        if (kind == 4) {
            operand->type = function_designator_type(operand->type);
            operand->type = change_function_reference_kind(operand->type, 2);
            return operand;
        }
        if (shape_flags & 8) {
            operand->type = change_function_reference_kind(operand->type, 2);
            return operand;
        }
        lvalue = modifiable_lvalue(operand, 0);
        if (!lvalue) {
            if (kind == 2) {
                lvalue = operand;
                if ((operand->flags & 7) == 5) return operand;
            } else if (kind == 3) {
                if (shape_flags & 0x400) report_syntax_error(102, token_names[operation]);
                return operand;
            } else {
                if (original_operation == TOK_LOGICAL_AND) return operand;
                report_syntax_error(102, token_names[operation]);
                return 0;
            }
        }
        attributes = operand->type->attributes;
        if ((lvalue->flags & 7) == 4) {
            symbol = lvalue->value.pointer;
            if (symbol->attributes & 2) {
                pointer_base_symbol = (FrontendSymbol *)symbol->type->related->field08;
                attributes = symbol->type->related->attributes;
            }
            if (normalized_Ol && options.option_qc) {
                inline_scopes[inline_scope_depth].flags |= 2;
                slot = find_inline_parameter_slot(inline_parameter_slots, inline_slot_count, symbol->index);
                if (slot != -1) {
                    inline_parameter_slots[(unsigned char)slot] = 0xffff;
                    region = (unsigned short)(inline_region_index - 1);
                    while (inline_base_region < region) {
                        for (index = 0; index < inline_scope_depth; ++index)
                            if (inline_scopes[inline_scope_depth-index].region == region) break;
                        if (index >= inline_scope_depth) {
                            if (inline_regions[region].left_slot == (unsigned char)slot)
                                inline_regions[region].left_slot = 0xff;
                            if (inline_regions[region].right_slot == (unsigned char)slot)
                                inline_regions[region].right_slot = 0xff;
                        }
                        --region;
                    }
                }
            }
            symbol->attributes |= 0x800;
            if (original_operation == TOK_BIT_AND && (symbol->storage[0] & 7) >= 5 &&
                (symbol->storage[0] & 7) <= 6) {
                report_syntax_error(103, token_names[operation]);
                return lvalue;
            }
        } else {
            if (lvalue->kind == TOK_BIT_FIELD && operation == TOK_BIT_AND)
                report_syntax_error(104, token_names[operation]);
            if (lvalue->kind == TOK_ARROW &&
                (((ExpressionNode *)lvalue)->value.children.left->flags & 7) == 3) {
                right = ((ExpressionNode *)lvalue)->value.children.right;
                right->value.integer += ((ExpressionNode *)lvalue)->value.children.left->value.integer;
                lvalue = right;
            } else {
                symbol = find_expression_symbol(lvalue);
                if (symbol) {
                    symbol->attributes |= 0x800;
                    if (!(attributes & 15)) attributes |= symbol->type->attributes & 15;
                    if (attributes & 4)
                        pointer_base_symbol = (FrontendSymbol *)symbol->type->related->field08;
                } else {
                    base = find_expression_type_base(lvalue);
                    if (base) {
                        shape = base->type->related;
                        if (shape->attributes & 4)
                            pointer_base_symbol = (FrontendSymbol *)shape->field08;
                        attributes = shape->attributes;
                    }
                }
            }
        }
        if (!(attributes & 15)) attributes = normalize_type_attributes(1, attributes);
        lvalue->type = make_qualified_type(operand->type, 1, attributes & 15, &pointer_base_symbol);
        return lvalue;

    case TOK_LOGICAL_NOT:
        if (!(operand->type->flags & 0x340) && !(shape_flags & 3)) {
            report_syntax_error(171, token_names[TOK_LOGICAL_NOT]);
            return result;
        }
        result->type = type_0064;
        if ((operand->flags & 7) != 3) return result;
        if (!(operand->type->flags & 0x240) && !(shape_flags & 1))
            return make_binary_expression(TOK_EQUAL, operand, make_integer_constant(0));
        value = operand->value.integer == 0;
        break;

    case TOK_DEREFERENCE:
        kind = shape_flags & 7;
        if (kind == 3) {
            if (!(shape_flags & 8))
                operand->type = change_function_reference_kind(operand->type, 1);
            return operand;
        }
        if (kind == 1) {
            result->type = dereference_pointer_type(operand->type);
            shape = result->type->related;
            if (shape) {
                if ((shape->flags & 7) != 2) {
                    if (scanner_flags & 0x800) return normalize_based_expression(result);
                    return result;
                }
                operand->flags |= 0x20;
                array = shape;
                operand->type = replace_outer_type_shape(result->type, 2,
                    (operand->type->related->attributes & 0xf77) | (array->attributes & 0x3000),
                    &array->field08);
                return operand;
            }
            if ((result->type->flags & 0x2fdf) != 0x80) return result;
        } else if (kind == 2)
            return make_binary_expression(TOK_LEFT_BRACKET, operand, make_integer_constant(0));
        else if (kind == 4) return operand;
        report_syntax_error(100);
        return ((ExpressionNode *)result)->value.children.left;

    case TOK_BIT_NOT:
        if (!(operand->type->flags & 0x240) || shape_flags) {
            report_syntax_error(171, token_names[TOK_BIT_NOT]);
            return result;
        }
        operand = promote_integer_expression(operand);
        result->type = operand->type;
        if ((operand->flags & 7) != 3) return result;
        value = ~operand->value.integer;
        break;

    case TOK_UNARY_MINUS:
        if (shape_flags || !(operand->type->flags & 0x340)) {
            report_syntax_error(171, token_names[TOK_UNARY_MINUS]);
            return result;
        }
        operand = promote_integer_expression(operand);
        result->type = operand->type;
        if ((operand->type->flags & 0x4000) ||
            (!options.extensions_enabled && options.option_J && (operand->type->flags & 0x2fdf) == 0x41 &&
             !(operand->type->flags & 0x8000)))
            type_warning(2, 146, operand->type);
        if ((operand->flags & 7) != 3) return result;
        if (operand->type->flags & 0x100) {
            if (subtract_extended(&extended_zero, operand->value.pointer, &number))
                compiler_warning(2, 56);
            operand->value.pointer = save_extended_constant(&number);
            return operand;
        }
        value = 0 - operand->value.integer;
        break;

    case TOK_LEFT_PAREN:
        if (!(result->type->flags & 0x100) || (operand->flags & 7) == 3 || shape_flags)
            return operand;
        return result;

    case TOK_UNARY_PLUS:
        if (shape_flags || !(operand->type->flags & 0x340))
            report_syntax_error(171, token_names[TOK_UNARY_PLUS]);
        operand = promote_integer_expression(operand);
        result->type = operand->type;
        if ((operand->flags & 7) != 3)
            return operand->type == result->type ? operand : result;
        if (!shape_flags && (operand->type->flags & 0x100)) return operand;
        value = operand->value.integer;
        break;
    }
    operand->type = result->type;
    operand->value.integer = value;
    return operand;
}

/* Original 0x3f530..0x3f5bc. Struct/union values permit lvalue and
   selection operations while remaining invalid arithmetic operands. */
int __stdcall allows_record_operands(unsigned char operation, unsigned char arity, int left_record)
{
    if (arity == 1)
        return operation == TOK_BIT_AND || operation == TOK_LEFT_PAREN ||
               token_categories[operation] == TOK_DEREFERENCE;
    if (arity == 2) {
        switch (operation) {
        case TOK_ASSIGN: case TOK_COLON: case TOK_COMMA: case TOK_INITIALIZING:
            return 1;
        case TOK_QUESTION: return !left_record;
        default: return 0;
        }
    }
    return 1;
}

/* Original 0x3f670..0x3f7ed. Recover the underlying address expression,
   diagnose writes to const data, and retain the Microsoft cast-lvalue rule. */
IrToken *__stdcall modifiable_lvalue(IrToken *expression, int modifying)
{
    IrToken *inner, *base;
    FrontendSymbol *symbol;
    unsigned inner_bytes, outer_bytes;
    int is_const;
    if (!expression || (expression->flags & 0x20)) return 0;
    while (expression && expression->kind == TOK_LEFT_PAREN)
        expression = ((ExpressionNode *)expression)->value.children.left;
    if (expression->kind == TOK_CAST) {
        if (!options.extensions_enabled) return 0;
        if (modifying) compiler_warning(4, 213);
        do {
            inner = ((ExpressionNode *)expression)->value.children.left;
            inner_bytes = type_size(inner->type);
            outer_bytes = type_size(expression->type);
            if (inner_bytes < outer_bytes) return 0;
            expression = inner;
        } while (expression->kind == TOK_CAST);
        if ((expression->flags & 7) == 4) {
            symbol = expression->value.pointer;
            symbol->attributes |= 0x800;
        } else {
            base = token_categories[expression->kind] == TOK_DEREFERENCE ?
                ((ExpressionNode *)expression)->value.children.left : expression;
            base = find_expression_type_base(base);
            if (base && (base->flags & 7) == 4) {
                symbol = base->value.pointer;
                symbol->attributes |= 0x800;
            }
        }
    }
    if (expression->kind == TOK_BASED_REFERENCE)
        expression = ((ExpressionNode *)expression)->value.children.left;
    if (expression->kind == TOK_SEGMENT_OFFSET ||
        token_categories[expression->kind] != TOK_DEREFERENCE) return 0;
    is_const = (effective_type_qualifiers(expression->type) & 0x1000) != 0;
    if ((expression->type->flags & 0x400) && !expression->type->related) {
        symbol = (FrontendSymbol *)expression->type->field08;
        is_const |= (symbol->metadata.value & 0x04000000) != 0;
    }
    expression = ((ExpressionNode *)expression)->value.children.left;
    if (expression->kind == TOK_MEMBER) {
        inner = ((ExpressionNode *)expression)->value.children.left;
        if (inner->kind == TOK_ARGUMENT_END && !inner->type->related) return 0;
    }
    if (modifying && is_const) report_syntax_error(166);
    return expression;
}

/* Original 0x3ddf0..0x3e1b5, dispatch 0x3e1b8..0x3e2d4.
   Validate the operator's scalar categories before applying conversions.
   Compound pointer assignment scales its right operand here. */
IrToken *__stdcall convert_binary_operands(IrToken *expression)
{
    ExpressionNode *node = (ExpressionNode *)expression;
    IrToken *left = node->value.children.left;
    IrToken *right = node->value.children.right;
    CType *left_type = left->type, *right_type = right->type;
    TypeShape *left_shape = left_type->related, *right_shape = right_type->related;
    unsigned short left_flags = left_shape ? left_shape->flags : 0;
    unsigned short right_flags = right_shape ? right_shape->flags : 0;
    unsigned char operation = expression->kind;
    int invalid = 0;
    switch (operation) {
    case TOK_BIT_AND: case TOK_AND_ASSIGN:
    case TOK_LEFT_SHIFT_ASSIGN: case TOK_LEFT_SHIFT:
    case TOK_REMAINDER: case TOK_REMAINDER_ASSIGN:
    case TOK_BIT_OR: case TOK_OR_ASSIGN:
    case TOK_RIGHT_SHIFT_ASSIGN: case TOK_RIGHT_SHIFT:
    case TOK_BIT_XOR: case TOK_XOR_ASSIGN:
        if (!(left_type->flags & 0x240) || left_flags) {
            report_function_operand(operation, 1);
            invalid = 1;
        }
        if (!(right->type->flags & 0x240) || right_flags) {
            report_function_operand(operation, 2);
            invalid = 1;
        }
        return invalid ? 0 : expression;
    case TOK_LOGICAL_AND: case TOK_LOGICAL_OR:
        if (!(left_type->flags & 0x340) && !(left_flags & 3)) {
            report_function_operand(operation, 1);
            invalid = 1;
        }
        if (!(right->type->flags & 0x340) && !(right_flags & 3)) {
            report_function_operand(operation, 2);
            invalid = 1;
        }
        return invalid ? 0 : expression;
    case TOK_ASSIGN: case TOK_INITIALIZING:
        if (right_shape && (right_shape->flags & 7) == 2)
            right->type = decay_array_expression_type(right);
        check_binary_type_conversion(operation, left, right);
        return expression;
    case TOK_DIVIDE: case TOK_MULTIPLY:
        if (!(left_type->flags & 0x340) || left_flags) {
            report_function_operand(operation, 1);
            invalid = 1;
        }
        if (!(right->type->flags & 0x340) || right_flags) {
            report_function_operand(operation, 2);
            invalid = 1;
        }
        return invalid ? 0 : expression;
    case TOK_DIVIDE_ASSIGN: case TOK_MULTIPLY_ASSIGN:
        if (!(left_type->flags & 0x140) || left_flags) {
            report_function_operand(operation, 1);
            invalid = 1;
        }
        if (!(right->type->flags & 0x340) || right_flags) {
            report_function_operand(operation, 2);
            invalid = 1;
        }
        if (invalid) return 0;
        check_binary_type_conversion(operation, left, right);
        return expression;
    case TOK_EQUAL: case TOK_GREATER: case TOK_GREATER_EQUAL:
    case TOK_LESS: case TOK_LESS_EQUAL: case TOK_NOT_EQUAL:
        if ((left->flags & 7) != 3 && (right->flags & 7) != 3 &&
            (left_type->flags & 0x40) && !left_flags &&
            (right_type->flags & 0x40) && !right_flags &&
            ((left_type->flags ^ right_type->flags) & 0x4000)) {
            if (!(left->kind == TOK_CAST && left_type == type_0064 &&
                  (((IrToken *)left->value.pointer)->type->flags & 0x4000)) &&
                !(right->kind == TOK_CAST && right_type == type_0064 &&
                  (((IrToken *)right->value.pointer)->type->flags & 0x4000)))
                conversion_diagnostic(18, 3, operation, left_type, right_type);
        }
        check_binary_type_conversion(operation, left, right);
        return expression;
    case TOK_MINUS_ASSIGN: case TOK_PLUS_ASSIGN:
        if (right_shape || !(right_type->flags & 0x340)) {
            report_function_operand(operation, 2);
            return 0;
        }
        if (left_type->flags & 0x200) {
            report_function_operand(operation, 1);
            return 0;
        }
        if (!left_shape && (left_type->flags & 0x140))
            check_binary_type_conversion(operation, left, right);
        else if ((left_flags & 7) == 1) {
            if (right_type->flags & 0x240) {
                constant_one = complete_type_size(left_type);
                node->value.children.right = make_binary_expression(TOK_MULTIPLY, right,
                    make_constant_token(TOK_LONG_CONSTANT, &constant_one));
            } else {
                report_syntax_error(114, token_names[operation]);
                return 0;
            }
        } else if ((left_flags & 7) == 3) {
            report_function_operand(operation, 1);
            return 0;
        }
        return expression;
    }
    return 0;
}

/* Original 0x29970..0x29ccf. Select a named aggregate member for . or ->,
   preserving the enclosing qualifiers and explicit bitfield descriptors. */
IrToken *__stdcall select_member_expression(unsigned char operation, IrToken *operand, char *name)
{
    CType *type, *result_type;
    TypeShape **shape_slot;
    TypeShape *shape;
    TypeDescription *description;
    FrontendSymbol *tag, *member;
    ExpressionNode *node;
    IrToken *result, *width, *offset;
    ArenaPool *pool = &compiler_arena.pools[2];
    unsigned kind, displacement, parameter;
    unsigned short qualifiers, flags;
    int preserve_value = 0;
    if (!operand) return 0;
    type = operand->type;
    flags = type->flags;
    kind = flags & 0x2fdf;
    if (kind != 0x412 && kind != 0x414 && kind != 0x418) {
        report_syntax_error(operation == TOK_ARROW ? 223 : 224, name);
        return 0;
    }
    tag = (FrontendSymbol *)type->field08;
    member = tag->associated.members;
    if (!member) {
        report_syntax_error(37, name, tag->name);
        return 0;
    }
    while (member && member->name != name) member = member->next;
    if (!member) {
        report_syntax_error(39, name, tag->name);
        return 0;
    }
    displacement = member->detail.value;
    shape_slot = &type->related;
    shape = type->related;
    if (operation == TOK_ARROW) {
        if (!shape) {
            report_syntax_error(232, type_specifier_name(flags));
            return 0;
        }
        if (shape->related || !(shape->flags & 3)) {
            report_syntax_error(223, member->name);
            return 0;
        }
        if ((shape->attributes & 7) == 4) {
            report_syntax_error(214);
            return 0;
        }
    } else if (shape) {
        if ((shape->flags & 3) && !shape->related)
            report_syntax_error(231, type_specifier_name(flags));
        else report_syntax_error(224, member->name);
    } else {
        operand = address_member_operand(operand);
        preserve_value = operand->flags & 0x20;
        if (operand->kind == TOK_ASSIGN && operand->type && !operand->type->related)
            preserve_value = 1;
    }
    qualifiers = type->attributes & 0x3000;
    if (token_categories[operand->kind] == TOK_MEMBER_OPERATOR) {
        ((ExpressionNode *)operand)->value.children.right->value.integer += displacement;
        result = operand;
    } else {
        if (pool->remaining < (int)sizeof(ExpressionNode)) refill_arena(&compiler_arena, pool);
        node = (ExpressionNode *)pool->next;
        pool->remaining -= sizeof(ExpressionNode);
        pool->next += sizeof(ExpressionNode);
        node->flags = 2;
        node->kind = operation;
        node->value.children.left = operand;
        node->value.children.right = make_integer_constant(displacement);
        result = (IrToken *)node;
    }
    result_type = qualify_member_type(member->type,
        operation == TOK_ARROW ? (*shape_slot)->attributes & 7 : type->attributes & 7);
    result->type = result_type;
    if (qualifiers) {
        shape = result_type->related;
        if (shape) {
            parameter = shape->field08;
            if ((shape->attributes & 0x3000) != qualifiers)
                result->type = replace_outer_type_shape(result_type, shape->flags,
                    qualifiers | shape->attributes, &parameter);
        } else if ((result_type->attributes & 0x3000) != qualifiers) {
            description = copy_type_description(result_type);
            description->attributes |= qualifiers;
            result->type = intern_type(description);
            --type_description_depth;
        }
    }
    if (member->attributes & 0x400) return result;
    if ((member->kind & 0x1f) == 2) {
        width = make_integer_constant(member->metadata.bitfield.width);
        offset = make_integer_constant(member->metadata.bitfield.offset);
        offset = make_ir_expression(TOK_COLON, offset, width);
        result = make_ir_expression(TOK_BIT_FIELD, result, offset);
        if (qualifiers) {
            description = copy_type_description(result->type);
            description->attributes |= qualifiers;
            result->type = intern_type(description);
            --type_description_depth;
        }
    }
    result = make_indirection(result);
    if (preserve_value) result->flags |= 0x20;
    return result;
}

/* Original 0x29930..0x2996d. Address the object selected by a member
   expression, retaining comma sequencing and discarding cast wrappers. */
IrToken *__stdcall address_member_operand(IrToken *operand)
{
    ExpressionNode *node;
    while (operand->kind == TOK_CAST || operand->kind == TOK_LEFT_PAREN)
        operand = ((ExpressionNode *)operand)->value.children.left;
    if (operand->kind == TOK_COMMA) {
        node = (ExpressionNode *)operand;
        node->value.children.right = address_member_operand(node->value.children.right);
        operand->type = node->value.children.right->type;
        return operand;
    }
    return make_unary_expression(TOK_LOGICAL_AND, operand);
}

/* Original 0x41490..0x414da. Type-name semantic values retain their
   declarator symbol until unsupported storage attributes are removed. */
IrToken *__stdcall make_cast_expression(FrontendSymbol *declarator, IrToken *operand)
{
    CType *type;
    if (!declarator || !operand) return 0;
    type = declarator->type;
    if (type->attributes & 15) {
        normalize_symbol_storage(type->attributes, declarator);
        type = clear_type_attributes(declarator->type, 7);
    }
    return convert_cast_expression(type, operand);
}
