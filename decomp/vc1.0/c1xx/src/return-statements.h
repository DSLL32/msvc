#ifndef RETURN_STATEMENTS_H
#define RETURN_STATEMENTS_H
#include "function-definitions.h"
#include <recon.h>
void emit_pending_function_cleanup(void);
IrToken *__stdcall finish_return_expression(IrToken *expression, int preserve_value);
IrToken *__stdcall new_right_typed_expression(unsigned char operation, IrToken *left, IrToken *right);
#endif
