#include "optimizer-sets.h"
#include "inlining.h"
#include <recon.h>

extern unsigned optimizer_initial_limit, optimizer_index_limit, optimizer_expression_limit;
extern unsigned optimizer_final_index_limit;
extern C2FunctionRegister *optimizer_memory_register;

typedef struct C2OptimizerRange {
    struct C2OptimizerRange *next;
    long displacement;
    unsigned bytes;
    unsigned number;
} C2OptimizerRange;

typedef struct C2OptimizerSetClass {
    struct C2OptimizerSetClass *next;
    C2RegisterSet *set;
    unsigned number;
    unsigned scope;
} C2OptimizerSetClass;



typedef struct C2OptimizerRangeMask {
    unsigned original_number;
    unsigned bit;
    unsigned overlaps;
} C2OptimizerRangeMask;

C2OptimizerRangeMask *optimizer_range_masks; /* 48e2ec */
C2RegisterSet *optimizer_output_indices;    /* 490b98 */
C2RegisterSet *optimizer_value_indices;     /* 491014 */
C2RegisterSet *optimizer_symbol_indices;    /* 491018 */

/* The final numbered range retains its original index in the first DWORD. */
C2OptimizerUse *__fastcall get_global_index_uses(unsigned number)
{
    if (number >= optimizer_index_limit)
        number = optimizer_range_masks[number - optimizer_index_limit].original_number;
    return optimizer_index_uses[number];
}

/* Range indices retain their original alias-list number. Kind-12 entries
 * group register uses; ordinary entries name their register directly. */
int __fastcall register_may_alias_index(C2FunctionRegister *reg, unsigned index)
{
    C2FunctionRegister *root = reg->canonical;
    C2OptimizerUse *use, *member;
    if (index >= optimizer_index_limit)
        index = optimizer_range_masks[index - optimizer_index_limit].original_number;
    switch (root->kind) {
    case 4:
    case 5:
        if (!(root->view_flags & 2)) return 0;
        if (root->view_flags & 4) {
            for (use = optimizer_index_uses[index]; use; use = use->next) {
                if (use->reg->kind == 12) {
                    for (member = use->reg->optimizer_uses; member; member = member->next)
                        if (member->reg == root) return 1;
                } else if (use->reg == root) return 1;
            }
        } else if (index >= optimizer_expression_limit) {
            for (use = optimizer_index_uses[index]; use; use = use->next)
                if (use->reg == root) return 1;
        }
        break;
    case 7:
    case 8:
        if ((root->view_flags & 4) && !(root->view_flags & 2)) return 0;
        for (use = optimizer_index_uses[index]; use; use = use->next)
            if (use->reg == root || use->reg == optimizer_memory_register) return 1;
        break;
    }
    return 0;
}

unsigned __fastcall optimizer_dense_sets_intersect(C2RegisterSet *first,
    C2RegisterSet *second);

int __fastcall optimizer_indices_overlap(unsigned first, unsigned second)
{
    C2OptimizerRangeMask *first_range, *second_range;
    if (first == second) return 1;
    if (first >= optimizer_index_limit) {
        first_range = &optimizer_range_masks[first - optimizer_index_limit];
        first = first_range->original_number;
        if (second >= optimizer_index_limit) {
            second_range = &optimizer_range_masks[second - optimizer_index_limit];
            if (first == second_range->original_number)
                return first_range->overlaps & (1U << (second_range->bit & 31));
        }
    }
    if (second >= optimizer_index_limit)
        second = optimizer_range_masks[second - optimizer_index_limit].original_number;
    return optimizer_dense_sets_intersect(optimizer_index_sets[first],
                                          optimizer_index_sets[second]);
}

C2RegisterSet *__fastcall finish_optimizer_records(C2Record *first,
    unsigned limit, unsigned original_limit, int aliases)
{
    C2Expression *record, *operand;
    C2OptimizerRange **ranges, *range, *other;
    C2OptimizerSetClass *classes = NULL, *set_class;
    C2OptimizerUse *use;
    C2FunctionRegister *reg;
    C2RegisterSet *range_uses, *result, *set;
    C2OptimizerRangeMask *mask;
    unsigned *replacements;
    unsigned side, number, added = 0, cursor, bit, scope, bytes;
    long displacement, end;

    ranges = c2_pool_allocate(4, limit * sizeof(*ranges));
    range_uses = new_register_set(6);
    for (record = (C2Expression *)first; record; record = record->next) {
        if (!(record->flags & 1)) continue;
        for (side = 0; side < 2; ++side) {
            for (operand = side ? record->left : record->tail.right;
                 operand; operand = operand->next) {
                if (operand->kind != 6) continue;
                operand->value.optimizer_index = 0;
                number = operand->immediate;
                if (aliases || number < original_limit ||
                    (operand->location.expression.properties & 0x80) ||
                    !get_optimizer_operand_range(operand, &displacement, &bytes))
                    continue;
                for (range = ranges[number]; range; range = range->next)
                    if (range->displacement == displacement && range->bytes == bytes) break;
                if (!range) {
                    range = c2_pool_allocate(4, sizeof(*range));
                    range->displacement = displacement;
                    range->bytes = bytes;
                    range->next = ranges[number];
                    ranges[number] = range;
                }
                if (!side) add_register_to_set(range_uses, number);
            }
        }
    }
    replacements = c2_pool_allocate(4, limit * sizeof(*replacements));
    for (number = 1; number < limit; ++number) {
        if (number == optimizer_initial_limit) {
            number = original_limit;
            if (number >= limit) break;
        }
        if (!optimizer_index_sets[number]) continue;
        range = ranges[number];
        if (range && range->next) {
            if (optimizer_set_contains(range_uses, number)) {
                for (; range; range = range->next) range->number = limit + added++;
                continue;
            }
            ranges[number] = NULL;
        }
        scope = 0;
        if (aliases)
            for (use = optimizer_index_uses[number]; use; use = use->next)
                if (use->reg->kind == 12 && use->reg != optimizer_memory_register) {
                    scope = use->reg->optimizer_scope_number;
                    break;
                }
        set = optimizer_index_sets[number];
        for (set_class = classes; set_class; set_class = set_class->next)
            if (optimizer_dense_sets_equal(set, set_class->set) && scope == set_class->scope) break;
        if (set_class) {
            replacements[number] = set_class->number;
            optimizer_index_uses[number] = NULL;
            optimizer_index_sets[number] = NULL;
        } else {
            set_class = c2_pool_allocate(4, sizeof(*set_class));
            set_class->set = set;
            set_class->number = number;
            set_class->scope = scope;
            set_class->next = classes;
            classes = set_class;
        }
    }
    release_optimizer_symbol_set(range_uses);
    optimizer_final_index_limit = added - 1 + optimizer_index_limit;
    if (!added) optimizer_range_masks = NULL;
    else {
        optimizer_range_masks = c2_pool_allocate(1, added * sizeof(*optimizer_range_masks));
        cursor = 0;
        for (number = original_limit; number < limit; ++number) {
            range = ranges[number];
            if (!range || !range->next) continue;
            bit = 0;
            for (other = range; other; other = other->next) {
                mask = &optimizer_range_masks[cursor++];
                mask->original_number = number;
                if (bit > 31) bit = 31;
                mask->bit = bit++;
            }
            for (; range; range = range->next) {
                displacement = range->displacement;
                end = (long)((unsigned)displacement + range->bytes);
                for (other = ranges[number]; other; other = other->next)
                    if (other->displacement < end &&
                        displacement < (long)((unsigned)other->displacement + other->bytes))
                        optimizer_range_masks[range->number - limit].overlaps |=
                            1U << (optimizer_range_masks[other->number - limit].bit & 31);
            }
        }
    }
    result = new_register_set(6);
    optimizer_output_indices = new_register_set(1);
    optimizer_symbol_indices = new_register_set(1);
    optimizer_value_indices = new_register_set(1);
    for (record = (C2Expression *)first; record; record = record->next) {
        if (!(record->flags & 1)) continue;
        for (side = 0; side < 2; ++side)
            for (operand = side ? record->left : record->tail.right;
                 operand; operand = operand->next) {
                if ((1U << (operand->kind & 31)) & 0x0e) {
                    reg = operand->value.symbol->input_register;
                    if (reg->kind == 7 || reg->kind == 8 || (reg->view_flags & 2)) {
                        number = ((C2InputSymbol *)reg->symbol)->detail.optimizer_number;
                        add_register_to_set(result, number);
                        add_register_to_set(optimizer_symbol_indices, number);
                    }
                } else if (operand->kind == 6 || operand->kind == 10) {
                    if (operand->kind == 6) {
                        number = operand->immediate;
                        range = ranges[number];
                        if (range && range->next) {
                            if (get_optimizer_operand_range(operand, &displacement, &bytes)) {
                                for (range = ranges[number]; range; range = range->next)
                                    if (range->displacement == displacement && range->bytes == bytes) break;
                                operand->immediate = range->number;
                            }
                        } else if (replacements[number]) operand->immediate = replacements[number];
                        number = operand->immediate;
                    } else {
                        number = operand->value.optimizer_index;
                        if (replacements[number]) operand->value.optimizer_index = replacements[number];
                        number = operand->value.optimizer_index;
                    }
                    add_register_to_set(result, number);
                    add_register_to_set(optimizer_value_indices, number);
                }
            }
    }
    merge_optimizer_symbol_sets(optimizer_output_indices, optimizer_symbol_indices);
    merge_optimizer_symbol_sets(optimizer_output_indices, optimizer_value_indices);
    return result;
}

/* The per-index conflict lists contain a self entry and symmetric neighbors. */
C2OptimizerConflict **optimizer_conflicts; /* 48fa38 */
extern unsigned optimizer_original_limit;

void __fastcall build_optimizer_conflicts(C2RegisterSet *records,
    unsigned final_index_limit, unsigned limit)
{
    unsigned first, second;
    C2OptimizerConflict *entry;
    optimizer_conflicts = c2_pool_allocate(1, final_index_limit * 4 + 4);
    for (first = 1; first <= final_index_limit; ++first) {
        if (!optimizer_set_contains(records, first)) continue;
        if (first < limit && !optimizer_index_sets[first]) continue;
        entry = c2_pool_allocate(1, sizeof(*entry));
        entry->number = first;
        entry->next = optimizer_conflicts[first];
        optimizer_conflicts[first] = entry;
        for (second = first + 1; second <= final_index_limit; ++second) {
            if (second < limit && !optimizer_index_sets[second]) continue;
            if (first < optimizer_original_limit && first >= optimizer_initial_limit &&
                second < optimizer_original_limit && second >= optimizer_initial_limit) continue;
            if (!optimizer_set_contains(records, second) ||
                !optimizer_indices_overlap(first, second)) continue;
            entry = c2_pool_allocate(1, sizeof(*entry));
            entry->number = second;
            entry->next = optimizer_conflicts[first];
            optimizer_conflicts[first] = entry;
            entry = c2_pool_allocate(1, sizeof(*entry));
            entry->number = first;
            entry->next = optimizer_conflicts[second];
            optimizer_conflicts[second] = entry;
        }
    }
}

extern int c2_exception_mode;
unsigned __fastcall optimize_function_returns(C2Function *function)
{
    if (function->return_storage || (function->flags & 0x40) ||
        (!c2_exception_mode && (function->symbol->additional_flags & 0x200)) ||
        (function->symbol->symbol.flags & 0x1000)) return 0;
    return optimize_return_successors(function, function->flow->first);
}

unsigned __fastcall optimize_return_successors(C2Function *function, C2BasicBlock *block)
{
    C2FlowEdge *edge, *next;
    unsigned changed = 0;
    for (edge = block->incoming; edge; edge = next) {
        next = edge->next_incoming;
        changed |= fold_return_block(function, edge->source);
    }
    return changed & 1;
}

int __fastcall is_empty_return_block(C2BasicBlock *block);
int __fastcall convert_tail_return(C2Function *function, C2BasicBlock *block, C2Expression *record);
void __fastcall remove_control_record(C2Expression *record);

unsigned __fastcall fold_return_block(C2Function *function, C2BasicBlock *block)
{
    C2Expression *record, *control = NULL, *assignment = NULL, *result;
    C2Expression *left, *right;
    if (is_empty_return_block(block)) return optimize_return_successors(function, block);
    record = ((C2Expression *)block->last_record)->previous;
    while (record && record->kind == 21 && record->opcode != 0x17e && record->opcode != 0x17f)
        record = record->previous;
    if (record->kind == 16 && !record->detail.data && record->opcode != 0x15c) {
        control = record;
        record = record->previous;
        while (record && record->kind == 21 && record->opcode != 0x17e && record->opcode != 0x17f)
            record = record->previous;
    }
    result = record;
    if (record->kind != 13) {
        if (record->kind != 14) return 0;
        result = record->previous;
        while (result && result->kind == 21 && result->opcode != 0x17e && result->opcode != 0x17f)
            result = result->previous;
        if (result->kind != 13) return 0;
        left = result->tail.right;
        right = record->left;
        if (!left || !right || !((1U << (left->kind & 31)) & 6) ||
            !((1U << (right->kind & 31)) & 6) ||
            left->value.symbol != right->value.symbol || left->value.symbol->kind != 3) return 0;
        assignment = record;
    }
    left = result->left;
    if (left->kind != 4 || left->value.symbol != &function->symbol->symbol ||
        convert_tail_return(function, block, result) != 1) return 0;
    if (assignment) unlink_and_release_input_record((C2Record *)assignment);
    if (control) remove_control_record(control);
    return 1;
}

int __fastcall is_empty_return_block(C2BasicBlock *block)
{
    C2Expression *first = ((C2Expression *)block->records)->next;
    C2Expression *last = ((C2Expression *)block->last_record)->previous;
    while (first && first->kind == 21 && first->opcode != 0x17e && first->opcode != 0x17f)
        first = first->next;
    while (last && last->kind == 21 && last->opcode != 0x17e && last->opcode != 0x17f)
        last = last->previous;
    if (first == last) return first->kind == 25;
    if (first->kind != 25 || last->kind != 16 || last->detail.data || last->opcode == 0x15c)
        return 0;
    first = first->next;
    while (first && first->kind == 21 && first->opcode != 0x17e && first->opcode != 0x17f)
        first = first->next;
    return first == last;
}

void __fastcall insert_function_scope_markers(C2Function *function);
void __fastcall normalize_flow_boundaries(C2Function *function);
void __fastcall order_function_blocks(C2Function *function);
void __fastcall number_function_blocks(C2Function *function);
void __fastcall compute_block_dominators(C2Function *function);
void __fastcall analyze_function_regions(C2Function *function, int mode);

void __fastcall prepare_function_liveness(C2Function *function)
{
    insert_function_scope_markers(function);
    normalize_flow_boundaries(function);
    order_function_blocks(function);
    number_function_blocks(function);
    compute_block_dominators(function);
    analyze_function_regions(function, 1);
}
