/* Unary C++ operator dispatch and builtin fallback. */
#include "parser.h"
#include "operators.h"

/* Original 0x5f0f0..0x5f206. Try class operator lookup while temporarily
   permitting its evaluation context, diagnose a failed conversion when no
   builtin form is allowed, then dispatch the builtin unary operation. */
IrToken *__stdcall unary_expression(unsigned char operation, IrToken *expression)
{
    IrToken *result;
    unsigned failure, classification;
    int saved_context;
    if (!expression) return 0;
    if (expression->kind == 0xa3) {
        report_syntax_error(0x113, format_symbol_name(expression->value.pointer));
        return 0;
    }
    if (is_overload_operand(expression)) {
        saved_context = (cpp_parser_flags & 0x10) != 0;
        if (saved_context) cpp_parser_flags &= ~0x10UL;
        failure = 0;
        result = resolve_operator_expression(operation, expression, 0, 1, &failure);
        if (result) {
            if (saved_context) cpp_parser_flags |= 0x10;
            return result;
        }
        if (failure) {
            classification = expression ? (unsigned short)is_unqualified_class_type(expression->type) : 0;
            if (!operator_has_builtin_fallback(operation, 1, classification)) {
                report_operator_resolution_failure(operation, 1, failure, expression->type, 0);
                if (saved_context) cpp_parser_flags |= 0x10;
                return 0;
            }
        }
        if (saved_context) cpp_parser_flags |= 0x10;
    }
    return builtin_unary_expression(operation, expression);
}

/* Original 0x3b8a0..0x3b8bb. Expression presence precedes type lookup. */
int __stdcall is_overload_operand(IrToken *expression)
{
    if (!expression) return 0;
    return is_class_name_type(expression->type);
}
