/* Builtin C++ operator candidates and their operand type patterns. */
#include "operators.h"
#include "keywords.h"
#include "lexer.h"
#include "call-expressions.h"
#include "cpp-context.h"
#include <recon.h>

/* Original [0x61540,0x615dc). Some class operations have a builtin form
   even after overload lookup fails; token categories supply the remainder. */
int __stdcall operator_has_builtin_fallback(unsigned char operation, unsigned char arity,
    unsigned classification)
{
    if (arity == 1) {
        if (operation == OP_BIT_AND || operation == OP_CALL) return 1;
        return token_categories[operation] == 0x46;
    }
    if (arity != 2) return 1;
    if (operation == OP_ASSIGN || operation == 0x3f || operation == 0x40 ||
        operation == 0x7e) return 1;
    if (operation == 0x5f) return classification == 0;
    return token_categories[operation] == 0x9f;
}

/* The pattern flag words are replaced in place by interned descriptors at
   initialization, matching the mutable operand union in the original table. */
#define BINARY(op, left, right, assign) { op, {{left}, {right}}, assign, 0 }
#define UNARY(op, type, assign) BINARY(op, type, 0, assign)
BuiltinOperator builtin_operators[] = { /* original 0xa7c70 */
    BINARY(OP_BIT_AND, 0x40, 0x40, 0),
    BINARY(OP_ASSIGN, 0x140, 0x81, 1),
    BINARY(OP_ASSIGN, 0x84, 0x81, 1),
    BINARY(OP_EQUAL, 0x140, 0x140, 0),
    BINARY(OP_EQUAL, 0x84, 0x81, 0),
    BINARY(OP_NOT_EQUAL, 0x140, 0x140, 0),
    BINARY(OP_NOT_EQUAL, 0x84, 0x81, 0),
    BINARY(OP_GREATER, 0x140, 0x140, 0),
    BINARY(OP_GREATER_EQUAL, 0x140, 0x140, 0),
    BINARY(OP_LESS, 0x140, 0x140, 0),
    BINARY(OP_LESS_EQUAL, 0x140, 0x140, 0),
    BINARY(OP_LOGICAL_AND, 0x40, 0x40, 0),
    BINARY(OP_LOGICAL_AND, 0x84, 0x40, 0),
    BINARY(OP_LOGICAL_AND, 0x40, 0x84, 0),
    BINARY(OP_LOGICAL_AND, 0x84, 0x84, 0),
    BINARY(OP_AND_ASSIGN, 0x40, 0x40, 1),
    BINARY(OP_BIT_OR, 0x40, 0x40, 0),
    BINARY(OP_OR_ASSIGN, 0x40, 0x40, 1),
    BINARY(OP_LOGICAL_OR, 0x40, 0x40, 0),
    BINARY(OP_LOGICAL_OR, 0x84, 0x40, 0),
    BINARY(OP_LOGICAL_OR, 0x40, 0x84, 0),
    BINARY(OP_LOGICAL_OR, 0x84, 0x84, 0),
    UNARY(OP_COMPLEMENT, 0x40, 0),
    BINARY(OP_BIT_XOR, 0x40, 0x40, 0),
    BINARY(OP_XOR_ASSIGN, 0x40, 0x40, 1),
    BINARY(OP_POST_DECREMENT, 0x40, 0x40, 1),
    UNARY(OP_PRE_DECREMENT, 0x40, 1),
    BINARY(OP_POST_INCREMENT, 0x40, 0x40, 1),
    UNARY(OP_PRE_INCREMENT, 0x40, 1),
    BINARY(OP_DIVIDE, 0x140, 0x140, 0),
    BINARY(OP_DIVIDE_ASSIGN, 0x140, 0x140, 1),
    UNARY(OP_NOT, 0x140, 0),
    UNARY(OP_NOT, 0x84, 0),
    UNARY(OP_NEGATE, 0x140, 0),
    BINARY(OP_SUBTRACT, 0x140, 0x140, 0),
    BINARY(OP_SUBTRACT, 0x88, 0x40, 0),
    BINARY(OP_SUBTRACT, 0x88, 0x81, 0),
    BINARY(OP_SUBTRACT_ASSIGN, 0x140, 0x140, 1),
    BINARY(OP_SUBTRACT_ASSIGN, 0x88, 0x40, 1),
    UNARY(OP_UNARY_PLUS, 0x140, 0),
    BINARY(OP_ADD, 0x140, 0x140, 0),
    BINARY(OP_ADD, 0x88, 0x40, 0),
    BINARY(OP_ADD, 0x40, 0x88, 0),
    BINARY(OP_ADD_ASSIGN, 0x140, 0x140, 1),
    BINARY(OP_ADD_ASSIGN, 0x88, 0x40, 1),
    BINARY(OP_REMAINDER, 0x140, 0x140, 0),
    BINARY(OP_REMAINDER_ASSIGN, 0x140, 0x140, 1),
    BINARY(OP_MULTIPLY, 0x140, 0x140, 0),
    UNARY(OP_DEREFERENCE, 0x88, 0),
    BINARY(OP_MULTIPLY_ASSIGN, 0x140, 0x140, 1),
    BINARY(OP_SHIFT_LEFT, 0x40, 0x40, 0),
    BINARY(OP_SHIFT_RIGHT, 0x40, 0x40, 0),
    BINARY(OP_SHIFT_LEFT_ASSIGN, 0x40, 0x40, 1),
    BINARY(OP_SHIFT_RIGHT_ASSIGN, 0x40, 0x40, 1),
    BINARY(OP_MEMBER_POINTER, 0x8c, 0x83, 0),
    BINARY(OP_SUBSCRIPT, 0x88, 0x40, 0),
    BINARY(OP_SUBSCRIPT, 0x40, 0x88, 0),
    {0}
};
#undef UNARY
#undef BINARY

char *name_arrow_operator, *name_call_operator; /* original 0x9f20c/0x9f208 */
IrToken *operator_zero; /* original 0x9f3c8 */

/* Original 0x77d50..0x77dd6. */
void initialize_operators(void)
{
    BuiltinOperator *entry;
    OperatorOperand *operand;
    name_arrow_operator = intern_operator_name(OP_ARROW);
    name_call_operator = intern_operator_name(OP_CALL);
    for (entry = builtin_operators; entry->token; ++entry) {
        entry->name = intern_operator_name(entry->token);
        for (operand = entry->operands; operand < entry->operands + 2; ++operand)
            if ((unsigned short)operand->initial_flags)
                operand->type = make_tagged_type((unsigned short)operand->initial_flags);
    }
    token_allocation_flags |= 0x20;
    operator_zero = make_integer_constant(0);
    token_allocation_flags &= ~0x20;
    initialize_allocation_operators();
}

/* Original 0x77860..0x7789d; argument occupies a stack slot but is a byte. */
char *__stdcall intern_operator_name(unsigned char token)
{
    if (token == OP_SUBSCRIPT)
        return intern_identifier("[]");
    if (token == OP_CALL)
        return intern_identifier("()");
    return intern_identifier(token_spellings[token]);
}

/* Original 0x4f7b0..0x4f7bf. Default near allocation/deallocation forms. */
void initialize_allocation_operators(void)
{
    declare_operator_new(1);
    declare_operator_delete(1);
}

/* Original 0x4f4e0..0x4f60a. Declare the builtin operator-new signature. */
void __stdcall declare_operator_new(unsigned short attributes)
{
    unsigned char saved_mode;
    SymbolScope *saved_scope;
    CppSymbol *declaration, *parameters = 0;
    CType *base;
    DeclarationType result;
    ArenaPool *pool;

    save_declaration_context(&saved_mode, &saved_scope);
    declaration = new_declarator(name_new);
    declaration->cpp.parts.token = 151;
    declaration->type->attributes = 0x10;
    declaration_kind = 3;
    add_declaration_parameter(0, &parameters, 0, type_4064_second);
    if (attributes & 4)
        add_declaration_parameter(0, &parameters, 0, type_0802);
    declaration = attach_function_parameters(declaration, parameters);
    pool = &compiler_arena.pools[2];
    if (pool->remaining < (int)sizeof(CType))
        refill_arena(&compiler_arena, pool);
    base = (CType *)pool->next;
    pool->remaining -= sizeof(CType);
    pool->next += sizeof(CType);
    base->flags = 0x80;
    result.type = make_qualified_type(base, 1, attributes, 0);
    result.storage[0] = (declaration_storage[0] & ~7) | 1;
    result.storage[1] = declaration_storage[1];
    result.storage[2] = declaration_storage[2];
    apply_declaration_type(&result, declaration);
    find_scope_symbol(global_scope, name_new, 0);
    restore_declaration_context(saved_mode, saved_scope);
}

/* Original 0x4f610..0x4f749. The runtime delete declaration takes void *;
   based-pointer configurations additionally supply the segment parameter. */
void __stdcall declare_operator_delete(unsigned short attributes)
{
    unsigned char saved_mode;
    SymbolScope *saved_scope;
    CppSymbol *declaration, *parameters = 0;
    CType *parameter_type;
    DeclarationType result;
    ArenaPool *pool;
    save_declaration_context(&saved_mode, &saved_scope);
    pool = &compiler_arena.pools[2];
    if (pool->remaining < (int)sizeof(CType)) refill_arena(&compiler_arena, pool);
    parameter_type = (CType *)pool->next;
    pool->remaining -= sizeof(CType);
    pool->next += sizeof(CType);
    parameter_type->flags = 0x80;
    parameter_type->related = intern_type_shape(new_type_shape(1, attributes, 0));
    add_declaration_parameter(0, &parameters, 0, intern_type((TypeDescription *)parameter_type));
    if (attributes & 4) {
        parameter_type->flags = 0x802;
        parameter_type->related = 0;
        add_declaration_parameter(0, &parameters, 0, intern_type((TypeDescription *)parameter_type));
    }
    declaration = new_declarator(name_delete);
    declaration->cpp.parts.token = 152;
    declaration->type->attributes = 0x10;
    declaration_kind = 3;
    declaration = attach_function_parameters(declaration, parameters);
    result.storage[0] = (declaration_storage[0] & ~7) | 1;
    result.storage[1] = declaration_storage[1];
    result.storage[2] = declaration_storage[2];
    result.type = (CType *)copy_type_description(type_0080);
    apply_declaration_type(&result, declaration);
    --type_description_depth;
    restore_declaration_context(saved_mode, saved_scope);
}

IrToken *__stdcall resolve_operator_overload(unsigned char operation, IrToken *left,
    IrToken *right, unsigned arity, int diagnose, unsigned *failure);

/* Original 0x77d00..0x77d22. The public expression path enables the
   diagnostic/fallback policy while retaining the optional failure result. */
IrToken *__stdcall resolve_operator_expression(unsigned char operation, IrToken *left,
    IrToken *right, unsigned arity, unsigned *failure)
{
    return resolve_operator_overload(operation, left, right, arity, 1, failure);
}

IrToken *__stdcall resolve_overloaded_arrow(IrToken *object);
OverloadSearch *__stdcall new_operator_search(unsigned char operation, IrToken *left,
    IrToken *right, CType *object_type);
int __stdcall rank_operator_operands(OverloadSearch *search, IrToken *left, IrToken *right);
IrToken *__stdcall make_selected_operator_call(OverloadCandidate *candidate,
    IrToken *left, IrToken *right);
OverloadCandidate *__stdcall report_operator_overload_failure(OverloadSearch *search);

/* Original 0x77aa0..0x77cf4. Postfix operators can fall back to prefix
   overloads under extensions, with a class-name diagnostic. Other operators
   retain their exclusions, arrow recursion, candidate-count/rank stages and
   distinct failure codes. Failed stages retain their original cleanup policy. */
IrToken *__stdcall resolve_operator_overload(unsigned char operation, IrToken *left,
    IrToken *right, unsigned arity, int diagnose, unsigned *failure)
{
    IrToken *result;
    OverloadSearch *search;
    OverloadCandidate *candidate;
    CType *object_type;
    switch (operation) {
    case 0x3f: case 0x59: case 0x5f: case 0x80: case 0x97: case 0x98: case 0x9d:
        return 0;
    case OP_POST_DECREMENT: case OP_POST_INCREMENT:
        if (options.configuration.extensions_enabled && arity && diagnose) {
            result = resolve_operator_overload(operation, left, operator_zero, 1, 0, 0);
            if (result) return result;
            result = resolve_operator_overload(operation == OP_POST_INCREMENT ?
                OP_PRE_INCREMENT : OP_PRE_DECREMENT, left, right, 1, 0, 0);
            if (result) {
                compiler_warning(1, operation == OP_POST_INCREMENT ? 620 : 621,
                    ((CppSymbol *)cpp_context_stack->items[left->type->field08])->name);
                return result;
            }
        }
        right = operator_zero;
        break;
    case OP_PRE_DECREMENT: case OP_PRE_INCREMENT:
        right = 0;
        break;
    case OP_ARROW:
        return resolve_overloaded_arrow(left);
    }
    object_type = arity ? left->type : 0;
    search = new_operator_search(operation, left, right, object_type);
    if (!search) {
        if (failure) *failure = 0x2009;
        return 0;
    }
    if (!validate_overload_counts(search)) {
        if (failure) *failure = 0x2009;
        return 0;
    }
    if (!rank_operator_operands(search, left, right)) {
        if (failure) *failure = 0x200a;
        return 0;
    }
    candidate = select_overload_candidate(search, failure, report_operator_overload_failure);
    if (!candidate) {
        if (failure && !*failure) *failure = 0x200b;
        return 0;
    }
    result = make_selected_operator_call(candidate, left, right);
    destroy_overload_search(search);
    return result;
}
