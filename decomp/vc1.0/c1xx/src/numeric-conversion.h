#ifndef NUMERIC_CONVERSION_H
#define NUMERIC_CONVERSION_H
#include "binary-expressions.h"
#include <recon.h>
IrToken *__stdcall try_class_pointer_initializer(CType *type, IrToken *expression);
IrToken *__stdcall invoke_user_conversion(CppSymbol *conversion, IrToken *expression, CType *type);
IrToken *__stdcall try_user_defined_conversion(CType *type, IrToken *expression, int convert_result);
IrToken *__stdcall new_cast_expression(CType *type, IrToken *expression);
IrToken *__stdcall convert_class_pointer(IrToken *expression, CType *type,
    CType *source, QualifiedDeclarator *path);
unsigned __stdcall convert_integral_constant(unsigned char operation, IrToken *expression,
    unsigned char shape_flags, unsigned short shape_attributes, unsigned short target_flags);
int extended_to_integer(const Extended *value, long *result);
int integer_to_extended(const long *value, Extended *result);
int unsigned_to_extended(const unsigned long *value, Extended *result);
int extended_to_double(const Extended *value, double *result);
int double_to_extended(const double *value, Extended *result);
int extended_to_float(const Extended *value, float *result);
int float_to_extended(const float *value, Extended *result);
#endif
