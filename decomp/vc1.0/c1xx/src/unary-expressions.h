#ifndef UNARY_EXPRESSIONS_H
#define UNARY_EXPRESSIONS_H
#include "operators.h"
#include "floating.h"
#include "inline-analysis.h"
IrToken *__stdcall fold_symbol_constant(IrToken *expression);
int __stdcall has_single_function(CppSymbol *symbol);
CppSymbol *__stdcall find_expression_symbol(IrToken *expression);
IrToken *__stdcall promote_arithmetic_expression(IrToken *expression);
void __stdcall type_warning(unsigned level, unsigned code, CType *type);
int __stdcall requires_address_temporary(IrToken *expression);
IrToken *__stdcall make_address_temporary(IrToken *expression);
IrToken *__stdcall builtin_binary_expression(unsigned char operation, IrToken *left, IrToken *right);
IrToken *__stdcall find_expression_type_base(IrToken *expression);
int __stdcall reject_special_indirection(unsigned char operation, unsigned char shape_flags);
int __stdcall check_expression_operands(unsigned char operation, unsigned char arity,
    IrToken *left, IrToken *right);
IrToken *__stdcall modifiable_lvalue(IrToken *expression, int mode);
CType *__stdcall dereference_pointer_type(CType *type);
CType *__stdcall change_function_reference_kind(CType *type, int mode);
CType *__stdcall function_designator_type(CType *type);
IrToken *__stdcall new_typed_unary_expression(unsigned char operation, CType *type, IrToken *operand);
IrToken *__stdcall qualify_based_storage(IrToken *expression);
#endif
