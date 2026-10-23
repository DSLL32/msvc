#ifndef BUILTIN_OPERATORS_H
#define BUILTIN_OPERATORS_H
#include "operators.h"
#include "call-expressions.h"
#include "class-type-sets.h"
#include <recon.h>
/* A class operand iterates conversion targets; a scalar operand is used once. */
typedef struct BuiltinOperandIterator {
    IrToken *expression;
    CType *category;
    unsigned require_lvalue;
    int class_conversion;
    ConversionTypeIterator conversions;
    CType *type;
} BuiltinOperandIterator;
typedef char BuiltinOperandIteratorSizeMustBe44[(sizeof(BuiltinOperandIterator) == 44) ? 1 : -1];
typedef char BuiltinOperandConversionsOffsetMustBe16[(offsetof(BuiltinOperandIterator, conversions) == 16) ? 1 : -1];
typedef char BuiltinOperandTypeOffsetMustBe40[(offsetof(BuiltinOperandIterator, type) == 40) ? 1 : -1];
void __stdcall begin_builtin_operand_types(BuiltinOperandIterator *iterator,
    IrToken *expression, CType *category, unsigned require_lvalue);
int __stdcall next_builtin_operand_type(BuiltinOperandIterator *iterator);
void __stdcall add_builtin_operator_candidate(OverloadSearch *search,
    BuiltinOperator *rule, CType *left, CType *right, unsigned *count);
#endif
