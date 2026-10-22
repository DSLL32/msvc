/* Structured-exception statement lowering into frontend IR. */
#include "parser.h"
#include "ir.h"
#include "diagnostics.h"
#include <recon.h>

LabelStack *seh_exit_labels; /* original 0x600a4 */

/* Original 0x335a0..0x335ee. Every protected body owns a leave label;
   the function attributes tell the backend that exception state is needed. */
void begin_try_block(void)
{
    if (function_symbol->storage[2] & 1)
        report_syntax_error(494, token_names[TOK_TRY]);
    write_ir_record(0x8f, 0);
    function_symbol->attributes |= 0x1100;
    ++exception_scope_depth[2];
    push_label_stack(&seh_exit_labels, 26);
}

/* Original 0x33500..0x3352d. __leave branches to the innermost
   protected statement's exit, which is shared by its normal completion. */
void emit_leave_statement(void)
{
    if (seh_exit_labels)
        write_ir_record(0x3a, seh_exit_labels->value.label);
    else
        report_syntax_error(703);
}

/* Original 0x335f0..0x33624. The exit label is defined after dropping
   the protected depth, so forward references retain their outer depth. */
void finish_try_block(void)
{
    adjust_seh_label_depths();
    --exception_scope_depth[2];
    emit_code_label(seh_exit_labels->value.label);
    seh_exit_labels = seh_exit_labels->next;
    if (!--label_stack_count) release_arena(3);
}

/* Original 0x2c710..0x2c75e. Leaving a protected region moves pending
   label references outward; already defined labels acquire an entry barrier. */
void adjust_seh_label_depths(void)
{
    FrontendSymbol *symbol;
    int depth;
    unsigned saved_depth;
    for (symbol = label_symbols; symbol; symbol = symbol->next) {
        depth = exception_scope_depth[2] + exception_scope_depth[1] + exception_scope_depth[0];
        saved_depth = (symbol->metadata.value >> 8) & 0xff;
        if (depth <= (int)saved_depth) {
            if (symbol->flags & 1)
                symbol->attributes |= 0x200;
            else
                symbol->metadata.value = (symbol->metadata.value & 0xffff00ffUL) |
                    ((unsigned char)(saved_depth - 1) << 8);
        }
    }
}

/* Original 0x33580..0x33597. Finally bodies have a separate nesting
   depth so exception filters can diagnose illegal handler combinations. */
void begin_finally_block(void)
{
    write_ir_record(0x8e, 0);
    ++exception_scope_depth[1];
}

/* Original 0x33650..0x3366c. Close a finally body after moving pending
   labels out of its scope, then emit the common handler-end record. */
void finish_finally_block(void)
{
    adjust_seh_label_depths();
    --exception_scope_depth[1];
    write_ir_record(0x90, 0);
}

/* Original 0x33670..0x33679. Exception-code/frame expressions are
   enabled only while the parser reads an __except filter expression. */
void enter_except_filter(void)
{
    scanner_flags |= 0x04000000UL;
}

/* Original 0x33530..0x33579. Filter evaluation precedes the handler
   marker. A filter nested in a finally body is diagnosed but still nested. */
void __stdcall begin_except_block(IrToken *filter)
{
    if (exception_scope_depth[1] > 0)
        report_syntax_error(702);
    else {
        emit_ir_expression(filter);
        write_ir_record(0x8d, filter);
    }
    ++exception_scope_depth[0];
}

/* Original 0x33680..0x33689. This is the high word of the existing
   scanner_flags object, shared with ordinary lexical context tracking. */
void leave_except_filter(void)
{
    scanner_flags &= 0xfbffffffUL;
}

/* Original 0x33630..0x3364c. Except handlers use their own depth,
   but the same label adjustment and end marker as finally handlers. */
void finish_except_block(void)
{
    adjust_seh_label_depths();
    --exception_scope_depth[0];
    write_ir_record(0x90, 0);
}
