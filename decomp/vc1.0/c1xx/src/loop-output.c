/* Loop labels, condition branches and optional QuickC loop analysis. */
#include "loop-output.h"
#include "expression-preparation.h"
#include "initialization.h"
#include "lexer.h"
#include "source-input.h"
#include <recon.h>

LabelStack *qc_loop_markers; /* original 0x91840 */
WorkList *qc_loop_analyses; /* original 0x91844 */

typedef char LabelStackSizeMustBe8[(sizeof(LabelStack) == 8) ? 1 : -1];
typedef char LabelStackLabelOffsetMustBe4[(offsetof(LabelStack, label) == 4) ? 1 : -1];

/* Original 0x507d0..0x507f5, stdcall 8. Label stacks use the same
   eight-byte link/payload nodes as the shared compiler work-list allocator. */
void __stdcall push_loop_label(LabelStack **stack, unsigned attributes)
{
    LabelStack *entry = (LabelStack *)push_work_list((WorkList **)stack, 3);
    entry->label = new_branch_label(attributes);
    ++label_stack_count;
}

/* Original 0x45600..0x4570d, stdcall 12. A loop owns control,
   continue and break labels. An increment sequence separates the first two;
   otherwise they share the same label while retaining distinct stack cells. */
void __stdcall emit_conditional_branch(IrToken *first, IrToken *condition,
    IrToken *increment)
{
    LabelStack *entry;
    push_loop_label(&control_labels, 8);
    push_loop_label(&continue_labels, 12);
    push_loop_label(&break_labels, 11);
    if (options.configuration.option_qc) push_loop_label(&qc_loop_markers, 0x20);
    if (first) emit_expression_statement(first);
    if (increment) increment = prepare_expression(increment, 2);
    if (options.configuration.option_qc) {
        if (!normalized_Oq && emit_qc_loop_control(condition, increment,
            push_work_list(&qc_loop_analyses, 3))) return;
        qc_loop_markers->label = 0;
    }
    if (increment) {
        write_ir_record(0x3a, control_labels->label);
        emit_label(continue_labels->label);
        emit_expression_statement(increment);
        entry = control_labels;
    } else {
        control_labels->label = continue_labels->label;
        entry = continue_labels;
    }
    emit_label(entry->label);
    if (condition) emit_condition_to_label(condition, break_labels->label, 0);
}

/* Original 0x45710..0x457f0, cdecl. Emit the back edge and break
   label, then release this loop's linked label and optional analysis frames. */
void finish_conditional_branch(void)
{
    if (options.configuration.option_qc) {
        if (!qc_loop_markers->label) {
            if (current_token_code == 0x85) record_line_information(1);
            write_ir_record(0x3a, continue_labels->label);
            emit_label(break_labels->label);
        } else {
            finish_qc_loop();
        }
        qc_loop_markers = qc_loop_markers->next;
        if (!normalized_Oq) qc_loop_analyses = qc_loop_analyses->next;
    } else {
        write_ir_record(0x3a, continue_labels->label);
        emit_label(break_labels->label);
    }
    control_labels = control_labels->next;
    continue_labels = continue_labels->next;
    break_labels = break_labels->next;
    label_stack_count -= options.configuration.option_qc ? 4 : 3;
    if (!label_stack_count) release_arena(3);
}
