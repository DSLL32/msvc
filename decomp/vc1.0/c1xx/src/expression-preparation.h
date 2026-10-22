#ifndef EXPRESSION_PREPARATION_H
#define EXPRESSION_PREPARATION_H
#include "function-definitions.h"
int __stdcall has_deferred_expression_symbols(unsigned mode);
IrToken *__stdcall new_temporary_reference(unsigned index, CType *type);
IrToken *__stdcall process_expression_records(IrToken *expression);
IrToken *__stdcall indirect_storage_expression(IrToken *reference);
void __stdcall emit_nonnull_expression_statement(IrToken *expression);
#endif
