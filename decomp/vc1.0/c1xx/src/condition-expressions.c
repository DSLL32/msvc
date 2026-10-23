/* Boolean expression preparation and constant-condition work records. */
#include "frontend-records.h"
#include "parser.h"
#include "unary-expressions.h"
#include "binary-expressions.h"
#include "return-statements.h"
#include "floating.h"
#include "cpp-context.h"
#include <recon.h>

/* Original 0x3eb50..0x3f061, stdcall8. Nonconstant class operands may
   need conversion; constants preserve their expression while recording the
   selected path or, in expression mode, the original integral value. */
IrToken *__stdcall boolean_expression(IrToken *expression, int mode)
{
    unsigned char category;
    unsigned selection;
    unsigned short flags;
    int truth;
    Extended number;
    FrontendRecord record;
    if (ir_records_suppressed || allow_incomplete_class) return expression;
    if (!expression) {
        if (mode) return expression;
        truth = 1;
        goto constant_condition;
    }
    if ((expression->flags & 7) != 5) expression = fold_symbol_constant(expression);
    category = expression->flags & 7;
    if (category != 3 && category != 5) {
        if (expression && is_unqualified_class_type(expression->type))
            expression = convert_class_condition(expression, mode, &selection);
        if (expression && is_unqualified_class_type(expression->type)) {
            prepare_expression(0, 2);
            if (selection & 0x2000)
                report_syntax_error(mode ? 450 : 451,
                    ((CppSymbol *)cpp_context_stack->items[expression->type->field08])->name);
            return new_right_typed_expression(0x40, expression, make_integer_constant(0));
        }
        return prepare_expression(expression, 3);
    }
    if (category == 5 || expression->type->related) {
        if (mode) return expression;
        truth = expression->value.integer != 0;
        goto constant_condition;
    }
    flags = expression->type->flags;
    if (flags & 0x40) {
        if (mode) {
            record_boolean_constant(expression->value.integer);
            return expression;
        }
        truth = expression->value.integer != 0;
    } else {
        if (mode) return expression;
        if (flags & 0x100) {
            number = *(Extended *)expression->value.pointer;
            truth = compare_extended(6, &number, &extended_zero) != 0;
        } else if (flags & 0x800)
            truth = expression->value.integer != 0;
        else return expression;
    }
constant_condition:
    record.kind = truth ? 0x3b : 0x3c;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
    return expression;
}
