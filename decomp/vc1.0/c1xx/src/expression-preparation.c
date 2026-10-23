/* Expression preparation and deferred-record cleanup. */
#include "expression-preparation.h"
#include "return-statements.h"
#include "lexer.h"
#include <setjmp.h>
#include <recon.h>

typedef char ExpressionJumpBufferSizeMustBe32[(sizeof(jmp_buf) == 32) ? 1 : -1];
#define INIT_EXPRESSION_MARKER() do { \
    marker.kind = 12; marker.flags = 0; marker.field02 = 0; \
    marker.sequence = 0; marker.next = 0; marker.previous = 0; \
    append_frontend_record(&marker, sizeof(marker)); \
} while (0)

/* Original 0x3e780..0x3eb43. Deferred symbol records can require keeping
   an expression result in a compiler temporary while records are processed. */
IrToken *__stdcall prepare_expression(IrToken *expression, int mode)
{
    jmp_buf recovery;
    IrToken *saved_expression, *converted, *one, *temporary, *assignment, *result;
    CType *type;
    unsigned index;
    FrontendRecord marker;
    DeclarationFrontendRecord retained, *record;
    if (syntax_diagnostic_count && !ir_record_error_context) {
        saved_expression = expression;
        if (ir_records_suppressed) return expression;
        ir_record_error_context = recovery;
        if (setjmp(recovery)) {
            ir_record_error_context = 0;
            return saved_expression;
        }
    }
    if (ir_records_suppressed || allow_incomplete_class) goto finished;
    if (ir_record_tail && ((ir_record_tail->kind & 0x3f) == 13 ||
                          (ir_record_tail->kind & 0x3f) == 0x3a)) {
        retained.header.kind = 0xcd;
        retained.header.flags = 0;
        retained.header.field02 = 0;
        retained.header.sequence = 0;
        retained.header.next = 0;
        retained.header.previous = 0;
        retained.symbol = 0;
        retained.field14 = 0;
        if (expression && has_deferred_expression_symbols(mode) && !expression->type) {
            converted = resolve_assignment_operand(type_tagged_0080, expression);
            if (!converted) converted = report_expression_error(expression->kind, expression);
            expression = converted;
            INIT_EXPRESSION_MARKER();
            one = make_integer_constant(1);
            converted = process_expression_records(expression);
            result = binary_expression(0x40, converted, one);
            if (result) expression = result;
        } else if (expression && has_deferred_expression_symbols(mode) &&
                   !(expression->type->flags == 0x80 && expression->type->related)) {
            /* The native test excludes a shaped 0x80 descriptor, rather
               than a plain void descriptor; retain that exact condition. */
            type = expression->type;
            if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
            if (next_symbol_index >= browser_symbol_limit && browser_filename)
                browser_symbol_rollover();
            index = next_symbol_index++;
            temporary = new_temporary_reference(index, type);
            assignment = new_left_typed_expression(OP_ASSIGN, temporary, expression);
            INIT_EXPRESSION_MARKER();
            expression = process_expression_records(assignment);
            temporary = new_temporary_reference(index, type);
            temporary = indirect_storage_expression(temporary);
            expression = new_ir_expression(0x40, expression, temporary, type);
        } else {
            INIT_EXPRESSION_MARKER();
            expression = process_expression_records(expression);
        }
        remove_last_frontend_record();
        ++ir_expression_depth;
        while (ir_expression_depth) {
            while (ir_record_tail && (ir_record_tail->kind & 0x3f) == 13) {
                if ((ir_record_tail->kind & 0xc0) == 0xc0) {
                    record = (DeclarationFrontendRecord *)ir_record_tail;
                    retained.symbol = record->symbol;
                    retained.field14 = record->field14;
                }
                remove_last_frontend_record();
            }
            --ir_expression_depth;
            if (!(mode & 2) || !ir_record_tail || (ir_record_tail->kind & 0x3f) != 0x3a)
                break;
            remove_last_frontend_record();
        }
        if (retained.symbol) append_frontend_record(&retained.header, sizeof(retained));
    }
finished:
    if (ir_record_error_context == recovery) ir_record_error_context = 0;
    return expression;
}

/* Original 0x3e3c0..0x3e605. Preserve a return value in a compiler
   temporary when processing pending cleanup expressions would replace it. */
IrToken *__stdcall finish_return_expression(IrToken *expression, int preserve_value)
{
    jmp_buf recovery;
    FrontendRecord marker;
    IrToken *saved_expression, *deferred, *cleanup, *temporary;
    CType *type;
    unsigned index;
    deferred = cleanup = 0;
    if (syntax_diagnostic_count && !ir_record_error_context) {
        saved_expression = expression;
        if (ir_records_suppressed) return expression;
        ir_record_error_context = recovery;
        if (setjmp(recovery)) {
            ir_record_error_context = 0;
            return saved_expression;
        }
    }
    if (ir_record_tail) {
        if ((ir_record_tail->kind & 0x3f) == 13)
            deferred = prepare_expression(0, 2);
        if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 8) {
            ir_record_tail->kind = (ir_record_tail->kind & 0xcb) | 11;
            goto process_return;
        }
    }
    marker.kind = 9;
    marker.flags = 0;
    marker.field02 = 0;
    marker.sequence = 0;
    marker.next = 0;
    marker.previous = 0;
    append_frontend_record(&marker, sizeof(marker));
process_return:
    if (!expression && preserve_value) {
        if (ir_record_error_context == recovery) ir_record_error_context = 0;
        return 0;
    }
    cleanup = process_expression_records(0);
    if (deferred || cleanup) {
        type = 0;
        temporary = 0;
        if (preserve_value && expression) {
            type = expression->type;
            if (type->flags != 0x80 || type->related) {
                if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
                if (next_symbol_index >= browser_symbol_limit && browser_filename)
                    browser_symbol_rollover();
                index = next_symbol_index++;
                temporary = new_temporary_reference(index, type);
                expression = new_left_typed_expression(0x3d, temporary, expression);
            }
        }
        if (deferred) {
            if (expression) expression = new_right_typed_expression(0x40, expression, deferred);
            else expression = deferred;
        }
        if (cleanup) {
            if (expression) expression = new_right_typed_expression(0x40, expression, cleanup);
            else expression = cleanup;
        }
        if (temporary) {
            temporary = new_temporary_reference(index, type);
            temporary = indirect_storage_expression(temporary);
            expression = new_ir_expression(0x40, expression, temporary, type);
        }
    }
    if (ir_record_error_context == recovery) ir_record_error_context = 0;
    return expression;
}

/* Original 0x61be0..0x61c8e. Assignments, increments, argument/call
   forms and volatile accesses cannot be duplicated. The native code reads the
   kind/category before its null test; retain that ordering here. */
int __stdcall expression_can_be_repeated(IrToken *expression)
{
    unsigned char kind, category, form;
    CType *type;
    for (;;) {
        kind = expression->kind;
        category = token_categories[kind];
        if (!expression) return 1;
        if (category == 0x6b || category == 0x7d || kind == 0x3d ||
            kind == 0x7a || kind == 0x81) return 0;
        type = expression->type;
        if (type->related) {
            if (type->related->attributes & 0x2000) return 0;
        } else if (type->attributes & 0x2000) return 0;
        form = expression->flags & 7;
        if (form == 2) {
            if (!expression_can_be_repeated(expression->value.pointer)) return 0;
            if (!expression_can_be_repeated(expression->tail.right)) return 0;
            return 1;
        }
        if (form != 1) return 1;
        expression = expression->value.pointer;
    }
}
