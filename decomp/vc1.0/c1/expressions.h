#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H
#include "parser.h"
#include "ir.h"
#include "types.h"
extern int target_signed_int_maximum;
IrToken * __stdcall promote_integer_expression(IrToken *expression);
CType * __stdcall usual_arithmetic_type(unsigned char operation, CType *left, CType *right);
IrToken * __stdcall make_indirection(IrToken *expression);
IrToken * __stdcall fold_binary_constants(IrToken *expression, IrToken *left, IrToken *right);
CType * __stdcall decay_array_expression_type(IrToken *expression);
IrToken * __stdcall convert_binary_operands(IrToken *expression);
void __stdcall check_binary_type_conversion(unsigned char operation, IrToken *left, IrToken *right);
void __stdcall report_function_operand(unsigned char operation, int argument);
int __stdcall check_expression_operands(unsigned char operation, unsigned char arity, IrToken *left, IrToken *right);
IrToken * __stdcall modifiable_lvalue(IrToken *expression, int modifying);
IrToken * __stdcall convert_expression(unsigned char operation, CType *type, IrToken *expression);
IrToken * __stdcall convert_scalar_expression(unsigned char operation, CType *type, IrToken *expression);
int __stdcall allows_record_operands(unsigned char operation, unsigned char arity, int left_record);


/* Ten x87 value bytes and a two-byte representation tag. */
typedef struct Extended {
    unsigned low, high;
    unsigned short exponent, format;
} Extended;
extern Extended extended_zero;
int compare_extended(unsigned operation, const Extended *left, const Extended *right);
int unsigned_to_extended(const unsigned *value, Extended *number);
int integer_to_extended(const int *value, Extended *number);
IrToken * __stdcall fold_floating_binary(IrToken *expression, Extended *left, Extended *right);
int __stdcall subtraction_overflows(unsigned left, unsigned right, unsigned short is_unsigned, unsigned short bytes);
int __stdcall multiplication_overflows(unsigned left, unsigned right, unsigned short is_unsigned, unsigned short bytes);

FrontendSymbol * __stdcall recover_undeclared_identifier(char *name);
FrontendSymbol * __stdcall make_implicit_declaration(char *name, unsigned char nesting);
IrToken * __stdcall reference_identifier_symbol(FrontendSymbol *symbol);
IrToken *__stdcall make_symbol_value(FrontendSymbol *symbol);
IrToken *__stdcall make_enumerator_constant(EnumeratorSymbol *symbol);
CType *__stdcall prepend_expression_type_shape(CType *type, unsigned short kind, unsigned short attributes, void *parameter);
unsigned __stdcall convert_integer_constant(unsigned char operation, IrToken *value,
    unsigned short shape_flags, unsigned short attributes, unsigned short type_flags);
FrontendSymbol *__stdcall declare_implicit_function(char *name);
typedef struct CallContext {
    struct CallContext *previous;
    CType **parameter;
    FrontendSymbol *symbol;
    char *name;
    CType *function_type;
    int parameter_count, argument_count, designator_depth;
    unsigned char flags, reserved[3];
} CallContext;
extern CallContext *call_context;
extern int call_designator_depth;
CType *__stdcall function_designator_type(CType *type);
void __stdcall push_call_context(CType *function_type);
FrontendSymbol *__stdcall find_call_designator_symbol(IrToken *expression);
ArgumentNode *__stdcall make_call_argument(IrToken *argument, int flags);
int __stdcall is_void_pointer_type(CType *type);

CType * __stdcall change_function_reference_kind(CType *type, int kind);
CType * __stdcall dereference_pointer_type(CType *type);
FrontendSymbol * __stdcall find_expression_symbol(IrToken *expression);
IrToken * __stdcall find_expression_type_base(IrToken *expression);
int __stdcall find_inline_parameter_slot(unsigned *slots, unsigned count, unsigned symbol_index);
void __stdcall type_warning(unsigned level, unsigned code, CType *type);
int subtract_extended(const Extended *left, const Extended *right, Extended *result);
Extended * __stdcall save_extended_constant(Extended *value);
unsigned __stdcall call_argument_bytes(IrToken *call);
unsigned __stdcall parameter_info_bytes(ParameterInfo *parameters);
IrToken *__stdcall address_member_operand(IrToken *operand);
IrToken *__stdcall convert_cast_expression(CType *type, IrToken *operand);
unsigned __stdcall classify_expression_conversion(CType *target, IrToken *operand, unsigned mode, unsigned flags);
IrToken *__stdcall finish_scalar_cast(unsigned char operation, CType *target, IrToken *operand);
unsigned __stdcall classify_type_conversion(CType *target, CType *source, unsigned mode, unsigned flags);
IrToken *__stdcall convert_pointer_expression(CType *target, IrToken *operand);
IrToken *__stdcall convert_pointer_representation(CType *target, IrToken *operand);
unsigned __stdcall classify_pointer_conversion(CType *target, CType *source, unsigned mode, unsigned flags);
unsigned __stdcall classify_void_pointer_conversion(unsigned result, TypeShape *target, TypeShape *source);
unsigned __stdcall classify_pointer_qualifiers(unsigned result, unsigned short target, unsigned short source);
unsigned __stdcall classify_pointer_model(unsigned result, unsigned short target, unsigned short source, TypeShape *shape);
int float_to_extended(const float *value, Extended *number);
int double_to_extended(const double *value, Extended *number);
#endif
