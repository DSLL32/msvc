#include "functions.h"
#include "global-optimizer.h"
#include "inlining.h"
#include "optimizer-translate.h"
#include <recon.h>

extern int c2_global_optimize, c2_favor_speed;
C2Expression *__fastcall next_final_marker(C2Expression *record);
void __fastcall remove_branch_record(C2Expression *record);
void __fastcall redirect_final_symbol_uses(C2Expression *record, C2Expression *target);
int __fastcall prune_after_final_transfer(C2Expression *record);
int __fastcall fold_final_branch_target(C2Expression *record);
int __fastcall invert_final_branch(C2Expression *record);
void __fastcall number_final_peephole_records(C2Function *function)
{
    C2Expression *record, *end;
    unsigned number = 0;
    record = (C2Expression *)function->flow->last->records;
    end = (C2Expression *)function->flow->first->last_record->next;
    for (; record != end; record = record->next) {
        if ((record->flags & 1) || record->kind == 25)
            record->peephole_number = (unsigned short)++number;
    }
}
int __fastcall final_record_precedes(C2Expression *first, C2Expression *second)
{ __asm { int 3 } return 0; }
int __fastcall merge_final_branch_tails(C2Expression *first, C2Expression *second)
{ __asm { int 3 } return 0; }

int __fastcall peephole_432f03(C2Expression *record)
{
    C2SymbolInputUse *use, *other;
    C2Expression *first, *second;
    unsigned group = 0;
    int changed = 0;

    for (use = record->tail.symbol_uses; use; use = use->next) {
        first = use->expression;
        use->peephole_group = first->kind == 16 && !first->detail.data &&
            first->opcode != 0x15c ? 0 : ~0U;
    }
restart_final_tail_groups:
    for (use = record->tail.symbol_uses; use; use = use->next)
        if (use->peephole_group != ~0U) break;
    if (!use) return changed;
    first = use->expression;
    ++group;
    other = use->next;
    while (other) {
        if (other->peephole_group >= group) {
            other = other->next;
            continue;
        }
        second = other->expression;
        if (final_record_precedes(first, second)) {
            if (merge_final_branch_tails(first, second)) {
                changed = 1;
                goto restart_final_tail_groups;
            }
            other->peephole_group = group;
            other = other->next;
        } else if (merge_final_branch_tails(second, first)) {
            changed = 1;
            other = use->next;
        } else {
            other->peephole_group = group;
            other = other->next;
        }
    }
    use->peephole_group = ~0U;
    goto restart_final_tail_groups;
}
/* The relation has no exposed address: share its proved semantic column
 * between the two consumers without creating another function or raw table. */
#define FINAL_BRANCH_SUBSUMES(later, earlier) \
    ((later) == 1 ? ((earlier) == 1 || (earlier) == 5 || (earlier) == 6 || \
                    (earlier) == 9 || (earlier) == 10) : \
     (later) == 3 ? ((earlier) == 2 || (earlier) == 3 || (earlier) == 5) : \
     (later) == 4 ? ((earlier) == 2 || (earlier) == 4 || (earlier) == 6) : \
     (later) == 7 ? ((earlier) == 2 || (earlier) == 7 || (earlier) == 9) : \
     (later) == 8 ? ((earlier) == 2 || (earlier) == 8 || (earlier) == 10) : \
     ((later) != 0 && (later) != 17 && (later) == (earlier)))
C2Expression *__fastcall peephole_432924(C2Expression *record)
{
    C2Expression *next = next_final_marker(record), *scan, *resume, *target;
    unsigned first_condition, next_condition;
    int subsumes;

    if (next->kind == 16 && !next->detail.data && next->opcode != 0x15c &&
        next->left->kind == 4) {
        target = ((C2InputSymbol *)next->left->value.symbol)->input_operand;
        if (target && target == ((C2InputSymbol *)record->left->value.symbol)->input_operand) {
            resume = record->next;
            remove_branch_record(record);
            return resume;
        }
    }
    for (scan = record; scan != next->next; scan = scan->next)
        if (scan->kind == 25) return NULL;
    if (next->kind != 16 || (!next->detail.data && next->opcode != 0x15c)) return NULL;
    first_condition = record->condition_code;
    next_condition = next->condition_code;
    /* The consumed relation column at48d330 admits these18 condition codes.
     * It states when the earlier branch makes the later branch redundant.
     * Other table columns, neighboring storage and larger domains are unknown. */
    if (first_condition > 17 || next_condition > 17) { __asm { int 3 } }
    subsumes = FINAL_BRANCH_SUBSUMES(next_condition, first_condition);
    if (!subsumes) return NULL;
    remove_branch_record(next);
    return record;
}
/* Original493058/5c have NULL initialized-file values. This owner is their
 * only direct writer: retain its cross-invocation pointer-identity cache. */
C2Expression *last_redirected_final_branch, *last_redirected_final_target;
C2InputSymbol *new_flow_label(void);
C2Expression *__fastcall label_after_final_record(C2Expression *record)
{
    C2Expression *scan, *label;

    scan = (record->flags & 1) ? record->next : record;
    while (scan->kind == 24) scan = scan->next;
    if (scan->kind == 25) return scan;
    label = symbol_input_expression(new_flow_label());
    insert_input_record((C2Record *)record, (C2Record *)label);
    return label;
}
void __fastcall redirect_allocation_branch(C2Expression *branch, C2Expression *label, int mode);

int __fastcall peephole_4329c6(C2Expression *record)
{
    C2Expression *next, *target = NULL;
    unsigned condition, other;
    int remaining = 4;

    next = next_final_marker(((C2InputSymbol *)record->left->value.symbol)->input_operand);
    if (next->kind != 16 || !next->detail.data || next == record) return 0;
    condition = record->condition_code;
    other = next->condition_code;
    if (condition > 17 || other > 17) { __asm { int 3 } }
    /* The second consumed column (48d378) is exactly the first relation
     * with its tested condition negated, over the same eighteen-row domain.
     * Code17 has no admitted relation and is not an inverse-table index. */
    if (other != 17 && FINAL_BRANCH_SUBSUMES(condition, input_inverse_condition[other])) {
        target = label_after_final_record(next);
        redirect_allocation_branch(record, target, 0);
        return 1;
    }
    do {
        if (next->kind != 16 || (!next->detail.data && next->opcode != 0x15c)) break;
        other = next->condition_code;
        if (other > 17) { __asm { int 3 } }
        if (!FINAL_BRANCH_SUBSUMES(condition, other)) break;
        if (next->left->kind != 4) {
            target = next->left;
            break;
        }
        target = ((C2InputSymbol *)next->left->value.symbol)->input_operand;
        if (!target) {
            target = next->left;
            break;
        }
        next = next_final_marker(target);
        --remaining;
    } while (remaining);
    if (target && (target != last_redirected_final_target || record != last_redirected_final_branch)) {
        redirect_allocation_branch(record, target, 0);
        last_redirected_final_target = target;
        last_redirected_final_branch = record;
        if (remaining > 0) return 1;
    }
    return 0;
}
C2Expression *__fastcall previous_peephole_marker(C2Expression *record);
int __fastcall final_label_allows_tail_merge(C2Expression *label)
{
    C2Expression *previous, *operand, *target;

    if (label->declaration->storage.input_references != 1) return 0;
    previous = previous_peephole_marker(label);
    if (!previous) return 0;
    if (previous->kind == 14) return 1;
    if (previous->kind != 16 || previous->detail.data || previous->opcode == 0x15c) return 0;
    operand = previous->left;
    if (operand->kind == 4) {
        target = ((C2InputSymbol *)operand->value.symbol)->input_operand;
        if (target && target == label) return 0;
    }
    return 1;
}
int __fastcall peephole_436275(C2Expression *first, C2Expression *second);
int __fastcall final_operand_lists_match(C2Expression *first, C2Expression *second)
{
    while (first) {
        while (first && first->kind == 10) first = first->next;
        while (second && second->kind == 10) second = second->next;
        if (!first) break;
        if (!second || !peephole_436275(first, second)) return 0;
        first = first->next;
        second = second->next;
    }
    while (second && second->kind == 10) second = second->next;
    return second == NULL;
}
int __fastcall final_special_record_matches(C2Expression *record)
{ __asm { int 3 } return 0; }

int __fastcall final_records_match(C2Expression *first, C2Expression *second)
{
    C2Expression *left, *right;

    if (first->opcode != second->opcode) return 0;
    if (first->kind == 21) return final_special_record_matches(first);
    switch (first->kind) {
    case 11: case 12: case 14:
        if ((first->type ^ second->type) & 0xfff) return 0;
        return final_operand_lists_match(first->tail.right, second->tail.right) &&
            final_operand_lists_match(first->left, second->left);
    case 13:
        if ((first->type ^ second->type) & 0xfff) return 0;
        return final_operand_lists_match(first->tail.right, second->tail.right) &&
            final_operand_lists_match(first->left, second->left) && first->detail.data == second->detail.data;
    case 16:
        if (first->condition_code == second->condition_code &&
            final_operand_lists_match(first->left, second->left)) return 1;
        if (first->condition_code != second->condition_code || first->left->kind != 4) return 0;
        left = ((C2InputSymbol *)first->left->value.symbol)->input_operand;
        if (!left || second->left->kind != 4) return 0;
        right = ((C2InputSymbol *)second->left->value.symbol)->input_operand;
        if (!right) return 0;
        return left->next == right || right->next == left;
    default:
        return 0;
    }
}
int __fastcall final_record_blocks_branch_motion(C2Expression *record, C2Expression *branch)
{ __asm { int 3 } return 0; }
void __fastcall remove_control_record(C2Expression *record);
void __fastcall delete_final_record_range(C2Expression *first, C2Expression *end)
{
    C2Expression *next, *previous;

    for (; first != end; first = next) {
        next = first->next;
        switch (first->kind) {
        case 16:
            remove_control_record(first);
            break;
        case 18:
            release_global_record(first);
            break;
        case 24:
            break;
        case 25:
            remove_symbol_record(first);
            break;
        default:
            previous = first->previous;
            previous->next = next;
            first->next->previous = previous;
            release_small_node(first);
            break;
        }
    }
}

int __fastcall peephole_432ada(C2Expression *record)
{
    C2Expression *label = ((C2InputSymbol *)record->left->value.symbol)->input_operand;
    C2Expression *first, *second;
    unsigned matched = 0;

    if (!final_label_allows_tail_merge(label)) return 0;
    first = record->next;
    second = label->next;
    while (first->kind != 25 && second->kind != 25 && second != record &&
           final_records_match(first, second) && !final_record_blocks_branch_motion(first, record)) {
        first = first->next;
        second = second->next;
        ++matched;
    }
    if (!matched) return 0;
    move_input_record_before((C2Record *)first, (C2Record *)record);
    delete_final_record_range(label->next, second);
    return 1;
}
C2Expression *__fastcall find_final_complementary_branch(C2Expression *record,
    C2Expression *branch) { __asm { int 3 } return NULL; }
C2InputSymbol *new_flow_label(void);
C2Record *__fastcall new_block_jump(C2Record *label, C2Record *prior);

int __fastcall peephole_432bbe(C2Expression *record)
{
    C2Expression *target, *match, *first, *second, *end, *label;
    C2SymbolInputUse *use;
    unsigned matched = 0;

    if (c2_favor_speed) return 0;
    target = ((C2InputSymbol *)record->left->value.symbol)->input_operand;
    match = find_final_complementary_branch(target, record);
    if (match) end = NULL;
    else {
        for (use = target->tail.symbol_uses; use; use = use->next) {
            first = use->expression;
            if (first->kind == 16 && !first->detail.data && first->opcode != 0x15c &&
                (match = find_final_complementary_branch(first, record)) != NULL) {
                end = use->expression->next;
                break;
            }
        }
    }
    if (!match) return 0;
    if (!end) end = match->next;
    else {
        for (first = match->next; first != end; first = first->next)
            if (first->kind != 16) return 0;
    }
    first = record->previous;
    second = match->previous;
    while (first->kind != 25 && second->kind != 25 && second != record &&
        final_records_match(first, second)) {
        first = first->previous;
        second = second->previous;
        ++matched;
    }
    if (!matched) return 0;
    delete_final_record_range(second->next, end);
    label = symbol_input_expression(new_flow_label());
    insert_input_record((C2Record *)first, (C2Record *)label);
    new_block_jump((C2Record *)label, (C2Record *)end);
    return 1;
}
C2Expression *__fastcall peephole_432ff1(C2Expression *record)
{
    for (record = record->previous; record; record = record->previous)
        if (record->flags & 1) return record;
    return NULL;
}
int __fastcall peephole_436275(C2Expression *first, C2Expression *second);
int __fastcall machine_register_used_after(C2Expression *record,
    struct C2MachineRegister *reg) { __asm { int 3 } return 0; }

int __fastcall final_comparisons_match(C2Expression *first, C2Expression *second,
    C2Expression *branch)
{
    C2Expression *target;
    unsigned condition;

    if (final_records_match(first, second)) return 1;
    if (first->opcode != second->opcode || ((first->type ^ second->type) & 0xfff) ||
        !peephole_436275(first->left, second->left->next) ||
        !peephole_436275(first->left->next, second->left) ||
        !peephole_436275(first->tail.right, second->tail.right)) return 0;
    if (first->opcode == 0x2f) {
        if (previous_peephole_marker(branch) != first) return 0;
        if (machine_register_used_after(branch->next,
            (struct C2MachineRegister *)branch->left->next->original_register)) return 0;
        target = ((C2InputSymbol *)branch->left->value.symbol)->input_operand->next;
        if (target->kind == 16 && (target->detail.data || target->opcode == 0x15c)) return 0;
        swap_global_operands(first);
        condition = branch->condition_code;
        /* Encountered byte relation48d250: codes3/4,5/6,7/8,9/10 swap;
         * 0..2 stay unchanged and11..17 map to the original code0. */
        if (condition > 17) { __asm { int 3 } }
        if (condition >= 3 && condition <= 10)
            branch->condition_code = (unsigned char)((condition & 1) ? condition + 1 : condition - 1);
        else if (condition >= 11) branch->condition_code = 0;
    }
    return 1;
}
void __fastcall remove_control_record(C2Expression *record);

int __fastcall peephole_432be2(C2Expression *first, C2Expression *last)
{
    C2Expression *target, *before, *after;
    int changed = 0;

    target = next_final_marker(((C2InputSymbol *)last->left->value.symbol)->input_operand);
    if ((first->flags & 4) && (target->flags & 4)) {
        before = first->previous;
        after = target->next;
        if ((before->flags & 1) && (after->flags & 1) &&
            (before->flags & 4) && (after->flags & 4) && before != target &&
            final_records_match(before, target)) target = after;
    }
    if (first != target && final_comparisons_match(first, target, last)) {
        target = label_after_final_record(target);
        redirect_allocation_branch(last, target, 0);
        changed = 1;
    }
    target = last->next;
    if ((target->opcode == 0x2f || target->opcode == 0x33 || target->opcode == 0x30) &&
        final_comparisons_match(first, target, last)) {
        after = target->next;
        if (after->kind == 16 && after->detail.data && last->condition_code == after->condition_code)
            remove_control_record(after);
        unlink_and_release_input_record((C2Record *)target);
        changed = 1;
    }
    return changed;
}
void __fastcall move_final_label_after(C2Expression *after, C2Expression *label)
{
    unlink_input_record((C2Record *)label);
    insert_input_record((C2Record *)after, (C2Record *)label);
}
void __fastcall merge_final_record_ranges(C2Expression *first, C2Expression *last,
    C2Expression *other_first, C2Expression *other_last)
{
    C2Expression *left, *right, *pending, *copy, *next;
    unsigned number;

    while (first && other_first) {
        while (first->kind == 25 && first != last) first = first->next;
        while (other_first->kind == 25 && other_first != other_last)
            other_first = other_first->next;
        pending = NULL;
        for (left = first->tail.right, right = other_first->tail.right;
             left && right; left = left->next, right = right->next) {
            if (left->kind == 6) {
                if (left->immediate == right->immediate) continue;
                number = left->immediate;
            } else if (left->kind == 10) {
                if (left->value.control == right->value.control) continue;
                number = left->value.control;
            } else continue;
            copy = new_optimizer_index_operand(number);
            copy->next = pending;
            pending = copy;
        }
        while (pending) {
            next = pending->next;
            pending->next = NULL;
            attach_optimizer_output(other_first, pending);
            pending = next;
        }
        /* Both original passes scan the result lists at +28. The second
         * attaches the collected indices as inputs; it does not scan +24. */
        pending = NULL;
        for (left = first->tail.right, right = other_first->tail.right;
             left && right; left = left->next, right = right->next) {
            if (left->kind == 6) {
                if (left->immediate == right->immediate) continue;
                number = left->immediate;
            } else if (left->kind == 10) {
                if (left->value.control == right->value.control) continue;
                number = left->value.control;
            } else continue;
            copy = new_optimizer_index_operand(number);
            copy->next = pending;
            pending = copy;
        }
        while (pending) {
            next = pending->next;
            pending->next = NULL;
            copy_input_operand(other_first, pending);
            pending = next;
        }
        while (first->kind == 25 && first != last) first = first->next;
        while (other_first->kind == 25 && other_first != other_last)
            other_first = other_first->next;
        if (first == last || other_first == other_last) return;
        first = first->next;
        other_first = other_first->next;
    }
}

int __fastcall peephole_4363c0(C2Expression *record)
{
    C2Expression *label = ((C2InputSymbol *)record->left->value.symbol)->input_operand;
    C2Expression *first = record, *second = label, *previous, *target;
    unsigned matched = 0;

    for (;;) {
        first = first->previous;
        second = second->previous;
        while (first->kind == 25) {
            if (first == label) goto final_common_tail_end;
            previous = first->previous;
            move_final_label_after(second, first);
            first = previous;
        }
        while (second->kind == 25) second = second->previous;
        if (second == record ||
            (first->kind == 16 && (first->detail.data || first->opcode == 0x15c) && c2_favor_speed) ||
            !final_records_match(first, second)) break;
        ++matched;
    }
final_common_tail_end:
    if (!matched) return 0;
    merge_final_record_ranges(first->next, record->previous, second->next, label->previous);
    target = label_after_final_record(second);
    redirect_allocation_branch(record, target, 0);
    delete_final_record_range(first->next, record);
    return 1;
}
C2Expression *__fastcall peephole_4364af(C2Expression *record)
{
    C2Expression *first = previous_peephole_marker(record), *second, *label;
    C2Expression *between, *branch, *input, *output, *scan;
    int move_first = 1;

    if (record->left->kind != 4) return NULL;
    label = ((C2InputSymbol *)record->left->value.symbol)->input_operand;
    if (!label) return NULL;
    second = previous_peephole_marker(label);
    if (!first || !second || !(first->flags & 1) || !(second->flags & 1) ||
        !first->tail.right || !first->left || !second->tail.right || !second->left ||
        !peephole_436275(first->tail.right, second->tail.right) ||
        (first->type & 0xf000) == 0x4000 || (second->type & 0xf000) == 0x4000)
        return NULL;
    if (first->tail.right->kind == 6 || first->left->kind == 6) move_first = 0;
    if (!move_first && (second->tail.right->kind == 6 || second->left->kind == 6))
        return NULL;
    between = previous_peephole_marker(second);
    if (!between || between->kind != 25 || previous_peephole_marker(between) != record)
        return NULL;
    branch = previous_peephole_marker(first);
    if (!branch || branch->kind != 16 || !branch->detail.data ||
        ((C2InputSymbol *)branch->left->value.symbol)->input_operand != between)
        return NULL;
    if (move_first) {
        for (input = branch->left; input; input = input->next)
            for (output = first->tail.right; output; output = output->next)
                if (input_operands_may_alias(input, output)) move_first = 0;
    }
    if (!move_first) {
        for (input = branch->left; input; input = input->next)
            for (output = second->tail.right; output; output = output->next)
                if (input_operands_may_alias(input, output)) return NULL;
    }
    if (move_first) {
        for (output = first->tail.right; output; output = output->next)
            for (input = second->left; input; input = input->next)
                if (input_operands_may_alias(output, input)) return NULL;
        remove_control_record(record);
        unlink_input_record((C2Record *)first);
        prepend_input_record((C2Record *)branch, (C2Record *)first);
        if (branch->condition_code > 16) { __asm { int 3 } }
        branch->condition_code = input_inverse_condition[branch->condition_code];
        redirect_allocation_branch(branch, label, 0);
        return label;
    }
    if (first->opcode != 0x34 || first->tail.right->kind != 1 ||
        first->left->kind != 1 || first->left->next->kind != 1 ||
        first->tail.right->original_register != first->left->original_register ||
        first->left->original_register != first->left->next->original_register ||
        (first->tail.right->next->kind == 1 &&
         first->tail.right->next->value.reg->kind == 3 &&
         first->tail.right->next->value.reg->value)) {
        for (output = second->tail.right; output; output = output->next)
            for (input = first->left; input; input = input->next)
                if (input_operands_may_alias(output, input)) return NULL;
    }
    for (scan = between->tail.right; scan; scan = scan->next)
        if (scan != branch) return NULL;
    remove_control_record(record);
    unlink_input_record((C2Record *)second);
    prepend_input_record((C2Record *)branch, (C2Record *)second);
    unlink_input_record((C2Record *)first);
    insert_input_record((C2Record *)branch, (C2Record *)first);
    redirect_allocation_branch(branch, label, 0);
    return label;
}
C2Expression *__fastcall resume_final_peephole_scan(C2Expression *record)
{
    C2Expression *previous;

    if (record) {
        for (previous = record->previous; previous; previous = previous->previous)
            if (previous->kind == 16 || previous->kind == 25) return previous;
    }
    return record;
}

int __fastcall optimize_final_record_pass(C2Function *function)
{
    C2Expression *record, *next, *end, *marker, *target, *result;
    unsigned changed, total = 0;

    number_final_peephole_records(function);
    record = (C2Expression *)function->flow->last->records->next;
    end = (C2Expression *)function->flow->first->last_record;
    while (record != end) {
        next = record->next;
        changed = 0;
        switch (record->kind) {
        case 25:
            if (record->location.line) {
                record_line_delta = record->location.line;
                record_source_line = function->source_line_base + record_line_delta;
            }
            if (!symbol_range_boundary(record) && simplify_symbol_record(record)) {
                changed = 1;
            } else if (c2_global_optimize && record->tail.right) {
                marker = next_final_marker(record);
                target = NULL;
                if (marker && marker->kind == 16 && !marker->detail.data &&
                    marker->opcode != 0x15c && marker->left->kind == 4)
                    target = ((C2InputSymbol *)marker->left->value.symbol)->input_operand;
                if (target && target != record && marker->opcode != 0x15b) {
                    redirect_final_symbol_uses(record, target);
                    changed = 1;
                } else if (!code_generation_state && peephole_432f03(record)) changed = 1;
            }
            break;
        case 18:
            if (record->location.line) {
                record_line_delta = record->location.line;
                record_source_line = function->source_line_base + record_line_delta;
            }
            /* Fall through to the transfer pruning shared with kind14. */
        case 14:
            if (prune_after_final_transfer(record)) { changed = 1; next = record; }
            break;
        case 16:
            if (record->location.line) {
                record_line_delta = record->location.line;
                record_source_line = function->source_line_base + record_line_delta;
            }
            if (!record->detail.data && record->opcode != 0x15c &&
                prune_after_final_transfer(record)) {
                changed = 1; next = record;
                break;
            }
            if (record->left->kind != 4 ||
                !((C2InputSymbol *)record->left->value.symbol)->input_operand ||
                record->opcode == 0x15c || record->opcode == 0x15b) break;
            if (fold_final_branch_target(record)) { changed = 1; break; }
            if (!c2_global_optimize) break;
            if (!record->detail.data && record->opcode != 0x15c) {
                if (!code_generation_state) {
                    if (peephole_4363c0(record)) { changed = 1; next = record; }
                    result = peephole_4364af(record);
                    if (result) { ++changed; next = result; }
                }
                break;
            }
            result = peephole_432924(record);
            if (result) { changed = 1; next = result; break; }
            if (invert_final_branch(record) || peephole_4329c6(record)) {
                changed = 1; next = record; break;
            }
            if (code_generation_state) break;
            if (peephole_432ada(record)) { changed = 1; break; }
            if (peephole_432bbe(record)) { changed = 1; next = record; break; }
            marker = peephole_432ff1(record);
            if ((marker->opcode == 0x2f || marker->opcode == 0x33 || marker->opcode == 0x30) &&
                peephole_432be2(marker, record)) { changed = 1; next = record; }
            break;
        }
        if (changed) {
            total += changed;
            next = resume_final_peephole_scan(next);
        }
        record = next;
    }
    return total;
}

void __fastcall optimize_final_record_sequence(C2Function *function)
{
    int pass;
    /* One initial pass plus at most16 further successful-change retries. */
    for (pass = 0; pass <= 16; ++pass)
        if (!optimize_final_record_pass(function)) break;
}

C2Expression *__fastcall previous_peephole_marker(C2Expression *record)
{
    for (record = record->previous; record; record = record->previous) {
        if ((record->flags & 1) || record->kind == 25) return record;
        if (record->kind == 24) break;
    }
    return NULL;
}
/* The genuine encoding entry receives a20-byte caller-owned scratch buffer.
 * The optional trailing pointer roles remain opaque until that body is reached. */
int __fastcall encode_final_record(C2Expression *record, unsigned char *encoding,
    unsigned char *marks, void *relocations);
int __fastcall final_record_code_size(C2Expression *record)
{
    unsigned char encoding[20];
    return encode_final_record(record, encoding, NULL, NULL);
}
void __fastcall move_record_range_before(C2Expression *destination,
    C2Expression *first, C2Expression *last)
{
    C2Expression *before = first->previous;

    if (before) before->next = last->next;
    if (last->next) last->next->previous = before;
    before = destination->previous;
    destination->previous = last;
    last->next = destination;
    first->previous = before;
    before->next = first;
}

/* Labels and the inactive0x138 marker may be crossed. Other non-machine
 * records bound the short instruction sequence considered for duplication. */
#define SEQUENCE_MACHINE_RECORD(record) ((record)->opcode == 0x138 || \
    !((record)->flags & 1) || ((record)->opcode > 0 && (record)->opcode < 0x11b))

void __fastcall optimize_final_branch_sequences(C2Function *function)
{
    C2Expression *record, *next, *end, *target, *last, *scan, *previous;
    C2Expression *copy, *source, *destination;
    int saw_label, size;

    record = (C2Expression *)function->flow->last->records->next;
    end = (C2Expression *)function->flow->first->last_record;
    while (record != end) {
        next = record->next;
        if (record->kind != 16 || record->detail.data || record->opcode == 0x15c ||
            record->left->kind != 4 ||
            !((C2InputSymbol *)record->left->value.symbol)->input_operand ||
            fold_final_branch_target(record)) goto next_sequence;
        target = ((C2InputSymbol *)record->left->value.symbol)->input_operand;
        saw_label = 0;
        for (last = target->next; last; last = last->next) {
            if (last->opcode == 0x138) continue;
            if (!SEQUENCE_MACHINE_RECORD(last)) break;
            switch (last->kind) {
            case 14: case 16: case 18: case 23: goto sequence_end;
            case 25: saw_label = 1; break;
            }
        }
sequence_end:
        if (last->kind == 23 || !SEQUENCE_MACHINE_RECORD(last) ||
            last->kind == 18 || last == record) goto next_sequence;
        if (last->kind == 16 && (last->detail.data || last->opcode == 0x15c ||
            last->left->kind != 4 ||
            !((C2InputSymbol *)last->left->value.symbol)->input_operand)) goto next_sequence;
        previous = previous_peephole_marker(target);
        if (!saw_label && previous->kind == 16 && !previous->detail.data &&
            previous->opcode != 0x15c &&
            (previous->left->kind != 4 ||
             ((C2InputSymbol *)previous->left->value.symbol)->input_operand != target)) {
            target->previous->next = last->next;
            last->next->previous = target->previous;
            last->next = NULL;
            link_code_nodes((C2Record *)record, (C2Record *)target);
            unlink_and_release_input_record((C2Record *)record);
            next = last;
            goto next_sequence;
        }
        size = 0;
        scan = target->next;
        do {
            if (scan->flags & 1) size += final_record_code_size(scan);
            if (scan == last) break;
            scan = scan->next;
        } while (size <= (c2_favor_speed ? 20 : 2));
        if (size > (c2_favor_speed ? 20 : 2)) goto next_sequence;
        source = target->next;
        destination = record;
        for (;;) {
            if (source->kind != 25) {
                copy = copy_input_expression(source);
                insert_input_record((C2Record *)destination, (C2Record *)copy);
                if (copy->kind == 16 && copy->left->kind == 4 &&
                    ((C2InputSymbol *)copy->left->value.symbol)->input_operand)
                    bind_symbol_input_use(((C2InputSymbol *)copy->left->value.symbol)->input_operand, copy);
                destination = copy;
            }
            if (source == last) break;
            source = source->next;
        }
        unlink_and_release_input_record((C2Record *)record);
        if (destination->kind == 16 && !destination->detail.data && destination->opcode != 0x15c)
            next = destination;
next_sequence:
        record = next;
    }

    end = (C2Expression *)function->flow->first->last_record;
    record = (C2Expression *)function->flow->last->records->next;
    while (record != end) {
        next = record->next;
        if (record->kind != 16 || record->detail.data || record->opcode == 0x15c ||
            record->left->kind != 4 ||
            !(target = ((C2InputSymbol *)record->left->value.symbol)->input_operand) ||
            next->kind != 25) goto next_range;
        previous = target->previous;
        if ((previous->kind != 16 || previous->detail.data || previous->opcode == 0x15c) &&
            previous->kind != 14) goto next_range;
        for (scan = next; scan && scan != target; scan = scan->next) ;
        if (scan) {
            destination = ((C2Expression *)function->flow->first->last_record)->previous;
            move_record_range_before(destination, next, previous);
            unlink_and_release_input_record((C2Record *)record);
        }
next_range:
        record = next;
    }
}

void __fastcall prune_final_transfer_ranges(C2Function *function)
{
    C2Expression *record, *next, *end;

    record = (C2Expression *)function->flow->last->records->next;
    end = (C2Expression *)function->flow->first->last_record;
    while (record != end) {
        next = record->next;
        if ((record->kind == 14 ||
            (record->kind == 16 && !record->detail.data && record->opcode != 0x15c)) &&
            prune_after_final_transfer(record)) next = record;
        record = next;
    }
}
