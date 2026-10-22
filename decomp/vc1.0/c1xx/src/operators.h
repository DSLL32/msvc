#ifndef OPERATORS_H
#define OPERATORS_H
#include "ir.h"
#include <recon.h>
/* Parser token codes, including the distinct prefix/postfix and unary forms. */
enum OperatorToken {
    OP_BIT_AND = 0x3a, OP_LOGICAL_AND = 0x3b, OP_AND_ASSIGN = 0x3c,
    OP_ASSIGN = 0x3d, OP_POST_DECREMENT = 0x41, OP_DIVIDE = 0x42,
    OP_DIVIDE_ASSIGN = 0x43, OP_EQUAL = 0x44, OP_NOT = 0x45,
    OP_DEREFERENCE = 0x46, OP_GREATER = 0x47, OP_GREATER_EQUAL = 0x48,
    OP_POST_INCREMENT = 0x49, OP_SUBSCRIPT = 0x4a, OP_SHIFT_LEFT_ASSIGN = 0x4b,
    OP_SHIFT_LEFT = 0x4c, OP_LESS = 0x4d, OP_LESS_EQUAL = 0x4e,
    OP_SUBTRACT = 0x4f, OP_SUBTRACT_ASSIGN = 0x50, OP_REMAINDER = 0x51,
    OP_REMAINDER_ASSIGN = 0x52, OP_MULTIPLY = 0x53, OP_MULTIPLY_ASSIGN = 0x54,
    OP_NOT_EQUAL = 0x55, OP_BIT_OR = 0x56, OP_OR_ASSIGN = 0x57,
    OP_LOGICAL_OR = 0x58, OP_ADD = 0x5a, OP_ADD_ASSIGN = 0x5b,
    OP_ARROW = 0x5c, OP_PRE_DECREMENT = 0x5d, OP_PRE_INCREMENT = 0x5e,
    OP_SHIFT_RIGHT_ASSIGN = 0x60, OP_SHIFT_RIGHT = 0x61, OP_COMPLEMENT = 0x62,
    OP_NEGATE = 0x63, OP_BIT_XOR = 0x64, OP_XOR_ASSIGN = 0x65,
    OP_CALL = 0x80, OP_UNARY_PLUS = 0x91, OP_MEMBER_POINTER = 0x9e
};
typedef union OperatorOperand {
    unsigned initial_flags;
    CType *type;
} OperatorOperand;
typedef struct BuiltinOperator {
    unsigned char token;
    OperatorOperand operands[2];
    int assignment;
    char *name;
} BuiltinOperator;
extern BuiltinOperator builtin_operators[];
extern char *token_spellings[213];
extern char *name_arrow_operator, *name_call_operator;
extern IrToken *operator_zero;
char *__stdcall intern_operator_name(unsigned char token);
int __stdcall is_overload_operand(IrToken *expression);
IrToken *__stdcall resolve_operator_expression(unsigned char operation, IrToken *left,
    IrToken *right, unsigned arity, unsigned *failure);
int __stdcall operator_has_builtin_fallback(unsigned char operation, unsigned char arity,
    unsigned classification);
int __stdcall report_operator_resolution_failure(unsigned char operation, unsigned char arity,
    unsigned failure, CType *left, CType *right);
void initialize_allocation_operators(void);
void __stdcall declare_operator_new(unsigned short attributes);
void __stdcall declare_operator_delete(unsigned short attributes);
typedef char BuiltinOperatorSizeMustBe20[(sizeof(BuiltinOperator) == 20) ? 1 : -1];
#endif
