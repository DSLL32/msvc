/* Loop labels, condition branches and optional QuickC loop analysis. */
#include "loop-output.h"
#include "expression-preparation.h"
#include "initialization.h"
#include "lexer.h"
#include "source-input.h"
#include "unary-expressions.h"
#include "frontend-records.h"
#include "operators.h"
#include "numeric-conversion.h"
#include <limits.h>
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

/* Original 0x50840..0x509be, with its thirty-row dispatch at
   0x509c0..0x50a37. Switch labels retain the promoted condition type. */
void __stdcall begin_switch_body(IrToken *expression)
{
    unsigned char category, operation;
    unsigned replacement;
    push_loop_label(&control_labels, 0x11);
    push_loop_label(&break_labels, 0x12);
    push_loop_label(&default_labels, 0x13);
    default_labels->label = 0;
    push_loop_label(&switch_labels, 0x14);
    switch_labels->label = 0;
    if (!expression) {
        control_labels->label->type = type_0064;
        begin_switch_body_region(0);
        return;
    }
    expression = boolean_expression(expression, 1);
    category = expression->flags & 7;
    operation = expression->kind;
    if (category == 1) {
        if (operation == 0x45)
            compiler_warning(1, 144, token_spellings[0x45]);
    } else if (category == 2) {
        switch (operation) {
        case 0x3b:
        case 0x44:
        case 0x58:
            replacement = operation == 0x3b ? 0x3a :
                operation == 0x58 ? 0x56 : 0x3d;
            compiler_warning(1, 145, token_spellings[operation],
                token_spellings[replacement]);
            break;
        case 0x47:
        case 0x48:
        case 0x4d:
        case 0x4e:
        case 0x55:
            compiler_warning(1, 144, token_spellings[operation]);
            break;
        }
    }
    if ((expression->type->flags & 0x240) && !expression->type->related) {
        expression = promote_arithmetic_expression(expression);
        control_labels->label->type = intern_type((TypeDescription *)expression->type);
    } else {
        report_syntax_error(50);
        control_labels->label->type = type_0064;
    }
    expression = new_typed_unary_expression(0x90, expression->type, expression);
    current_label_symbol = (CppSymbol *)control_labels->label->value.pointer;
    emit_expression_tree(expression);
    begin_switch_body_region(expression->type);
}

/* Original 0x9fc28..0x9fc3b: five width-indexed magnitude limits.
   The following short belongs to current_declaration_scope, not this table. */
const unsigned case_unsigned_maximum[5] = { 0, UCHAR_MAX, USHRT_MAX, 0, ULONG_MAX };

/* Original 0x50ad0..0x50c75, stdcall4. Convert and insert a case in
   the switch's ordered dispatch list, retaining each source label record. */
void __stdcall add_case_label(IrToken *expression)
{
    ArenaPool *pool = &compiler_arena.pools[3];
    SwitchCaseEntry *entry, *position, *previous;
    unsigned original_value, magnitude, value;
    unsigned short flags;
    int unsigned_order;
    expression = fold_symbol_constant(expression);
    if ((expression->flags & 7) != 3) {
        report_syntax_error(51);
        return;
    }
    if (!(expression->type->flags & 0x240)) {
        report_syntax_error(52);
        return;
    }
    original_value = expression->value.integer;
    expression->type = control_labels->label->type;
    flags = expression->type->flags;
    magnitude = (int)original_value < 0 ? 0U - original_value : original_value;
    if (case_unsigned_maximum[flags & 15] < magnitude)
        compiler_warning(1, 78, original_value);
    value = convert_integral_constant(0x6d, expression, 0, 0, flags);
    if (pool->remaining < (int)sizeof(SwitchCaseEntry))
        refill_arena(&compiler_arena, pool);
    entry = (SwitchCaseEntry *)pool->next;
    pool->remaining -= sizeof(SwitchCaseEntry);
    pool->next += sizeof(SwitchCaseEntry);
    entry->value = value;
    if (!default_label) {
        default_label = new_branch_label(0x13);
        emit_label(default_label);
    }
    record_case_label(expression);
    entry->label_index = ((CppSymbol *)default_label->value.pointer)->index;
    ++((CppSymbol *)default_label->value.pointer)->references;
    unsigned_order = (flags & 0x4000) ||
        (options.configuration.option_J && (flags & 0x2fdf) == 0x41 &&
         !(flags & 0x8000));
    position = (SwitchCaseEntry *)default_labels->label;
    previous = 0;
    while (position && (unsigned_order ? position->value < value :
        (int)position->value < (int)value)) {
        previous = position;
        position = position->next;
    }
    if (position && position->value == value) {
        report_syntax_error(49, value);
        return;
    }
    entry->next = position;
    if (previous) previous->next = entry;
    else default_labels->label = (IrToken *)entry;
}

/* Original 0x50800..0x50832, cdecl. A do-loop owns distinct entry,
   continue and break labels before its body is compiled. */
void prepare_do_body(void)
{
    push_loop_label(&control_labels, 5);
    push_loop_label(&continue_labels, 7);
    push_loop_label(&break_labels, 6);
    emit_label(control_labels->label);
}

/* Original 0x50a40..0x50ac6, cdecl. Serialize the ordered case list
   with its controlling label, default target and lexical scope depth. */
void write_switch_dispatch(void)
{
    IrToken *dispatch;
    CppSymbol *control;
    dispatch = new_left_typed_expression(0x8f, control_labels->label,
        default_labels->label);
    dispatch->type = control_labels->label->type;
    control = (CppSymbol *)control_labels->label->value.pointer;
    control->flags |= 4;
    control->auxiliary.reference.scope_depth = declaration_mode;
    current_label_symbol = (CppSymbol *)(switch_labels->label ?
        switch_labels->label : break_labels->label)->value.pointer;
    emit_expression_tree(dispatch);
    emit_label(break_labels->label);
}

/* Original0x45290..0x45485, stdcall12. Preserve the arena allocation
   even when a condition or increment cannot use the compact qc loop form. */
int __stdcall emit_qc_loop_control(IrToken *condition, IrToken *increment,
    WorkList *analysis)
{
    ArenaPool *pool = &compiler_arena.pools[3];
    QcLoopState *loop;
    unsigned char operation, category;
    if (pool->remaining < (int)sizeof(QcLoopState)) refill_arena(&compiler_arena, pool);
    loop = (QcLoopState *)pool->next;
    pool->remaining -= sizeof(QcLoopState);
    pool->next += sizeof(QcLoopState);
    analysis->value.pointer = loop;
    if (!condition) return 0;
    operation = condition->kind;
    category = token_categories[operation];
    if (category == OP_NOT) {
        if (!describe_qc_operand(condition->value.pointer, &loop->left)) return 0;
        loop->right.value.integer = 0;
        loop->right.operation = 7;
        operation = OP_EQUAL;
    } else if (category == 0x6f) {
        if (operation >= 7 && operation <= 10 && !condition->value.integer) {
            write_ir_record(0x3a, break_labels->label);
            return 0;
        }
        operation = 0;
    } else if (category == 0x78 || category == 0x86) {
        if (!describe_qc_operand(condition->value.pointer, &loop->left)) return 0;
        if (!describe_qc_operand(condition->tail.right, &loop->right)) return 0;
    } else {
        if (!describe_qc_operand(condition, &loop->left)) return 0;
        loop->right.value.integer = 0;
        loop->right.operation = 7;
        operation = OP_NOT_EQUAL;
    }
    if (increment) {
        category = token_categories[increment->kind];
        if (category != 0x6b && category != 0x7d) return 0;
        if (!describe_qc_operand(increment->value.pointer, &loop->target)) return 0;
        if (!describe_qc_operand(increment->tail.right, &loop->step)) return 0;
        loop->increment = increment->kind;
    }
    loop->condition = operation;
    write_ir_record(0x3a, qc_loop_markers->label);
    emit_label(continue_labels->label);
    return 1;
}

/* Original0x45160..0x45285, stdcall8. Compact operands keep the
   actual symbol/value word; an indexed operand owns an adjacent8B pair. */
int __stdcall describe_qc_operand(IrToken *expression, QcOperand *operand)
{
    IrToken *left, *right;
    QcOperand *pair;
    ArenaPool *pool = &compiler_arena.pools[3];
    operand->operation = expression->kind;
    if (expression->kind == 2 || (expression->kind >= 7 && expression->kind <= 10)) {
        operand->value.integer = expression->value.integer;
        return 1;
    }
    if (expression->kind == OP_DEREFERENCE || expression->kind == 0xb1) {
        left = expression->value.pointer;
        if (!left || left->kind != 2) return 0;
        operand->value.integer = left->value.integer;
        return 1;
    }
    if (expression->kind == OP_SUBSCRIPT) {
        left = expression->value.pointer;
        right = expression->tail.right;
        if (!left || !right || left->kind != 2) return 0;
        if (token_categories[right->kind] != 0x6f || !(right->type->flags & 0x40)) return 0;
        if (pool->remaining < 2 * (int)sizeof(QcOperand)) refill_arena(&compiler_arena, pool);
        pair = (QcOperand *)pool->next;
        pool->remaining -= 2 * sizeof(QcOperand);
        pool->next += 2 * sizeof(QcOperand);
        operand->value.pair = pair;
        pair[0].value.integer = left->value.integer;
        pair[0].operation = left->kind;
        pair[1].value.integer = right->value.integer;
        pair[1].operation = right->kind;
        return 1;
    }
    return 0;
}

/* Original0x45540..0x455f1. Expand the compact increment and test at
   the loop's back edge, retaining right-before-left evaluation order. */
void finish_qc_loop(void)
{
    QcLoopState *loop = qc_loop_analyses->value.pointer;
    IrToken *left, *right;
    emit_label(control_labels->label);
    if (current_token_code == 0x85) record_line_information(1);
    if (loop->increment) {
        right = expand_qc_operand(&loop->step);
        left = expand_qc_operand(&loop->target);
        emit_expression_statement(new_left_typed_expression(loop->increment, left, right));
    }
    emit_label(qc_loop_markers->label);
    if (!loop->condition) write_ir_record(0x3a, continue_labels->label);
    else {
        right = expand_qc_operand(&loop->right);
        left = expand_qc_operand(&loop->left);
        emit_condition_to_label(binary_expression(loop->condition, left, right),
            continue_labels->label, 1);
    }
    emit_label(break_labels->label);
}
