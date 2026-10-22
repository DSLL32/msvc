/* Function call designators and argument conversion contexts. */
#include "expressions.h"

CallContext *call_context; /* original 0x6b5d4 */
int call_designator_depth; /* original 0x6bf6c */
typedef char CallContext_must_be_36_bytes[sizeof(CallContext) == 36 ? 1 : -1];

/* Original 0x2b960..0x2bb83. Establish prototype/diagnostic state before
   arguments are parsed, including the target's intrinsic and SEH rules. */
IrToken *__stdcall prepare_call_expression(IrToken *function)
{
    IrToken *designator, *result = 0;
    FrontendSymbol *symbol;
    unsigned short kind, operation;
    if (!function) return 0;
    kind = function->type->related ? function->type->related->flags & 7 : 0;
    if (kind == 4) function->type = function_designator_type(function->type);
    else if (kind != 3) {
        report_syntax_error(64);
        return 0;
    }
    push_call_context(drop_first_type_shape(function->type));
    designator = function;
    while (token_categories[designator->kind] == TOK_DEREFERENCE)
        designator = ((ExpressionNode *)designator)->value.children.left;
    symbol = find_call_designator_symbol(designator);
    if (symbol) {
        call_context->symbol = symbol;
        if ((symbol->kind & 0x1f) == 6) {
            operation = symbol->detail.intrinsic.operation;
            if ((operation == 217 && !(scanner_flags & 0x04000000) && !exception_scope_depth[0]) ||
                (operation == 218 && !(scanner_flags & 0x04000000)) ||
                (operation == 215 && !exception_scope_depth[1]))
                report_syntax_error(707, symbol->name);
            if (operation == 216 && (function_symbol->storage[2] & 1))
                report_syntax_error(490, symbol->name);
            result = make_ir_expression(TOK_INTRINSIC, make_integer_constant(operation), 0);
            call_context->flags |= 1;
        }
        call_context->name = symbol->name;
        call_context->designator_depth = call_designator_depth;
        if (symbol->name == name_setjmp || symbol->name == name_catch) {
            if (function_symbol->storage[2] & 1) report_syntax_error(490, symbol->name);
            else function_symbol->attributes |= 0x200;
        }
    } else call_context->name = "function through ptr";
    if (!call_context->parameter && !(call_context->flags & 2) &&
        (!symbol || !(symbol->attributes & 1))) {
        if ((call_context->function_type->related->attributes & 0x70) == 0x40)
            compiler_warning(1, 72, call_context->name);
        else compiler_warning(2, 71, call_context->name);
        if (symbol) symbol->attributes |= 1;
    }
    if (!result) result = make_ir_expression(TOK_ARGUMENT_END, function, 0);
    result->type = drop_first_type_shape(call_context->function_type);
    return result;
}

/* Original 0x2bb90..0x2bc22. The function shape owns the interned
   parameter descriptor; each nested call keeps an independent cursor. */
void __stdcall push_call_context(CType *function_type)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    CallContext *context;
    ParameterInfo *parameters;
    if (pool->remaining < (int)sizeof(CallContext)) refill_arena(&compiler_arena, pool);
    context = (CallContext *)pool->next;
    pool->remaining -= sizeof(CallContext);
    pool->next += sizeof(CallContext);
    context->previous = call_context;
    context->function_type = function_type;
    parameters = (ParameterInfo *)function_type->related->field08;
    context->parameter_count = (int)parameters->count;
    context->parameter = parameters->types;
    if (context->parameter_count) context->flags |= 8;
    else context->flags &= ~8;
    if (function_type->related->flags & 0x4000) context->flags |= 2;
    else if (function_type->related->flags & 0x8000) context->flags |= 4;
    call_context = context;
}

/* Original 0x2b8f0..0x2b956. Follow the designator's value-producing
   branch and count call-result wrappers for later argument diagnostics. */
FrontendSymbol *__stdcall find_call_designator_symbol(IrToken *expression)
{
    IrToken *right;
    call_designator_depth = 1;
    for (;;) {
        switch (expression->kind) {
        case TOK_IDENTIFIER: return expression->value.pointer;
        case TOK_COLON:
            right = ((ExpressionNode *)expression)->value.children.right;
            expression = expression->type == right->type ? right :
                         ((ExpressionNode *)expression)->value.children.left;
            break;
        case TOK_ARGUMENT_END:
            ++call_designator_depth;
            /* Fall through to the expression's underlying designator. */
        case TOK_MEMBER: case TOK_LEFT_BRACKET: case TOK_BASED_REFERENCE:
            expression = ((ExpressionNode *)expression)->value.children.left;
            break;
        default: return 0;
        }
    }
}

/* Original 0x2bc30..0x2bc71. Retain both the reversed parser chain and
   a circular link used to visit arguments in their source order. */
ArgumentNode *__stdcall append_argument(ArgumentNode *arguments, IrToken *argument, int flags)
{
    ArgumentNode *node;
    if (!argument) return arguments;
    node = make_call_argument(argument, flags);
    if (!node) return 0;
    node->previous = arguments;
    if (!arguments) node->next = node;
    else {
        node->next = arguments->next;
        arguments->next = node;
    }
    return node;
}

/* Original 0x2bc80..0x2c1c4. Attach the declared parameter type where
   a prototype applies; otherwise retain the default-promotion sentinel. */
ArgumentNode *__stdcall make_call_argument(IrToken *argument, int flags)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    ArgumentNode *node;
    CType *parameter;
    TypeShape *shape, *copy;
    TypeDescription *description;
    unsigned kind;
    if (pool->remaining < (int)sizeof(ArgumentNode)) refill_arena(&compiler_arena, pool);
    node = (ArgumentNode *)pool->next;
    pool->remaining -= sizeof(ArgumentNode);
    pool->next += sizeof(ArgumentNode);
    node->flags = 2;
    node->kind = TOK_ARGUMENT;
    node->expression = argument;
    if (flags) {
        node->type = flags == 1 ? type_0000 : argument->type;
        return node;
    }
    if (!call_context) return 0;
    ++call_context->argument_count;
    shape = argument->type->related;
    if ((call_context->function_type->related->attributes & 7) == 3 &&
        shape && (shape->flags & 7) == 3 && (shape->attributes & 7) != 3)
        report_syntax_error(187);
    if (argument->type->flags == 0x80 && !(shape && (shape->flags & 3))) {
        if (call_context->designator_depth > 1)
            report_syntax_error(174, call_context->name, call_context->argument_count,
                call_context->designator_depth);
        else report_syntax_error(95, call_context->name, call_context->argument_count);
        node->type = type_0064;
        return node;
    }
    if (call_context->parameter_count > 0 && *call_context->parameter &&
        !(call_context->flags & 2)) {
        parameter = *call_context->parameter;
        if (parameter->related && (parameter->related->attributes & 4))
            node->expression = convert_based_pointer(parameter, node->expression);
        if (is_void_pointer_type(*call_context->parameter)) {
            if (!argument->type->related) {
                if (!(argument->flags & 0x80)) {
                    if (argument->type->flags & 0x40) {
                        if (call_context->designator_depth > 1)
                            compiler_warning(1, 88, call_context->name, call_context->argument_count,
                                call_context->designator_depth);
                        else compiler_warning(1, 22, call_context->name, call_context->argument_count);
                    } else {
                        if (call_context->designator_depth > 1)
                            report_syntax_error(173, call_context->name, call_context->argument_count,
                                call_context->designator_depth);
                        else report_syntax_error(172, call_context->name, call_context->argument_count);
                    }
                }
            } else if (!check_assignment_conversion(TOK_ARGUMENT_END, *call_context->parameter, argument)) {
                if (call_context->designator_depth > 1)
                    compiler_warning(1, 88, call_context->name, call_context->argument_count,
                        call_context->designator_depth);
                else compiler_warning(1, 22, call_context->name, call_context->argument_count);
            }
        } else if (!check_assignment_conversion(TOK_ARGUMENT_END, *call_context->parameter, argument)) {
            if (call_context->designator_depth > 1)
                compiler_warning(1, 89, call_context->name, call_context->argument_count,
                    call_context->designator_depth);
            else compiler_warning(1, 24, call_context->name, call_context->argument_count);
        }
        node->type = *call_context->parameter;
        if (*call_context->parameter) {
            ++call_context->parameter;
            --call_context->parameter_count;
        }
        return node;
    }
    if ((call_context->flags & 1) && argument->type->field08 && !argument->type->related)
        report_syntax_error(294, call_context->name, call_context->argument_count);
    if (call_context->flags & 2) {
        if (!(call_context->flags & 0x10)) {
            compiler_warning(1, 87, call_context->name);
            call_context->flags |= 0x10;
        }
    } else if (call_context->parameter_count <= 0 && !(call_context->flags & 4) &&
        (call_context->flags & 8) && !(call_context->flags & 0x20)) {
        if (options.extensions_enabled) compiler_warning(1, 20, call_context->name);
        else report_syntax_error(197, call_context->name);
        call_context->flags |= 0x20;
    }
    shape = argument->type->related;
    if (shape) {
        kind = shape->flags & 7;
        if (kind == 3 && !normalized_Oq) {
            node->type = argument->type;
            if (pointer_type_size(shape->attributes) < code_pointer_bytes) {
                if (pool->remaining < (int)sizeof(TypeShape)) refill_arena(&compiler_arena, pool);
                copy = (TypeShape *)pool->next;
                pool->remaining -= sizeof(TypeShape);
                pool->next += sizeof(TypeShape);
                *copy = *argument->type->related;
                copy = intern_type_shape(copy);
                description = &type_description_stack[++type_description_depth];
                *description = *argument->type;
                description->related = copy;
                node->type = intern_type(description);
                --type_description_depth;
            }
            return node;
        }
        if ((kind == 1 || kind == 2) && (shape->attributes & 4)) {
            if (!(call_context->flags & 8))
                compiler_warning(1, 23, call_context->name, call_context->argument_count);
            else if (call_context->flags & 4)
                compiler_warning(1, 25, call_context->name, call_context->argument_count);
        }
    }
    node->type = type_0000;
    return node;
}

/* Original 0x2c220..0x2c39d. Finish checking this call's arity and
   record its stack cleanup size before restoring the enclosing call. */
IrToken *__stdcall finish_call_arguments(IrToken *call)
{
    FrontendSymbol *symbol;
    TypeShape *shape;
    unsigned bytes;
    int expected;
    if (!call_context) return 0;
    if (call_context->flags & 1) {
        expected = call_context->symbol->detail.intrinsic.flags & 7;
        if (call_context->argument_count > expected)
            report_syntax_error(167, call_context->name);
        else if (call_context->argument_count < expected)
            report_syntax_error(168, call_context->name);
    } else if (call_context->parameter_count > 0 && *call_context->parameter &&
        !(call_context->flags & 2)) report_syntax_error(198, call_context->name);
    symbol = call_context->symbol;
    if (symbol && symbol->type->related) {
        shape = symbol->type->related;
        if (!(shape->flags & 7)) shape = shape->related;
        if ((shape->flags & 7) == 4 &&
            ((symbol->type->related->attributes & 0x70) == 0x60 ||
             (symbol->type->related->attributes & 0x70) == 0x40)) {
            bytes = call_argument_bytes(call);
            if (symbol->attributes & 0x4000) {
                if (((symbol->attributes & 0x800) || (shape->flags & 0x800)) &&
                    symbol->detail.value != bytes)
                    report_syntax_error(708, call_context->name);
            } else {
                if (!(symbol->attributes & 0x800) && !(shape->flags & 0x800))
                    bytes = parameter_info_bytes((ParameterInfo *)shape->field08);
                symbol->detail.value = bytes;
                symbol->attributes |= 0x4000;
            }
        }
    }
    call_context = call_context->previous;
    if (scanner_flags & 0x800) call = normalize_based_expression(call);
    return call;
}

/* Original 0x2c1d0..0x2c21f. Arguments without a declared parameter
   retain the sentinel type and use the expression's promoted size. */
unsigned __stdcall call_argument_bytes(IrToken *call)
{
    unsigned bytes = 0;
    ArgumentNode *argument;
    CType *type;
    if (!call) return 0;
    argument = (ArgumentNode *)((ExpressionNode *)call)->value.children.right;
    while (argument) {
        type = argument->type;
        if (type == type_0000) type = argument->expression->type;
        bytes += (parameter_type_size(type, 1) + 3) & ~3;
        argument = argument->previous;
    }
    return bytes;
}

/* Original 0x34020..0x34059. Prototype parameter sizes use their declared
   floating precision; each stack slot is rounded to a four-byte boundary. */
unsigned __stdcall parameter_info_bytes(ParameterInfo *parameters)
{
    unsigned bytes = 0, count = parameters->count;
    CType **type = parameters->types;
    while (count--) {
        bytes += (parameter_type_size(*type, 0) + 3) & ~3;
        ++type;
    }
    return bytes;
}
