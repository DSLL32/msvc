/* Cleanup expressions associated with the frontend's nested statement records. */
#include "record-processing.h"
#include "expression-preparation.h"
#include "return-statements.h"
#include "statement-scopes.h"
#include "constructor-initializers.h"
#include "cpp-context.h"
#include "lexer.h"
#include <setjmp.h>
#include <recon.h>

/* Original 0xa0a38: adjacent entries describe both directions of each pair.
   The pairs group compound, selection, iteration and switch delimiters. */
const unsigned char frontend_record_mates[36] = {
    0x10, 0x0f, 0x18, 0x13, 0x12, 0x15, 0x14, 0x17, 0x16, 0x11,
    0x1e, 0x1b, 0x1a, 0x1d, 0x1c, 0x19,
    0x24, 0x21, 0x20, 0x23, 0x22, 0x1f,
    0x2c, 0x27, 0x26, 0x29, 0x28, 0x2b, 0x2a, 0x25,
    0x32, 0x2f, 0x2e, 0x31, 0x30, 0x2d
};

/* Original 0x41a30..0x41abe. A function-begin record terminates a backward
   search, even when an unmatched delimiter would otherwise increase depth. */
FrontendRecord *__stdcall find_frontend_record(FrontendRecord *from,
    unsigned char kind, int recover_on_missing)
{
    unsigned char mate, found_kind;
    int depth = 1;
    mate = FRONTEND_RECORD_MATE(kind);
    while (from) {
        found_kind = from->kind & 0x3f;
        if (found_kind == 1) {
            if (kind == 1) return from;
            from = 0;
        } else {
            if (found_kind == kind) {
                if (!--depth) return from;
            } else if (found_kind == mate) ++depth;
            from = from->previous;
        }
    }
    if (recover_on_missing && ir_record_error_context) {
        reset_frontend_records();
        ir_records_suppressed = 1;
        longjmp(ir_record_error_context, -1);
    }
    return 0;
}

/* Original 0x41ac0..0x41b3e. Unlike backward lookup, the forward search
   has no function-begin barrier. Paired delimiters still count nesting. */
FrontendRecord *__stdcall find_forward_frontend_record(FrontendRecord *from,
    unsigned char kind, int recover_on_missing)
{
    unsigned char mate = FRONTEND_RECORD_MATE(kind), found_kind;
    int depth = 1;
    while (from) {
        found_kind = from->kind & 0x3f;
        if (found_kind == kind) {
            if (!--depth) return from;
        } else if (found_kind == mate) ++depth;
        from = from->next;
    }
    if (recover_on_missing && ir_record_error_context) {
        reset_frontend_records();
        ir_records_suppressed = 1;
        longjmp(ir_record_error_context, -1);
    }
    return 0;
}

/* Original 0x40fc0..0x417e0, with three dispatch tables through 0x419e0.
   An enclosing error-recovery context belongs to its caller; only a context
   installed in this invocation is cleared here. */
IrToken *__stdcall process_expression_records(IrToken *expression)
{
    jmp_buf recovery;
    IrToken *saved_expression, *payload;
    FrontendRecord *record, *boundary, marker;
    GotoFrontendRecord *jump;
    TypeShape *function_shape;
    unsigned kind, previous_kind, convention;
    int depth, active;

    if (syntax_diagnostic_count) {
        if (!ir_record_error_context) {
            saved_expression = expression;
            if (ir_records_suppressed) return expression;
            ir_record_error_context = recovery;
            if (setjmp(recovery)) {
                ir_record_error_context = 0;
                return saved_expression;
            }
        }
        if (syntax_diagnostic_count) goto finished;
    }
    if (ir_function_error_count < error_count) goto finished;
    record = ir_record_tail;
    kind = record->kind & 0x3f;
    switch (kind) {
    case 4:
        boundary = ir_record_tail->previous;
        while ((boundary->kind & 0x3f) == 0x10) boundary = boundary->previous;
        previous_kind = boundary->kind & 0x3f;
        if (previous_kind == 9 || previous_kind == 11 ||
            (previous_kind >= 0x36 && previous_kind <= 0x37)) goto finished;
        /* Fall through to the function's parameter cleanup. */
    case 10:
        boundary = find_frontend_record(ir_record_tail->previous, 1, 1);
        if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 10)
            remove_last_frontend_record();
        record = boundary;
        if ((boundary->next->kind & 0x3f) == 2) {
            function_shape = ((FunctionRecord *)boundary)->function->type->related;
            if (function_shape && !(function_shape->flags & 0x77))
                function_shape = function_shape->related;
            convention = function_shape->attributes & 0x70;
            switch (convention) {
            case 0x20: case 0x40:
                record = find_forward_frontend_record(boundary->next, 3, 1);
                while ((record->kind & 0x3f) != 2) {
                    if ((record->kind & 0x3f) == 5)
                        expression = append_symbol_destructor(expression,
                            ((SymbolFrontendRecord *)record)->symbol);
                    record = record->previous;
                }
                break;
            case 0x10: case 0x50: case 0x60: case 0x70:
                while ((record->kind & 0x3f) != 3) {
                    if ((record->kind & 0x3f) == 5)
                        expression = append_symbol_destructor(expression,
                            ((SymbolFrontendRecord *)record)->symbol);
                    record = record->next;
                }
                break;
            }
        }
        break;
    case 9: case 11:
        boundary = ir_record_tail;
        if (boundary && kind == 9)
            boundary = find_frontend_record(boundary->previous, 8, 1);
        previous_kind = boundary->previous->kind & 0x3f;
        if (previous_kind == 9 || previous_kind == 11 ||
            (previous_kind >= 0x36 && previous_kind <= 0x39)) goto finished;
        record = record->previous;
        while ((record->kind & 0x3f) != 1) {
            if ((record->kind & 0x3f) == 14) {
                expression = collect_declaration_cleanup(expression, record->previous, 2);
                if (record->kind & 0x80)
                    expression = append_symbol_destructor(expression,
                        ((SymbolFrontendRecord *)record)->symbol);
            } else if ((record->kind & 0x3f) == 16)
                record = find_frontend_record(record->previous, 15, 0);
            if (!record) break;
            record = record->previous;
        }
        marker.kind = 10; marker.flags = 0; marker.field02 = 0;
        marker.sequence = 0; marker.next = 0; marker.previous = 0;
        append_frontend_record(&marker, sizeof(marker));
        expression = process_expression_records(expression);
        break;
    case 12:
        depth = ir_expression_depth;
        for (record = ir_record_tail->previous; record; record = record->previous) {
            previous_kind = record->kind & 0x3f;
            if (previous_kind != 13 && previous_kind != 0x3a) break;
            if (previous_kind == 0x3a) {
                if (!depth) break;
                --depth;
            } else if (!(record->kind & 0x40)) {
                payload = ((ExpressionFrontendRecord *)record)->expression;
                if (record->kind & 0x80) {
                    expression = expression ?
                        new_right_typed_expression(0x40, expression, payload) : payload;
                } else expression = append_expression_destructor(expression, payload);
            }
        }
        break;
    case 16:
        active = 1;
        do {
            record = record->previous;
            previous_kind = record->kind & 0x3f;
            if (previous_kind == 14) {
                if (!active) break;
                expression = collect_declaration_cleanup(expression, record->previous, 2);
                if (record->kind & 0x80)
                    expression = append_symbol_destructor(expression,
                        ((SymbolFrontendRecord *)record)->symbol);
            } else if (previous_kind == 15) active = 0;
            else if (previous_kind == 16)
                record = find_frontend_record(record->previous, 15, 1);
        } while (active);
        break;
    case 0x15: case 0x17: case 0x1d: case 0x21: case 0x2b: case 0x31:
        if ((ir_record_tail->previous->kind & 0x3f) == 16) goto finished;
        record = find_frontend_record(ir_record_tail->previous,
            (unsigned char)FRONTEND_RECORD_MATE(kind), 1);
        do {
            record = record->next;
            previous_kind = record->kind & 0x3f;
        } while (previous_kind == 0x35 || previous_kind == 0x33 || previous_kind == 0x34);
        record = skip_declaration_records(record);
        if ((record->kind & 0x3f) == 14) {
            expression = collect_declaration_cleanup(expression, record->previous, 2);
            if (record->kind & 0x80)
                expression = append_symbol_destructor(expression,
                    ((SymbolFrontendRecord *)record)->symbol);
        }
        break;
    case 0x35:
        for (jump = (GotoFrontendRecord *)ir_goto_records; jump; jump = jump->next_goto)
            if (jump->label == ((LabelFrontendRecord *)record)->label)
                resolve_goto_cleanup(&jump->header, record, 1);
        break;
    case 0x36:
        previous_kind = ir_record_tail->previous->kind & 0x3f;
        if (previous_kind == 9 || previous_kind == 11 ||
            (previous_kind >= 0x36 && previous_kind <= 0x39)) goto finished;
        boundary = find_label_record(((LabelFrontendRecord *)ir_record_tail)->label);
        resolve_goto_cleanup(record, boundary, 3);
        break;
    case 0x38: case 0x39:
        record = ir_record_tail->previous;
        previous_kind = record->kind & 0x3f;
        if (previous_kind == 9 || previous_kind == 11 ||
            (previous_kind >= 0x36 && previous_kind <= 0x39)) goto finished;
        while ((record->kind & 0x3f) != 1) {
            previous_kind = record->kind & 0x3f;
            switch (previous_kind) {
            case 14:
                expression = collect_declaration_cleanup(expression, record->previous, 2);
                if (record->kind & 0x80)
                    expression = append_symbol_destructor(expression,
                        ((SymbolFrontendRecord *)record)->symbol);
                break;
            case 0x10: case 0x1e: case 0x24: case 0x2c: case 0x32:
                record = find_frontend_record(record->previous,
                    (unsigned char)FRONTEND_RECORD_MATE(previous_kind), 1);
                break;
            case 0x19: case 0x1f: case 0x25:
                goto finished;
            case 0x2d:
                if (kind == 0x39) goto finished;
                break;
            }
            record = record->previous;
        }
        break;
    }
finished:
    if (ir_record_error_context == recovery) ir_record_error_context = 0;
    return expression;
}

/* Original 0x3d8d0..0x3d9fd. Close the function record chain before checking
   fallthrough and deferred goto cleanups, then release the chain's state. */
void finish_function_records(void)
{
    jmp_buf recovery;
    FrontendRecord marker, *function;
    if (ir_records_suppressed || error_count == ir_function_error_count) {
        if (!ir_record_tail || (ir_record_tail->kind & 0x3f) != 1) {
            marker.kind = 4; marker.flags = 0; marker.field02 = 0;
            marker.sequence = 0; marker.next = 0; marker.previous = 0;
            append_frontend_record(&marker, sizeof(marker));
            emit_nonnull_expression_statement(process_expression_records(0));
        } else remove_last_frontend_record();
        if (!has_unresolved_labels()) {
            if (syntax_diagnostic_count && !ir_record_error_context) {
                if (ir_records_suppressed) return;
                ir_record_error_context = recovery;
                if (setjmp(recovery)) {
                    ir_record_error_context = 0;
                    return;
                }
            }
            if (!function_paths_return()) {
                function = find_frontend_record(ir_record_tail, 1, 1);
                report_syntax_error(202,
                    format_symbol_name(((FunctionRecord *)function)->function));
            } else if (!error_count) finish_goto_cleanups();
            if (ir_record_error_context == recovery) ir_record_error_context = 0;
        }
    }
    reset_frontend_records();
}

/* Original 0x42090..0x420a1: nullable cleanup expressions do not introduce
   an empty statement into the intermediate stream. */
void __stdcall emit_nonnull_expression_statement(IrToken *expression)
{
    if (expression) emit_expression_statement(expression);
}

/* Original 0x3b5d0..0x3b5fa. Reserved case/default labels have a nonzero
   detail discriminator; ordinary labels must have acquired a definition. */
int has_unresolved_labels(void)
{
    CppSymbol *label;
    for (label = label_symbols; label; label = label->next)
        if (!(label->flags & 4) && !label->detail.value) return 1;
    return 0;
}

/* Original 0x420b0..0x421be. Void and C++ special-member functions have
   separate completion rules; ordinary functions require a terminal path. */
int function_paths_return(void)
{
    FrontendRecord *cursor, *end;
    CppSymbol *function;
    CType *type;
    TypeShape *shape;
    unsigned kind;
    if (!ir_record_head) return 1;
    cursor = ir_record_head;
    if ((cursor->kind & 0x3f) != 1) return 1;
    function = ((FunctionRecord *)cursor)->function;
    type = function->type;
    shape = type->related;
    if ((type->flags & 0x80) && shape && (shape->flags & 0x77) == 4 && !shape->related)
        return 1;
    if (options.configuration.extensions_enabled && cpp_context_stack->items[type->field08] &&
        is_unqualified_class_type(type) && has_class_initialization(type)) return 1;
    if (function->declaration_flags & 0x3000) return 1;
    end = find_forward_frontend_record(cursor, 4, 1);
    if ((end->previous->kind & 0x3f) == 16) {
        do end = end->previous;
        while ((end->previous->kind & 0x3f) == 16);
        kind = end->previous->kind & 0x3f;
        if (kind == 11 || kind == 9) return 1;
    }
    return scan_return_paths(&cursor, 4) == 0;
}

/* Original 0x41fb0..0x4201d. Emit deferred cleanup islands for resolved
   gotos. The two high parser bits track this emission and SEH diagnostics. */
void finish_goto_cleanups(void)
{
    GotoFrontendRecord *jump;
    FrontendRecord *label;
    for (jump = (GotoFrontendRecord *)ir_goto_records; jump; jump = jump->next_goto) {
        label = find_label_record(jump->label);
        if (label) {
            emit_label(jump->cleanup_label);
            cpp_parser_flags |= 0x40000000U;
            resolve_goto_cleanup(&jump->header, label, 2);
            write_ir_record(0x3a, symbol_reference(jump->label));
            cpp_parser_flags &= 0x3fffffffU;
        }
    }
}
