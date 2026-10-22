/* Builtin C++ operator candidates and their operand type patterns. */
#include "operators.h"
#include "keywords.h"
#include <recon.h>

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
