#ifndef BINARY_EXPRESSIONS_H
#define BINARY_EXPRESSIONS_H
#include "unary-expressions.h"
#include "constructor-initializers.h"
#include "expression-records.h"
#include "expression-preparation.h"
#include <recon.h>
IrToken *__stdcall convert_based_expression(CType *type, IrToken *expression);
IrToken *__stdcall new_literal_constant(unsigned char kind, void *value);
IrToken *__stdcall fold_binary_constant(IrToken *expression, IrToken *left, IrToken *right);
IrToken *__stdcall apply_member_pointer(IrToken *expression);
IrToken *__stdcall convert_pointer_operands(IrToken *expression, IrToken *left, IrToken *right);
CType *__stdcall common_binary_type(unsigned char operation, CType *left, CType *right);
int __stdcall reject_member_pointer_arithmetic(IrToken *expression);
IrToken *__stdcall convert_binary_operands(IrToken *expression);
void __stdcall warn_pointer_conversion(unsigned char operation, IrToken *left, IrToken *right);
void __stdcall report_invalid_operand(unsigned char operation, int position);
IrToken *__stdcall convert_class_condition(IrToken *expression, int mode, SourceConversionSearch *selection);
IrToken *__stdcall convert_numeric_constant(unsigned char operation, CType *type, IrToken *expression);
extern unsigned integer_constant_scratch;
extern long maximum_signed_int_constant;
void __stdcall resolve_untyped_binary_operand(unsigned char operation, IrToken **operand, IrToken *other);
IrToken * __stdcall compare_member_pointer_expression(IrToken *expression);
int __stdcall check_conversion_warning(unsigned char operation, CType *target, IrToken *value);
int __stdcall check_type_conversion_warning(unsigned char operation, CType *target, CType *source);
int __stdcall integer_constant_overflows(unsigned short width, short target_unsigned,
    short source_unsigned, unsigned value);
int __stdcall equivalent_shape_attributes(unsigned short left, unsigned short right);
void __stdcall report_incompatible_type_shapes(unsigned char operation, CType *target, CType *source);
int __stdcall check_conversion_qualifiers(unsigned char operation, unsigned short target, unsigned short source, int first_layer);
void __stdcall check_based_pointer_conversion(unsigned char operation, TypeShape *target, TypeShape *source);
IrToken *__stdcall make_assignment_lvalue(IrToken *expression);
IrToken *__stdcall make_comma_lvalue(IrToken *expression);
IrToken *__stdcall make_conditional_lvalue(IrToken *expression);
int __stdcall has_assignment_member_model(CType *type);
void __stdcall report_conversion_rank_failure(unsigned char operation, unsigned rank,
    CType *target, CType *source);
IrToken *__stdcall convert_operand_if_needed(unsigned char operation, CType *type, IrToken *expression);
IrToken *__stdcall fold_floating_binary(IrToken *expression, Extended *left, Extended *right);
int __stdcall multiplication_overflows(unsigned left, unsigned right, short unsigned_arithmetic, unsigned short bytes);
int __stdcall subtraction_overflows(int left, int right, short unsigned_arithmetic, unsigned short bytes);
IrToken *__stdcall prepare_value_operand(unsigned char operation, IrToken *expression);
void __stdcall record_conditional_cleanup(IrToken *expression);
#endif
