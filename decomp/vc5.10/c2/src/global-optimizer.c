#include "global-optimizer.h"
#include "global-expressions.h"
#include "optimizer-translate.h"
#include "inlining.h"
#include <recon.h>

/* Original DWORD owners used by the global expression and loop passes. */
int global_optimization_active;              /* 48e2b0 */
int global_aliases_changed;                  /* 48fa3c */
C2FunctionRegister *global_pending_registers; /* 49100c; -1-terminated chain. */
C2RegisterSet *global_expression_registers;   /* 48fa78 */
int global_optimization_phase;               /* 49101c */
int global_region_limited;                   /* 48fa7c */
int global_block_state;                      /* 48fa68 */
int global_motion_enabled;                   /* 48fa70 */
int global_region_motion_enabled;            /* 48fa80 */
int global_region_analysis_enabled;          /* 48fa84 */
/* Initialized image DWORD, compared against a signed block-number span. */
int global_region_size_limit = 100;          /* 491038 */
C2RegisterSet *global_motion_registers;       /* 49103c */
C2RegisterSet *global_motion_inputs;          /* 49102c */
C2RegisterSet *global_motion_outputs;         /* 491030 */

typedef char C2GlobalDefinitionsOffset[
    offsetof(C2BasicBlock, global_definitions) == 84 ? 1 : -1];
typedef char C2RegionRegistersOffset[
    offsetof(C2FlowRegion, global_registers) == 52 ? 1 : -1];
typedef char C2GlobalDefinitionOffset[
    offsetof(C2FunctionRegister, global_expression) == 24 ? 1 : -1];

void __fastcall optimize_global_flow(C2Function *function)
{
    C2BasicBlock *block, *next, *header, *last;
    C2FlowRegion *region;
    C2GlobalExpression *definition;
    C2Expression *operand;
    C2RegisterSet *selected, *active;
    C2FlowEdge *edge;
    unsigned number;
    int valid;

    initialize_global_optimizer();
    reset_input_pool(13);
    reset_input_pool(6);
    reset_input_pool(3);
    global_optimization_active = 1;
    initialize_global_registers(function);
    global_aliases_changed = 0;
    global_pending_registers = (C2FunctionRegister *)-1;
    global_expression_registers = new_register_set(13);
    global_optimization_phase = 0;
    prepare_global_blocks(function);
    if (global_aliases_changed) rewrite_global_aliases(function, 0);
    initialize_global_flow_analysis(function);
    build_global_flow_analysis(function);
    rewrite_global_function(function, 7);
    finish_global_function_rewrite(function);
    finish_global_flow_analysis(function);
    if (function->flow_regions) {
        prepare_region_register_sets(function);
        analyze_region_register_sets(function);
        solve_region_register_sets(function);
        prepare_region_register_sets(function);
    }
    prepare_global_expressions(function);
    prepare_global_assignments(function);
    global_optimization_phase = 1;
    global_region_limited = 0;
    global_block_state = 0;
    global_motion_enabled = 1;
    global_region_motion_enabled = 1;
    global_region_analysis_enabled = 1;
    for (block = function->flow->last; block; block = next) {
        region = block->block_scope;
        next = block->previous;
        if (region && region->header == block) {
            header = region->header;
            last = region->last;
            validate_global_region(region);
            if (!(region->flags & 1)) {
                if (!global_region_limited ||
                    global_region_size_limit <
                        (int)(short)region->last->number84 -
                        (int)(short)region->header->number84 + 1) {
                    selected = new_register_set(4);
                    active = region->global_registers;
                    for (number = next_optimizer_symbol(region->header->next->global_definitions);
                         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
                        definition = lookup_function_register(number)->global_expression;
                        if (definition->opcode == 0x131 ||
                            operation_is_comparison(definition->opcode)) {
                            valid = 1;
                            for (operand = definition->operands; operand;
                                 operand = operand->next) {
                                switch (operand->kind) {
                                case 1: case 2:
                                    if (!optimizer_set_contains(active, operand->value.reg->number))
                                        valid = 0;
                                    break;
                                case 3: case 4: case 5: case 7: case 8:
                                    break;
                                default:
                                    valid = 0;
                                    break;
                                }
                            }
                            if (valid) add_register_to_set(selected, number);
                        }
                    }
                    if (selected->links.first)
                        for (edge = region->header->incoming; edge;
                             edge = edge->next_incoming)
                            merge_optimizer_symbol_sets(edge->source->global_definitions, selected);
                    release_optimizer_symbol_set(selected);
                } else solve_global_register_sets(header, last);
                if (global_region_motion_enabled) prepare_region_motion(header, last);
            }
        }
        optimize_global_block(block, function);
    }
    global_block_state = 0;
    finish_global_block_pass(function);
    clear_global_block_marks(function);
    clear_global_register_marks(function);
    if (global_aliases_changed) rewrite_global_aliases(function, 1);
    rewrite_global_function(function, 5);
    finish_global_function_rewrite(function);
    if (function->flow_regions) {
        prepare_global_function_cleanup(function);
        prepare_region_register_sets(function);
        analyze_region_register_sets(function);
        solve_region_register_sets(function);
        prepare_region_register_sets(function);
        finish_region_register_sets(function);
    }
    prepare_global_assignments(function);
    solve_global_register_sets(function->flow->last, function->flow->first);
    global_optimization_phase = 2;
    global_region_limited = 0;
    global_region_motion_enabled = 0;
    global_region_analysis_enabled = 1;
    global_motion_enabled = 0;
    global_motion_registers = new_temporary_register_set();
    global_motion_inputs = new_temporary_register_set();
    global_motion_outputs = new_temporary_register_set();
    initialize_region_motion_sets();
    for (region = function->flow_regions; region; region = region->next)
        optimize_region_motion(region, function);
    release_region_motion_sets();
    global_optimization_phase = 3;
    finish_region_motion(function);
    solve_global_register_sets(function->flow->last, function->flow->first);
    global_region_limited = 1;
    global_region_motion_enabled = 0;
    global_region_analysis_enabled = 0;
    global_motion_enabled = 1;
    for (block = function->flow->last; block; block = next) {
        region = block->block_scope;
        next = block->previous;
        if (region && region->header == block) {
            header = region->header;
            last = region->last;
            validate_global_region(region);
            if (!(region->flags & 1)) {
                if (!global_region_limited ||
                    global_region_size_limit <
                        (int)(short)region->last->number84 -
                        (int)(short)region->header->number84 + 1) {
                    selected = new_register_set(4);
                    active = region->global_registers;
                    for (number = next_optimizer_symbol(region->header->next->global_definitions);
                         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
                        definition = lookup_function_register(number)->global_expression;
                        if (definition->opcode == 0x131 ||
                            operation_is_comparison(definition->opcode)) {
                            valid = 1;
                            for (operand = definition->operands; operand;
                                 operand = operand->next) {
                                switch (operand->kind) {
                                case 1: case 2:
                                    if (!optimizer_set_contains(active, operand->value.reg->number))
                                        valid = 0;
                                    break;
                                case 3: case 4: case 5: case 7: case 8:
                                    break;
                                default:
                                    valid = 0;
                                    break;
                                }
                            }
                            if (valid) add_register_to_set(selected, number);
                        }
                    }
                    if (selected->links.first)
                        for (edge = region->header->incoming; edge;
                             edge = edge->next_incoming)
                            merge_optimizer_symbol_sets(edge->source->global_definitions, selected);
                    release_optimizer_symbol_set(selected);
                } else solve_global_register_sets(header, last);
                if (global_region_motion_enabled) prepare_region_motion(header, last);
            }
        }
        optimize_global_block(block, function);
        region = block->block_scope;
        if (region && region->last == block) finish_global_region(region);
    }
    if (global_aliases_changed) rewrite_global_aliases(function, 0);
    clear_global_block_marks(function);
    clear_global_register_marks(function);
    rewrite_global_function(function, 1);
    finish_global_function_rewrite(function);
    if (global_aliases_changed) {
        rewrite_global_aliases(function, 0);
        rewrite_global_function(function, 1);
    }
    prepare_global_function_cleanup(function);
    restore_global_function(function);
    finish_global_function(function);
    reset_input_pool(13);
    reset_input_pool(6);
    reset_input_pool(3);
    global_optimization_active = 0;
    release_global_optimizer();
}

/* Hash-chain payload is deliberately incomplete until its actual consumer.
 * This initializer establishes all101 pointer cells; no node layout is implied. */
C2GlobalExpression *global_expression_buckets[101]; /* 48f8a0 */
unsigned global_expression_count;                   /* 48fa9c */
unsigned global_expression_cursor;                  /* 48fa98 */
C2FunctionRegister *global_cached_expression_registers;            /* 48fa94 */
C2RegisterSet **global_index_registers;              /* 48fa6c */
C2RegisterSet **global_symbol_registers;             /* 48fa88 */
C2RegisterSet **global_index_direct_uses;            /* 48fb14 */
C2RegisterSet **global_index_variant_uses;           /* 48fa50 */
C2RegisterSet *global_pending_expressions;           /* 48fa74 */
extern unsigned optimizer_final_index_limit;
extern C2RegisterSet *optimizer_symbol_indices, *optimizer_value_indices;

/* Scratch sets are assigned on each nonempty region analysis. No pre-analysis
 * contents or defaults are consumed by this initializer. */
C2RegisterSet *region_register_domain;       /* 497414 */
C2RegisterSet *region_defined_registers;     /* 497418 */
C2RegisterSet *region_examined_registers;    /* 49741c */
C2RegisterSet *__fastcall new_region_register_domain(unsigned pool)
{
    C2RegisterSet *set = new_register_set(pool);
    merge_optimizer_symbol_sets(set, tracked_register_set);
    merge_optimizer_symbol_sets(set, secondary_register_set);
    return set;
}
void __fastcall clear_region_block_flags(C2Function *function)
{
    C2BasicBlock *block;
    for (block = function->flow->last; block; block = block->previous)
        block->flags &= ~1U;
}
void __fastcall analyze_single_region_registers(C2FlowRegion *region)
{
    C2FlowRegion *child;
    C2BasicBlock *block, *first, *end;
    C2Expression *record, *operand, *base, *use_operand;
    C2FunctionRegister *reg, *variant;
    C2OptimizerUse *use, *current, *saved;
    C2RegisterSet *set;
    unsigned number, block_flags;
    int invariant;

    for (child = region->children; child; child = child->next)
        analyze_single_region_registers(child);
    first = region->header;
    end = region->last->previous;
    for (block = first; block != end; block = block->previous) {
        if (block->flags & 1) continue;
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (operand = record->tail.right; operand; operand = operand->next) {
                switch (operand->kind) {
                case 1: case 2:
                    reg = operand->value.reg;
                    if (reg->value) break;
                    for (variant = reg->canonical; variant; variant = variant->variant_next)
                        if (variant == reg ||
                            (variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
                             reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes)))
                            add_register_to_set(region_defined_registers, variant->number);
                    break;
                case 6: case 10:
                    number = operand->kind == 6 ? operand->immediate : operand->value.optimizer_index;
                    if (test_and_add_global_register(region_examined_registers, number)) break;
                    use = get_global_index_uses(number);
                    saved = NULL;
                    while (use) {
                        current = use;
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            current = reg->optimizer_uses;
                            if (!current) {
                                use = use->next;
                                continue;
                            }
                            reg = current->reg;
                        }
                        for (variant = reg->canonical; variant; variant = variant->variant_next)
                            if (variant == reg ||
                                (variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
                                 reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes)))
                                add_register_to_set(region_defined_registers, variant->number);
                        use = current->next;
                        if (!use && saved) {
                            use = saved->next;
                            saved = NULL;
                        }
                    }
                    break;
                }
            }
        }
    }
    set = new_register_set(12);
    region->global_registers = set;
    copy_optimizer_symbol_set_into(set, region_register_domain);
    subtract_optimizer_symbol_sets(set, region_defined_registers);
    for (block = first; block != end; block = block->previous) {
        if (block->flags & 1) continue;
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (operand = record->left; operand; operand = operand->next) {
                operand->location.expression.comparison &= 0x7f;
                invariant = 0;
                switch (operand->kind) {
                case 1: case 2:
                    reg = operand->value.reg;
                    if (!reg->value && !optimizer_set_contains(region_defined_registers, reg->number))
                        invariant = 1;
                    break;
                case 3: case 4: case 7: case 8:
                    invariant = 1;
                    break;
                case 6:
                    use = get_global_index_uses(operand->immediate);
                    saved = NULL;
                    while (use) {
                        current = use;
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            current = reg->optimizer_uses;
                            if (!current) {
                                use = use->next;
                                continue;
                            }
                            reg = current->reg;
                        }
                        for (variant = reg->canonical; variant; variant = variant->variant_next)
                            if (optimizer_set_contains(region_defined_registers, variant->number))
                                goto next_operand;
                        use = current->next;
                        if (!use && saved) {
                            use = saved->next;
                            saved = NULL;
                        }
                    }
                    /* The indexed address still has its actual base register. */
                case 5:
                    if (operand->opcode != 0x122) invariant = 1;
                    else {
                        base = operand->aggregate_value;
                        if (!(((1U << (base->kind & 31)) & 6) && base->value.reg->value) &&
                            !optimizer_set_contains(region_defined_registers, base->value.reg->number))
                            invariant = 1;
                    }
                    break;
                }
                if (invariant) operand->location.expression.comparison |= 0x80;
next_operand: ;
            }
        }
    }
    for (block = first; block != end; block = block->previous) {
        block_flags = block->flags;
        if (block_flags & 1) continue;
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            operand = record->tail.right;
            if (!operand || !((1U << (operand->kind & 31)) & 6) ||
                !operand->value.reg->value || (operand->location.expression.comparison & 0x80)) continue;
            for (base = record->left; base; base = base->next)
                if (!(base->location.expression.comparison & 0x80)) break;
            if (base) continue;
            operand->location.expression.comparison |= 0x80;
            follow_optimizer_result(record, &use_operand);
            use_operand->location.expression.comparison |= 0x80;
        }
        block->flags = block_flags | 1;
    }
}
void __fastcall detach_input_operand(C2Expression *owner, C2Expression *operand);
void __fastcall solve_region_register_sets(C2Function *function)
{
    C2Expression *record, *next, *end, *result, *consumer, *use;
    C2Expression *first, *second, *increment, *selected, *scan, *output;
    if (!function->flow_regions) return;
    end = (C2Expression *)function->flow->first->last_record;
    for (record = (C2Expression *)function->flow->last->records->next;
         record != end; record = next) {
        next = record->next;
        if (record->opcode != 0x141) continue;
        result = record->tail.right;
        if (!((1U << (result->kind & 31)) & 6) || !result->value.reg->value) continue;
        consumer = follow_optimizer_result(record, &use);
        if (consumer->opcode != 0x141) continue;
        if (compare_input_operands(result, consumer->left)) swap_global_operands(consumer);
        if (compare_input_operands(result, consumer->left)) continue;
        first = record->left;
        second = first->next;
        increment = consumer->left->next;
        if (!(increment->location.expression.comparison & 0x80)) continue;
        if (first->location.expression.comparison & 0x80) {
            if ((second->location.expression.comparison & 0x80) || second->kind != 2) continue;
            selected = second;
        } else {
            if (!(second->location.expression.comparison & 0x80) || first->kind != 2) continue;
            selected = first;
        }
        detach_input_operand(record, selected);
        detach_input_operand(consumer, increment);
        copy_input_operand(record, increment);
        copy_input_operand(consumer, selected);
        record->tail.right->location.expression.comparison |= 0x80;
        consumer->left->location.expression.comparison |= 0x80;
        next = record->next;
        for (scan = next; scan != consumer; scan = scan->next) {
            output = scan->tail.right;
            if (output && (!((1U << (output->kind & 31)) & 6) ||
                           !output->value.reg->value)) break;
        }
        move_input_record_before((C2Record *)scan, (C2Record *)record);
    }
}

void __fastcall analyze_region_register_sets(C2Function *function)
{
    C2FlowRegion *region;
    if (!function->flow_regions) return;
    region_register_domain = new_region_register_domain(4);
    region_defined_registers = new_register_set(4);
    region_examined_registers = new_register_set(4);
    clear_region_block_flags(function);
    for (region = function->flow_regions; region; region = region->next) {
        analyze_single_region_registers(region);
        clear_register_set(region_defined_registers);
        clear_register_set(region_examined_registers);
    }
    reset_input_pool(4);
}

C2FlowEdge *__fastcall find_flow_edge(C2BasicBlock *source, C2BasicBlock *destination);

/* Assigned from the real region entry's definitions immediately before the
 * motion children consume it; no earlier set contents/default is inferred. */
C2RegisterSet *global_region_definitions; /* 49742c */
extern int global_available_additions_suppressed;
void __fastcall coalesce_region_assignments(C2FlowRegion *region);
void __fastcall prepare_region_motion_assignments(C2FlowRegion *region, int mode);
void __fastcall build_region_motion_sets(C2FlowRegion *region);
void __fastcall set_region_motion_anchor(C2FlowRegion *region);
void __fastcall move_global_region_block(C2BasicBlock *block, C2FlowRegion *region,
    C2Function *function);
void __fastcall finish_region_motion_candidates(C2FlowRegion *region, C2Function *function);

extern int global_rewrite_state;
void __fastcall prepare_region_motion_assignments(C2FlowRegion *region, int mode)
{
    if (mode & 2) global_rewrite_state = 1;
    clear_global_rewrite_marks(region->rewrite_entry, region->last);
    propagate_global_rewrites((mode & 1) ? region->rewrite_entry : region->header, region->last);
    global_rewrite_state = 0;
}

/* Both sets are assigned from the actual entry predecessor after the region
 * solver; no contents or initial object extent are inferred here. */
C2RegisterSet *region_entry_uses;      /* 497428, predecessor+48 */
C2RegisterSet *region_entry_alias_out; /* 497424, predecessor+60 */
void __fastcall solve_region_motion_availability(C2FlowRegion *region);
void __fastcall collect_region_motion_definitions(C2FlowRegion *region);

void __fastcall solve_region_motion_block(C2BasicBlock *block, int entry, int *changed);

extern C2RegisterSet *global_temporary_registers;
void __fastcall solve_region_motion_block(C2BasicBlock *block, int entry, int *changed)
{
    C2FlowEdge *edge;
    C2FlowRegion *region;
    C2RegisterSet *input;
    edge = block->incoming;
    input = block->rewrite_mode4[5];
    if (edge) {
        if (!entry) {
            copy_optimizer_symbol_set_into(input, edge->source->rewrite_mode4[6]);
            for (edge = edge->next_incoming; edge; edge = edge->next_incoming)
                intersect_optimizer_symbol_sets(input, edge->source->rewrite_mode4[6]);
        } else {
            region = block->block_scope;
            if (edge->source->block_scope != region) edge = edge->next_incoming;
            /* The original assumes another incoming edge exists after skipping
             * the outside-region edge; it reads that edge without a NULL test. */
            copy_optimizer_symbol_set_into(input, edge->source->rewrite_mode4[6]);
            for (edge = edge->next_incoming; edge; edge = edge->next_incoming)
                if (edge->source->block_scope == region)
                    intersect_optimizer_symbol_sets(input, edge->source->rewrite_mode4[6]);
        }
    }
    update_available_register_transfer(block->rewrite_mode4[6], global_temporary_registers,
        input, block->rewrite_mode4[1], changed);
}

/* Exact insertion cursor, assigned before the region block walker. */
C2Record *region_motion_anchor; /* 497420 */
void __fastcall set_region_motion_anchor(C2FlowRegion *region)
{
    region_motion_anchor = region->rewrite_entry->last_record;
}

extern C2Expression *global_next_record;
extern unsigned diagnostic_position;
int __fastcall block_reaches_region_exits(C2BasicBlock *block, C2FlowRegion *region);
int __fastcall region_register_is_live(C2FunctionRegister *reg, C2RegisterSet *uses,
    C2RegisterSet *aliases);

int __fastcall optimizer_dense_contains(C2RegisterSet *set, unsigned number);
int __fastcall region_register_is_live(C2FunctionRegister *reg, C2RegisterSet *uses,
    C2RegisterSet *aliases)
{
    C2FunctionRegister *canonical, *variant;
    unsigned uncovered, required;
    long first, last, unit, count;

    if (optimizer_set_contains(uses, reg->number)) return 1;
    canonical = reg->canonical;
    for (variant = canonical; variant; variant = variant->variant_next) {
        if (variant == reg) continue;
        if (variant->displacement < (long)(reg->value_bytes + (unsigned)reg->displacement) &&
            reg->displacement < (long)(variant->value_bytes + (unsigned)variant->displacement) &&
            optimizer_set_contains(uses, variant->number)) break;
    }
    if (!variant) return 0;
    if ((long)canonical->value_bytes > 0x100) return 1;
    uncovered = ~0U;
    for (variant = canonical; variant; variant = variant->variant_next)
        if (optimizer_set_contains(aliases, variant->number)) {
            first = variant->displacement / 8;
            last = (long)(variant->value_bytes + (unsigned)variant->displacement - 1) / 8;
            for (unit = first; unit <= last; ++unit)
                uncovered &= ~(1U << (unit & 31));
        }
    count = (long)(canonical->value_bytes + 7) / 8;
    /* Keep the original x86 shift-count wrap, including its zero mask at32. */
    required = (1U << (count & 31)) - 1;
    return (uncovered & required) != 0;
}

int __fastcall block_reaches_region_exits(C2BasicBlock *block, C2FlowRegion *region)
{
    C2FlowBlockLink *exit;
    for (exit = region->exits; exit; exit = exit->next)
        if (!optimizer_dense_contains(exit->block->dominators, (int)(short)block->number84))
            return 0;
    return 1;
}

void __fastcall move_global_region_block(C2BasicBlock *block, C2FlowRegion *region,
    C2Function *function)
{
    C2FlowEdge *edge;
    C2Expression *record, *end, *output, *next, *input, *moved, *definition;
    C2FunctionRegister *reg;
    C2RegisterSet *input_set;
    unsigned number;
    int movable, saw_operation;

    input_set = block->rewrite_mode4[5];
    if (block == region->header) {
        for (edge = block->incoming; edge; edge = edge->next_incoming)
            if (edge->source->block_scope == block->block_scope) {
                copy_optimizer_symbol_set_into(input_set, edge->source->rewrite_mode4[6]);
                break;
            }
        for (edge = block->incoming; edge; edge = edge->next_incoming)
            if (edge->source->block_scope == block->block_scope)
                intersect_optimizer_symbol_sets(input_set, edge->source->rewrite_mode4[6]);
    } else {
        edge = block->incoming;
        if (edge) {
            copy_optimizer_symbol_set_into(input_set, edge->source->rewrite_mode4[6]);
            for (edge = edge->next_incoming; edge; edge = edge->next_incoming)
                intersect_optimizer_symbol_sets(input_set, edge->source->rewrite_mode4[6]);
        }
    }
    global_available_registers = block->rewrite_mode4[6];
    copy_optimizer_symbol_set_into(global_available_registers, input_set);
    movable = block_reaches_region_exits(block, region);
    end = (C2Expression *)block->last_record;
    saw_operation = 0;
    for (record = (C2Expression *)block->records->next; record != end; record = global_next_record) {
        global_next_record = record->next;
        if (record->location.line) {
            diagnostic_position = function->source_line_base + record->location.line;
            record_line_delta = record->location.line;
        }
        if (!(record->flags & 1)) continue;
        if (record->kind == 11) saw_operation = 1;
        if (!movable) goto update_record;
        output = record->tail.right;
        if (record->opcode == 0x131) {
            if (((1U << (output->kind & 31)) & 6) && output->value.reg->value)
                goto update_record;
            reg = record->value.reg;
            if (!((1U << (output->kind & 31)) & 6) ||
                region_register_is_live(output->value.reg, region_entry_uses,
                    region_entry_alias_out)) goto update_record;
        } else {
            if (!output || !((1U << (output->kind & 31)) & 6)) goto update_record;
            reg = output->value.reg;
            if (!reg->global_expression) goto update_record;
        }
        number = reg->number;
        if (!optimizer_set_contains(global_available_registers, number) ||
            input_has_side_effects(record)) goto update_record;
        if (record->opcode == 0x141 || record->opcode == 0x142) {
            next = record->next;
            if (next->opcode == 0x131 && !compare_input_operands(output, next->left)) {
                if (!compare_input_operands(next->tail.right, record->left) ||
                    !compare_input_operands(next->tail.right, record->left->next))
                    goto update_record;
            }
        }
        if (record->kind == 17 || record->kind == 15)
            input = reg->global_expression->operands;
        else input = record->left;
        for (; input; input = input->next)
            if (((1U << (input->kind & 31)) & 6) && input->value.reg->global_expression &&
                !optimizer_set_contains(global_region_definitions, input->value.reg->number))
                goto update_record;
        moved = record;
        if (record->precision_flags == 3) moved = copy_input_expression(record);
        else unlink_input_record((C2Record *)record);
        if (moved->kind == 17) {
            for (input = moved->left; input; input = input->next)
                if (((1U << (input->kind & 31)) & 6) && input->value.reg->value) {
                    definition = input->value.reg->value;
                    unlink_input_record((C2Record *)definition);
                    prepend_input_record(region_motion_anchor, (C2Record *)definition);
                }
        } else if (moved->kind == 15) {
            for (input = moved->left; input; input = input->next)
                if (((1U << (input->kind & 31)) & 6) && input->value.reg->value) {
                    definition = input->value.reg->value;
                    unlink_input_record((C2Record *)definition);
                    prepend_input_record(region_motion_anchor, (C2Record *)definition);
                }
        }
        prepend_input_record(region_motion_anchor, (C2Record *)moved);
        mark_global_register_available(reg);
        add_register_to_set(region->header->global_available_in, number);
        add_register_to_set(global_region_definitions, number);
        add_register_to_set(global_motion_outputs, number);
        if (moved->opcode == 0x131 && ((1U << (output->kind & 31)) & 6) &&
            output->value.reg->global_expression) {
            number = moved->tail.right->value.reg->number;
            add_register_to_set(region->header->global_available_in, number);
            add_register_to_set(global_region_definitions, number);
            add_register_to_set(global_motion_outputs, number);
        }
        continue;
update_record:
        update_global_available_registers(record);
    }
    if (saw_operation && !find_next_global_operation((C2Expression *)block->records))
        global_aliases_changed = 1;
}

void __fastcall collect_region_motion_definitions(C2FlowRegion *region)
{
    C2BasicBlock *block;
    clear_register_set(global_motion_registers);
    for (block = region->header; block != region->last->previous; block = block->previous)
        merge_optimizer_symbol_sets(global_motion_registers, block->rewrite_mode4[2]);
}

void __fastcall solve_region_motion_availability(C2FlowRegion *region)
{
    C2BasicBlock *first = region->header, *last = region->last, *block, *end;
    int changed;
    copy_optimizer_symbol_set_into(last->rewrite_mode4[6], last->global_definitions);
    do {
        changed = 0;
        copy_optimizer_symbol_set_into(first->rewrite_mode4[5], last->rewrite_mode4[6]);
        end = last->previous;
        for (block = first; block != end; block = block->previous)
            solve_region_motion_block(block, block == first, &changed);
    } while (changed);
}

void __fastcall build_region_motion_sets(C2FlowRegion *region)
{
    C2FlowBlockLink *exit;
    C2FlowEdge *edge;
    C2BasicBlock *destination;
    solve_region_motion_availability(region);
    region_entry_uses = region->header->next->rewrite_mode1[3];
    region_entry_alias_out = region->header->next->alias_out;
    clear_register_set(global_motion_inputs);
    for (exit = region->exits; exit; exit = exit->next)
        for (edge = exit->block->outgoing; edge; edge = edge->next_outgoing) {
            destination = edge->destination;
            if (destination->block_scope != region)
                merge_optimizer_symbol_sets(global_motion_inputs, destination->rewrite_mode1[2]);
        }
    collect_region_motion_definitions(region);
    clear_register_set(global_motion_outputs);
}

void __fastcall coalesce_region_assignments(C2FlowRegion *region)
{
    C2BasicBlock *block;
    C2Expression *record, *output, *input, *next;
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (record->opcode != 0x131) continue;
            output = record->tail.right;
            if (!((1U << (output->kind & 31)) & 6) || output->value.reg->kind != 3) continue;
            input = record->left;
            if (!((1U << (input->kind & 31)) & 6) || input->value.reg->value) continue;
            next = record->next;
            if (next->opcode != 0x131 || compare_input_operands(output, next->left)) continue;
            if (input_has_side_effects(record->left) || input_has_side_effects(next->left)) continue;
            attach_input_expression(next, next->left, record->left);
            finish_global_assignment(next, 1);
        }
}

extern int global_substitution_check_enabled;
void __fastcall region_candidate_pass_3caf9(C2FlowRegion *region);
void __fastcall region_candidate_pass_3ccb2(C2FlowRegion *region);
void __fastcall region_candidate_pass_3d127(C2FlowRegion *region);
unsigned __fastcall region_candidate_pass_3d37b(C2FlowRegion *region);
void __fastcall region_candidate_pass_3ba96(C2FlowRegion *region);
int __fastcall region_candidate_pass_3d4a4(C2FlowRegion *region, C2Function *function);
unsigned __fastcall region_candidate_pass_3d93c(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3de8b(C2FlowRegion *region, C2Function *function, unsigned changes);
void __fastcall region_candidate_pass_3c245(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3c33f(C2FlowRegion *region);
void __fastcall region_candidate_pass_3c3a0(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3c4a5(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3c87c(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3bbb3(C2FlowRegion *region);
void __fastcall region_candidate_pass_3e5ed(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3e73e(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3e91d(C2FlowRegion *region);
void __fastcall region_candidate_pass_3e9fe(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3ea57(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3ec06(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3ec67(C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3edd3(C2FlowRegion *region);

/* Candidate preparation clears its three real region DWORDs at the end.
 * Their later list/count meanings remain with their reached consumers. */
C2Expression *region_deferred_assignment; /* 497434, last two-use rewrite. */
/* Original initialized DWORD497454 is NULL; the counter factory owns later writes. */
C2Expression *region_iteration_counter;
void __fastcall remove_input_operand(C2Expression *owner, C2Expression *operand);
C2Expression *__fastcall new_region_binary_expression(unsigned opcode,
    unsigned short type, C2Expression *left, C2Expression *right,
    C2InputLink link, C2Record *prior)
{
    C2Expression *value, *record;
    value = new_global_value_expression(opcode, type, left, right);
    link_global_expression_dependencies(value->value.reg);
    record = (C2Expression *)allocate_small_node(11);
    record->opcode = opcode;
    record->flags |= 1;
    record->type = type;
    if ((1U << (value->kind & 31)) & 0x7fe) {
        if (value->location.expression.properties & 0x10)
            value = copy_input_expression(value);
        value->location.expression.properties |= 0x10;
    }
    record->tail.right = value;
    if ((1U << (left->kind & 31)) & 0x7fe) {
        if (left->location.expression.properties & 0x10)
            left = copy_input_expression(left);
        left->location.expression.properties |= 0x10;
    }
    record->left = left;
    if ((1U << (right->kind & 31)) & 0x7fe) {
        if (right->location.expression.properties & 0x10)
            right = copy_input_expression(right);
        right->location.expression.properties |= 0x10;
    }
    left->next = right;
    retain_aggregate_operands(record);
    if (link) link(prior, (C2Record *)record);
    record->location.line = record_line_delta;
    return record;
}
int __fastcall region_operand_is_invariant(C2Expression *operand);

int __fastcall region_operand_is_invariant(C2Expression *operand)
{
    C2FunctionRegister *reg;
    if ((1U << (operand->kind & 31)) & 0x180) return 1;
    if ((1U << (operand->kind & 31)) & 0x18) return 1;
    if (operand->kind != 2) return 0;
    reg = operand->value.reg;
    if (reg->rewrite_flags & 2) return 1;
    if (reg->value || optimizer_set_contains(global_motion_registers, reg->number)) return 0;
    if (reg->kind != 3) return 1;
    return optimizer_set_contains(global_region_definitions, reg->number) != 0;
}

void __fastcall region_candidate_pass_3caf9(C2FlowRegion *region)
{
    C2BasicBlock *block;
    C2Expression *record, *next, *constant, *operation;
    reset_input_pool(3);
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = next) {
            next = record->next;
            record->flags &= ~0x40;
            if (record->precision_flags == 7 || record->precision_flags == 8)
                record->precision_flags = 0;
            if (record->opcode == 0x137 && (record->type & 0xf000) == 0x1000 &&
                !input_has_side_effects(record)) {
                constant = new_integer_expression(record->type, ~(unsigned __int64)0);
                operation = new_region_binary_expression(0x143, record->type, record->left,
                    constant, prepend_input_record, (C2Record *)record);
                attach_input_expression(record, record->left, operation->tail.right);
                record->opcode = 0x131;
                finish_global_assignment(record, 1);
            }
            if (record->opcode == 0x142 && (record->type & 0xf000) == 0x1000 &&
                region_operand_is_invariant(record->left) &&
                !region_operand_is_invariant(record->left->next) &&
                !input_has_side_effects(record)) {
                constant = new_integer_expression(record->type, ~(unsigned __int64)0);
                operation = new_region_binary_expression(0x143, record->type, record->left->next,
                    constant, prepend_input_record, (C2Record *)record);
                operation = new_region_binary_expression(0x141, record->type, operation->tail.right,
                    record->left, prepend_input_record, (C2Record *)record);
                attach_input_expression(record, record->left, operation->tail.right);
                remove_input_operand(record, record->left->next);
                record->opcode = 0x131;
                finish_global_assignment(record, 1);
            }
        }
    region->pending28_word = 0;
    region->pending2c_word = 0;
    region->allocation_next = NULL;
    region_deferred_assignment = NULL;
}

extern C2RegisterSet *region_motion_sets[11];
int __fastcall global_operand_is_restricted(C2Expression *operand)
{
    if (!((1U << (operand->kind & 31)) & 6)) return 0;
    return optimizer_set_contains(region_motion_sets[9], operand->value.reg->number) != 0;
}

void __fastcall region_candidate_pass_3ccb2(C2FlowRegion *region)
{
    C2BasicBlock *block;
    C2Expression *record, *operand;
    C2OptimizerUse *use, *saved, *current;
    C2FunctionRegister *reg, *canonical, *variant, *other;
    unsigned number;
    int wide;

    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind == 6 || operand->kind == 10) {
                    use = get_global_index_uses(operand->kind == 6 ? operand->immediate :
                        operand->value.optimizer_index);
                    saved = NULL;
                    while (use) {
                        current = use;
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            current = reg->optimizer_uses;
                            if (!current) { use = use->next; continue; }
                            reg = current->reg;
                        }
                        for (variant = reg->canonical; variant; variant = variant->variant_next)
                            add_register_to_set(region_motion_sets[4], variant->number);
                        use = current->next;
                        if (!use && saved) { use = saved->next; saved = NULL; }
                    }
                } else if (((1U << (operand->kind & 31)) & 6) &&
                    operand->value.reg->canonical->symbol) {
                    add_register_to_set(region_motion_sets[8], operand->value.reg->number);
                    if (input_has_side_effects(operand))
                        add_register_to_set(region_motion_sets[4], operand->value.reg->number);
                }
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                if ((1U << (operand->kind & 31)) & 6) {
                    reg = operand->value.reg;
                    if (!reg->value) {
                        number = reg->number;
                        if (input_has_side_effects(operand))
                            add_register_to_set(region_motion_sets[4], number);
                        if (block->block_scope != region)
                            add_register_to_set(region_motion_sets[4], number);
                    }
                } else if (operand->kind == 6 || operand->kind == 10) {
                    use = get_global_index_uses(operand->kind == 6 ? operand->immediate :
                        operand->value.optimizer_index);
                    saved = NULL;
                    while (use) {
                        current = use;
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            current = reg->optimizer_uses;
                            if (!current) { use = use->next; continue; }
                            reg = current->reg;
                        }
                        for (variant = reg->canonical; variant; variant = variant->variant_next)
                            add_register_to_set(region_motion_sets[4], variant->number);
                        use = current->next;
                        if (!use && saved) { use = saved->next; saved = NULL; }
                    }
                }
            }
            if (record->opcode == 0x131 && (record->type & 0xf000) == 0x5000 &&
                record->tail.right->kind == 2) reg = record->tail.right->value.reg;
            else if (record->opcode == 0x13e && record->left->kind == 2)
                reg = record->left->value.reg;
            else reg = NULL;
            if (reg)
                for (variant = reg->canonical; variant; variant = variant->variant_next)
                    if (variant == reg ||
                        (variant->displacement < (long)(reg->value_bytes + (unsigned)reg->displacement) &&
                         reg->displacement < (long)(variant->value_bytes + (unsigned)variant->displacement))) {
                        add_register_to_set(region_motion_sets[4], variant->number);
                        add_register_to_set(region_motion_sets[2], variant->number);
                    }
        }
    for (number = next_optimizer_symbol(region_motion_sets[8]); number != ~0U;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        canonical = reg->canonical;
        wide = canonical->type == 0x1008 || canonical->type == 0x2008;
        for (variant = canonical; variant; variant = variant->variant_next) {
            if (wide) {
                add_register_to_set(region_motion_sets[4], number);
                add_register_to_set(region_motion_sets[4], variant->number);
                continue;
            }
            if (reg != variant &&
                ((long)(variant->value_bytes + (unsigned)variant->displacement) <= reg->displacement ||
                 (long)(reg->value_bytes + (unsigned)reg->displacement) <= variant->displacement)) continue;
            if (reg->displacement == variant->displacement) continue;
            add_register_to_set(region_motion_sets[4], number);
            add_register_to_set(region_motion_sets[2], number);
            if (!global_type_has_register_class(variant->type)) continue;
            add_register_to_set(region_motion_sets[4], variant->number);
            add_register_to_set(region_motion_sets[2], variant->number);
            for (other = canonical; other; other = other->variant_next)
                if ((other == variant ||
                    (variant->displacement < (long)(other->value_bytes + (unsigned)other->displacement) &&
                     other->displacement < (long)(variant->value_bytes + (unsigned)variant->displacement))) &&
                    global_type_has_register_class(other->type)) {
                    add_register_to_set(region_motion_sets[4], other->number);
                    add_register_to_set(region_motion_sets[2], other->number);
                }
        }
    }
}

int __fastcall global_operand_is_restricted(C2Expression *operand);
unsigned __fastcall test_and_remove_region_register(C2RegisterSet *set, unsigned number);

void __fastcall region_candidate_pass_3d127(C2FlowRegion *region)
{
    C2BasicBlock *block;
    C2Expression *record, *output;
    unsigned opcode, number;
    int changed, left_candidate, left_invariant, right_candidate, right_invariant, remove;

    copy_optimizer_symbol_set_into(region_motion_sets[10], region_motion_sets[9]);
    for (block = region->header; block != region->last->previous; block = block->previous) {
        if (block->block_scope != region) continue;
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            opcode = record->opcode;
            if (opcode == 0x131) {
                if (record->precision_flags == 1 || record->precision_flags == 14) continue;
            } else if (opcode != 0x141 && opcode != 0x142) continue;
            if (((1U << (record->type >> 12)) & 0xe) && record->tail.right->kind == 2)
                add_register_to_set(region_motion_sets[9], record->tail.right->value.reg->number);
        }
    }
    subtract_optimizer_symbol_sets(region_motion_sets[9], region_motion_sets[4]);
    do {
        changed = 0;
        for (block = region->header; block != region->last->previous; block = block->previous)
            for (record = (C2Expression *)block->records->next;
                 record != (C2Expression *)block->last_record; record = record->next) {
                opcode = record->opcode;
                remove = 0;
                if (opcode == 0x131) {
                    if (global_operand_is_restricted(record->tail.right) &&
                        !global_operand_is_restricted(record->left)) remove = 1;
                } else if (opcode == 0x141 || opcode == 0x142) {
                    if (global_operand_is_restricted(record->tail.right)) {
                        left_candidate = global_operand_is_restricted(record->left);
                        left_invariant = region_operand_is_invariant(record->left);
                        right_candidate = global_operand_is_restricted(record->left->next);
                        right_invariant = region_operand_is_invariant(record->left->next);
                        if ((!left_candidate && !left_invariant) ||
                            (!right_candidate && !right_invariant) ||
                            (left_invariant && right_invariant) ||
                            (left_candidate && right_candidate) ||
                            (opcode == 0x142 && !left_candidate)) remove = 1;
                    }
                } else if (record->flags & 1) {
                    output = record->tail.right;
                    if (output && output->kind == 2 && !output->value.reg->value &&
                        test_and_remove_region_register(region_motion_sets[9], output->value.reg->number))
                        changed = 1;
                }
                if (remove) {
                    number = record->tail.right->value.reg->number;
                    if (!optimizer_set_contains(region_motion_sets[10], number)) {
                        remove_optimizer_symbol(region_motion_sets[9], number);
                        changed = 1;
                    }
                }
            }
    } while (changed);
}

unsigned __fastcall replace_region_register_variant(C2Expression *operand, C2Expression *owner);
unsigned __fastcall replace_region_register_variant(C2Expression *operand, C2Expression *owner)
{
    C2FunctionRegister *reg = operand->value.reg, *variant;
    C2Expression *copy, *conversion;
    for (variant = reg->canonical; variant; variant = variant->variant_next) {
        if (variant->displacement > reg->displacement ||
            (long)(variant->value_bytes + (unsigned)variant->displacement) <
            (long)(reg->value_bytes + (unsigned)reg->displacement)) continue;
        if (!optimizer_set_contains(region_motion_sets[9], variant->number) || variant == reg) continue;
        copy = copy_input_expression(operand);
        retarget_global_operand(copy, variant->type);
        conversion = new_linked_global_expression(0x135, operand->type, copy,
            prepend_input_record, (C2Record *)owner);
        operand->value.reg = conversion->tail.right->value.reg;
        return 1;
    }
    return 0;
}

unsigned __fastcall region_candidate_pass_3d37b(C2FlowRegion *region)
{
    C2BasicBlock *block;
    C2Expression *record, *operand;
    unsigned changed, record_changed;
    changed = 0;
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            record_changed = 0;
            for (operand = record->left; operand; operand = operand->next)
                if (operand->kind == 2 && operand->value.reg && operand->value.reg->canonical->symbol)
                    record_changed |= replace_region_register_variant(operand, record);
            if (record_changed) {
                update_global_record_registers(record, 0);
                changed = 1;
            }
        }
    return changed;
}

void __fastcall add_region_register_use(C2FunctionRegister *reg, C2Expression *record);
void __fastcall add_region_register_definition(C2FunctionRegister *reg, C2Expression *record);

void __fastcall add_region_register_use(C2FunctionRegister *reg, C2Expression *record)
{
    C2OptimizerRecordLink *previous = reg->global_uses, *link;
    link = (C2OptimizerRecordLink *)c2_pool_allocate(3, 8);
    link->record = record;
    link->next = previous;
    reg->global_uses = link;
}

void __fastcall add_region_register_definition(C2FunctionRegister *reg, C2Expression *record)
{
    C2OptimizerRecordLink *previous = reg->global_definitions, *link;
    link = (C2OptimizerRecordLink *)c2_pool_allocate(3, 8);
    link->record = record;
    link->next = previous;
    reg->global_definitions = link;
}

void __fastcall region_candidate_pass_3ba96(C2FlowRegion *region)
{
    C2FunctionRegister *reg;
    C2BasicBlock *block;
    C2Expression *record, *operand;
    unsigned number;
    if (!region_motion_sets[9]->links.first) return;
    for (number = next_optimizer_symbol(region_motion_sets[9]); number != ~0U;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        reg->global_uses = NULL;
        reg->global_definitions = NULL;
    }
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1) || record->precision_flags == 2 || record->precision_flags == 1)
                continue;
            for (operand = record->left; operand; operand = operand->next)
                if (global_operand_is_restricted(operand)) add_region_register_use(operand->value.reg, record);
            for (operand = record->tail.right; operand; operand = operand->next)
                if (global_operand_is_restricted(operand))
                    add_region_register_definition(operand->value.reg, record);
        }
}

C2Expression *__fastcall previous_global_code_record(C2Expression *record);
void __fastcall remove_control_record(C2Expression *record);
C2BasicBlock *__fastcall find_input_record_block(C2Expression *record);
int __fastcall compare_region_iteration_order(C2Expression *comparison, C2Expression *update);
C2Expression *__fastcall find_region_initial_value(C2Expression *operand, C2FlowRegion *region);
C2Expression *__fastcall compute_region_iteration_count(C2FlowRegion *region,
    C2Expression *first, C2Expression *bound, C2Expression *step, int inclusive,
    int order, C2Expression *branch);

C2BasicBlock *__fastcall find_input_record_block(C2Expression *record)
{
    for (; record; record = record->previous)
        if (record->kind == 24) return ((C2Record *)record)->owner.block;
    return NULL;
}

C2Expression *__fastcall previous_global_code_record(C2Expression *record)
{
    record = record->previous;
    while (record && !(record->flags & 1)) {
        if (record->kind == 24) return NULL;
        record = record->previous;
    }
    return record;
}

int __fastcall compare_region_iteration_order(C2Expression *comparison, C2Expression *update)
{
    C2BasicBlock *first, *second;
    C2Expression *record;
    first = find_input_record_block(comparison);
    second = find_input_record_block(update);
    if (!optimizer_dense_contains(second->dominators, (int)(short)first->number84)) return 0;
    if (first != second) return 1;
    for (record = comparison; record; record = record->next) {
        if (record->kind == 24) return 0;
        if (record == update) return 1;
    }
    return 0;
}

C2Expression *__fastcall find_region_initial_value(C2Expression *operand, C2FlowRegion *region)
{
    C2Expression *assignment, *result;
    assignment = new_binary_expression(0x131, operand->type, operand, operand, NULL, NULL);
    global_available_registers = region->rewrite_entry->global_available_in;
    global_record_needs_rewrite(assignment);
    result = copy_input_expression(assignment->left);
    release_small_node(assignment);
    return result;
}

/* These two statement boundaries enclose the temporary region expressions.
 * Their links are written by initialize_region_expression_list before use. */
C2Record *region_expression_head; /* 497438 */
C2Record *region_expression_tail; /* 497430 */
C2Record *__cdecl new_statement_boundary(void);
void __cdecl initialize_region_expression_list(void)
{
    region_expression_head = new_statement_boundary();
    region_expression_tail = new_statement_boundary();
    region_expression_tail->previous = region_expression_head;
    region_expression_head->next = region_expression_tail;
}
void __cdecl clear_region_expression_list(void)
{
    C2Record *record, *next;
    for (record = region_expression_head; record; record = next) {
        next = record->next;
        if (record->kind != 24) unlink_and_release_input_record(record);
    }
}
void __cdecl release_region_expression_list(void)
{
    clear_region_expression_list();
    release_small_node(region_expression_head);
    release_small_node(region_expression_tail);
    region_expression_head = NULL;
    region_expression_tail = NULL;
}
C2Expression *__fastcall compare_region_constants(C2Expression *left,
    C2Expression *right, int condition);
C2Expression *__fastcall convert_region_operand(unsigned short type,
    C2Expression *operand, C2Record *prior)
{
    C2Expression *last = (C2Expression *)prior->previous;
    convert_input_expression(&operand, type, &last);
    return operand;
}
C2FunctionRegister *__fastcall renew_region_output_register(C2Expression *operand)
{
    C2FunctionRegister *reg = new_temporary_register(operand->type);
    operand->value.reg = reg;
    operand->original_register = reg;
    return reg;
}

C2Expression *__fastcall collect_region_value_records(C2FunctionRegister *reg,
    C2Expression **first, C2Expression **last)
{
    C2Expression *record, *operand, *next, *replacement, *result, *subfirst, *sublast;
    C2FunctionRegister *old;
    unsigned short type;
    record = copy_input_expression(reg->value);
    *first = record;
    *last = record;
    for (operand = record->left; operand; operand = next) {
        next = operand->next;
        if (!((1U << (operand->kind & 31)) & 6) || !operand->value.reg->value) continue;
        replacement = collect_region_value_records(operand->value.reg, &subfirst, &sublast);
        type = operand->type;
        if (replacement->type != type) {
            replacement->type = type;
            replacement->value.reg = new_address_register(replacement->value.reg,
                                                           type, type & 0xfff, 0);
        }
        attach_input_expression(record, operand, replacement);
        link_code_nodes((C2Record *)sublast, (C2Record *)*first);
        *first = subfirst;
    }
    result = NULL;
    for (operand = record->tail.right; operand; operand = operand->next) {
        if (!((1U << (operand->kind & 31)) & 6) || !operand->value.reg->value) continue;
        old = operand->value.reg;
        renew_region_output_register(operand);
        operand->value.reg->value = record;
        if (old->canonical != reg->canonical) continue;
        result = copy_input_expression(operand);
        type = reg->type;
        if (result->type != type) {
            result->type = type;
            result->value.reg = new_address_register(result->value.reg, type, type & 0xfff, 0);
        }
    }
    if (!result) c2_report_internal("E:\\utc\\src\\\\P2\\sdsu.c", 0xa5);
    return result;
}

extern int expression_warning_issued; /* Existing48fb18 owner. */
C2Expression *__fastcall simplify_statement_expressions(C2Record *record);
void __fastcall simplify_region_statement(C2Expression *record)
{
    C2Expression *output;
    expression_warning_issued = 0;
    switch (record->kind) {
    case 11: case 12: case 13: case 15: case 17:
        output = record->tail.right;
        if (output && ((1U << (output->kind & 31)) & 6) && output->value.reg->value) break;
        /* Fall through: only unbound results are simplified directly. */
    case 14: case 16: case 18: case 19:
        simplify_statement_expressions((C2Record *)record);
        break;
    }
}
int __cdecl check_region_integer_operation(unsigned left, unsigned right,
    int sign, int bytes);

C2Expression *__fastcall new_select_expression(unsigned opcode, unsigned short type,
    C2Expression *condition, C2Expression *left, C2Expression *right,
    unsigned char comparison, C2InputLink link, C2Record *prior);

C2Expression *__fastcall compare_region_constants(C2Expression *left,
    C2Expression *right, int condition)
{
    C2Record *tail;
    C2Expression *comparison, *one, *zero, *record;
    unsigned short type;
    clear_region_expression_list();
    tail = region_expression_tail;
    type = common_input_value_type(left->type, right->type);
    comparison = new_comparison_expression(0x151, type, left, right, prepend_input_record, tail);
    one = new_integer_expression(0x1004, 1);
    zero = new_integer_expression(0x1004, 0);
    new_select_expression(0x15e, 0x1004, comparison->tail.right, zero, one,
                          (unsigned char)condition, prepend_input_record, tail);
    prepare_global_record(comparison, NULL, NULL);
    record = (C2Expression *)region_expression_tail->previous;
    if (record->opcode == 0x131) return copy_input_expression(record->left);
    return NULL;
}

C2Expression *__fastcall compute_region_iteration_count(C2FlowRegion *region,
    C2Expression *first, C2Expression *bound, C2Expression *step, int inclusive,
    int order, C2Expression *branch)
{
    C2Expression *difference, *nonzero, *test, *record, *constant, *result;
    C2Expression *list, *last, *next, *operand;
    C2Record *tail;
    unsigned first_value, bound_value, step_value;
    unsigned char condition;
#ifdef __linux__
    int immediate_inputs;
    immediate_inputs = first->kind == 7 && bound->kind == 7;
#endif

    condition = branch->convention;
    initialize_region_expression_list();
    /* Original only saves these words for immediate operands. The eventual
     * constant-result test otherwise consumes unwritten automatic storage. */
    if (first->kind == 7) first_value = first->integer.low;
    if (bound->kind == 7) bound_value = bound->integer.low;
    step_value = step->integer.low;
    nonzero = NULL;
    operand = copy_input_expression(bound);
    difference = compare_region_constants(copy_input_expression(first), operand, 3);
    if (difference) {
        constant = new_integer_expression(0x1004, 0);
        test = compare_region_constants(copy_input_expression(step), constant, 3);
        if (test) nonzero = compare_region_constants(difference, test, 2);
    }
    clear_region_expression_list();
    tail = region_expression_tail;
    first = convert_region_operand(0x2004, first, tail);
    bound = convert_region_operand(0x2004, bound, tail);
    step = convert_region_operand(0x2004, step, tail);
    if (first->kind == 7 && bound->kind == 7 &&
        check_region_integer_operation(bound->integer.low - first->integer.low,
                                       step->integer.low, 1, 4))
        return NULL;
    record = new_folded_binary_expression(0x142, 0x2004, bound, first, prepend_input_record, tail);
    record = new_folded_binary_expression(0x141, 0x2004, record->tail.right, step,
                                          prepend_input_record, tail);
    if (inclusive) {
        constant = new_integer_expression(0x2004, 1);
        record = new_folded_binary_expression(0x142, 0x2004, record->tail.right, constant,
                                              prepend_input_record, tail);
    }
    record = new_folded_binary_expression(0x149, 0x2004, record->tail.right, step,
                                          prepend_input_record, tail);
    if (order) {
        constant = new_integer_expression(0x2004, 1);
        record = new_folded_binary_expression(0x141, 0x2004, record->tail.right, constant,
                                              prepend_input_record, tail);
    }
    collect_region_value_records(record->tail.right->value.reg, &list, &last);
    clear_region_expression_list();
    while (list) {
        next = list->next;
        prepend_input_record(tail, (C2Record *)list);
        list = next;
    }
    record = (C2Expression *)tail->previous;
    operand = record->tail.right;
    constant = new_typed_register_expression(0x120, operand->type, operand->value.reg);
    constant->value.reg->value = NULL;
    replace_input_result(record, operand, constant);
    simplify_region_statement(record);
    record = (C2Expression *)tail->previous;
    if (record->opcode == 0x131 &&
        (record->left->kind == 2 || record->left->kind == 3 || record->left->kind == 7))
        operand = record->left;
    else {
        for (operand = (C2Expression *)region_expression_head->next;
             operand != (C2Expression *)region_expression_tail; operand = operand->next)
            update_global_record_registers(operand, 1);
        unlink_and_release_input_record((C2Record *)tail->previous);
        operand = ((C2Expression *)tail->previous)->tail.right;
    }
    result = copy_input_expression(operand);
    if (result->kind == 7) {
        if (!nonzero) {
            release_small_node(result);
            result = NULL;
            /* Original continues to the result-kind read below after this
             * release. Preserve that genuine NULL-read defect, not a default. */
        } else if (nonzero->integer.value == 1) result->integer.value = 0;
    }
    if (!(branch->flags & 0x80) && result->kind != 7) goto reject;
    if (result->kind == 7) {
#ifdef __linux__
        /* The original comparison below may use incidental Win32 stack words.
         * Host automatic storage cannot reproduce those numerical inputs. */
        if (!immediate_inputs) recon_unsupported("region iteration constant inputs");
#endif
        if (result->integer.low * step_value == bound_value + order - first_value) goto done;
    } else if (step_value == 1) goto done;
    if (condition != 2) goto done;
reject:
    release_small_node(result);
    result = NULL;
done:
    release_region_expression_list();
    return result;
}

int __fastcall region_candidate_pass_3d4a4(C2FlowRegion *region, C2Function *function)
{
    C2FlowBlockLink *exit;
    C2FlowEdge *edge;
    C2BasicBlock *block, *assignment_block;
    C2Expression *branch, *comparison, *variable, *bound, *record, *assignment, *update;
    C2Expression *step, *first, *temporary, *trip;
    C2StatementLink *use;
    unsigned count;
    int valid, inclusive, order;
    unsigned char condition;

    valid = 1;
    trip = NULL;
    comparison = NULL;
    branch = NULL;
    count = 0;
    for (exit = region->exits; exit; exit = exit->next) {
        if (exit->block != region->last) goto finish;
        ++count;
    }
    if (count != 1) goto finish;
    count = 0;
    for (edge = region->header->incoming; edge; edge = edge->next_incoming) {
        if (edge->source != region->rewrite_entry && edge->source != region->last) goto finish;
        ++count;
    }
    if (count != 2) goto finish;
    branch = previous_global_code_record((C2Expression *)region->last->last_record);
    if (branch->kind != 16) goto finish;
    count = 0;
    for (use = branch->detail.statement_links; use; use = use->next) ++count;
    if (count != 1) goto finish;
    comparison = branch->detail.statement_links->record;
    if (comparison->opcode != 0x151 || !((1U << (comparison->type >> 12)) & 0xe)) goto finish;
    if (!global_operand_is_restricted(comparison->left) ||
        !region_operand_is_invariant(comparison->left->next)) {
        if (!global_operand_is_restricted(comparison->left->next) ||
            !region_operand_is_invariant(comparison->left)) goto finish;
        swap_global_operands(comparison);
        /* Original48d250 swaps ordered comparison operands; this is distinct
         * from the existing comparison-negation column at48d1a8. */
        condition = branch->convention;
        if (condition > 10) { __asm { int 3 } }
        branch->convention = condition <= 2 ? condition :
            (condition & 1) ? condition + 1 : condition - 1;
    }
    variable = comparison->left;
    condition = branch->convention;
    bound = copy_input_expression(variable->next);
    if (!region_operand_is_invariant(bound) || !((1U << (bound->type >> 12)) & 0xe)) goto finish;
    assignment = NULL;
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next)
            if (record->opcode == 0x131 && !compare_input_operands(record->tail.right, variable)) {
                if (assignment) goto finish;
                assignment = record;
            }
    if (!assignment) goto finish;
    assignment_block = find_input_record_block(assignment);
    if (assignment_block->block_scope != region) {
        c2_report_internal("E:\\utc\\src\\\\P2\\globlopt.c", 0xf6c);
        goto finish;
    }
    if (!optimizer_dense_contains(region->last->dominators, (int)(short)assignment_block->number84)) goto finish;
    update = assignment->previous;
    if (update->opcode != 0x141 && update->opcode != 0x142) goto finish;
    if (!global_operand_is_restricted(update->left) ||
        !region_operand_is_invariant(update->left->next)) {
        if (update->opcode != 0x141 || !region_operand_is_invariant(update->left) ||
            !global_operand_is_restricted(update->left->next)) goto finish;
        swap_global_operands(update);
    }
    order = compare_region_iteration_order(comparison, update);
    if (compare_input_operands(update->tail.right, assignment->left) ||
        compare_input_operands(assignment->tail.right, update->left)) goto finish;
    step = copy_input_expression(update->left->next);
    if (step->kind != 7) goto finish;
    if (update->opcode == 0x142) step->integer.value = 0 - step->integer.value;
    first = find_region_initial_value(variable, region);
    if (!first || !((1U << (first->type >> 12)) & 0xe)) goto finish;
    inclusive = 0;
    switch (condition) {
    case 2:
        inclusive = 1;
        if ((__int64)step->integer.value < 0) goto reverse;
        break;
    case 3:
        inclusive = 1;
        /* fall through */
    case 5:
        if ((__int64)step->integer.value <= 0) goto finish;
        break;
    case 4:
        inclusive = 1;
        /* fall through */
    case 6:
        if ((__int64)step->integer.value >= 0) goto finish;
reverse:
        step->integer.value = 0 - step->integer.value;
        temporary = first; first = bound; bound = temporary;
        break;
    default:
        goto finish;
    }
    if (!register_types_match(first->type, bound->type) ||
        (first->type & 0xfff) > 4 || (bound->type & 0xfff) > 4 ||
        (step->type & 0xfff) > 4) goto finish;
    trip = compute_region_iteration_count(region, first, bound, step, inclusive, order, branch);
    if (trip && trip->kind == 7 && !trip->integer.value) {
        release_small_node(trip);
        trip = NULL;
        valid = 0;
    }
finish:
    if (!trip) region->iteration_count = NULL;
    else {
        region->iteration_count = retain_input_result(trip);
        if ((variable->type & 0xfff) < 4)
            add_register_to_set(region_motion_sets[5], variable->value.reg->number);
    }
    if (valid) {
        region->iteration_comparison = comparison;
        return valid;
    }
    unlink_and_release_input_record((C2Record *)comparison);
    remove_control_record(branch);
    remove_global_successor(region->last, region->header, function);
    region->exits = NULL;
    return valid;
}

C2OptimizerRecordLink *region_candidates;          /* 49743c */
C2OptimizerRecordLink *deferred_region_candidates; /* 497440 */
/* These independent DWORDs have only reset writers in the recovered prefix.
 * Their later list/index roles are deliberately not inferred from zero/-1. */
C2OptimizerRecordLink *seen_region_candidates; /* 497444, cleared by the actual begin pass. */
unsigned region_candidate_word_497448;
C2FunctionRegister *region_pending_register_tail; /* 49744c; same chain tail. */
C2FunctionRegister *region_pending_registers; /* 497450; real -1-terminated global_next chain. */
void __cdecl begin_region_candidate_work(void)
{
    region_candidates = NULL;
    deferred_region_candidates = NULL;
    seen_region_candidates = NULL;
    region_candidate_word_497448 = 0;
    region_pending_registers = (C2FunctionRegister *)-1;
    region_pending_register_tail = (C2FunctionRegister *)-1;
}
C2OptimizerRecordLink *__fastcall collect_region_candidates(C2FlowRegion *region, unsigned mode);
C2OptimizerRecordLink *__fastcall pop_region_candidate_link(C2OptimizerRecordLink *head,
    C2Expression **record)
{
    C2OptimizerRecordLink *next;
    if (!head) {
        *record = NULL;
        return NULL;
    }
    next = head->next;
    *record = head->record;
    return next;
}
C2Expression *__cdecl pop_region_candidate(void)
{
    C2Expression *record;
    region_candidates = pop_region_candidate_link(region_candidates, &record);
    return record;
}
C2Expression *__cdecl pop_deferred_region_candidate(void)
{
    C2Expression *record;
    deferred_region_candidates = pop_region_candidate_link(deferred_region_candidates, &record);
    return record;
}
int __fastcall count_region_record_uses(C2OptimizerRecordLink *uses)
{
    unsigned count = 0;
    for (; uses; uses = uses->next) ++count;
    return (int)count;
}
int __fastcall region_operand_overlaps_motion_inputs(C2Expression *operand)
{
    C2FunctionRegister *reg, *variant;
    if (!((1U << (operand->kind & 31)) & 6)) return 0;
    reg = operand->value.reg;
    for (variant = reg->canonical; variant; variant = variant->variant_next) {
        if (variant != reg &&
            (reg->displacement >= (long)((unsigned)variant->displacement + variant->value_bytes) ||
             variant->displacement >= (long)((unsigned)reg->displacement + reg->value_bytes))) continue;
        if (optimizer_set_contains(global_motion_inputs, variant->number)) return 1;
    }
    return 0;
}
void __fastcall retarget_region_operand(C2Expression *operand, unsigned short type)
{
    C2FunctionRegister *previous = NULL, *current;
    unsigned old_number, new_number;
    int invariant = 0;
    if ((1U << (operand->kind & 31)) & 6) {
        previous = operand->value.reg;
        invariant = (previous->rewrite_flags >> 1) & 1;
    }
    retarget_global_operand(operand, type);
    if (invariant && ((1U << (operand->kind & 31)) & 6))
        operand->value.reg->rewrite_flags |= 2;
    if (previous && previous != operand->value.reg) {
        current = operand->value.reg;
        old_number = previous->number;
        new_number = current->number;
        if (optimizer_set_contains(region_motion_sets[0], old_number))
            add_register_to_set(region_motion_sets[0], new_number);
        if (optimizer_set_contains(global_motion_registers, old_number))
            add_register_to_set(global_motion_registers, new_number);
        if (optimizer_set_contains(global_region_definitions, old_number))
            add_register_to_set(global_region_definitions, new_number);
        if (optimizer_set_contains(global_motion_outputs, old_number))
            add_register_to_set(global_motion_outputs, new_number);
        if (optimizer_set_contains(global_motion_inputs, old_number))
            add_register_to_set(global_motion_inputs, new_number);
        if (optimizer_set_contains(region_entry_uses, old_number))
            add_register_to_set(region_entry_uses, new_number);
    }
}
C2Expression *__fastcall fold_region_binary_expression(C2Expression *left,
    C2Expression *right, unsigned opcode)
{
    C2Record *tail;
    C2Expression *first, *second, *record, *list, *last, *next, *operand;
    C2FunctionRegister *reg, *view;
    unsigned short type;

    clear_region_expression_list();
    tail = region_expression_tail;
    type = left->type;
    first = copy_input_expression(left);
    second = copy_input_expression(right);
    retarget_region_operand(second, type);
    record = new_folded_binary_expression(opcode, type, first, second,
                                          prepend_input_record, tail);
    collect_region_value_records(record->tail.right->value.reg, &list, &last);
    clear_region_expression_list();
    while (list) {
        next = list->next;
        prepend_input_record(tail, (C2Record *)list);
        list = next;
    }
    record = (C2Expression *)tail->previous;
    for (operand = record->left; operand; operand = operand->next)
        retarget_region_operand(operand, type);
    reg = record->tail.right->value.reg;
    if (!reg->canonical->variant_next) reg->value = NULL;
    else bind_input_temporary(reg, NULL);
    simplify_region_statement(record);
    record = (C2Expression *)tail->previous;
    if (record->opcode == 0x142 && record->left->kind == 7 &&
        record->left->next->kind == 3) {
        reg = record->left->next->value.reg;
        if (reg->displacement >= (long)record->left->integer.low) {
            view = new_address_register(reg->canonical, 0x5000, 0,
                (long)((unsigned)reg->displacement - record->left->integer.low));
            remove_input_operand(record, record->left);
            record->opcode = 0x137;
            record->left->value.reg = view;
        }
    }
    return record;
}
C2Expression *__fastcall find_region_independent_value(C2Expression *first,
    C2Expression *second);
C2Expression *__fastcall reduce_region_difference(C2Expression *value,
    C2Expression *first, C2Expression *difference);
int __fastcall region_value_depends_on(C2Expression *value, C2Expression *input);
C2Expression *__fastcall new_region_iteration(C2Expression *value, unsigned short type,
    C2Expression *step, unsigned opcode, C2FunctionRegister *source,
    unsigned mode, C2FlowRegion *region, C2Function *function);
C2Expression *__fastcall combine_region_iterations(C2Expression *difference,
    C2Expression *first, C2Expression *second, C2Expression *step,
    unsigned opcode, C2FlowRegion *region, C2Function *function)
{
    C2Expression *value, *record;
    C2FunctionRegister *reg = NULL;
    unsigned short type;
    if ((difference->opcode == 0x131 &&
         ((1U << (difference->left->kind & 31)) & 0x18)) ||
        (difference->opcode == 0x142 &&
         ((1U << (difference->left->kind & 31)) & 0x18) &&
         ((1U << (difference->left->next->kind & 31)) & 0x18))) {
        value = find_region_independent_value(first, second);
        if (value) {
            value = reduce_region_difference(value, first, difference->left);
            reg = value->value.reg;
            type = value->type;
        }
    } else if (difference->opcode == 0x137 &&
               ((1U << (difference->left->kind & 31)) & 0x18) &&
               region_value_depends_on(second, first)) {
        value = first;
        reg = first->value.reg;
        type = first->type;
    }
    if (!reg) return NULL;
    if (!reg->global_definitions)
        return new_region_iteration(value, type, step, opcode, first->value.reg,
                                    1, region, function);
    for (record = (C2Expression *)region->rewrite_entry->records->next;
         record != (C2Expression *)region->rewrite_entry->last_record;
         record = record->next)
        if (record->opcode == 0x131 && !compare_input_operands(value, record->tail.right))
            return record;
    return NULL;
}
C2Expression *__fastcall constant_region_difference(C2Expression *record)
{
    C2Expression *value, *result = NULL;
    C2FunctionRegister *reg;
    if (record->opcode == 0x131) {
        if (region_operand_is_invariant(record->left)) return record->left;
    } else if (record->opcode == 0x142 &&
               region_operand_is_invariant(record->left) &&
               region_operand_is_invariant(record->left->next)) {
        reg = record->tail.right->value.reg;
        value = new_global_value_expression(0x142, record->type, record->left,
                                            record->left->next);
        release_function_register(reg);
        result = replace_input_result(record, record->tail.right, value);
        unlink_input_record((C2Record *)record);
        prepend_input_record(region_motion_anchor, (C2Record *)record);
        reg = result->value.reg;
        reg->rewrite_flags |= 6;
        add_register_to_set(region_motion_sets[0], reg->number);
        add_register_to_set(global_region_definitions, reg->number);
    }
    return result;
}
int __fastcall global_range_overlaps_operand(C2Expression *first,
    C2Expression *last, C2Expression *operand)
{
    C2Expression *end = last->next, *input;
    for (; first != end; first = first->next) {
        if (!(first->flags & 1)) continue;
        for (input = first->left; input; input = input->next)
            if (input_operands_may_alias(input, operand)) return 1;
        for (input = first->tail.right; input; input = input->next)
            if (input_operands_may_alias(input, operand)) return 1;
    }
    return 0;
}
int __fastcall region_substitution_conflicts(C2Expression *operand,
    C2Expression *first, C2Expression *second)
{
    C2Expression *swap;
    if (!compare_region_iteration_order(first, second)) {
        swap = first;
        first = second;
        second = swap;
    }
    first = first->next->next;
    if (first == second) return 0;
    return global_range_overlaps_operand(first, second->previous, operand) != 0;
}
C2Expression *__fastcall pop_region_register_use(C2FunctionRegister *reg)
{
    C2OptimizerRecordLink *link = reg->global_uses;
    C2Expression *record;
    if (!link) {
        reg->global_uses = NULL;
        return NULL;
    }
    record = link->record;
    reg->global_uses = link->next;
    return record;
}
C2Expression *__fastcall pop_region_register_definition(C2FunctionRegister *reg)
{
    C2OptimizerRecordLink *link = reg->global_definitions;
    C2Expression *record;
    if (!link) {
        reg->global_definitions = NULL;
        return NULL;
    }
    record = link->record;
    reg->global_definitions = link->next;
    return record;
}
void __fastcall substitute_region_register(C2Expression *operand,
    C2Expression *replacement, C2Expression *difference, int *uses)
{
    C2FunctionRegister *reg = operand->value.reg, *new_reg = replacement->value.reg;
    C2Expression *record, *input, *value, *offset, *operation;
    unsigned short type;
    if (reg == new_reg) return;
    while ((record = pop_region_register_use(reg)) != NULL) {
        if (record->precision_flags == 4) {
            unlink_and_release_input_record((C2Record *)record);
            continue;
        }
        if (difference->kind == 7 && !difference->integer.value) {
            while ((input = find_register_input(record, reg)) != NULL) {
                type = input->type;
                value = copy_input_expression(replacement);
                retarget_region_operand(value, type);
                attach_input_expression(record, input, value);
                update_global_record_registers(record, 0);
                add_region_register_use(new_reg, record);
                ++*uses;
            }
        } else {
            while ((input = find_register_input(record, reg)) != NULL) {
                type = input->type;
                value = copy_input_expression(replacement);
                retarget_region_operand(value, type);
                offset = copy_input_expression(difference);
                retarget_region_operand(offset, type);
                operation = new_region_binary_expression(0x141, type, value, offset,
                                                          prepend_input_record, (C2Record *)record);
                attach_input_expression(record, input, operation->tail.right);
                update_global_record_registers(record, 0);
                add_region_register_use(new_reg, operation);
                ++*uses;
            }
        }
    }
    unlink_and_release_input_record((C2Record *)pop_region_register_definition(reg));
}
C2OptimizerRecordLink *__fastcall new_region_candidate_link(C2OptimizerRecordLink *head,
    C2Expression *record)
{
    C2OptimizerRecordLink *link = c2_pool_allocate(3, sizeof(*link));
    link->next = head;
    link->record = record;
    return link;
}
void __fastcall defer_region_candidate(C2Expression *record)
{
    deferred_region_candidates = new_region_candidate_link(deferred_region_candidates, record);
}
void __fastcall append_region_candidate(C2Expression *record)
{
    region_candidates = new_region_candidate_link(region_candidates, record);
}

int __fastcall region_operand_is_narrow_candidate(C2Expression *operand);
int __fastcall region_definitions_are_nonincremental(C2Expression *operand)
{
    C2OptimizerRecordLink *definition;
    for (definition = operand->value.reg->global_definitions; definition; definition = definition->next)
        if (definition->record->precision_flags != 4) return 1;
    return 0;
}
unsigned __fastcall region_operand_can_move(C2Expression *operand)
{
    if (!((1U << (operand->kind & 31)) & 6)) return 0;
    return optimizer_set_contains(region_entry_uses, operand->value.reg->number);
}
C2Expression *__fastcall region_intrinsic_operand(unsigned index, C2Expression *record);
C2Expression *__fastcall find_region_matching_value(C2Expression *operand, C2FlowRegion *region)
{
    C2OptimizerRecordLink *link;
    for (link = region->initial_updates; link; link = link->next)
        if (!compare_input_operands(operand, link->record->left)) return link->record;
    return NULL;
}
C2Expression *__fastcall find_region_index_operand(C2Expression *operand);

C2OptimizerRecordLink *__fastcall collect_region_candidates(C2FlowRegion *region, unsigned mode)
{
    C2BasicBlock *block, *entry;
    C2Expression *record, *prior, *input, *output, *other, *source, *cursor;
    C2OptimizerRecordLink *definition;
    C2GlobalRegisterLink *assignment;
    C2FunctionRegister *reg, *source_reg;
    C2GlobalExpression *expression;
    unsigned number, low, high;

    if (!region_motion_sets[9]->links.first) return NULL;
    switch (mode) {
    case 1:
        for (block = region->header; block != region->last->previous; block = block->previous)
            for (record = (C2Expression *)block->records->next;
                 record != (C2Expression *)block->last_record; record = record->next)
                if (record->precision_flags == 4) record->precision_flags = 0;
        for (number = next_optimizer_symbol(region_motion_sets[9]); number != ~0U;
             number = next_optimizer_symbol(NULL)) {
            reg = lookup_function_register(number);
            for (definition = reg->global_definitions; definition; definition = definition->next) {
                record = definition->record;
                if (record->opcode != 0x131) continue;
                prior = record->previous;
                if ((prior->opcode == 0x141 || prior->opcode == 0x142) &&
                    !compare_input_operands(record->left, prior->tail.right) &&
                    !compare_input_operands(record->tail.right, prior->left)) {
                    record->precision_flags = 4;
                    prior->precision_flags = 4;
                }
            }
        }
        for (block = region->header; block != region->last->previous; block = block->previous)
            for (record = (C2Expression *)block->records->next;
                 record != (C2Expression *)block->last_record; record = record->next) {
                if (record->flags & 0x40) break;
                if (record->opcode != 0x135 && record->opcode != 0x141 && record->opcode != 0x143) continue;
                if (!((1U << (record->type >> 12)) & 0xe)) continue;
                input = record->left;
                if (record->opcode == 0x135) {
                    if ((record->type & 0xfff) == 4 && global_operand_is_restricted(input) &&
                        region_operand_is_narrow_candidate(input)) {
                        append_region_candidate(record);
                        record->control_flags = 6;
                    }
                } else if (record->opcode == 0x141) {
                    if (!global_operand_is_restricted(input) || !region_operand_is_invariant(input->next)) {
                        if (!region_operand_is_invariant(input) || !global_operand_is_restricted(input->next))
                            continue;
                        swap_global_operands(record);
                    }
                    if (record->precision_flags != 4 && !region_definitions_are_nonincremental(record->left) &&
                        region_operand_can_move(record->left)) {
                        append_region_candidate(record);
                        record->flags |= 0x40;
                    }
                } else if (record->opcode == 0x143) {
                    if (global_operand_is_restricted(input) && region_operand_is_invariant(input->next)) {
                        if (optimizer_set_contains(global_motion_outputs, input->value.reg->number)) continue;
                    } else {
                        if (!region_operand_is_invariant(input) || !global_operand_is_restricted(input->next) ||
                            optimizer_set_contains(global_motion_outputs, input->next->value.reg->number)) continue;
                        swap_global_operands(record);
                    }
                    append_region_candidate(record);
                    record->flags |= 0x40;
                }
            }
        break;
    case 2:
        for (block = region->header; block != region->last->previous; block = block->previous)
            for (record = (C2Expression *)block->records->next;
                 record != (C2Expression *)block->last_record; record = record->next)
                if (record->opcode == 0x151 && ((1U << (record->type >> 12)) & 0xe) &&
                    ((global_operand_is_restricted(record->left) &&
                      region_operand_is_invariant(record->left->next)) ||
                     (region_operand_is_invariant(record->left) &&
                      global_operand_is_restricted(record->left->next))))
                    append_region_candidate(record);
        break;
    case 3:
        for (block = region->header; block != region->last->previous; block = block->previous)
            for (record = (C2Expression *)block->records->next;
                 record != (C2Expression *)block->last_record; record = record->next) {
                if (record->opcode == 0x131) {
                    input = record->left;
                    output = record->tail.right;
                    if (output->kind != 6 || !global_operand_is_restricted(output->aggregate_value) ||
                        input_has_side_effects(record)) continue;
                    if (!region_operand_is_invariant(input)) {
                        prior = record->previous;
                        if (prior->opcode == 0x133 && prior->left->kind == 6 &&
                            !compare_input_operands(prior->tail.right, input) &&
                            global_operand_is_restricted(prior->left->aggregate_value) &&
                            !input_has_side_effects(prior)) source = prior->left;
                        else {
                            if ((record->type & 0xf000) != 0x5000 || input->kind != 6 ||
                                !global_operand_is_restricted(input->aggregate_value) ||
                                input_has_side_effects(input)) continue;
                            source = input;
                        }
                        if (input_operands_may_alias(output, source)) continue;
                    }
                    append_region_candidate(record);
                } else if (record->kind == 17) {
                    if (record->detail.intrinsic == 0xad) {
                        if (global_operand_is_restricted(region_intrinsic_operand(1, record)) &&
                            region_operand_is_invariant(region_intrinsic_operand(2, record)) &&
                            region_operand_is_invariant(region_intrinsic_operand(3, record)))
                            append_region_candidate(record);
                    } else if (record->detail.intrinsic == 0xac) {
                        if (!global_operand_is_restricted(region_intrinsic_operand(1, record)) ||
                            !global_operand_is_restricted(region_intrinsic_operand(2, record)) ||
                            !region_operand_is_invariant(region_intrinsic_operand(3, record))) continue;
                        input = find_region_index_operand(record->left);
                        output = find_region_index_operand(record->tail.right);
                        if (!input_operands_may_alias(input, output)) append_region_candidate(record);
                    }
                }
            }
        break;
    case 4:
        entry = region->rewrite_entry;
        prior = (C2Expression *)entry->records->next;
        for (number = next_optimizer_symbol(region_motion_sets[9]); number != ~0U;
             number = next_optimizer_symbol(NULL)) {
            reg = lookup_function_register(number);
            if (!optimizer_set_contains(entry->rewrite_mode1[2], number)) continue;
            for (assignment = reg->global_assignments; assignment; assignment = assignment->next) {
                expression = assignment->reg->global_expression;
                if (!optimizer_set_contains(entry->global_definitions, assignment->reg->number)) continue;
                other = expression->operands->next;
                if (((1U << (other->kind & 31)) & 6) && other->value.reg->value) continue;
                record = new_binary_expression(0x131, expression->type, expression->operands,
                    other, prepend_input_record, (C2Record *)prior);
                record->precision_flags = 9;
                break;
            }
        }
        for (number = next_optimizer_symbol(region_motion_sets[9]); number != ~0U;
             number = next_optimizer_symbol(NULL)) {
            reg = lookup_function_register(number);
            for (definition = reg->global_definitions; definition; definition = definition->next) {
                record = definition->record;
                if (record->opcode != 0x131) continue;
                prior = record->previous;
                if ((prior->opcode == 0x141 || prior->opcode == 0x142) &&
                    !compare_input_operands(record->left, prior->tail.right) &&
                    !compare_input_operands(record->tail.right, prior->left)) {
                    record->precision_flags = 4;
                    prior->precision_flags = 4;
                }
            }
        }
        for (record = (C2Expression *)entry->records->next;
             record != (C2Expression *)entry->last_record; record = record->next)
            if (record->opcode == 0x131 && record->precision_flags != 1 &&
                global_operand_is_restricted(record->tail.right)) {
                definition = record->tail.right->value.reg->global_definitions;
                if (definition && !definition->next && definition->record->precision_flags == 4)
                    append_region_candidate(record);
            }
        break;
    case 5:
        entry = region->rewrite_entry;
        for (record = (C2Expression *)entry->records->next;
             record != (C2Expression *)entry->last_record; record = record->next) {
            if (record->opcode != 0x131 || !global_operand_is_restricted(record->left) ||
                !find_region_matching_value(record->tail.right, region)) continue;
            reg = record->left->value.reg;
            definition = reg->global_definitions;
            if (!definition || definition->next || definition->record->precision_flags != 4 ||
                !compare_region_iteration_order(record, definition->record)) continue;
            append_region_candidate(record);
            for (cursor = record->next; cursor != region->iteration_comparison->next; cursor = cursor->next)
                if ((cursor->flags & 1) && find_register_input(cursor, reg)) {
                    pop_region_candidate();
                    break;
                }
        }
        break;
    case 6:
        entry = region->rewrite_entry;
        for (record = (C2Expression *)entry->records->next;
             record != (C2Expression *)entry->last_record; record = record->next) {
            if (record->opcode != 0x131 || !global_operand_is_restricted(record->tail.right)) continue;
            reg = record->tail.right->value.reg;
            expression = reg->global_expression;
            if (!(reg->rewrite_flags & 1) || !reg->global_uses || !expression) continue;
            input = expression->operands;
            source_reg = input->value.reg;
            other = input->next;
            if (!region_operand_can_move(input) || !source_reg->global_uses) continue;
            if (expression->opcode == 0x14d) append_region_candidate(record);
            else if (expression->opcode == 0x14e) {
                if (other->kind == 7) append_region_candidate(record);
            } else if (expression->opcode == 0x14f && other->kind == 7) {
                low = other->integer.low;
                high = other->integer.high;
                if (!(((low - 1) & low) | ((high - (low == 0)) & high))) append_region_candidate(record);
            }
        }
        break;
    }
    return region_candidates;
}

unsigned __fastcall region_candidate_pass_3d93c(C2FlowRegion *region, C2Function *function)
{
    C2Expression *first, *second, *definition, *first_update, *second_update;
    C2Expression *first_step, *second_step, *difference, *combined, *offset_first, *offset_second;
    C2Expression *record, *next;
    C2FunctionRegister *reg;
    C2BasicBlock *first_block, *second_block;
    unsigned changed;
    int first_uses, second_uses, spare_uses;
    int first_multiple, second_multiple, first_unsigned, second_unsigned, reverse;
    unsigned first_opcode, second_opcode;

    spare_uses = 0;
    changed = 0;
    begin_region_candidate_work();
    collect_region_candidates(region, 4);
    initialize_region_expression_list();
    for (first = pop_region_candidate(); first; first = pop_region_candidate()) {
        reg = first->tail.right->value.reg;
        definition = reg->global_definitions->record;
        first_update = definition->previous;
        first_opcode = first_update->opcode;
        first_unsigned = !(definition->left->location.expression.comparison & 0x10);
        first_step = first_update->left->next;
        first_block = find_input_record_block(first_update);
        first_uses = count_region_record_uses(reg->global_uses);
        first_multiple = region_operand_overlaps_motion_inputs(first->tail.right);
        for (second = pop_region_candidate(); second; second = pop_region_candidate()) {
            reg = second->tail.right->value.reg;
            definition = reg->global_definitions->record;
            second_update = definition->previous;
            second_opcode = second_update->opcode;
            second_unsigned = !(definition->left->location.expression.comparison & 0x10);
            second_step = second_update->left->next;
            second_block = find_input_record_block(second_update);
            second_multiple = region_operand_overlaps_motion_inputs(second->tail.right);
            if (!register_types_match(first->type, second->type) ||
                compare_input_operands(first_step, second_step) ||
                first_opcode != second_opcode || first_block != second_block ||
                (first_multiple && second_multiple) || (first_unsigned && second_unsigned))
                goto defer_second;
            second_uses = count_region_record_uses(reg->global_uses);
            reverse = second_uses < first_uses || first_multiple || first_unsigned;
            if (reverse) {
                difference = fold_region_binary_expression(second->left, first->left, 0x142);
                combined = combine_region_iterations(difference, second->tail.right,
                    first->tail.right, second_step, second_opcode, region, function);
                if (!combined) {
                    difference = fold_region_binary_expression(second->left, first->left, 0x142);
                    difference = constant_region_difference(difference);
                    if (!difference || second_multiple || second_unsigned ||
                        region_substitution_conflicts(second->tail.right, first_update, second_update))
                        goto defer_second;
                    substitute_region_register(second->tail.right, first->tail.right,
                                               difference, &first_uses);
                    changed = 1;
                    continue;
                }
                if (first_multiple || second_multiple || first_unsigned || second_unsigned) break;
            } else {
                difference = fold_region_binary_expression(first->left, second->left, 0x142);
                combined = combine_region_iterations(difference, first->tail.right,
                    second->tail.right, first_step, first_opcode, region, function);
                if (!combined) {
                    difference = fold_region_binary_expression(first->left, second->left, 0x142);
                    difference = constant_region_difference(difference);
                    if (!difference || region_substitution_conflicts(first->tail.right,
                                                                     first_update, second_update))
                        goto defer_second;
                    substitute_region_register(first->tail.right, second->tail.right,
                                               difference, &second_uses);
                    changed = 1;
                    defer_region_candidate(second);
                    while ((record = pop_region_candidate()) != NULL) defer_region_candidate(record);
                    break;
                }
                if (second_multiple || second_unsigned) break;
            }
            difference = fold_region_binary_expression(first->left, combined->left, 0x142);
            offset_first = copy_input_expression(difference);
            difference = fold_region_binary_expression(second->left, combined->left, 0x142);
            offset_second = copy_input_expression(difference);
            if ((offset_first->opcode == 0x131 || offset_first->opcode == 0x137) &&
                (offset_second->opcode == 0x131 || offset_second->opcode == 0x137)) {
                if (offset_first->opcode == 0x131) {
                    substitute_region_register(first->tail.right, combined->tail.right,
                                               offset_first->left, &spare_uses);
                    changed = 1;
                }
                if (offset_second->opcode == 0x131) {
                    substitute_region_register(second->tail.right, combined->tail.right,
                                               offset_second->left, &spare_uses);
                    changed = 1;
                }
                while ((record = pop_region_candidate()) != NULL) defer_region_candidate(record);
            }
            release_small_node(offset_first);
            release_small_node(offset_second);
            break;
defer_second:
            defer_region_candidate(second);
        }
        while ((record = pop_deferred_region_candidate()) != NULL) append_region_candidate(record);
    }
    release_region_expression_list();
    for (record = (C2Expression *)region->rewrite_entry->records->next;
         record != (C2Expression *)region->rewrite_entry->last_record; record = next) {
        next = record->next;
        if (record->precision_flags == 9) unlink_and_release_input_record((C2Record *)record);
    }
    return changed;
}

C2Expression *__fastcall next_final_marker(C2Expression *record);
void __fastcall refresh_region_candidate_sources(C2FlowRegion *region, C2Function *function)
{
    C2FlowEdge *edge;
    C2BasicBlock *block;
    C2Expression *record;
    if (region_motion_sets[1]->links.first)
        for (edge = region->header->incoming; edge; edge = edge->next_incoming)
            merge_optimizer_symbol_sets(edge->source->global_definitions, region_motion_sets[1]);
    for (block = region->header; block != region->last->previous; block = block->previous)
        optimize_global_block(block, function);
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next)
            if (record->opcode == 0x131 && record->value.reg &&
                global_operand_is_restricted(record->tail.right))
                add_register_to_set(region_motion_sets[6], record->value.reg->number);
    if (region_motion_sets[1]->links.first)
        for (edge = region->header->incoming; edge; edge = edge->next_incoming)
            subtract_optimizer_symbol_sets(edge->source->global_definitions, region_motion_sets[1]);
}
int __fastcall region_operand_is_volatile(C2Expression *operand)
{
    unsigned short flags;
    if (operand->kind == 7) return 0;
    if (operand->kind == 3 && (operand->value.reg->kind == 7 || operand->value.reg->kind == 8)) return 0;
    if (operand->kind == 2) {
        flags = operand->value.reg->region_motion_flags;
        if ((flags & 2) && !(flags & 4)) return 0;
    }
    return 1;
}
unsigned __fastcall canonical_region_opcode(unsigned opcode)
{
    switch (opcode) {
    case 0x135: return 0x150;
    case 0x141: return 0x14d;
    case 0x142: return 0x14e;
    case 0x143: return 0x14f;
    case 0x14d: return 0x141;
    case 0x14e: return 0x142;
    case 0x150: return 0x135;
    default: return 0x143;
    }
}
C2Expression *__fastcall build_region_operation(unsigned short type, unsigned opcode,
    C2Expression *left, C2Expression *right)
{
    C2Expression *value;
    C2FunctionRegister *reg;
    unsigned number;
    int is_volatile;
    value = new_global_value_expression(canonical_region_opcode(opcode), type, left, right);
    reg = value->value.reg;
    if (reg->rewrite_flags & 3) return value;
    if (region_pending_registers == (C2FunctionRegister *)-1) region_pending_registers = reg;
    else region_pending_register_tail->global_next = reg;
    region_pending_register_tail = reg;
    reg->global_next = (C2FunctionRegister *)-1;
    if (!region_operand_is_invariant(left) || (right && !region_operand_is_invariant(right))) {
        number = reg->number;
        reg->rewrite_flags |= 1;
        add_register_to_set(region_motion_sets[9], number);
        add_register_to_set(global_motion_registers, number);
        reg->flags |= 0x40;
    } else {
        reg->rewrite_flags |= 2;
        is_volatile = region_operand_is_volatile(left) || (right && region_operand_is_volatile(right));
        reg->region_motion_flags = (reg->region_motion_flags & 0xfffb) | (is_volatile << 2);
        add_register_to_set(region_motion_sets[0], reg->number);
    }
    if ((1U << (left->kind & 31)) & 6) merge_global_register_identity(left->value.reg, reg);
    if (right && ((1U << (right->kind & 31)) & 6)) merge_global_register_identity(right->value.reg, reg);
    return value;
}
int __fastcall region_candidate_link_contains(C2OptimizerRecordLink *head, C2Expression *record)
{
    for (; head; head = head->next)
        if (head->record == record) return 1;
    return 0;
}
int __fastcall mark_region_candidate_seen(C2Expression *record)
{
    if (region_candidate_link_contains(seen_region_candidates, record)) return 0;
    seen_region_candidates = new_region_candidate_link(seen_region_candidates, record);
    return 1;
}
void __fastcall remove_region_register_use(C2FunctionRegister *reg, C2Expression *record)
{
    C2OptimizerRecordLink *head = reg->global_uses, *previous = NULL, *link;
    for (link = head; link; link = link->next) {
        if (link->record == record) break;
        previous = link;
    }
    if (link) {
        if (previous) previous->next = link->next;
        else head = link->next;
    }
    reg->global_uses = head;
}
void __fastcall rewrite_region_candidate_uses(C2Expression *operand, C2Expression *record)
{
    C2Expression *child, *previous;
    if (!operand || record->opcode == 0x131 || record->kind != 11 || !record->tail.right ||
        compare_input_operands(operand, record->tail.right) || !mark_region_candidate_seen(record)) return;
    previous = record->previous;
    for (child = record->left; child; child = child->next) {
        if (global_operand_is_restricted(child)) remove_region_register_use(child->value.reg, record);
        rewrite_region_candidate_uses(child, previous);
    }
}
unsigned __fastcall canonical_region_opcode(unsigned opcode);
C2FunctionRegister *__fastcall convert_region_register(C2FunctionRegister *reg,
    unsigned short type, unsigned first, unsigned second);
void __fastcall retarget_region_operand(C2Expression *operand, unsigned short type);
C2Expression *__fastcall materialize_region_operand(unsigned short type,
    C2Expression *operand, C2Record *prior)
{
    C2GlobalExpression *expression;
    C2Expression *left, *right, *record, *result, *input;
    unsigned kind_mask, opcode, attributes;
    unsigned short working_type;
    if (!operand) return NULL;
    kind_mask = 1U << (operand->kind & 31);
    if ((kind_mask & 6) && operand->value.reg->global_expression) {
        expression = operand->value.reg->global_expression;
        opcode = expression->opcode;
        switch (opcode) {
        case 0x13b: case 0x15e: case 0x15f:
            return operand;
        case 0x14d: case 0x14e: case 0x14f: case 0x150:
            opcode = canonical_region_opcode(opcode);
            break;
        default:
            if (region_operand_can_move(operand) && !(operand->value.reg->rewrite_flags & 2)) return operand;
            for (input = expression->operands; input; input = input->next)
                if (((1U << (input->kind & 31)) & 6) && input->value.reg->value) return operand;
            break;
        }
        working_type = register_types_match(type, expression->type) ? type : expression->type;
        right = expression->operands->next;
        left = materialize_region_operand(working_type, expression->operands, prior);
        right = materialize_region_operand(working_type, right, prior);
        if (right) record = new_folded_binary_expression(opcode, working_type, left, right,
                                                         prepend_input_record, prior);
        else record = new_assignment_expression(opcode, working_type, left, prepend_input_record, prior);
        if (working_type != type)
            record = new_assignment_expression(0x135, type, record->tail.right, prepend_input_record, prior);
        return record->tail.right;
    }
    if (kind_mask & 0x60) {
        attributes = operand->immediate;
        expression = operand->value.reg->global_expression;
        opcode = expression->opcode;
        input = expression->operands;
        working_type = register_types_match(type, expression->type) ? type : expression->type;
        left = materialize_region_operand(input->type, input, prior);
        result = new_aggregate_expression(opcode, working_type, NULL, 0, left, NULL, attributes);
        if (working_type != type) {
            record = new_assignment_expression(opcode, type, result, prepend_input_record, prior);
            return record->tail.right;
        }
        return result;
    }
    if (kind_mask & 6) {
        if (type == operand->type || !register_types_match(type, operand->type)) return operand;
        result = copy_input_expression(operand);
        result->value.reg = convert_region_register(result->value.reg, type, 0, 0);
        result->type = type;
        return result;
    }
    if (operand->kind == 7 && type != operand->type && register_types_match(type, operand->type))
        retarget_region_operand(operand, type);
    return operand;
}

C2Expression *__cdecl pop_seen_region_candidate(void)
{
    C2Expression *record;
    seen_region_candidates = pop_region_candidate_link(seen_region_candidates, &record);
    return record;
}
void __cdecl finish_region_candidate_registers(void)
{
    C2Expression *record, *operand;
    while ((record = pop_seen_region_candidate()) != NULL) {
        for (operand = record->left; operand; operand = operand->next)
            if (global_operand_is_restricted(operand)) remove_region_register_use(operand->value.reg, record);
        unlink_and_release_input_record((C2Record *)record);
    }
}

void __fastcall region_candidate_pass_3de8b(C2FlowRegion *region, C2Function *function, unsigned changes)
{
    C2Expression *candidate, *value, *assignment, *marker, *target;
    C2Expression *first, *second, *left, *right, *transformed, *other, *carried_left;
    C2Expression *record, *insertion, *definition_record;
    C2FunctionRegister *reg, *old_output, *pending;
    C2GlobalExpression *expression;
    C2OptimizerRecordLink *definition;
    unsigned pass, opcode, inner_opcode, number;
    unsigned short line, type;
    int direct;

    /* The original reuses the incoming changes DWORD as a carried operand
     * pointer in its diagnosed-error continuations. Preserve that defined
     * value; the separate right-operand local remains genuinely unwritten
     * until a normal selection supplies it. */
    carried_left = (C2Expression *)changes;
    set_region_motion_anchor(region);
    marker = next_final_marker((C2Expression *)region->entry->records);
    line = marker->location.line;
    if (changes) {
        refresh_region_candidate_sources(region, function);
        prepare_region_motion_assignments(region, 0);
        region_candidate_pass_3d127(region);
        region_candidate_pass_3ba96(region);
    }
    for (pass = 0; pass < 32; ++pass) {
        begin_region_candidate_work();
        if (!collect_region_candidates(region, 1)) break;
        while ((candidate = pop_region_candidate()) != NULL) {
            if (candidate->location.line) {
                record_line_delta = candidate->location.line;
                diagnostic_position = function->source_line_base + candidate->location.line;
            }
            type = candidate->type;
            value = build_region_operation(type, candidate->opcode, candidate->left, candidate->left->next);
            reg = value->value.reg;
            old_output = candidate->tail.right->value.reg;
            assignment = new_binary_expression(0x131, type, candidate->tail.right, value,
                                                insert_input_record, (C2Record *)candidate);
            update_global_record_registers(assignment, 0);
            add_region_register_use(reg, assignment);
            number = old_output->number;
            if (optimizer_set_contains(global_motion_outputs, number) &&
                !test_and_add_global_register(region_motion_sets[7], number))
                add_register_to_set(region_motion_sets[1], assignment->value.reg->number);
            rewrite_region_candidate_uses(candidate->tail.right, candidate);
        }
        for (pending = region_pending_registers; pending != (C2FunctionRegister *)-1;
             pending = pending->global_next) {
            if (line) {
                record_line_delta = line;
                diagnostic_position = function->source_line_base + line;
            }
            expression = pending->global_expression;
            type = expression->type;
            opcode = canonical_region_opcode(expression->opcode);
            first = expression->operands;
            if (!global_operand_is_restricted(first) && opcode != 0x142 && opcode != 0x135) {
                second = first;
                first = first->next;
            } else second = first->next;
            value = materialize_region_operand(type, first, region_motion_anchor);
            if (opcode == 0x135)
                record = new_assignment_expression(0x135, type, value, prepend_input_record, region_motion_anchor);
            else {
                other = materialize_region_operand(type, second, region_motion_anchor);
                record = new_folded_binary_expression(opcode, type, value, other,
                                                       prepend_input_record, region_motion_anchor);
            }
            target = new_typed_register_expression(0x120, type, pending);
            new_binary_expression(0x131, type, target, record->tail.right,
                                  prepend_input_record, region_motion_anchor);
            inner_opcode = opcode;
            definition = first->kind == 2 ? first->value.reg->global_definitions : NULL;
            for (; definition; definition = definition->next) {
                record = definition->record;
                insertion = record;
                if ((record->opcode == 0x141 || record->opcode == 0x142) &&
                    record->next->opcode == 0x131 &&
                    !compare_input_operands(record->tail.right, record->next->left) &&
                    !compare_input_operands(record->next->tail.right, record->left))
                    insertion = record->next;
                if (insertion->location.line) {
                    record_line_delta = insertion->location.line;
                    diagnostic_position = function->source_line_base + insertion->location.line;
                }
                direct = 0;
                if (record->opcode == 0x131) {
                    left = record->left;
                    if (((1U << (left->kind & 31)) & 6) && left->value.reg->global_expression) {
                        expression = left->value.reg->global_expression;
                        inner_opcode = expression->opcode;
                        switch (inner_opcode) {
                        case 0x14d: case 0x14e:
                            inner_opcode = canonical_region_opcode(inner_opcode);
                            /* fall through */
                        case 0x141: case 0x142:
                            left = expression->operands;
                            if (!global_operand_is_restricted(left) && inner_opcode != 0x142) {
                                right = left;
                                left = left->next;
                            } else right = left->next;
                            break;
                        case 0x14f: case 0x150:
                            transformed = build_region_operation(type, opcode, left, second);
                            direct = 1;
                            break;
                        default:
                            /* The original diagnoses this unexpected operation
                             * and falls into the carried-operand continuation. */
                            c2_report_internal("E:\\utc\\src\\\\P2\\globlopt.c", 0xbd1);
                            left = carried_left;
                            break;
                        }
                    } else if (((1U << (left->kind & 31)) & 6) && left->value.reg->value) {
                        definition_record = left->value.reg->value;
                        left = definition_record->left;
                        right = left->next;
                        inner_opcode = definition_record->opcode;
                    } else {
                        transformed = build_region_operation(type, opcode, left, second);
                        direct = 1;
                    }
                } else if (record->opcode == 0x141 || record->opcode == 0x142) {
                    expression = record->tail.right->value.reg->global_expression;
                    inner_opcode = expression->opcode;
                    left = expression->operands;
                    if (!global_operand_is_restricted(left)) {
                        right = left;
                        left = left->next;
                    } else right = left->next;
                } else {
                    /* As above, no invented operands for the original error
                     * continuation. The genuine reporter remains pending. */
                    c2_report_internal("E:\\utc\\src\\\\P2\\globlopt.c", 0xbfa);
                    left = carried_left;
                }
                carried_left = left;
                if (direct) {
                    assignment = new_binary_expression(0x131, type, target, transformed,
                                                        insert_input_record, (C2Record *)insertion);
                    finish_global_assignment(assignment, 1);
                } else {
                    transformed = build_region_operation(type, opcode, left, second);
                    other = right;
                    if (opcode == 0x143 || opcode == 0x135)
                        other = build_region_operation(type, opcode, right, second);
                    if (inner_opcode == 0x135)
                        value = new_assignment_expression(0x135, type, transformed,
                                                           insert_input_record, (C2Record *)insertion);
                    else value = new_folded_binary_expression(inner_opcode, type, transformed, other,
                                                               insert_input_record, (C2Record *)insertion);
                    assignment = new_binary_expression(0x131, type, target, value->tail.right,
                                                        insert_input_record, (C2Record *)value);
                }
                add_region_register_definition(pending, assignment);
            }
        }
        finish_region_candidate_registers();
        refresh_region_candidate_sources(region, function);
        prepare_region_motion_assignments(region, 0);
        region_candidate_pass_3d127(region);
        region_candidate_pass_3ba96(region);
    }
    begin_region_candidate_work();
}

extern C2Expression *global_pending_assignment; /* Existing49721c owner. */
void __fastcall simplify_region_record_range(C2Expression *first, C2Expression *last, C2Function *function)
{
    C2Expression *record, *next, *end;
    end = last->next;
    for (record = first; record != end; record = next) {
        next = record->next;
        if (record->location.line) {
            record_line_delta = record->location.line;
            diagnostic_position = function->source_line_base + record->location.line;
        }
        if ((record->flags & 1) && record->precision_flags != 2) simplify_region_statement(record);
    }
    end = last->next;
    for (record = first; record != end; record = next) {
        next = record->next;
        if (record->opcode == 0x131 && ((1U << (record->tail.right->kind & 31)) & 6) &&
            record->tail.right->value.reg->value && !global_assignment_restored(record))
            restore_global_assignment(record);
    }
}
void __fastcall simplify_region_entry_block(C2BasicBlock *block, C2Function *function)
{
    simplify_region_record_range((C2Expression *)block->records,
                                 (C2Expression *)block->last_record, function);
}
void __fastcall region_candidate_pass_3c245(C2FlowRegion *region, C2Function *function)
{
    C2BasicBlock *entry = region->rewrite_entry;
    C2Expression *record;
    global_available_registers = entry->global_available_in;
    for (record = (C2Expression *)entry->records->next;
         record != (C2Expression *)entry->last_record; record = record->next)
        if (record->flags & 1) {
            global_record_needs_rewrite(record);
            global_pending_assignment = NULL;
        }
    simplify_region_entry_block(entry, function);
}

void __fastcall region_candidate_pass_3c33f(C2FlowRegion *region)
{
    C2BasicBlock *entry = region->rewrite_entry;
    C2Expression *record;
    C2OptimizerRecordLink *link;
    for (record = (C2Expression *)entry->records->next;
         record != (C2Expression *)entry->last_record; record = record->next)
        if (record->opcode == 0x131 && global_operand_is_restricted(record->tail.right)) {
            link = c2_pool_allocate(13, sizeof(*link));
            link->record = copy_input_expression(record);
            link->next = region->assignment_copies;
            region->assignment_copies = link;
        }
}

extern int global_removed_register_use; /* Existing48fa5c owner. */
void __fastcall region_candidate_pass_3c3a0(C2FlowRegion *region, C2Function *function)
{
    C2BasicBlock *block;
    C2Expression *record, *next, *end;
    if (!region_motion_sets[9]->links.first) return;
    global_region_limited = 0;
    for (block = region->header; block != region->last->previous; block = block->previous) {
        optimize_global_block(block, function);
        end = (C2Expression *)block->last_record;
        for (record = (C2Expression *)block->records->next; record != end; record = next) {
            next = record->next;
            if (record->opcode >= 0x141 && record->opcode <= 0x142 &&
                global_operand_is_restricted(record->left) &&
                region_operand_is_invariant(record->left->next) &&
                record->next->opcode == 0x131 &&
                !compare_input_operands(record->tail.right, record->next->left) &&
                !compare_input_operands(record->next->tail.right, record->left)) {
                record->precision_flags = 4;
                record->next->precision_flags = 4;
                next = record->next->next;
            } else if (record->precision_flags == 4) record->precision_flags = 0;
        }
    }
    do {
        global_removed_register_use = 0;
        prepare_region_motion_assignments(region, 3);
    } while (global_removed_register_use);
}

C2Expression *__fastcall find_region_address_update(C2FunctionRegister *reg);
C2Expression *__fastcall find_region_initial_constant(C2Expression *operand, C2FlowRegion *region)
{
    C2Expression *record, *value;
    C2BasicBlock *entry;
    if ((1U << (operand->kind & 31)) & 0x180) return operand;
    entry = region->rewrite_entry;
    for (record = (C2Expression *)entry->records->next;
         record != (C2Expression *)entry->last_record; record = record->next) {
        if (record->opcode != 0x131 || compare_input_operands(record->tail.right, operand)) continue;
        value = record->left;
        if ((1U << (value->kind & 31)) & 0x180) operand = value;
        break;
    }
    return operand;
}
unsigned short __fastcall region_copy_intrinsic_type(C2Expression *record, unsigned *flags);
int __fastcall region_copy_conflicts(C2Expression *destination, C2Expression *source,
    C2Expression *record, C2FlowRegion *region);
C2Expression *__fastcall build_region_copy_count(C2FlowRegion *region, C2Function *function,
    unsigned __int64 bytes);
void __fastcall emit_region_memory_copy(C2Expression *destination, unsigned destination_attributes,
    C2Expression *source, unsigned source_attributes, unsigned short type, unsigned char flags,
    C2Expression *count, C2Record *prior);

void __fastcall region_candidate_pass_3c4a5(C2FlowRegion *region, C2Function *function)
{
    C2Expression *candidate, *assignment, *intrinsic, *source_load, *destination, *source;
    C2Expression *destination_address, *source_address, *fill_value, *length_value;
    C2Expression *destination_update, *source_update, *destination_step, *source_step;
    C2Expression *count, *cleanup;
    C2FunctionRegister *destination_reg, *source_reg;
    C2BasicBlock *destination_block, *source_block;
    unsigned destination_opcode, source_opcode, destination_attributes, source_attributes, flags;
    unsigned short destination_type, source_type, copy_type;
    unsigned __int64 bytes, step, magnitude;
    int fill;

    if (!region->iteration_count) return;
    begin_region_candidate_work();
    if (!collect_region_candidates(region, 3)) return;
    while ((candidate = pop_region_candidate()) != NULL) {
        source_address = NULL;
        if (candidate->location.line) {
            record_line_delta = candidate->location.line;
            diagnostic_position = function->source_line_base + candidate->location.line;
        }
        if (!block_reaches_region_exits(find_input_record_block(candidate), region)) continue;
        if (candidate->opcode == 0x131) {
            assignment = candidate;
            intrinsic = NULL;
            flags = 0;
            destination = candidate->tail.right;
            destination_type = copy_type = destination->type;
            destination_address = destination->aggregate_value;
            destination_attributes = destination->immediate;
            bytes = destination_type & 0xfff;
            destination_reg = destination_address->value.reg;
            destination_update = find_region_address_update(destination_reg);
            if (!destination_update) continue;
            destination_opcode = destination_update->opcode;
            destination_step = find_region_initial_constant(destination_update->left->next, region);
            destination_block = find_input_record_block(destination_update);
            if (region_operand_is_invariant(candidate->left)) {
                fill = 1;
                fill_value = candidate->left;
                source_load = NULL;
                source_type = destination_type;
                source_reg = destination_reg;
                source_attributes = destination_attributes;
                source_update = destination_update;
                source_opcode = destination_opcode;
                source_step = destination_step;
                source_block = destination_block;
            } else {
                fill = 0;
                if ((candidate->type & 0xf000) == 0x5000) {
                    source_load = NULL;
                    source = candidate->left;
                } else {
                    source_load = candidate->previous;
                    source = source_load->left;
                }
                source_address = source->aggregate_value;
                source_attributes = source->immediate;
                source_type = source->type;
                source_reg = source_address->value.reg;
                source_update = find_region_address_update(source_reg);
                if (!source_update) continue;
                source_opcode = source_update->opcode;
                source_step = find_region_initial_constant(source_update->left->next, region);
                source_block = find_input_record_block(source_update);
            }
        } else {
            assignment = NULL;
            source_load = NULL;
            intrinsic = candidate;
            flags = 0;
            destination_address = region_intrinsic_operand(1, candidate);
            length_value = find_region_initial_constant(region_intrinsic_operand(3, candidate), region);
            copy_type = region_copy_intrinsic_type(candidate, &flags);
            if (length_value->kind != 7) continue;
            bytes = length_value->integer.value;
            destination_attributes = find_region_index_operand(candidate->tail.right)->value.optimizer_index;
            destination_reg = destination_address->value.reg;
            destination_type = destination_address->type;
            destination_update = find_region_address_update(destination_reg);
            if (!destination_update) continue;
            destination_opcode = destination_update->opcode;
            destination_step = find_region_initial_constant(destination_update->left->next, region);
            destination_block = find_input_record_block(destination_update);
            if (candidate->detail.intrinsic == 0xad) {
                fill = 1;
                fill_value = region_intrinsic_operand(2, candidate);
                source_type = destination_type;
                source_reg = destination_reg;
                source_attributes = destination_attributes;
                source_update = destination_update;
                source_opcode = destination_opcode;
                source_step = destination_step;
                source_block = destination_block;
            } else {
                fill = 0;
                source_address = region_intrinsic_operand(2, candidate);
                source_attributes = find_region_index_operand(candidate->left)->value.optimizer_index;
                source_type = source_address->type;
                source_reg = source_address->value.reg;
                source_update = find_region_address_update(source_reg);
                if (!source_update) continue;
                source_opcode = source_update->opcode;
                source_step = find_region_initial_constant(source_update->left->next, region);
                source_block = find_input_record_block(source_update);
            }
        }
        if (!register_types_match(destination_type, source_type) ||
            compare_input_operands(destination_step, source_step) ||
            destination_opcode != source_opcode || destination_block != source_block ||
            destination_step->kind != 7 || !block_reaches_region_exits(destination_block, region) ||
            !compare_region_iteration_order(candidate, destination_update) ||
            !compare_region_iteration_order(candidate, source_update)) continue;
        destination = assignment ? assignment->tail.right : find_region_index_operand(intrinsic->tail.right);
        if (fill) source = NULL;
        else if (source_load) source = source_load->left;
        else source = assignment ? assignment->left : find_region_index_operand(intrinsic->left);
        if (region_copy_conflicts(destination, source, candidate, region)) continue;
        step = destination_step->integer.value;
        magnitude = (__int64)step < 0 ? 0 - step : step;
        if (bytes != magnitude || destination_opcode != 0x141 || (__int64)step <= 0) continue;
        count = build_region_copy_count(region, function, bytes);
        if (fill)
            emit_region_memory_copy(destination_address, destination_attributes, fill_value, 0,
                                    copy_type, (unsigned char)flags, count, region_motion_anchor);
        else {
            emit_region_memory_copy(destination_address, destination_attributes, source_address,
                                    source_attributes, copy_type, (unsigned char)flags, count, region_motion_anchor);
            if (source_load) {
                remove_region_register_use(source_reg, source_load);
                unlink_and_release_input_record((C2Record *)source_load);
            }
        }
        if (assignment) remove_region_register_use(destination_reg, assignment);
        else {
            cleanup = intrinsic->left->next->next->value.reg->value;
            unlink_and_release_input_record((C2Record *)cleanup);
            cleanup = intrinsic->left->next->value.reg->value;
            if (!fill) remove_region_register_use(source_reg, cleanup);
            unlink_and_release_input_record((C2Record *)cleanup);
            cleanup = intrinsic->left->value.reg->value;
            remove_region_register_use(destination_reg, cleanup);
            unlink_and_release_input_record((C2Record *)cleanup);
        }
        unlink_and_release_input_record((C2Record *)(assignment ? assignment : intrinsic));
    }
}

void __fastcall finish_region_motion_candidates(C2FlowRegion *region, C2Function *function)
{
    unsigned changes;
    if (region->entry->flags & 0x2000000) return;
    region_candidate_pass_3caf9(region);
    region_candidate_pass_3ccb2(region);
    region_candidate_pass_3d127(region);
    changes = region_candidate_pass_3d37b(region);
    region_candidate_pass_3ba96(region);
    if (region_candidate_pass_3d4a4(region, function)) {
        global_substitution_check_enabled = 1;
        changes |= region_candidate_pass_3d93c(region, function);
        region_candidate_pass_3de8b(region, function, changes);
        region_candidate_pass_3c245(region, function);
        region_candidate_pass_3c33f(region);
        region_candidate_pass_3c3a0(region, function);
        region_candidate_pass_3c4a5(region, function);
        region_candidate_pass_3ba96(region);
        region_candidate_pass_3d93c(region, function);
        region_candidate_pass_3ba96(region);
        region_candidate_pass_3c87c(region, function);
        region_candidate_pass_3c3a0(region, function);
        region_candidate_pass_3ba96(region);
        region_candidate_pass_3bbb3(region);
        region_candidate_pass_3ba96(region);
        region_candidate_pass_3e5ed(region, function);
        region_candidate_pass_3ba96(region);
        region_candidate_pass_3e73e(region, function);
        region_candidate_pass_3e91d(region);
        region_candidate_pass_3e9fe(region, function);
        region_candidate_pass_3ea57(region, function);
        region_candidate_pass_3ec06(region, function);
        region_candidate_pass_3ec67(region, function);
        global_substitution_check_enabled = 0;
    }
    region_candidate_pass_3edd3(region);
}

void __fastcall optimize_region_motion(C2FlowRegion *region, C2Function *function)
{
    C2FlowRegion *child;
    C2BasicBlock *block;
    for (child = region->children; child; child = child->next)
        optimize_region_motion(child, function);
    validate_global_region(region);
    if (region->flags & 9) return;
    if ((int)(short)region->last->number84 - (int)(short)region->header->number84 + 1 <=
        global_region_size_limit) {
        coalesce_region_assignments(region);
        prepare_region_motion_assignments(region, 1);
    }
    build_region_motion_sets(region);
    global_region_definitions = region->rewrite_entry->global_definitions;
    set_region_motion_anchor(region);
    global_available_additions_suppressed = 1;
    for (block = region->header; block != region->last->previous; block = block->previous)
        if (!(block->flags & 0x2000000)) move_global_region_block(block, region, function);
    global_available_additions_suppressed = 0;
    if ((int)(short)region->last->number84 - (int)(short)region->header->number84 + 1 <=
        global_region_size_limit)
        finish_region_motion_candidates(region, function);
}

void __fastcall finish_region_register_sets(C2Function *function)
{
    C2Expression *record, *next, *end, *output, *consumer, *operand;
    end = (C2Expression *)function->flow->first->last_record;
    for (record = (C2Expression *)function->flow->last->records->next;
         record != end; record = next) {
        next = record->next;
        if (!(record->flags & 1)) continue;
        if (record->opcode == 0x131) {
            output = record->tail.right;
            if (((1U << (output->kind & 31)) & 6) && output->value.reg->value &&
                global_assignment_restored(record)) continue;
        }
        if (record->opcode == 0x151) {
            consumer = follow_optimizer_result(record, &operand);
            if (consumer->kind == 15) continue;
        } else if (record->kind == 15)
            record = record->left->value.reg->value;
        update_global_record_registers(record, 1);
    }
}

void __fastcall prepare_region_motion(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end;
    C2Expression *record, *output;
    C2GlobalDependency *link;
    C2FunctionRegister *reg;
    C2FlowEdge *edge;

    end = last->previous;
    for (block = first; block != end; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (record->opcode != 0x131 || record->precision_flags != 7) continue;
            output = record->tail.right;
            if (!((1U << (output->kind & 31)) & 6)) continue;
            for (link = output->value.reg->global_dependents; link; link = link->next_dependent) {
                reg = link->dependent;
                if (!reg->number) continue;
                for (edge = first->incoming; edge; edge = edge->next_incoming)
                    remove_optimizer_symbol(edge->source->global_definitions, reg->number);
            }
        }
}

void __fastcall validate_global_region(C2FlowRegion *region)
{
    C2Expression *record;
    if (region->flags & 1) return;
    if (!find_flow_edge(region->last, region->header)) region->flags |= 1;
    record = (C2Expression *)region->rewrite_entry->records;
    while (record && record->kind != 25) {
        if (record->flags & 1) {
            if (record->kind == 16 && !record->detail.data && record->opcode != 0x15c)
                region->flags |= 1;
            return;
        }
        record = record->previous;
    }
}

void __fastcall prepare_region_register_sets(C2Function *function)
{
    C2Expression *record, *operand;
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (!(record->flags & 1)) continue;
        for (operand = record->left; operand; operand = operand->next)
            operand->location.expression.comparison &= 0x7f;
        for (operand = record->tail.right; operand; operand = operand->next)
            operand->location.expression.comparison &= 0x7f;
    }
}

C2BasicBlock *__fastcall find_block_backedge_target(C2BasicBlock *block)
{
    C2FlowEdge *edge;
    C2BasicBlock *target = NULL;
    for (edge = block->outgoing; edge; edge = edge->next_outgoing)
        if ((short)edge->destination->number84 <= (short)block->number84)
            target = edge->destination;
    return target;
}
C2BasicBlock *__fastcall find_global_loop_header(C2BasicBlock *block)
{
    C2BasicBlock *header = find_block_backedge_target(block), *target;
    while (block != header) {
        if (global_block_has_backedge(block)) {
            target = find_block_backedge_target(block);
            if ((short)target->number84 < (short)header->number84) header = target;
        }
        block = block->next;
    }
    return header;
}

void initialize_global_optimizer(void)
{
    unsigned number, bytes, bucket;
    C2OptimizerUse *use, *saved, *current;
    C2FunctionRegister *reg;

    reset_input_pool(14);
    global_expression_count = 0;
    for (bucket = 0; bucket < 101; ++bucket) global_expression_buckets[bucket] = NULL;
    global_expression_cursor = 0;
    global_cached_expression_registers = (C2FunctionRegister *)-1;
    bytes = optimizer_final_index_limit * 4 + 4;
    global_index_registers = c2_pool_allocate(14, bytes);
    global_symbol_registers = c2_pool_allocate(14, bytes);
    global_index_direct_uses = c2_pool_allocate(14, bytes);
    global_index_variant_uses = c2_pool_allocate(14, bytes);
    for (number = next_optimizer_symbol(optimizer_symbol_indices);
         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
        global_symbol_registers[number] = new_register_set(14);
        global_index_registers[number] = new_register_set(14);
    }
    for (number = next_optimizer_symbol(optimizer_value_indices);
         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
        global_index_registers[number] = new_register_set(14);
        global_index_direct_uses[number] = new_register_set(14);
        global_index_variant_uses[number] = new_register_set(14);
        use = get_global_index_uses(number);
        saved = NULL;
        while (use) {
            reg = use->reg;
            current = use;
            if (reg->kind == 12) {
                saved = use;
                current = reg->optimizer_uses;
                if (!current) {
                    use = use->next;
                    continue;
                }
                reg = current->reg;
            }
            add_register_to_set(global_index_direct_uses[number], reg->number);
            for (; reg; reg = reg->variant_next)
                add_register_to_set(global_index_variant_uses[number], reg->number);
            use = current->next;
            if (!use && saved) {
                use = saved->next;
                saved = NULL;
            }
        }
    }
    global_pending_expressions = new_register_set(14);
}

C2RegisterSet *global_initial_registers; /* 48fa4c; captured 48e2c0 owner. */
C2RegisterSet *global_temporary_registers; /* 491034 */
unsigned global_register_state; /* 48fa34: encountered reset DWORD only. */

void __fastcall initialize_global_registers(C2Function *function)
{
    C2RegisterBlock *block;
    C2FunctionRegister *reg;
    C2Expression *record;
    for (block = first_function_register_block; block; block = block->next)
        for (reg = block->records; reg < block->cursor; ++reg)
            if (reg->kind != 16) {
                memset(&reg->dependent_uses, 0,
                    sizeof(*reg) - offsetof(C2FunctionRegister, dependent_uses));
                reg->next_free = NULL;
            }
    global_initial_registers = secondary_register_set;
    global_temporary_registers = new_register_set(13);
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        record->control_flags = 0;
        record->pending13 = 0;
    }
    global_register_state = 0;
}

void __fastcall prepare_global_blocks(C2Function *function)
{
    C2BasicBlock *block, *next;
    for (block = function->flow->last; block; block = next) {
        next = block->previous;
        if (!(block->flags & 0x02000000U)) prepare_global_block(block, function);
    }
}

C2Expression *global_next_record; /* 48fa8c; child may update traversal continuation. */
void __fastcall prepare_global_block(C2BasicBlock *block, C2Function *function)
{
    C2Expression *record = ((C2Expression *)block->records)->next;
    while (record != (C2Expression *)block->last_record) {
        global_next_record = record->next;
        if (record->kind == 11 ||
            (record->opcode == 0x15d && record->left->kind == 7))
            prepare_global_record(record, block, function);
        record = global_next_record;
    }
}

/* Encountered switch-range prefix: only links, signed bounds and target are
 * consumed here. Neither the reserved bytes nor an allocation extent is claimed. */
typedef struct C2GlobalSwitchRange {
    struct C2GlobalSwitchRange *next;
    unsigned char pending04[8];
    long low, high;
    C2Expression *target;
} C2GlobalSwitchRange;
typedef char C2GlobalSwitchLowOffset[offsetof(C2GlobalSwitchRange, low) == 12 ? 1 : -1];
typedef char C2GlobalSwitchTargetOffset[offsetof(C2GlobalSwitchRange, target) == 20 ? 1 : -1];
C2Expression *__fastcall truncate_global_integer(unsigned short type,
    C2Expression *operand)
{
    unsigned __int64 previous;
    C2Real *real;
    union { unsigned bits; int signed_value; } low;

    if ((1U << (type >> 12)) & 0xe) {
        previous = operand->integer.value;
        operand->integer.value = normalize_integer_value(type, previous);
        if (previous != operand->integer.value)
            operand->location.expression.comparison |= 0x40;
    } else {
        real = (C2Real *)c2_pool_allocate(0, 12);
        /* The converter uses the saved original low DWORD, even though the
         * recursive call also truncates and marks the input expression. */
        low.bits = operand->integer.low;
        if ((operand->type & 0xf000) == 0x1000) {
            truncate_global_integer(0x1004, operand);
            c2_real_from_int(&low.signed_value, real);
        } else {
            truncate_global_integer(0x2004, operand);
            c2_real_from_unsigned(&low.bits, real);
        }
        operand = new_real_expression(type, real);
    }
    operand->type = type;
    return operand;
}

void __fastcall optimize_global_assignment(C2Expression *record, C2BasicBlock *block)
{
    C2Expression *first, *second, *variable, *constant, *consumer, *use;
    C2Expression *scan, *previous_value, *added, *input;
    C2BranchTarget *target;
    C2BasicBlock *destination;
    C2GlobalSwitchRange *range;
    C2Record *anchor;
    unsigned __int64 value;
    unsigned short type;

    if (!block->outgoing) return;
    if (record->opcode == 0x151) {
        first = record->left;
        second = first->next;
        if ((((1U << (first->kind & 31)) & 6) && first->value.reg->value) ||
            (((1U << (second->kind & 31)) & 6) && second->value.reg->value)) return;
        consumer = follow_optimizer_result(record, &use);
        if (consumer->kind != 16 ||
            (consumer->convention != 1 && consumer->convention != 2)) return;
        if ((1U << (first->kind & 31)) & 0x180) {
            variable = second;
            constant = first;
            if ((1U << (variable->kind & 31)) & 0x180) return;
        } else {
            variable = first;
            constant = second;
        }
        if (constant->kind != 7 || !((1U << (variable->kind & 31)) & 6) ||
            variable->value.reg->value) return;
        if (consumer->convention == 1) {
            target = ((C2InputSymbol *)consumer->left->value.symbol)->branch_target;
            if (!target->block) target = get_symbol_branch_target(target, 24);
            destination = target->block;
            anchor = (C2Record *)target;
        } else {
            /* The predecessor's statement marker and the branch target both
             * carry their basic-block owner at offset20. Retain the actual
             * marker as the insertion anchor, rather than its block. */
            anchor = block->previous->records;
            destination = anchor->owner.block;
        }
        if (!destination->incoming || destination->incoming->next_incoming ||
            !block->outgoing->next_outgoing) return;
        value = constant->integer.value;
        previous_value = NULL;
        for (scan = consumer->previous; scan != record; scan = scan->previous) {
            if (scan->opcode == 0x131) {
                if (compare_input_operands(variable, scan->tail.right)) return;
                previous_value = scan->left;
            } else if (scan->opcode == 0x141 || scan->opcode == 0x142) {
                if (compare_input_operands(previous_value, scan->tail.right)) return;
                input = scan->left;
                if (compare_input_operands(variable, input)) return;
                input = input->next;
                if (input->kind != 7) return;
                if (scan->opcode == 0x141) value += input->integer.value;
                else value -= input->integer.value;
            } else return;
        }
        if (value != constant->integer.value) {
            constant = copy_input_expression(constant);
            constant->integer.value = value;
        }
        added = new_binary_expression(0x131, record->type, variable, constant,
            insert_input_record, anchor);
        added->precision_flags = 1;
        finish_global_assignment(added, 1);
    } else {
        variable = record->left;
        if (((1U << (variable->kind & 31)) & 6) && variable->value.reg->kind == 3) {
            scan = record->previous;
            if (scan->opcode == 0x131 &&
                !compare_input_operands(scan->tail.right, variable)) variable = scan->left;
        }
        if ((((1U << (variable->kind & 31)) & 6) && variable->value.reg->value) ||
            variable->kind == 7) return;
        type = variable->type;
        for (range = (C2GlobalSwitchRange *)record->aggregate_value; range; range = range->next) {
            if (range->low != range->high) continue;
            target = (C2BranchTarget *)range->target;
            destination = target->block;
            if (!destination->incoming || destination->incoming->next_incoming ||
                destination->outgoing->next_outgoing) continue;
            value = normalize_integer_value(type, (unsigned __int64)(__int64)range->low);
            constant = new_integer_expression(type, value);
            added = new_binary_expression(0x131, type, variable, constant,
                insert_input_record, (C2Record *)target);
            added->precision_flags = 1;
            finish_global_assignment(added, 1);
        }
    }
}

C2Expression *__fastcall fold_integer_operation(C2Expression *record, unsigned short type,
    C2Expression *left, C2Expression *right);
C2Expression *__fastcall fold_real_operation(C2Expression *record, unsigned short type,
    C2Expression *left, C2Expression *right);
int __fastcall compare_input_operands(C2Expression *left, C2Expression *right);
void __fastcall remove_statement_expression(C2Expression *statement, C2Expression *record);
int c2_real_compare(int condition, C2Real *left, C2Real *right);

C2Expression *__fastcall prepare_global_record(C2Expression *record,
    C2BasicBlock *block, C2Function *function)
{
    C2Expression *left, *right, *value, *branch, *operand, *replacement, *target;
    C2FunctionRegister *reg;
    C2InputSymbol *symbol;
    C2BranchTarget *label;
    C2BasicBlock *destination;
    C2GlobalSwitchRange *range;
    unsigned __int64 first, second;
    unsigned char condition;
    int signed_compare, result;

    if (record->opcode != 0x15d && input_has_side_effects(record)) {
        if (record->opcode != 0x16c || record->left->kind != 7) return record;
        remove_global_record(record);
        return NULL;
    }
    value = fold_integer_operation(record, record->type, record->left, record->left->next);
    if (!value) value = fold_real_operation(record, record->type, record->left, record->left->next);
    if (value) {
        if (record->left->next) remove_input_operand(record, record->left->next);
        if (value != record->left) attach_input_expression(record, record->left, value);
        record->opcode = 0x131;
        return record;
    }
    if (!((1U << ((record->type >> 12) & 31)) & 0x0e) &&
        record->opcode != 0x135 && record->opcode != 0x151) return record;
    left = record->left;
    right = left->next;
    if (record->opcode == 0x141) {
        if (left->kind == 7) {
            swap_global_operands(record);
            left = record->left;
            right = left->next;
        }
        if (left->kind == 3) {
            if (right->kind != 7) goto compare_or_constant;
            if ((long)((unsigned)left->value.reg->displacement + right->integer.low) >= 0) {
                left->value.reg = new_address_register(left->value.reg, 0x5000, 0,
                    (long)right->integer.low);
                goto assignment;
            }
        }
        if (right->kind == 7 && !right->integer.value) goto assignment;
    } else if (record->opcode == 0x143) {
        if (left->kind == 7) {
            swap_global_operands(record);
            left = record->left;
            right = left->next;
        }
        if (right->kind == 7) {
            if (!right->integer.value) {
                swap_global_operands(record);
                right = record->left->next;
                goto assignment;
            }
            if (right->integer.low == 1 && !right->integer.high) goto assignment;
        }
    } else if (record->opcode == 0x131 &&
               !compare_input_operands(record->tail.right, left)) {
        remove_global_record(record);
        return NULL;
    }
compare_or_constant:
    if (record->opcode == 0x151) {
        if (!compare_input_operands(left, right)) {
            first = second = 1;
            signed_compare = (record->type & 0xf000) == 0x4000 ||
                             (record->type & 0xf000) == 0x1000;
            goto evaluate_branch;
        }
        if (left->kind == 8 && right->kind == 8) {
            condition = global_expression_condition(record);
            signed_compare = 0;
            switch (condition) {
            case 1:
                /* The original performs this comparison, then continues. */
                c2_real_compare(1, left->value.real, right->value.real);
                break;
            case 2:
                first = (__int64)c2_real_compare(1, left->value.real, right->value.real);
                second = 1;
                goto evaluate_branch;
            case 3:
                second = (__int64)c2_real_compare(4, left->value.real, right->value.real);
                first = !second;
                goto evaluate_branch;
            case 5:
                second = (__int64)c2_real_compare(5, left->value.real, right->value.real);
                first = !second;
                goto evaluate_branch;
            case 6:
                /* Both calls are observable in the original function graph. */
                c2_real_compare(3, left->value.real, right->value.real);
            case 4:
                first = (__int64)c2_real_compare(2, left->value.real, right->value.real);
                second = !first;
                goto evaluate_branch;
            }
        }
        if (left->kind == 3 && right->kind == 3 &&
            left->value.reg->canonical == right->value.reg->canonical) {
            signed_compare = 0;
            condition = global_expression_condition(record);
            first = (__int64)left->value.reg->displacement;
            second = (__int64)right->value.reg->displacement;
            goto evaluate_branch;
        }
        if ((record->type & 0xf000) == 0x2000) {
            signed_compare = 0;
            condition = global_expression_condition(record);
            switch (condition) {
            case 3: case 6:
                if (right->kind == 7 && !right->integer.value) {
                    first = 1; second = 0;
                    goto evaluate_branch;
                }
                break;
            case 4: case 5:
                if (left->kind != 7) return record;
                if (!left->integer.value) {
                    first = 0; second = 1;
                    goto evaluate_branch;
                }
                break;
            }
        }
    }
    if (left->kind != 7 || (right && right->kind != 7)) return record;
    first = left->integer.value;
    second = right ? right->integer.value : 0;
    signed_compare = (record->type & 0xf000) == 0x1000;
    if (record->opcode != 0x151) {
        if (record->opcode != 0x15d) return record;
        for (range = (C2GlobalSwitchRange *)record->aggregate_value; range; range = range->next) {
            if (evaluate_integer_comparison(signed_compare, record->type & 0xfff,
                    (__int64)range->low, first, 5) &&
                evaluate_integer_comparison(signed_compare, record->type & 0xfff,
                    first, (__int64)range->high, 5)) break;
            remove_global_successor(block, ((C2BranchTarget *)range->target)->block, function);
        }
        target = record->detail.data;
        if (range) {
            remove_global_successor(block, ((C2BranchTarget *)target)->block, function);
            target = range->target;
        }
        new_call_expression(0x15a, 0, target, NULL, NULL,
            insert_input_record, (C2Record *)record);
        release_global_record(record);
        return NULL;
    }
evaluate_branch:
    branch = follow_optimizer_result(record, &operand);
    if (branch->kind == 15) {
        result = evaluate_integer_comparison(signed_compare, record->type & 0xfff,
            first, second, (unsigned char)branch->detail.intrinsic);
        target = branch->left->next;
        if (!result) target = target->next;
        replacement = new_binary_expression(0x131, branch->type, branch->tail.right,
            target, insert_input_record, (C2Record *)branch);
        if ((1U << (replacement->tail.right->kind & 31)) & 6) {
            reg = replacement->tail.right->value.reg;
            if (reg->value) {
                if (!reg->canonical->variant_next) reg->value = replacement;
                else bind_input_temporary(reg, replacement);
            }
        }
        remove_global_record(record);
        remove_global_record(branch);
        global_next_record = replacement->next;
        return replacement;
    }
    if (!global_motion_enabled) return record;
    result = evaluate_integer_comparison(signed_compare, record->type & 0xfff,
        first, second, branch->convention);
    remove_statement_expression(branch, record);
    remove_global_record(record);
    if (!result) {
        symbol = (C2InputSymbol *)branch->left->value.symbol;
        label = symbol->branch_target;
        destination = label->block;
        if (!destination) destination = get_symbol_branch_target(label, 24)->block;
        if (global_next_record == branch) global_next_record = branch->next;
        remove_global_record(branch);
    } else {
        destination = block->previous;
        branch->opcode = 0x15a;
        remove_input_operand(branch, operand);
        branch->convention = 0;
    }
    remove_global_successor(block, destination, function);
    return NULL;
assignment:
    if (right) remove_input_operand(record, right);
    record->opcode = 0x131;
    record->left->type = record->type;
    if (record->left->kind == 7)
        truncate_global_integer(record->left->type, record->left);
    return record;
}

typedef char C2GlobalNextRegisterOffset[
    offsetof(C2FunctionRegister, global_next) == 44 ? 1 : -1];
typedef char C2GlobalRegisterFollowersOffset[
    offsetof(C2FunctionRegister, global_followers) == 68 ? 1 : -1];
typedef char C2GlobalRegisterDefinitionsOffset[
    offsetof(C2FunctionRegister, global_definitions) == 72 ? 1 : -1];
typedef char C2GlobalRegisterUsesOffset[
    offsetof(C2FunctionRegister, global_uses) == 76 ? 1 : -1];
extern unsigned diagnostic_position;
void __fastcall retain_restored_aggregate(C2Expression *owner, C2Expression *operand);
#define GLOBAL_REGISTER_OPERAND(operand) ((1U << ((operand)->kind & 31)) & 6)

void __fastcall initialize_global_flow_analysis(C2Function *function)
{
    C2Expression *record, *cursor, *end, *operand, *result, *temporary, *inserted;
    C2FunctionRegister *destination, *source, *reg;
    C2OptimizerRecordLink *link;
    C2OptimizerUse *use;
    unsigned short type;
    int first_insert;

    end = (C2Expression *)function->flow->first->last_record;
    cursor = ((C2Expression *)function->flow->last->records)->next;
    while ((record = cursor) != end) {
        cursor = record->next;
        if (!(record->flags & 1) || record->control_flags) continue;
        record->control_flags = 1;
        if (record->location.line) {
            record_line_delta = record->location.line;
            diagnostic_position = function->source_line_base + record->location.line;
        }
        for (operand = record->left; operand; operand = operand->next) {
            if (((1U << (operand->kind & 31)) & 0x60) && operand->opcode != 0x122) {
                temporary = new_integer_expression(0x2004,
                    (__int64)operand->detail.aggregate_displacement);
                inserted = new_assignment_expression(0x133, 0x2004, temporary,
                    prepend_input_record, (C2Record *)record);
                operand->opcode = 0x122;
                operand->detail.aggregate_displacement = 0;
                operand->aggregate_value = copy_input_expression(inserted->tail.right);
                operand->aggregate_value->location.expression.properties |= 0x20;
                operand->aggregate_value->location.expression.properties |= 0x10;
                retain_restored_aggregate(record, operand);
            }
        }
        for (operand = record->tail.right; operand; operand = operand->next) {
            if (((1U << (operand->kind & 31)) & 0x60) && operand->opcode != 0x122) {
                temporary = new_integer_expression(0x2004,
                    (__int64)operand->detail.aggregate_displacement);
                inserted = new_assignment_expression(0x133, 0x2004, temporary,
                    prepend_input_record, (C2Record *)record);
                operand->opcode = 0x122;
                operand->detail.aggregate_displacement = 0;
                operand->aggregate_value = copy_input_expression(inserted->tail.right);
                operand->aggregate_value->location.expression.properties |= 0x20;
                operand->aggregate_value->location.expression.properties |= 0x10;
                retain_restored_aggregate(record, operand);
            }
        }
        if (record->opcode == 0x13f) {
            record->kind = 11;
            type = record->type & 0xfff;
            record->opcode = 0x131;
            if (type && (type < 3 || type == 4)) {
                type |= 0x2000;
                retarget_global_operand(record->tail.right, type);
                operand = record->tail.right;
                if (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->value)
                    materialize_global_register_operand(operand, operand->value.reg);
                retarget_global_operand(record->left, type);
                operand = record->left;
                if (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->value)
                    materialize_global_register_operand(operand, operand->value.reg);
                record->type = type;
            }
        }
        if (!((1U << (record->kind & 31)) & 0x1000) && record->kind != 19) {
            if (record->opcode == 0x131) {
                result = record->tail.right;
                operand = record->left;
                if (GLOBAL_REGISTER_OPERAND(result) && result->value.reg->kind == 3 &&
                    GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->kind == 3)
                    result->value.reg->flags |= operand->value.reg->flags & 0x40;
                if (result->kind == 2 || operand->kind == 2) {
                    destination = NULL;
                    if (GLOBAL_REGISTER_OPERAND(result) && result->value.reg->kind == 3 &&
                        !result->value.reg->value) {
                        destination = result->value.reg->canonical;
                        if (!destination->global_next) {
                            destination->global_next = global_pending_registers;
                            global_pending_registers = destination;
                        }
                        link = c2_pool_allocate(3, sizeof(*link));
                        link->record = record;
                        link->next = destination->global_definitions;
                        destination->global_definitions = link;
                    }
                    source = NULL;
                    if (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->kind == 3 &&
                        !operand->value.reg->value) {
                        source = operand->value.reg->canonical;
                        if (!source->global_next) {
                            source->global_next = global_pending_registers;
                            global_pending_registers = source;
                        }
                        link = c2_pool_allocate(3, sizeof(*link));
                        link->record = record;
                        link->next = source->global_uses;
                        source->global_uses = link;
                    }
                    if (destination && source) {
                        use = c2_pool_allocate(3, sizeof(*use));
                        use->reg = destination;
                        use->next = source->global_followers;
                        source->global_followers = use;
                    }
                }
            } else {
                result = record->tail.right;
                if (result && GLOBAL_REGISTER_OPERAND(result) &&
                    !result->value.reg->value && result->value.reg->kind == 3) {
                    type = result->type;
                    if ((type & 0xf000) != 0x5000) {
                        temporary = new_temporary_operand(type);
                        inserted = new_binary_expression(0x131, type, result, temporary,
                            insert_input_record, (C2Record *)record);
                        replace_input_result(record, result, temporary);
                        temporary->value.reg->value = record;
                        inserted->flags |= 0x40;
                        cursor = inserted;
                    }
                }
            }
            first_insert = 1;
            for (operand = record->left; operand; operand = operand->next) {
                if ((operand->location.expression.properties & 0x20) &&
                    (operand->location.expression.properties & 0x40) &&
                    (!GLOBAL_REGISTER_OPERAND(operand) || !operand->value.reg->value)) {
                    inserted = new_assignment_expression(0x133, operand->type, operand,
                        prepend_input_record, (C2Record *)record);
                    operand = attach_input_expression(record, operand, inserted->tail.right);
                    inserted->control_flags = 1;
                }
                if (record->opcode != 0x131 && GLOBAL_REGISTER_OPERAND(operand) &&
                    !operand->value.reg->value && operand->value.reg->kind == 3) {
                    type = operand->type;
                    if ((type & 0xf000) != 0x5000) {
                        temporary = new_temporary_operand(type);
                        inserted = new_binary_expression(0x131, type, temporary, operand,
                            prepend_input_record, (C2Record *)record);
                        operand = attach_input_expression(record, operand, temporary);
                        temporary->value.reg->value = inserted;
                        if (first_insert) {
                            first_insert = 0;
                            cursor = inserted;
                        }
                        inserted->flags |= 0x40;
                    }
                }
            }
            result = record->tail.right;
            if (record->opcode == 0x131 && GLOBAL_REGISTER_OPERAND(result) &&
                result->value.reg->value && record->left->kind == 6)
                record->opcode = 0x133;
            else {
                for (operand = record->left; operand; operand = operand->next) {
                    if (((1U << (operand->kind & 31)) & 0x7e) &&
                        (operand->kind == 6 ||
                         ((operand->location.expression.properties & 0x40) &&
                          (!GLOBAL_REGISTER_OPERAND(operand) || !operand->value.reg->value)))) {
                        inserted = new_assignment_expression(0x133, operand->type, operand,
                            prepend_input_record, (C2Record *)record);
                        operand = attach_input_expression(record, operand, inserted->tail.right);
                        inserted->control_flags = 1;
                        mark_global_precision(inserted);
                    }
                }
            }
            mark_global_precision(record);
        }
        operand = record->left;
        if (record->opcode == 0x16c && operand && GLOBAL_REGISTER_OPERAND(operand)) {
            reg = operand->value.reg;
            if (reg->value && reg->value->opcode == 0x151) reg->value->opcode = 0x142;
        }
        if (record->kind == 17 && record->detail.intrinsic > 0xab &&
            record->detail.intrinsic < 0xae)
            append_global_attribute(record, 0x2001, 1);
    }
}

extern int c2_basic_mode;
void __fastcall mark_global_precision(C2Expression *record)
{
    C2Expression *value, *operand;
    if (record->opcode == 0x138) record->precision_flags = 3;
    else if (record->opcode == 0x135) {
        if ((record->type & 0xf000) == 0x4000 &&
            (record->left->type & 0xf000) == 0x4000 &&
            (record->left->type & 0xfff) <= (record->type & 0xfff)) {
            record->precision_flags = 3;
            if (GLOBAL_REGISTER_OPERAND(record->tail.right) &&
                record->tail.right->value.reg->value) {
                value = follow_optimizer_result(record, &operand);
                if (value && value->opcode == 0x130 &&
                    follow_optimizer_result(value, &operand)->opcode == 0x158)
                    record->precision_flags = 0;
            }
        }
    } else if (record->opcode == 0x133) {
        if ((record->type & 0xf000) == 0x4000) record->precision_flags = 3;
        if (GLOBAL_REGISTER_OPERAND(record->tail.right) &&
            record->tail.right->value.reg->value) {
            value = follow_optimizer_result(record, &operand);
            if (value && value->opcode == 0x130) record->precision_flags = 0;
        }
    } else if (record->opcode == 0x14c)
        record->left->value.reg->value->precision_flags = 3;
    if ((record->type & 0xf000) == 0x4000 && c2_basic_mode)
        record->precision_flags = 3;
}

void __fastcall build_global_flow_analysis(C2Function *function)
{
    C2FunctionRegister *pending, *reg, *next, *temporary_reg, *source_reg;
    C2OptimizerUse *use;
    C2OptimizerRecordLink *link;
    C2Expression *source, *definition, *record, *following, *end;
    C2Expression *value, *operand, *replacement;
    unsigned short type, destination_type, saved_type;
    int deferred, valid, compatible;

    pending = global_pending_registers;
    if (pending == (C2FunctionRegister *)-1) return;
    do {
        global_pending_registers = (C2FunctionRegister *)-1;
        for (reg = pending; reg != (C2FunctionRegister *)-1; reg = next) {
            next = reg->global_next;
            deferred = 0;
            for (use = reg->global_followers; use; use = use->next)
                if (use->reg->global_next) {
                    deferred = 1;
                    break;
                }
            if (deferred) {
                reg->global_next = global_pending_registers;
                global_pending_registers = reg;
                continue;
            }
            link = reg->global_uses;
            if (link && !link->next && reg->global_definitions &&
                (source = link->record) != NULL && source->tail.right->kind == 2 &&
                !source->tail.right->value.reg->value && !input_has_side_effects(source)) {
                type = source->type;
                valid = 1;
                for (link = reg->global_definitions; link; link = link->next) {
                    definition = link->record;
                    if (!global_rewrite_order_is_valid(source, definition) ||
                        global_range_changes_operand(definition->next, source->previous,
                            source->tail.right) ||
                        global_range_changes_operand(definition->next, source->previous,
                            source->left)) {
                        valid = 0;
                        break;
                    }
                    destination_type = definition->tail.right->type;
                    if (!register_types_match(destination_type, type)) {
                        compatible = 0;
                        switch (definition->left->kind) {
                        case 1: case 2: case 6: case 7:
                            if (destination_type == type) compatible = 1;
                            else if ((1U << ((destination_type >> 12) & 31)) & 0x0e) {
                                if (((1U << ((type >> 12) & 31)) & 0x0e) ||
                                    (type & 0xf000) == 0x5000)
                                    compatible = (type & 0xfff) <= (destination_type & 0xfff);
                            } else if ((destination_type & 0xf000) == 0x5000)
                                compatible = (type & 0xfff) <= (destination_type & 0xfff);
                            break;
                        }
                        if (!compatible) {
                            valid = 0;
                            break;
                        }
                    }
                }
                if (valid) {
                    for (link = reg->global_definitions; link; link = link->next) {
                        definition = link->record;
                        replace_input_result(definition, definition->tail.right,
                            source->tail.right);
                        operand = definition->left;
                        if (operand->kind && operand->kind < 3)
                            operand->value.reg = new_address_register(operand->value.reg,
                                type, 0, 0);
                        else if (operand->kind == 7) truncate_global_integer(type, operand);
                        definition->left->type = type;
                        definition->type = type;
                    }
                    unlink_and_release_input_record((C2Record *)source);
                }
            }
            reg->global_next = NULL;
            reg->global_uses = NULL;
            reg->global_definitions = NULL;
            reg->global_followers = NULL;
        }
        pending = global_pending_registers;
    } while (pending != (C2FunctionRegister *)-1);
    global_pending_registers = (C2FunctionRegister *)-1;
    end = (C2Expression *)function->flow->first->last_record;
    for (record = ((C2Expression *)function->flow->last->records)->next;
         record != end; record = following) {
        following = record->next;
        if (!(record->flags & 0x40)) continue;
        record->flags &= ~0x40;
        operand = record->tail.right;
        if (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->value) {
            temporary_reg = operand->value.reg;
            value = follow_optimizer_result(record, &operand);
            if (temporary_reg == operand->value.reg) {
                saved_type = operand->type;
                replacement = attach_input_expression(value, operand, record->left);
                replacement->type = saved_type;
                unlink_and_release_input_record((C2Record *)record);
                release_function_register(temporary_reg);
            }
        } else {
            operand = record->left;
            if (GLOBAL_REGISTER_OPERAND(operand)) {
                source_reg = operand->value.reg;
                value = source_reg->value;
                if (value && value->kind != 13) {
                    operand = value->tail.right;
                    temporary_reg = operand->value.reg;
                    if (temporary_reg == source_reg) {
                        saved_type = operand->type;
                        replacement = replace_input_result(value, operand, record->tail.right);
                        replacement->type = saved_type;
                        unlink_and_release_input_record((C2Record *)record);
                        release_function_register(temporary_reg);
                    }
                }
            }
        }
    }
}

void __fastcall rewrite_global_function(C2Function *function, unsigned mode)
{
    C2BasicBlock *block;

    reset_input_pool(6);
    for (block = function->flow->last; block; block = block->previous) {
        if (mode & 1) {
            block->rewrite_mode1[0] = new_register_set(6);
            block->rewrite_mode1[1] = new_register_set(6);
            block->rewrite_mode1[2] = new_register_set(6);
            block->rewrite_mode1[3] = new_register_set(6);
            block->rewrite_mode1[4] = new_register_set(6);
            block->rewrite_mode1[5] = new_register_set(6);
            block->rewrite_mode1[6] = new_register_set(6);
            block->rewrite_mode1[7] = new_register_set(6);
        } else {
            block->rewrite_mode1[0] = NULL;
            block->rewrite_mode1[1] = NULL;
            block->rewrite_mode1[2] = NULL;
            block->rewrite_mode1[3] = NULL;
            block->rewrite_mode1[4] = NULL;
            block->rewrite_mode1[5] = NULL;
            block->rewrite_mode1[6] = NULL;
            block->rewrite_mode1[7] = NULL;
        }
        if (mode & 2) {
            block->rewrite_mode2[1] = new_register_set(6);
            block->rewrite_mode2[0] = new_register_set(6);
            block->rewrite_mode2[2] = new_register_set(6);
            block->rewrite_mode2[3] = new_register_set(6);
            block->rewrite_mode2[4] = new_register_set(6);
            block->rewrite_mode2[5] = new_register_set(6);
        } else {
            block->rewrite_mode2[1] = NULL;
            block->rewrite_mode2[0] = NULL;
            block->rewrite_mode2[2] = NULL;
            block->rewrite_mode2[3] = NULL;
            block->rewrite_mode2[4] = NULL;
            block->rewrite_mode2[5] = NULL;
        }
        if (mode & 4) {
            block->rewrite_mode4[0] = new_register_set(6);
            block->rewrite_mode4[1] = new_register_set(6);
            block->rewrite_mode4[2] = new_register_set(6);
            block->rewrite_mode4[3] = new_register_set(6);
            block->rewrite_mode4[4] = new_register_set(6);
            block->rewrite_mode4[5] = new_register_set(6);
            block->rewrite_mode4[6] = new_register_set(6);
        } else {
            block->rewrite_mode4[0] = NULL;
            block->rewrite_mode4[1] = NULL;
            block->rewrite_mode4[2] = NULL;
            block->rewrite_mode4[3] = NULL;
            block->rewrite_mode4[4] = NULL;
            block->rewrite_mode4[5] = NULL;
            block->rewrite_mode4[6] = NULL;
        }
    }
}

/* Actual DWORD 48fa54, reset before propagation; no pointer role is inferred. */
int global_rewrite_state;

void __fastcall finish_global_function_rewrite(C2Function *function)
{
    C2BasicBlock *first, *last;

    first = function->flow->last;
    last = function->flow->first;
    global_rewrite_state = 0;
    clear_global_rewrite_marks(first, last);
    propagate_global_rewrites(first, last);
}

void __fastcall clear_global_rewrite_marks(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *end;

    end = first->next;
    for (; last != end; last = last->next)
        last->flags &= ~0x04000000;
}

C2FunctionRegister *global_rewrite_registers; /* 48fa44, next_free-linked work list. */
C2RegisterSet *global_rewrite_exclusions;     /* 48fa48. */

void __fastcall propagate_global_rewrites(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end, *header;
    C2FlowEdge *edge;
    C2Expression *record, *operand, *before, *previous;
    C2RegisterSet *inputs, *outputs;
    C2FunctionRegister *reg;
    C2FlowRegion *region;
    int has_assignments;

    if (!last->previous)
        copy_optimizer_symbol_set_into(last->rewrite_mode1[3], global_initial_registers);
    end = first->next;
    for (block = last; block != end; block = block->next)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (operand = record->left; operand; operand = operand->next) {
                if ((1U << (operand->kind & 31)) & 6) {
                    operand->value.reg->rewrite_seen = 0;
                    operand->value.reg->rewrite_queued = 0;
                    operand->location.expression.comparison &= ~0x10;
                } else if (operand->kind == 6 && operand->value.reg) {
                    operand->value.reg->rewrite_seen = 0;
                    operand->value.reg->rewrite_queued = 0;
                }
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                if ((1U << (operand->kind & 31)) & 6) {
                    operand->value.reg->rewrite_seen = 0;
                    operand->value.reg->rewrite_queued = 0;
                    operand->location.expression.comparison &= ~0x10;
                } else if (operand->kind == 6 && operand->value.reg) {
                    operand->value.reg->rewrite_seen = 0;
                    operand->value.reg->rewrite_queued = 0;
                }
            }
        }
    prepare_global_alias_uses();
    global_rewrite_registers = NULL;
    inputs = new_register_set(6);
    outputs = new_register_set(6);
    global_rewrite_exclusions = new_global_exclusion_set(6);
    for (block = last; block != end; block = block->next) {
        if (global_block_has_backedge(block) && !(block->flags & 0x04000000)) {
            header = find_global_loop_header(block);
            region = header->block_scope;
            if (region && region->header == header && region->rewrite_entry)
                header = region->rewrite_entry;
            prepare_global_loop(header, block);
        }
        edge = block->outgoing;
        if (edge) {
            copy_optimizer_symbol_set_into(block->rewrite_mode1[3],
                edge->destination->rewrite_mode1[2]);
            copy_optimizer_symbol_set_into(block->rewrite_mode1[7],
                edge->destination->rewrite_mode1[6]);
            for (edge = edge->next_outgoing; edge; edge = edge->next_outgoing) {
                merge_optimizer_symbol_sets(block->rewrite_mode1[3],
                    edge->destination->rewrite_mode1[2]);
                intersect_optimizer_symbol_sets(block->rewrite_mode1[7],
                    edge->destination->rewrite_mode1[6]);
            }
        }
        copy_optimizer_symbol_set_into(inputs, block->rewrite_mode1[3]);
        copy_optimizer_symbol_set_into(outputs, block->rewrite_mode1[7]);
        while ((reg = global_rewrite_registers) != NULL) {
            reg->rewrite_queued = 0;
            global_rewrite_registers = reg->next_free;
        }
        before = (C2Expression *)block->records;
        has_assignments = 0;
        for (record = ((C2Expression *)block->last_record)->previous;
             record != before; record = previous) {
            previous = record->previous;
            if (!(record->flags & 1)) continue;
            if (record->kind == 11) has_assignments = 1;
            rewrite_global_record(record, inputs, outputs);
        }
        intersect_optimizer_symbol_sets(outputs, global_rewrite_exclusions);
        copy_optimizer_symbol_set_into(block->rewrite_mode1[2], inputs);
        copy_optimizer_symbol_set_into(block->rewrite_mode1[6], outputs);
        if (has_assignments && !next_global_code_record(before))
            global_aliases_changed = 1;
    }
    release_optimizer_symbol_set(inputs);
    release_optimizer_symbol_set(outputs);
    release_optimizer_symbol_set(global_rewrite_exclusions);
}

void prepare_global_alias_uses(void)
{
    C2RegisterSet *variants, *uses;
    C2FunctionRegister *reg, *variant;
    C2OptimizerConflict *conflict;
    unsigned number;

    variants = new_register_set(6);
    for (number = next_optimizer_symbol(optimizer_live_registers);
         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        clear_register_set(variants);
        for (variant = reg; variant; variant = variant->variant_next)
            add_register_to_set(variants, variant->number);
        for (conflict = optimizer_conflicts[
                 ((C2InputSymbol *)reg->symbol)->detail.optimizer_number];
             conflict; conflict = conflict->next) {
            uses = global_index_variant_uses[conflict->number];
            if (uses) merge_optimizer_symbol_sets(uses, variants);
        }
    }
    release_optimizer_symbol_set(variants);
}

C2RegisterSet *__fastcall new_global_exclusion_set(int pool)
{
    C2RegisterSet *result;
    C2FunctionRegister *reg, *canonical;
    unsigned number;

    result = new_register_set(pool);
    for (number = next_optimizer_symbol(tracked_register_set);
         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        canonical = reg->canonical;
        if (canonical != reg &&
            (canonical->displacement < reg->displacement ||
             (long)(canonical->value_bytes + canonical->displacement) <
             (long)(reg->value_bytes + reg->displacement)))
            add_register_to_set(result, reg->number);
    }
    for (number = next_optimizer_symbol(secondary_register_set);
         number != (unsigned)-1; number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        canonical = reg->canonical;
        if (canonical != reg &&
            (canonical->displacement < reg->displacement ||
             (long)(canonical->value_bytes + canonical->displacement) <
             (long)(reg->value_bytes + reg->displacement)))
            add_register_to_set(result, reg->number);
    }
    return result;
}

C2FunctionRegister *__fastcall lookup_function_register(unsigned number)
{
    return &function_register_blocks[number >> 5]->records[number & 31];
}

int __fastcall global_block_has_backedge(C2BasicBlock *block)
{
    C2FlowEdge *edge;

    for (edge = block->outgoing; edge; edge = edge->next_outgoing)
        if ((short)edge->destination->number84 <= (short)block->number84)
            return 1;
    return 0;
}

C2RegisterSet *global_flow_temporary; /* Actual 491020 writer and set consumers. */

void __fastcall finish_global_flow_analysis(C2Function *function)
{
    C2BasicBlock *first, *last, *block, *end;

    first = function->flow->last;
    last = function->flow->first;
    global_flow_temporary = new_register_set(6);
    end = first->next;
    for (block = last; block != end; block = block->next)
        initialize_global_block_sets(block);
    release_optimizer_symbol_set(global_flow_temporary);
    solve_global_block_sets(first, last);
    finish_global_record_sets(first, last);
}

C2RegisterSet *global_flow_kills; /* 491024, current block+96. */
C2RegisterSet *global_flow_uses;  /* 491028, current block+100. */

void __fastcall initialize_global_block_sets(C2BasicBlock *block)
{
    C2Expression *record;

    global_flow_kills = block->rewrite_mode2[0];
    global_flow_uses = block->rewrite_mode2[1];
    clear_register_set(global_flow_temporary);
    clear_register_set(global_flow_uses);
    clear_register_set(global_flow_kills);
    clear_register_set(block->rewrite_mode2[2]);
    clear_register_set(block->rewrite_mode2[3]);
    clear_register_set(block->rewrite_mode2[4]);
    clear_register_set(block->rewrite_mode2[5]);
    for (record = ((C2Expression *)block->last_record)->previous;
         record != (C2Expression *)block->records; record = record->previous)
        if (record->flags & 1) collect_global_record_sets(record);
}

void __fastcall solve_global_block_sets(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end;
    int changed;

    end = first->next;
    do {
        changed = 0;
        for (block = last; block != end; block = block->next)
            update_global_block_sets(block, &changed);
    } while (changed);
}

void __fastcall update_global_block_sets(C2BasicBlock *block, int *changed)
{
    C2RegisterSet *temporary, *scratch, *common, *different, *uses, *result, *input;
    C2FlowEdge *edge;
    C2BasicBlock *successor;

    temporary = new_register_set(6);
    scratch = new_register_set(6);
    common = block->rewrite_mode2[4];
    clear_register_set(common);
    different = block->rewrite_mode2[5];
    clear_register_set(different);
    edge = block->outgoing;
    if (edge) {
        successor = edge->destination;
        copy_global_set_intersection(common, successor->rewrite_mode1[2],
            successor->rewrite_mode2[2]);
        merge_optimizer_symbol_sets(different, successor->rewrite_mode2[3]);
        for (edge = edge->next_outgoing; edge; edge = edge->next_outgoing) {
            successor = edge->destination;
            copy_global_set_intersection(temporary, successor->rewrite_mode1[2],
                successor->rewrite_mode2[2]);
            copy_global_set_intersection(scratch, temporary, common);
            merge_optimizer_symbol_sets(different, scratch);
            merge_optimizer_symbol_sets(different, successor->rewrite_mode2[3]);
            xor_global_register_sets(common, temporary);
        }
        subtract_optimizer_symbol_sets(common, different);
    }
    copy_optimizer_symbol_difference(temporary, common, block->rewrite_mode2[0]);
    uses = block->rewrite_mode2[1];
    result = block->rewrite_mode2[3];
    copy_global_set_intersection(result, temporary, uses);
    copy_optimizer_symbol_difference(scratch, different, block->rewrite_mode2[0]);
    merge_optimizer_symbol_sets(result, scratch);
    copy_optimizer_symbol_difference(scratch, block->rewrite_mode1[0], uses);
    merge_optimizer_symbol_sets(result, scratch);
    input = block->rewrite_mode2[2];
    copy_optimizer_symbol_difference(scratch, block->rewrite_mode1[2], input);
    merge_optimizer_symbol_sets(result, scratch);
    xor_global_register_sets(temporary, uses);
    subtract_optimizer_symbol_sets(temporary, result);
    if (!*changed && !optimizer_symbol_sets_equal(temporary, input)) *changed = 1;
    copy_optimizer_symbol_set_into(input, temporary);
    release_optimizer_symbol_set(temporary);
    release_optimizer_symbol_set(scratch);
}

void __fastcall copy_global_set_intersection(C2RegisterSet *destination,
    C2RegisterSet *source, C2RegisterSet *mask)
{
    C2RegisterSetNode *input, *spare, *node, *next;

    input = source->links.first;
    spare = destination->links.first;
    node = spare;
    while (input) {
        if (spare) {
            node = spare;
            spare = spare->next;
        } else node = insert_register_set_node(destination, node);
        node->group = input->group;
        node->bits = input->bits;
        input = input->next;
    }
    while (spare) {
        next = spare->next;
        remove_register_set_node(destination, spare);
        spare = next;
    }
    intersect_optimizer_symbol_sets(destination, mask);
}

void __fastcall xor_global_register_sets(C2RegisterSet *destination, C2RegisterSet *source)
{
    C2RegisterSetNode *input, *node, **link;
    unsigned group;

    for (input = source->links.first; input; input = input->next) {
        group = input->group & ~31U;
        link = &destination->links.first;
        for (node = *link; node && node->group < group; node = node->next)
            link = &node->next;
        if (!node || node->group != group) {
            node = new_register_set_node(destination);
            node->group = group;
            node->next = *link;
            *link = node;
        }
        node->bits ^= input->bits;
        if (!node->bits) remove_register_set_node(destination, node);
    }
}

/* Original497220: the current block's available expression-register set. */
C2RegisterSet *global_available_registers;
typedef char C2GlobalAvailableInOffset[
    offsetof(C2BasicBlock, global_available_in) == 80 ? 1 : -1];

void __fastcall optimize_global_block(C2BasicBlock *block, C2Function *function)
{
    C2FlowEdge *edge;
    C2Expression *record, *last;
    int operations;

    if (block->flags & 0x02000000U) return;
    edge = block->incoming;
    if (edge) {
        copy_optimizer_symbol_set_into(block->global_available_in, edge->source->global_definitions);
        for (edge = edge->next_incoming; edge; edge = edge->next_incoming)
            intersect_optimizer_symbol_sets(block->global_available_in, edge->source->global_definitions);
    }
    global_available_registers = block->global_definitions;
    copy_optimizer_symbol_set_into(global_available_registers, block->global_available_in);
    last = (C2Expression *)block->last_record;
    operations = 0;
    for (record = ((C2Expression *)block->records)->next; record != last; record = global_next_record) {
        global_next_record = record->next;
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record->location.line;
        }
        if (record->flags & 1) {
            if (record->kind == 11) operations = 1;
            optimize_global_record(record, block, function);
        }
    }
    if (operations && !find_next_global_operation((C2Expression *)block->records))
        global_aliases_changed = 1;
}

/* Original49721c is an initialized NULL expression pointer, maintained by
 * the global record rewriting helpers. */
C2Expression *global_pending_assignment;
typedef char C2GlobalDefinitionRecordOffset[
    offsetof(C2FunctionRegister, global_definition_record) == 72 ? 1 : -1];

void __fastcall optimize_global_record(C2Expression *record,
    C2BasicBlock *block, C2Function *function)
{
    C2Expression *result, *selected, *operand, *prior, *replacement;
    C2FunctionRegister *reg;
    C2BasicBlock *destination;
    C2InputSymbol *symbol;
    C2BranchTarget *label;
    C2RegisterSet *set;
    int classification, take_first, rewrite;

    if (global_region_limited || (global_block_state && record->opcode == 0x151)) {
        if (record->opcode == 0x131) {
            if (global_assignment_is_redundant(record, record->left) &&
                !input_has_side_effects(record)) {
                if ((record->type & 0xf000) != 0x5000 ||
                    !((1U << (record->tail.right->kind & 31)) & 6)) {
                    remove_global_record(record);
                    record = NULL;
                } else if (record->tail.right->value.reg->kind != 3) {
                    remove_global_record(record);
                    record = NULL;
                }
            }
        } else if (record->opcode == 0x151 || record->opcode == 0x15e) {
            result = record->tail.right;
            if (record->opcode == 0x15e &&
                ((1U << (result->kind & 31)) & 6) &&
                result->value.reg->global_expression && !result->value.reg->value &&
                optimizer_set_contains(global_available_registers, result->value.reg->number)) {
                if (!input_has_side_effects(record)) {
                    remove_global_record(record);
                    record = NULL;
                }
            } else if (record->value.reg) {
                classification = classify_global_register(global_available_registers, record->value.reg);
                if (classification == 1 || classification == 2) {
                    take_first = classification == 1;
                    if (record->opcode == 0x15e) {
                        selected = record;
                        prior = record->left->value.reg->value;
                    } else {
                        selected = follow_optimizer_result(record, &operand);
                        /* The original retains its incoming function context
                         * in this slot until the 15e branch replaces it. */
                        prior = (C2Expression *)function;
                    }
                    if (selected->kind == 16) {
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
                        /* Original defect: the 15e path bypasses the fresh
                         * local operand writer, but this subpath reads it. */
                        if (record->opcode == 0x15e && take_first)
                            recon_unsupported("uninitialized global branch operand");
#endif
                        remove_statement_expression(selected, record);
                        remove_global_record(record);
                        if (take_first) {
                            destination = block->previous;
                            selected->opcode = 0x15a;
                            remove_input_operand(selected, operand);
                            selected->convention = 0;
                        } else {
                            symbol = (C2InputSymbol *)selected->left->value.symbol;
                            label = symbol->branch_target;
                            destination = label->block;
                            if (!destination) destination = get_symbol_branch_target(label, 24)->block;
                            if (global_next_record == selected) global_next_record = selected->next;
                            remove_global_record(selected);
                        }
                        remove_global_successor(block, destination, function);
                        record = NULL;
                    } else if (selected->kind == 15) {
                        if (take_first) replacement = selected->left->next;
                        else replacement = selected->left->next->next;
                        record = new_binary_expression(0x131, selected->type,
                            selected->tail.right, replacement, insert_input_record, (C2Record *)selected);
                        finish_global_assignment(record, 1);
                        result = record->tail.right;
                        if (((1U << (result->kind & 31)) & 6) && result->value.reg->value) {
                            reg = result->value.reg;
                            if (!reg->canonical->variant_next) reg->value = record;
                            else bind_input_temporary(reg, record);
                        }
                        remove_global_record(prior);
                        remove_global_record(selected);
                        global_next_record = record->next;
                    }
                }
            }
        } else {
            result = record->tail.right;
            if (result && ((1U << (result->kind & 31)) & 6) &&
                result->value.reg->global_expression &&
                global_register_is_available(result->value.reg) && !input_has_side_effects(record)) {
                if (record->precision_flags != 3) {
                    remove_global_record(record);
                    record = NULL;
                } else if (global_pending_assignment != record) record = NULL;
            }
        }
        if (!record) return;
        substitute_available_global_operands(record);
    }
    rewrite = global_record_needs_rewrite(record);
    if (rewrite || global_pending_assignment == record) {
        if (global_pending_assignment == record) global_pending_assignment = NULL;
        record = prepare_global_record(record, block, function);
        if (!record) return;
        update_global_record_registers(record, 0);
        if (global_region_limited || (global_block_state && record->opcode == 0x151)) {
            record = optimize_limited_global_record(record, block, function);
            if (!record) return;
        }
    } else if (record->opcode == 0x151) {
        record = prepare_global_record(record, block, function);
        if (!record) return;
        update_global_record_registers(record, 0);
    }
    if (global_region_motion_enabled && record->opcode == 0x131 &&
        record->precision_flags != 1 && ((1U << (record->tail.right->kind & 31)) & 6)) {
        record->value.reg->global_definition_record = record;
        if (record->precision_flags == 7 || record->precision_flags == 8) {
            set = new_temporary_register_set();
            copy_optimizer_symbol_set_into(set, global_available_registers);
            prepend_input_record((C2Record *)record, new_global_register_set_record(set));
        }
    }
    update_global_available_registers(record);
}

void __fastcall finish_global_block_pass(C2Function *function)
{
    C2Record *record, *next, *last;

    last = function->flow->first->last_record;
    record = function->flow->last->records->next;
    while (record != last) {
        next = record->next;
        if (record->kind == 26) {
            release_optimizer_symbol_set(record->owner.registers);
            remove_global_record((C2Expression *)record);
        }
        record = next;
    }
}

void __fastcall clear_global_block_marks(C2Function *function)
{
    C2Expression *record, *next, *last;

    last = (C2Expression *)function->flow->first->last_record;
    record = (C2Expression *)function->flow->last->records->next;
    while (record != last) {
        next = record->next;
        if (record->precision_flags == 1) unlink_and_release_input_record((C2Record *)record);
        record = next;
    }
}

void __fastcall clear_global_register_marks(C2Function *function)
{
    C2Expression *record, *next, *last;

    last = (C2Expression *)function->flow->first->last_record;
    record = (C2Expression *)function->flow->last->records->next;
    while (record != last) {
        next = record->next;
        if (record->precision_flags == 2) unlink_and_release_input_record((C2Record *)record);
        record = next;
    }
}

void __fastcall solve_global_register_sets(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end;
    C2FlowEdge *edge;
    int changed;

    end = last->previous;
    for (block = first; block != end; block = block->previous)
        initialize_global_available_sets(block);
    for (block = first; block != end; block = block->previous) {
        if (!block->block_scope || (block->block_scope->flags & 1)) continue;
        for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
            if ((short)block->number84 >= (short)edge->destination->number84) {
                fill_global_available_set(block->global_definitions, next_function_register_number);
                break;
            }
        }
    }
    do {
        changed = 0;
        for (block = first; block != end; block = block->previous)
            update_global_available_sets(block, &changed);
    } while (changed);
}

/* These three current-block pointers are assigned before each traversal. */
C2RegisterSet *global_available_generations; /* 49722c, block+68. */
C2RegisterSet *global_available_kills;       /* 497224, block+72. */
C2RegisterSet *global_available_uses;        /* 497228, block+76. */

void __fastcall initialize_global_available_sets(C2BasicBlock *block)
{
    C2Expression *record;

    global_available_generations = block->rewrite_mode4[0];
    global_available_kills = block->rewrite_mode4[1];
    global_available_uses = block->rewrite_mode4[2];
    clear_register_set(global_available_generations);
    clear_register_set(global_available_kills);
    clear_register_set(global_available_uses);
    for (record = (C2Expression *)block->records->next;
         record != (C2Expression *)block->last_record; record = record->next)
        if (record->flags & 1) collect_global_available_record(record, block);
}

void __fastcall collect_global_available_record(C2Expression *record, C2BasicBlock *block)
{
    C2Expression *output, *operand;
    C2FunctionRegister *reg, *dependent, *canonical, *variant, *saved;
    C2GlobalDependency *link;
    C2InputSymbol *symbol;
    C2RegisterSet *effects;
    unsigned was_available, now_available, number, opcode;
    int keep_assignment;

    ++global_register_state;
    opcode = record->opcode;
    saved = NULL;
    if (opcode == 0x131 && record->tail.right->kind == 6) {
        operand = record->tail.right->aggregate_value;
        if (!GLOBAL_REGISTER_OPERAND(operand) || !operand->value.reg->value) {
            saved = operand->value.reg;
            was_available = optimizer_set_contains(global_available_generations, saved->number);
            add_register_to_set(global_available_generations, saved->number);
            remove_optimizer_symbol(global_available_kills, saved->number);
        }
    }
    for (output = record->tail.right; output; output = output->next) {
        switch (output->kind) {
        case 1: case 2:
            if (record->precision_flags == 1) break;
            reg = output->value.reg;
            if (!reg->global_expression || (reg->rewrite_flags & 1) || record->precision_flags == 14)
                invalidate_global_available_register(reg, NULL);
            else {
                for (link = reg->global_dependents; link; link = link->next_dependent) {
                    dependent = link->dependent;
                    if (dependent->global_expression &&
                        (dependent->global_expression->opcode == 0x131 ||
                         operation_is_comparison(dependent->global_expression->opcode))) {
                        number = dependent->number;
                        add_register_to_set(global_available_kills, number);
                        add_register_to_set(global_available_uses, number);
                        remove_optimizer_symbol(global_available_generations, number);
                    }
                }
                canonical = reg->canonical;
                for (variant = canonical; variant; variant = variant->variant_next) {
                    if (variant != reg &&
                        variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
                        reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes))
                        invalidate_global_available_variants(variant, canonical);
                }
            }
            if (reg->kind == 7 || reg->kind == 8 || (reg->view_flags & 2)) {
                symbol = (C2InputSymbol *)reg->canonical->symbol;
                effects = global_symbol_registers[symbol->detail.optimizer_number];
                merge_optimizer_symbol_sets(global_available_uses, effects);
                merge_optimizer_symbol_sets(global_available_kills, effects);
                subtract_optimizer_symbol_sets(global_available_generations, effects);
            }
            break;
        case 6:
            effects = global_index_registers[output->immediate];
            merge_optimizer_symbol_sets(global_available_uses, effects);
            merge_optimizer_symbol_sets(global_available_kills, effects);
            subtract_optimizer_symbol_sets(global_available_generations, effects);
            number = output->value.reg->number;
            add_register_to_set(global_available_generations, number);
            remove_optimizer_symbol(global_available_kills, number);
            break;
        case 9:
            c2_report_internal("E:\\utc\\src\\\\P2\\globdf.c", 0xf3d);
            break;
        case 10:
            effects = global_index_registers[output->value.optimizer_index];
            merge_optimizer_symbol_sets(global_available_uses, effects);
            merge_optimizer_symbol_sets(global_available_kills, effects);
            subtract_optimizer_symbol_sets(global_available_generations, effects);
            break;
        }
    }
    output = record->tail.right;
    keep_assignment = 1;
    if (output) {
        if (GLOBAL_REGISTER_OPERAND(output) && output->value.reg->global_expression) {
            number = output->value.reg->number;
            add_register_to_set(global_available_generations, number);
            remove_optimizer_symbol(global_available_kills, number);
        } else if (saved) {
            number = saved->number;
            now_available = optimizer_set_contains(global_available_generations, number);
            keep_assignment = now_available != 0;
            if (!was_available && now_available) {
                add_register_to_set(global_available_kills, number);
                add_register_to_set(global_available_uses, number);
                remove_optimizer_symbol(global_available_generations, number);
            }
        }
    }
    if (opcode == 0x131 && keep_assignment) {
        number = record->value.reg->number;
        add_register_to_set(global_available_generations, number);
        remove_optimizer_symbol(global_available_kills, number);
    }
}

void __fastcall update_global_available_sets(C2BasicBlock *block, int *changed)
{
    C2FlowEdge *edge;

    edge = block->incoming;
    if (edge) {
        copy_optimizer_symbol_set_into(block->global_available_in, edge->source->global_definitions);
        for (edge = edge->next_incoming; edge; edge = edge->next_incoming)
            intersect_optimizer_symbol_sets(block->global_available_in, edge->source->global_definitions);
    }
    update_available_register_transfer(block->global_definitions, block->rewrite_mode4[0],
        block->global_available_in, block->rewrite_mode4[1], changed);
}

void __fastcall update_available_register_transfer(C2RegisterSet *destination,
    C2RegisterSet *generated, C2RegisterSet *input, C2RegisterSet *killed, int *changed)
{
    C2RegisterSet *saved;
    C2RegisterSetNode *source, *spare, *node, *next;

    if (*changed) {
        update_optimizer_alias_set(destination, input, killed, generated);
        return;
    }
    saved = new_register_set(destination->pool);
    source = destination->links.first;
    spare = saved->links.first;
    node = spare;
    while (source) {
        if (spare) {
            node = spare;
            spare = spare->next;
        } else node = insert_register_set_node(saved, node);
        node->group = source->group;
        node->bits = source->bits;
        source = source->next;
    }
    while (spare) {
        next = spare->next;
        remove_register_set_node(saved, spare);
        spare = next;
    }
    update_optimizer_alias_set(destination, input, killed, generated);
    *changed = !optimizer_symbol_sets_equal(destination, saved);
    release_optimizer_symbol_set(saved);
}

/* Eleven contiguous temporary-set pointer owners at 497230..49725c.
 * Their initializer overwrites each cell before the region pass uses it. */
C2RegisterSet *region_motion_sets[11];

void initialize_region_motion_sets(void)
{
    C2RegisterBlock *block;
    C2FunctionRegister *reg;

    for (block = first_function_register_block; block; block = block->next) {
        for (reg = block->records; reg < block->cursor; ++reg) {
            if (reg->kind == 16) continue;
            reg->rewrite_flags &= ~6;
            reg->global_definitions = NULL;
            reg->global_uses = NULL;
        }
    }
    region_motion_sets[9] = new_temporary_register_set();
    region_motion_sets[5] = new_temporary_register_set();
    region_motion_sets[0] = new_temporary_register_set();
    region_motion_sets[4] = new_temporary_register_set();
    region_motion_sets[2] = new_temporary_register_set();
    region_motion_sets[3] = new_temporary_register_set();
    region_motion_sets[10] = new_temporary_register_set();
    region_motion_sets[7] = new_temporary_register_set();
    region_motion_sets[1] = new_temporary_register_set();
    region_motion_sets[6] = new_temporary_register_set();
    region_motion_sets[8] = new_temporary_register_set();
}

void release_region_motion_sets(void)
{
    release_optimizer_symbol_set(region_motion_sets[9]);
    release_optimizer_symbol_set(region_motion_sets[5]);
    release_optimizer_symbol_set(region_motion_sets[0]);
    release_optimizer_symbol_set(region_motion_sets[3]);
    release_optimizer_symbol_set(region_motion_sets[10]);
    release_optimizer_symbol_set(region_motion_sets[4]);
    release_optimizer_symbol_set(region_motion_sets[2]);
    release_optimizer_symbol_set(region_motion_sets[7]);
    release_optimizer_symbol_set(region_motion_sets[1]);
    release_optimizer_symbol_set(region_motion_sets[6]);
    release_optimizer_symbol_set(region_motion_sets[8]);
}

void __fastcall finish_region_motion(C2Function *function)
{
    C2Expression *record;
    C2BasicBlock *block;

    /* The original carries the incoming function pointer until a block marker
     * supplies the current block. Do not invent a NULL default for it. */
    block = (C2BasicBlock *)function;
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (record->kind == 24) {
            block = ((C2Record *)record)->owner.block;
        } else if (record->opcode == 0x151) finish_region_selection(record, block);
    }
}

void __fastcall prepare_global_function_cleanup(C2Function *function)
{
    C2FlowGraph *flow;

    flow = function->flow;
    global_rewrite_state = 0;
    clear_global_rewrite_marks(flow->last, flow->first);
    global_rewrite_exclusions = new_global_exclusion_set(6);
    prepare_global_loop(flow->last, flow->first);
    prepare_global_cleanup_records(function);
    release_optimizer_symbol_set(global_rewrite_exclusions);
}

void __fastcall prepare_global_loop(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end;

    end = first->next;
    for (block = last; block != end; block = block->next)
        if (!(block->flags & 0x04000000U)) initialize_global_loop_block(block);
    solve_global_loop_sets(first, last);
}

/* Current block views, assigned by the initializer; original 49720c..49721c. */
C2RegisterSet *global_loop_record_sets[4];

void __fastcall initialize_global_loop_block(C2BasicBlock *block)
{
    C2Expression *record;

    global_loop_record_sets[1] = block->rewrite_mode1[0];
    global_loop_record_sets[2] = block->rewrite_mode1[1];
    global_loop_record_sets[0] = block->rewrite_mode1[4];
    global_loop_record_sets[3] = block->rewrite_mode1[5];
    clear_register_set(global_loop_record_sets[1]);
    clear_register_set(global_loop_record_sets[2]);
    clear_register_set(global_loop_record_sets[0]);
    clear_register_set(global_loop_record_sets[3]);
    clear_register_set(block->rewrite_mode1[2]);
    clear_register_set(block->rewrite_mode1[3]);
    clear_register_set(block->rewrite_mode1[6]);
    clear_register_set(block->rewrite_mode1[7]);
    if (!block->previous)
        copy_optimizer_symbol_set_into(block->rewrite_mode1[3], global_initial_registers);
    for (record = ((C2Expression *)block->last_record)->previous;
         record != (C2Expression *)block->records; record = record->previous)
        if (record->flags & 1) collect_global_loop_record(record, block);
    intersect_optimizer_symbol_sets(global_loop_record_sets[0], global_rewrite_exclusions);
    block->flags |= 0x04000000U;
}

void __fastcall collect_global_loop_record(C2Expression *record, C2BasicBlock *block)
{
    C2Expression *operand;
    C2FunctionRegister *reg, *variant;
    C2InputSymbol *symbol;
    C2RegisterSet *direct, *variants;
    unsigned number, index;

    if ((global_rewrite_state && record->precision_flags == 4) ||
        record->precision_flags == 2 || record->precision_flags == 1) return;
    for (operand = record->tail.right; operand; operand = operand->next) {
        if (!operand->kind || operand->kind >= 3 ||
            (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->value)) continue;
        reg = operand->value.reg;
        for (variant = reg->canonical; variant; variant = variant->variant_next) {
            if (reg->displacement <= variant->displacement &&
                (long)((unsigned)variant->displacement + variant->value_bytes) <=
                (long)((unsigned)reg->displacement + reg->value_bytes)) {
                number = variant->number;
                add_register_to_set(global_loop_record_sets[2], number);
                remove_optimizer_symbol(global_loop_record_sets[1], number);
                add_register_to_set(global_loop_record_sets[0], number);
                remove_optimizer_symbol(global_loop_record_sets[3], number);
            }
        }
        if (reg->global_expression && reg->global_expression->opcode == 0x122)
            mark_global_loop_dependents(reg);
    }
    for (operand = record->left; operand; operand = operand->next) {
        switch (operand->kind) {
        case 1: case 2:
            if (GLOBAL_REGISTER_OPERAND(operand) && operand->value.reg->value) break;
            reg = operand->value.reg;
            number = reg->number;
            add_register_to_set(global_loop_record_sets[1], number);
            remove_optimizer_symbol(global_loop_record_sets[2], number);
            for (variant = reg->canonical; variant; variant = variant->variant_next) {
                if (variant == reg ||
                    (variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
                     reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes))) {
                    number = variant->number;
                    remove_optimizer_symbol(global_loop_record_sets[0], number);
                    add_register_to_set(global_loop_record_sets[3], number);
                }
            }
            break;
        case 3:
            if (record->opcode != 0x130) break;
            reg = operand->value.reg;
            symbol = (C2InputSymbol *)reg->canonical->symbol;
            if (!symbol || symbol->symbol.kind != 1 || (symbol->attributes & 0x70) != 0x10 ||
                symbol->symbol.name || !(symbol->attributes & 0x40000)) break;
            for (variant = reg->canonical; variant; variant = variant->variant_next) {
                if (reg->displacement <= variant->displacement &&
                    (long)((unsigned)variant->displacement + variant->value_bytes) <=
                    (long)((unsigned)reg->displacement + reg->value_bytes)) {
                    number = variant->number;
                    add_register_to_set(global_loop_record_sets[2], number);
                    remove_optimizer_symbol(global_loop_record_sets[1], number);
                    add_register_to_set(global_loop_record_sets[0], number);
                    remove_optimizer_symbol(global_loop_record_sets[3], number);
                }
            }
            if (reg->global_expression && reg->global_expression->opcode == 0x122)
                mark_global_loop_dependents(reg);
            break;
        case 6: case 10:
            index = operand->kind == 6 ? operand->immediate : operand->value.optimizer_index;
            direct = global_index_direct_uses[index];
            variants = global_index_variant_uses[index];
            merge_optimizer_symbol_sets(global_loop_record_sets[1], direct);
            subtract_optimizer_symbol_sets(global_loop_record_sets[2], direct);
            merge_optimizer_symbol_sets(global_loop_record_sets[3], variants);
            subtract_optimizer_symbol_sets(global_loop_record_sets[0], variants);
            break;
        }
    }
}

void __fastcall solve_global_loop_sets(C2BasicBlock *first, C2BasicBlock *last)
{
    C2BasicBlock *block, *end;
    C2FlowEdge *edge;
    int changed;

    end = first->next;
    for (block = last; block != end; block = block->next) {
        if (!block->block_scope || (block->block_scope->flags & 1)) continue;
        for (edge = block->incoming; edge; edge = edge->next_incoming) {
            if ((short)block->number84 <= (short)edge->source->number84 &&
                edge->source->outgoing->next_outgoing) {
                copy_optimizer_symbol_set_into(block->rewrite_mode1[6], global_rewrite_exclusions);
                break;
            }
        }
    }
    do {
        changed = 0;
        for (block = last; block != end; block = block->next)
            update_global_loop_sets(block, &changed);
    } while (changed);
}

void __fastcall update_global_loop_sets(C2BasicBlock *block, int *changed)
{
    C2FlowEdge *edge;

    edge = block->outgoing;
    if (edge) {
        copy_optimizer_symbol_set_into(block->rewrite_mode1[3], edge->destination->rewrite_mode1[2]);
        copy_optimizer_symbol_set_into(block->rewrite_mode1[7], edge->destination->rewrite_mode1[6]);
        for (edge = edge->next_outgoing; edge; edge = edge->next_outgoing) {
            merge_optimizer_symbol_sets(block->rewrite_mode1[3], edge->destination->rewrite_mode1[2]);
            intersect_optimizer_symbol_sets(block->rewrite_mode1[7], edge->destination->rewrite_mode1[6]);
        }
    }
    update_available_register_transfer(block->rewrite_mode1[2], block->rewrite_mode1[0],
        block->rewrite_mode1[3], block->rewrite_mode1[1], changed);
    update_available_register_transfer(block->rewrite_mode1[6], block->rewrite_mode1[4],
        block->rewrite_mode1[7], block->rewrite_mode1[5], changed);
}

extern int global_rewrite_required;

void __fastcall prepare_global_cleanup_records(C2Function *function)
{
    C2RegisterBlock *register_block;
    C2FunctionRegister *reg;
    C2BasicBlock *block;
    C2Expression *record, *previous;
    C2RegisterSet *available, *work, *temporary;

    for (register_block = first_function_register_block; register_block;
         register_block = register_block->next) {
        for (reg = register_block->records; reg < register_block->cursor; ++reg) {
            if (reg->kind == 16) continue;
            reg->rewrite_seen = 0;
            reg->rewrite_queued = 0;
            reg->global_definitions = NULL;
            reg->global_uses = NULL;
        }
    }
    global_rewrite_required = 0;
    available = new_register_set(6);
    work = new_register_set(6);
    temporary = new_register_set(6);
    for (block = function->flow->first; block; block = block->next) {
        copy_optimizer_symbol_set_into(available, block->rewrite_mode1[3]);
        clear_register_set(temporary);
        record = ((C2Expression *)block->last_record)->previous;
        while (record != (C2Expression *)block->records) {
            previous = record->previous;
            if ((record->flags & 1) && record->kind != 20)
                rewrite_global_cleanup_record(record, available, work, temporary);
            record = previous;
        }
    }
}

void __fastcall restore_global_function(C2Function *function)
{
    C2Expression *record, *next, *last, *source, *result;

    last = (C2Expression *)function->flow->first->last_record;
    for (record = (C2Expression *)function->flow->last->records->next;
         record != last; record = next) {
        next = record->next;
        if (record->opcode == 0x133) {
            record->opcode = 0x131;
            source = record->left;
            if (source->kind == 7 || (source->location.expression.properties & 0x40)) continue;
        } else if (record->opcode == 0x138) {
            result = record->tail.right;
            source = record->left;
            if (GLOBAL_REGISTER_OPERAND(result) && result->value.reg->value &&
                GLOBAL_REGISTER_OPERAND(source) && source->value.reg->global_expression)
                record->opcode = 0x131;
        }
        if (record->opcode == 0x131 && !global_assignment_restored(record))
            restore_global_assignment(record);
    }
}

void release_global_optimizer(void)
{
    release_global_expression_cache();
    reset_input_pool(14);
}

void __fastcall fill_global_available_set(C2RegisterSet *set, unsigned count)
{
    C2RegisterSetNode *node = set->links.first, *previous = NULL;
    unsigned groups = (count + 31) >> 5;
    unsigned group, number = 0;

    for (group = 1; group < groups; ++group) {
        if (!node) node = insert_register_set_node(set, previous);
        node->group = number;
        number += 32;
        node->bits = ~0U;
        previous = node;
        node = node->next;
    }
    if (number < count) {
        if (!node) node = insert_register_set_node(set, previous);
        node->group = number;
        /* The original ORs the last partial group, retaining its other bits
         * and every later node. This is a fill operation, not a reset. */
        do { node->bits |= 1U << (number & 31); } while (++number < count);
    }
}

void __fastcall invalidate_global_available_register(C2FunctionRegister *reg,
    C2FunctionRegister *canonical)
{
    C2FunctionRegister *head, *variant;
    if (reg->global_invalidation_epoch == global_register_state) return;
    if (reg->global_dependency_set) {
        merge_optimizer_symbol_sets(global_available_uses, reg->global_dependency_set);
        merge_optimizer_symbol_sets(global_available_kills, reg->global_dependency_set);
        subtract_optimizer_symbol_sets(global_available_generations, reg->global_dependency_set);
        reg->global_invalidation_epoch = global_register_state;
    }
    head = reg->canonical;
    if (head == canonical) return;
    for (variant = head; variant; variant = variant->variant_next) {
        if (variant != reg &&
            variant->displacement < (long)((unsigned)reg->displacement + reg->value_bytes) &&
            reg->displacement < (long)((unsigned)variant->displacement + variant->value_bytes))
            invalidate_global_available_register(variant, head);
    }
}

int __fastcall global_operand_rejects_substitution(C2Expression *operand)
{
    if (!((1U << (operand->kind & 31)) & 6)) return 0;
    return optimizer_set_contains(region_motion_sets[2], operand->value.reg->number) != 0;
}

void __fastcall rewrite_region_comparison(C2Expression *record,
    C2FlowRegion *region, C2Function *function);
void __fastcall region_candidate_pass_3c87c(C2FlowRegion *region, C2Function *function)
{
    C2Expression *record, *consumer, *result, *left, *right, *operation;
    C2FunctionRegister *reg;
    C2GlobalExpression *expression;
    unsigned short line, type;
    unsigned opcode;
    set_region_motion_anchor(region);
    record = next_final_marker((C2Expression *)region->entry->records);
    line = record->location.line;
    begin_region_candidate_work();
    if (!collect_region_candidates(region, 2)) return;
    while ((record = pop_region_candidate()) != NULL) {
        consumer = follow_optimizer_result(record, &result);
        if (consumer->kind == 16) {
            if (record->location.line) {
                record_source_line = function->source_line_base + record->location.line;
                record_line_delta = record->location.line;
            }
            rewrite_region_comparison(record, region, function);
        }
    }
    for (reg = region_pending_registers; reg != (C2FunctionRegister *)-1; reg = reg->global_next) {
        if (line) {
            record_source_line = function->source_line_base + line;
            record_line_delta = line;
        }
        expression = reg->global_expression;
        type = expression->type;
        opcode = expression->opcode;
        right = expression->operands->next;
        left = materialize_region_operand(type, expression->operands, region_motion_anchor);
        right = materialize_region_operand(type, right, region_motion_anchor);
        operation = new_folded_binary_expression(canonical_region_opcode(opcode), type,
                                                 left, right, prepend_input_record,
                                                 region_motion_anchor);
        result = new_typed_register_expression(0x120, type, reg);
        new_binary_expression(0x131, type, result, operation->tail.right,
                              prepend_input_record, region_motion_anchor);
    }
}

/* This comparison-choice chain exposes only the fields consumed here.
 * The DWORD at +12 has no recovered role or default. */
typedef struct C2RegionComparisonChoice {
    C2FunctionRegister *reg;
    C2Expression *operand;
    unsigned opcode;
    unsigned char pending0c[4];
    struct C2RegionComparisonChoice *next;
} C2RegionComparisonChoice;
typedef char C2RegionComparisonChoiceNextOffset[
    offsetof(C2RegionComparisonChoice, next) == 16 ? 1 : -1];
C2Expression *__fastcall fold_region_invariant(C2Expression *operand, C2FlowRegion *region)
{
    C2Record *tail;
    C2Expression *value, *record;
    C2FunctionRegister *reg;
    unsigned short type;
    if (((1U << (operand->kind & 31)) & 0x180) || operand->kind == 3) return operand;
    clear_region_expression_list();
    tail = region_expression_tail;
    type = operand->type;
    value = materialize_region_operand(type, operand, tail);
    new_assignment_expression(0x131, type, value, prepend_input_record, tail);
    global_available_registers = region->rewrite_entry->global_available_in;
    for (record = (C2Expression *)region_expression_head;
         record != (C2Expression *)region_expression_tail->next; record = record->next)
        if (record->flags & 1) global_record_needs_rewrite(record);
    global_pending_assignment = NULL;
    record = (C2Expression *)tail->previous;
    reg = record->tail.right->value.reg;
    if (!reg->canonical->variant_next) reg->value = NULL;
    else bind_input_temporary(reg, NULL);
    simplify_region_statement(record);
    record = (C2Expression *)tail->previous;
    release_function_register(record->tail.right->value.reg);
    return record->left;
}
int __fastcall region_choice_dependency_is_valid(C2FunctionRegister *candidate,
    C2FunctionRegister *input)
{
    C2GlobalExpression *expression = candidate->global_expression;
    if (!expression || (expression->opcode != 0x14d && expression->opcode != 0x14f)) return 0;
    return expression->operands->value.reg == input;
}
C2Expression *__fastcall region_other_operand(C2FunctionRegister *reg, C2FunctionRegister *input)
{
    C2Expression *operand = reg->global_expression->operands;
    if (((1U << (operand->kind & 31)) & 6) && operand->value.reg == input)
        operand = operand->next;
    return operand;
}
int __fastcall region_register_has_external_use(C2FunctionRegister *reg)
{
    C2OptimizerRecordLink *use;
    for (use = reg->global_uses; use; use = use->next)
        if (use->record->precision_flags != 4) return 1;
    return 0;
}
int __fastcall score_region_dependency_chain(C2FunctionRegister *reg, C2FunctionRegister *input,
    unsigned depth)
{
    C2GlobalDependency *dependency;
    int best, score;
    if (region_operand_is_volatile(region_other_operand(reg, input))) return -1;
    best = region_register_has_external_use(reg) ? (int)depth : -1;
    for (dependency = reg->global_dependents; dependency; dependency = dependency->next_dependent) {
        if (!region_choice_dependency_is_valid(dependency->dependent, reg)) continue;
        score = score_region_dependency_chain(dependency->dependent, reg, depth + 1);
        if (score > best) best = score;
    }
    return best;
}
int __fastcall region_operation_is_safe(unsigned short type, unsigned opcode,
    C2Expression *left, C2Expression *right, C2FlowRegion *region);
C2Expression *__fastcall find_region_initial_expression(C2Expression *operand, C2FlowRegion *region);
C2RegionComparisonChoice *__fastcall new_region_comparison_choice(C2FunctionRegister *reg,
    C2Expression *operand, unsigned opcode, C2FunctionRegister *source);
C2RegionComparisonChoice *__fastcall next_region_comparison_choice(C2FlowRegion *region,
    C2RegionComparisonChoice *previous, C2FunctionRegister *reg, C2Expression **bound,
    unsigned short type, unsigned nonnegative, unsigned nonpositive)
{
    C2GlobalDependency *dependency;
    C2FunctionRegister *candidate, *best = reg;
    C2Expression *other, *value, *initial, *saved;
    C2RegionComparisonChoice *choice;
    unsigned opcode;
    int score, best_score = -1;
    /* Both actual callers set EDX to the preceding choice (or NULL).
     * This entry never consumes that register; its five stack slots remain. */
    for (dependency = reg->global_dependents; dependency; dependency = dependency->next_dependent) {
        candidate = dependency->dependent;
        if (!region_choice_dependency_is_valid(candidate, reg)) continue;
        opcode = canonical_region_opcode(candidate->global_expression->opcode);
        other = region_other_operand(candidate, reg);
        score = score_region_dependency_chain(candidate, reg, 1);
        if (score <= best_score) continue;
        if (region_operand_is_volatile(other) && !region_operand_is_volatile(*bound)) continue;
        if (opcode == 0x143 && other->kind == 7 && (other->type & 0xf000) == 0x1000 &&
            (long)other->integer.high < 0) nonnegative = nonpositive = 1;
        if (nonnegative && !region_operation_is_safe(type, opcode, *bound, other, region)) continue;
        if (nonpositive) {
            initial = new_typed_register_expression(0x120, reg->type, reg);
            initial = find_region_initial_expression(initial, region);
            if (!region_operand_is_invariant(initial) ||
                !region_operation_is_safe(type, opcode, initial, other, region)) continue;
        }
        value = fold_region_invariant(other, region);
        value = copy_input_expression(value);
        value = fold_region_binary_expression(*bound, value, opcode);
        if (value->opcode == 0x131) {
            saved = copy_input_expression(value->left);
            best_score = score;
            best = candidate;
        }
    }
    if (best == reg) return NULL;
    other = region_other_operand(best, reg);
    opcode = canonical_region_opcode(best->global_expression->opcode);
    choice = new_region_comparison_choice(best, other, opcode, reg);
    *bound = copy_input_expression(saved);
    return choice;
}
C2RegionComparisonChoice *__fastcall collect_region_comparison_choices(C2FunctionRegister *reg,
    C2Expression *bound, C2FlowRegion *region, unsigned short type,
    unsigned nonnegative, unsigned nonpositive)
{
    C2RegionComparisonChoice *head, *choice, *next;
    head = next_region_comparison_choice(region, NULL, reg, &bound, type,
                                         nonnegative, nonpositive);
    if (!head) return NULL;
    choice = head;
    while ((next = next_region_comparison_choice(region, choice, choice->reg, &bound,
                                                 type, nonnegative, nonpositive)) != NULL) {
        choice->next = next;
        choice = next;
    }
    return head;
}
C2Expression *__fastcall build_region_comparison_operand(unsigned short type,
    unsigned opcode, C2Expression *left, C2Expression *right, C2FlowRegion *region);
int __fastcall region_definition_is_available(C2Expression *record, C2FunctionRegister *reg);
int __fastcall final_replacement_is_blocked(C2Expression *first,
    C2Expression *last, C2Expression *operand);
void __fastcall rewrite_region_comparison(C2Expression *record,
    C2FlowRegion *region, C2Function *function)
{
    C2Expression *bound, *induction, *definition, *update, *offset, *value, *input, *consumer;
    C2FunctionRegister *reg, *replacement;
    C2RegionComparisonChoice *choice;
    unsigned short type;
    unsigned update_opcode, opcode, nonnegative, nonpositive, condition;
    long delta;
    initialize_region_expression_list();
    if (global_operand_is_restricted(record->left)) {
        bound = record->left->next;
        induction = record->left;
    } else {
        bound = record->left;
        induction = bound->next;
    }
    reg = induction->value.reg;
    definition = reg->global_definitions->record;
    if (definition->precision_flags != 4) return;
    update = definition->previous;
    offset = NULL;
    update_opcode = 0;
    if (update->precision_flags == 4 && (update->opcode == 0x141 || update->opcode == 0x142)) {
        offset = update->left->next;
        update_opcode = update->opcode;
    }
    nonnegative = nonpositive = 1;
    if (offset && region_operand_is_invariant(offset)) {
        value = fold_region_invariant(offset, region);
        if (value->kind == 7) {
            delta = (long)value->integer.low;
            if (update->opcode == 0x142) delta = (long)(0U - (unsigned)delta);
            nonnegative = delta >= 0;
            nonpositive = delta <= 0;
        }
    }
    choice = collect_region_comparison_choices(reg, bound, region, record->type,
                                                nonnegative, nonpositive);
    if (!choice) return;
    type = record->type;
    do {
        replacement = choice->reg;
        input = new_typed_register_expression(0x120, type, replacement);
        opcode = choice->opcode;
        value = build_region_comparison_operand(type, opcode, bound, choice->operand, region);
        induction = attach_input_expression(record, induction, input);
        bound = attach_input_expression(record, bound, value);
        if (offset) {
            if (opcode == 0x143)
                offset = build_region_comparison_operand(offset->type, opcode, offset,
                                                         choice->operand, region);
            else offset = copy_input_expression(offset);
        }
        if (opcode == 0x143 && (record->type & 0xf000) == 0x1000 &&
            choice->operand->kind == 7 && (choice->operand->type & 0xf000) == 0x1000 &&
            (long)choice->operand->integer.high < 0) {
            global_expression_condition(record);
            consumer = follow_optimizer_result(record, &input);
            condition = consumer->convention;
            /* The already recovered BYTE column at48d250 exchanges order. */
            if (condition > 10) { __asm { int 3 } }
            consumer->convention = condition <= 2 ? condition :
                (condition & 1) ? condition + 1 : condition - 1;
        }
        if (!final_replacement_is_blocked((C2Expression *)region->rewrite_entry->records,
                (C2Expression *)region->rewrite_entry->last_record, induction)) {
            global_available_registers = region_motion_sets[6];
            global_record_needs_rewrite(record);
            if (find_register_input(record, replacement) &&
                !region_definition_is_available(record, replacement)) {
                if (!offset) c2_report_internal("E:\\utc\\src\\\\P2\\globlopt.c", 0xe6d);
                new_region_iteration(induction, type, offset, update_opcode, reg, 0, region, function);
            }
        }
        update_global_record_registers(record, 0);
        choice = choice->next;
    } while (choice);
    release_region_expression_list();
}

int __fastcall compare_region_residues(C2Expression *value, C2Expression *initial, unsigned opcode);
C2Expression *__fastcall combine_region_offsets(C2Expression *other, C2Expression *operand);
int __fastcall rewrite_region_aggregate_use(C2Expression *record, C2Expression *owner,
    C2Expression *operand, unsigned opcode, C2Expression *source, C2Expression *initial, unsigned apply);
void __fastcall clear_region_value_uses(C2FunctionRegister *reg);
void __fastcall remove_region_update_use(C2FunctionRegister *reg, C2Expression *record);
void __fastcall rewrite_region_limit(C2Expression *record, C2Expression *bound,
    C2Expression *initial, unsigned opcode, C2Expression *source);
void __fastcall materialize_region_saved_value(C2Expression *operand, C2FlowRegion *region);
C2Expression *__fastcall materialize_region_aggregate(C2Expression *record, C2Expression *operand);
void __fastcall prune_detached_operands(C2Expression *record);
void __fastcall finish_restored_expression(C2Expression *record);
void __fastcall region_candidate_pass_3bbb3(C2FlowRegion *region)
{
    C2Expression *candidate, *record, *source, *initial, *value, *input, *other, *next;
    C2Expression *assignment, *comparison, *owner, *first, *aggregate, *materialized;
    C2Expression *last_operation;
    C2FunctionRegister *reg, *input_reg;
    C2GlobalExpression *expression;
    C2OptimizerRecordLink *use;
    unsigned opcode;
    int valid, compatible;
#ifdef __linux__
    int last_operation_written = 0;
#endif

    begin_region_candidate_work();
    initialize_region_expression_list();
    collect_region_candidates(region, 6);
    while ((candidate = pop_region_candidate()) != NULL) {
        reg = candidate->tail.right->value.reg;
        expression = reg->global_expression;
        source = expression->operands;
        input_reg = source->value.reg;
        if (!input_reg->global_uses) continue;
        initial = find_region_initial_constant(source->next, region);
        opcode = canonical_region_opcode(expression->opcode);
        valid = 1;
        assignment = comparison = NULL;
        for (use = reg->global_uses; use && valid; use = use->next) {
            record = use->record;
            if (record->precision_flags == 4) continue;
            if (record->opcode == 0x131 &&
                ((1U << (record->left->kind & 31)) & 6) && record->left->value.reg == reg) {
                if (assignment || (input_reg->rewrite_flags & 1)) valid = 0;
                assignment = record;
                continue;
            }
            if (record->opcode == 0x151) {
                input = record->left;
                if (region_operand_is_invariant(input->next)) {
                    value = fold_region_invariant(input->next, region);
                    compatible = value->kind == 7 && initial->kind == 7 &&
                        compare_region_residues(value, initial, opcode);
                    if (!compatible)
                        compatible = opcode == 0x141 && value->kind == 3 && initial->kind == 3 &&
                            value->value.reg->canonical == initial->value.reg->canonical;
                    if (compatible) continue;
                } else if (((1U << (input->kind & 31)) & 6) && input->value.reg == reg) {
                    if (comparison || (input_reg->rewrite_flags & 1)) valid = 0;
                    comparison = record;
                    continue;
                }
            }
            if ((record->type & 0xf000) == 0x5000) {
                valid = 0;
                break;
            }
            if (record->opcode == 0x141 && (record->type & 0xfff) == 4) {
                other = record->left;
                for (input = other; input; input = input->next) {
                    if (((1U << (input->kind & 31)) & 6) && input->value.reg == reg &&
                        !(input->location.expression.properties & 0x20)) {
                        if (other == input) other = other->next;
                        value = combine_region_offsets(other, input);
                        if (value) {
                            record->opcode = 0x13d;
                            remove_input_operand(record, input);
                            attach_input_expression(record, other, value);
                        }
                        break;
                    }
                }
            }
            for (input = record->left; input; input = input->next) {
                if (!((1U << (input->kind & 31)) & 6) || input->value.reg != reg) continue;
                if (input->location.expression.properties & 0x20) {
                    owner = find_aggregate_operand_owner(record, input, &first);
                    if (rewrite_region_aggregate_use(record, owner, input, opcode, source, initial, 0))
                        continue;
                }
                valid = 0;
            }
        }
        if (comparison && (!assignment || comparison->previous != assignment ||
                           compare_input_operands(assignment->left, comparison->left))) valid = 0;
        if (!valid) {
            clear_region_value_uses(reg);
            continue;
        }
        while ((record = pop_region_register_use(reg)) != NULL) {
            if (record->precision_flags == 4) {
                remove_region_update_use(reg, record->next);
                unlink_and_release_input_record((C2Record *)record->next);
                unlink_and_release_input_record((C2Record *)record);
            } else if (record->opcode == 0x131 &&
                       ((1U << (record->left->kind & 31)) & 6) && record->left->value.reg == reg) {
                last_operation = new_region_binary_expression(opcode, record->type, source, initial,
                                                              prepend_input_record, (C2Record *)record);
#ifdef __linux__
                last_operation_written = 1;
#endif
                attach_input_expression(record, record->left, last_operation->tail.right);
                add_region_register_use(input_reg, last_operation);
                defer_region_candidate(record);
            } else if (record->opcode == 0x151 &&
                       ((1U << (record->left->kind & 31)) & 6) && record->left->value.reg == reg) {
                if (!comparison) {
                    value = fold_region_invariant(record->left->next, region);
                    rewrite_region_limit(record, value, initial, opcode, source);
                } else {
                    attach_input_expression(record, record->left, assignment->tail.right);
                    if (global_operand_is_restricted(assignment->tail.right)) {
                        /* Original reuses its +3c local from the latest assignment
                         * constructor, even if no such write has occurred yet.
                         * Preserve the carried/unwritten Win32 pointer, not NULL. */
#ifdef __linux__
                        if (!last_operation_written)
                            recon_unsupported("region comparison incidental stack pointer");
#endif
                        add_region_register_use(assignment->tail.right->value.reg, last_operation);
                    }
                    defer_region_candidate(record);
                }
            } else {
                for (input = record->left; input; input = next) {
                    next = input->next;
                    if (!((1U << (input->kind & 31)) & 6) || input->value.reg != reg) continue;
                    owner = find_aggregate_operand_owner(record, input, &first);
                    rewrite_region_aggregate_use(record, owner, input, opcode, source, initial, 1);
                    defer_region_candidate(record);
                }
            }
        }
        prune_input_tree(candidate, 1);
        if (initial->kind == 2 && initial->value.reg &&
            ((initial->value.reg->rewrite_flags & 2) ||
             optimizer_set_contains(global_motion_outputs, initial->value.reg->number)))
            materialize_region_saved_value(initial, region);
    }
    while ((record = pop_deferred_region_candidate()) != NULL) {
        prune_detached_operands(record);
        switch (record->opcode) {
        case 0x131:
            aggregate = record->tail.right;
            if (((1U << (aggregate->kind & 31)) & 0x60) &&
                ((aggregate->aggregate_value && global_operand_is_restricted(aggregate->aggregate_value)) ||
                 (aggregate->aggregate_second && global_operand_is_restricted(aggregate->aggregate_second)))) {
                materialized = materialize_region_aggregate(record, aggregate);
                aggregate->opcode = 0x122;
                materialized->location.expression.properties |= 0x20;
                aggregate->tail.right = NULL;
                if (aggregate->aggregate_value) release_small_node(aggregate->aggregate_value);
                aggregate->aggregate_value = retain_input_result(materialized);
                if (aggregate->aggregate_second) release_small_node(aggregate->aggregate_second);
                aggregate->aggregate_second = NULL;
                aggregate->detail.data = NULL;
                aggregate->location.expression.properties &= 0xf0;
            }
            if (!((1U << (record->left->kind & 31)) & 0x60)) break;
            /* An aggregate input uses the same cleanup as opcode133. */
        case 0x133:
            aggregate = record->left;
            if ((aggregate->aggregate_value && global_operand_is_restricted(aggregate->aggregate_value)) ||
                (aggregate->aggregate_second && global_operand_is_restricted(aggregate->aggregate_second))) {
                materialized = materialize_region_aggregate(record, aggregate);
                aggregate->opcode = 0x122;
                materialized->location.expression.properties |= 0x20;
                aggregate->tail.right = NULL;
                if (aggregate->aggregate_value) release_small_node(aggregate->aggregate_value);
                aggregate->aggregate_value = retain_input_result(materialized);
                if (aggregate->aggregate_second) release_small_node(aggregate->aggregate_second);
                aggregate->aggregate_second = NULL;
                aggregate->detail.data = NULL;
                aggregate->location.expression.properties &= 0xf0;
            }
            break;
        case 0x13d:
            materialized = materialize_region_aggregate(record, record->left);
            attach_input_expression(record, record->left, materialized);
            record->opcode = 0x131;
            break;
        case 0x151:
            break;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\globlopt.c", 0x1f36);
            break;
        }
        finish_restored_expression(record);
        update_global_record_registers(record, 0);
    }
    prepare_region_motion_assignments(region, 1);
    release_region_expression_list();
}

C2Expression *__fastcall new_region_iteration_counter(C2FlowRegion *region, C2Function *function)
{
    C2Expression *counter = region_iteration_counter, *record, *value;
    if (!counter) {
        counter = new_input_temporary(0x2004);
        counter->value.reg->rewrite_flags |= 1;
        add_register_to_set(region_motion_sets[9], counter->value.reg->number);
        set_region_motion_anchor(region);
        record = next_final_marker((C2Expression *)region->entry->records);
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record->location.line;
        }
        value = materialize_region_operand(0x2004, region->iteration_count, region_motion_anchor);
        new_binary_expression(0x131, 0x2004, counter, value, prepend_input_record, region_motion_anchor);
        region_iteration_counter = retain_input_result(counter);
    }
    return counter;
}
void __fastcall region_candidate_pass_3e5ed(C2FlowRegion *region, C2Function *function)
{
    C2Expression *comparison, *update, *step, *counter, *operation, *assignment, *consumer, *result;
    C2FunctionRegister *reg;
    C2OptimizerRecordLink *use;
    if (!region->iteration_count) return;
    comparison = region->iteration_comparison;
    reg = comparison->left->value.reg;
    if (!comparison->left->next) {
        update = find_region_address_update(reg);
        step = update->left->next;
        if (update->opcode == 0x141) {
            if (step->kind == 7 && (step->integer.low & step->integer.high) == 0xffffffffU) return;
        } else if (step->kind == 7 && step->integer.low == 1 && !step->integer.high) return;
    }
    for (use = reg->global_uses; use; use = use->next)
        if (use->record->precision_flags != 4 && use->record != comparison) return;
    if (!reg->global_definitions || reg->global_definitions->next) return;
    counter = new_region_iteration_counter(region, function);
    if (comparison->location.line) {
        record_line_delta = comparison->location.line;
        record_source_line = function->source_line_base + comparison->location.line;
    }
    step = new_integer_expression(0x2004, 1);
    operation = new_folded_binary_expression(0x142, 0x2004, counter, step,
                                            prepend_input_record, (C2Record *)comparison);
    operation->precision_flags = 4;
    add_region_register_use(reg, operation);
    assignment = new_binary_expression(0x131, 0x2004, counter, operation->tail.right,
                                       prepend_input_record, (C2Record *)comparison);
    assignment->precision_flags = 4;
    add_region_register_definition(reg, assignment);
    comparison->type = 0x2004;
    attach_input_expression(comparison, comparison->left, counter);
    step = new_integer_expression(0x2004, 0);
    attach_input_expression(comparison, comparison->left->next, step);
    consumer = follow_optimizer_result(comparison, &result);
    consumer->convention = 4;
    comparison->value.reg = new_global_expression_register(0x155, 0x2004,
                                                          comparison->left, comparison->left->next);
    prepare_region_motion_assignments(region, 3);
}

C2Expression *__fastcall new_region_typed_counter(C2FlowRegion *region,
    unsigned short type, C2Function *function);
void __fastcall region_candidate_pass_3e73e(C2FlowRegion *region, C2Function *function)
{
    C2FunctionRegister *reg;
    C2OptimizerRecordLink *definitions, *uses;
    C2Expression *update, *assignment, *operand, *step, *counter, *operation, *inserted;
    unsigned number;
    unsigned short type;
    int changed = 0, two_uses;
    if (!region->iteration_count) return;
    begin_region_candidate_work();
    set_region_motion_anchor(region);
    for (number = next_optimizer_symbol(region_motion_sets[9]); number != (unsigned)-1;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        definitions = reg->global_definitions;
        if (!definitions || definitions->next) continue;
        uses = reg->global_uses;
        if (!uses) continue;
        assignment = definitions->record;
        if (!uses->next) {
            update = uses->record;
            two_uses = 0;
        } else {
            if (uses->next->next ||
                !region_candidate_link_contains(uses, region->iteration_comparison)) continue;
            update = uses->record;
            if (update == region->iteration_comparison) update = uses->next->record;
            two_uses = 1;
        }
        if (update->precision_flags != 4 || assignment->precision_flags != 4 ||
            update->opcode != 0x141 || assignment->opcode != 0x131 ||
            !region_operand_overlaps_motion_inputs(assignment->tail.right) ||
            !compare_region_iteration_order(assignment, region->iteration_comparison)) continue;
        type = assignment->type;
        operand = assignment->tail.right;
        step = update->left->next;
        retarget_region_operand(step, type);
        counter = new_region_typed_counter(region, type, function);
        operation = new_folded_binary_expression(0x143, type, counter, step,
                                                 prepend_input_record, region_motion_anchor);
        operation = new_folded_binary_expression(update->opcode, type, operand,
                                                 operation->tail.right, prepend_input_record,
                                                 region_motion_anchor);
        inserted = new_binary_expression(assignment->opcode, type, operand, operation->tail.right,
                                         prepend_input_record, region_motion_anchor);
        changed = 1;
        if (two_uses) region_deferred_assignment = inserted;
        else {
            remove_region_update_use(reg, assignment);
            remove_region_register_use(reg, update);
            mark_region_candidate_seen(update);
            mark_region_candidate_seen(assignment);
        }
    }
    finish_region_candidate_registers();
    if (changed) region_candidate_pass_3c245(region, function);
}

void __fastcall region_candidate_pass_3e91d(C2FlowRegion *region)
{
    C2BasicBlock *entry;
    C2Expression *record, *result, *definition, *update, *copy, *value;
    C2OptimizerRecordLink *definitions, *link;
    if (!region->iteration_count) return;
    entry = region->rewrite_entry;
    for (record = (C2Expression *)entry->records->next;
         record != (C2Expression *)entry->last_record; record = record->next) {
        if (record->opcode != 0x131) continue;
        result = record->tail.right;
        if (!global_operand_is_restricted(result)) continue;
        definitions = result->value.reg->global_definitions;
        if (!definitions || definitions->next) continue;
        definition = definitions->record;
        if (definition->precision_flags != 4) continue;
        update = definition->previous;
        if (update->precision_flags != 4 || (update->opcode != 0x141 && update->opcode != 0x142) ||
            !compare_region_iteration_order(update, region->iteration_comparison)) continue;
        link = c2_pool_allocate(13, sizeof(*link));
        copy = copy_input_expression(update);
        value = find_region_initial_constant(copy->left->next, region);
        if (value != copy->left->next) attach_input_expression(copy, copy->left->next, value);
        link->record = copy;
        link->next = region->initial_updates;
        region->initial_updates = link;
    }
}

void __fastcall region_candidate_pass_3e9fe(C2FlowRegion *region, C2Function *function)
{
    C2FlowRegion *child;
    C2Expression *candidate, *result, *matching, *update, *step, *initial, *assignment;
    C2FunctionRegister *reg;
    for (child = region->children; child; child = child->next) {
        if (!child->initial_updates) continue;
        begin_region_candidate_work();
        collect_region_candidates(child, 5);
        while ((candidate = pop_region_candidate()) != NULL) {
            result = candidate->tail.right;
            matching = find_region_matching_value(result, child);
            reg = candidate->left->value.reg;
            update = find_region_address_update(reg);
            if (!update || matching->opcode != update->opcode) continue;
            step = matching->left->next;
            initial = find_region_initial_constant(update->left->next, region);
            if (step->kind != 7 || initial->kind != 7 || child->iteration_count->kind != 7) continue;
            if (step->integer.value * child->iteration_count->integer.value != initial->integer.value)
                continue;
            assignment = update->next;
            attach_input_expression(assignment, assignment->left, result);
            remove_region_register_use(reg, update);
            unlink_and_release_input_record((C2Record *)update);
            update_global_record_registers(assignment, 0);
        }
    }
}

void __fastcall release_function_flow_edge(C2Function *function, C2FlowEdge *edge);
void __fastcall region_candidate_pass_3ea57(C2FlowRegion *region, C2Function *function)
{
    C2BasicBlock *first_block = region->header, *last_block = region->last, *block;
    C2Expression *record, *comparison, *operand, *consumer, *result;
    C2FunctionRegister *reg;
#ifdef __linux__
    int record_written = 0;
#endif
    if (!region_motion_sets[9]->links.first) return;
    for (block = first_block; block != last_block->previous; block = block->previous) {
        for (record = (C2Expression *)block->records->next;
#ifdef __linux__
             (record_written = 1),
#endif
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            if (record->precision_flags != 4 && record->precision_flags != 1 &&
                record->precision_flags != 2) goto inspect;
            if (record->opcode == 0x131 && region_operand_overlaps_motion_inputs(record->tail.right)) {
                if (!region_deferred_assignment) return;
                if (compare_input_operands(region_deferred_assignment->tail.right, record->tail.right))
                    goto cleanup;
            }
        }
    }
inspect:
    /* With no blocks, original +10 automatic storage was never written.
     * Preserve that Win32 defect; native stack bytes cannot name its record. */
#ifdef __linux__
    if (!record_written) recon_unsupported("region termination incidental stack record");
#endif
    comparison = record;
    if (comparison->opcode == 0x151) {
        operand = comparison->left;
        if (global_operand_is_restricted(operand) && region_operand_is_invariant(operand->next)) {
            consumer = follow_optimizer_result(comparison, &record);
            if (consumer->kind == 16) {
                record = comparison->next;
                while (record->precision_flags == 4 &&
                       !region_operand_overlaps_motion_inputs(record->tail.right)) record = record->next;
                if (record == consumer && !next_global_code_record(consumer) && block == last_block) {
                    release_function_flow_edge(function, find_flow_edge(last_block, first_block));
                    reg = operand->value.reg;
                    remove_region_register_use(reg, comparison);
                    record = pop_region_register_use(reg);
                    unlink_and_release_input_record((C2Record *)record);
                    record = pop_region_register_definition(reg);
                    unlink_and_release_input_record((C2Record *)record);
                    unlink_and_release_input_record((C2Record *)comparison);
                    remove_control_record(consumer);
                    global_aliases_changed = 1;
                    prepare_region_motion_assignments(region, 3);
                    if (region->iteration_count) release_small_node(region->iteration_count);
                    region->iteration_count = NULL;
                    region->iteration_comparison = NULL;
                    region->initial_updates = NULL;
                    return;
                }
            }
        }
    }
cleanup:
    if (region_deferred_assignment) {
        operand = region_deferred_assignment->left;
        if (((1U << (operand->kind & 31)) & 6) && operand->value.reg->value)
            prune_cached_input(operand);
        unlink_and_release_input_record((C2Record *)region_deferred_assignment);
    }
}

void __fastcall finish_region_block_records(C2BasicBlock *block, unsigned mode)
{
    C2Expression *record, *next;
    C2Expression *last = (C2Expression *)block->last_record;
    for (record = (C2Expression *)block->records->next; record != last; record = next) {
        next = record->next;
        if (record->flags & 1) update_global_record_registers(record, mode);
    }
}
void __fastcall region_candidate_pass_3ec06(C2FlowRegion *region, C2Function *function)
{
    C2BasicBlock *block = region->rewrite_entry;
    prepare_global_block(block, function);
    finish_region_block_records(block, 1);
    global_region_limited = 0;
    optimize_global_block(block, function);
}

void __fastcall region_candidate_pass_3ec67(C2FlowRegion *region, C2Function *function)
{
    C2BasicBlock *block, *entry;
    C2Expression *record, *next, *output;
    C2FunctionRegister *input_reg, *output_reg;
    if (!region_motion_sets[9]->links.first) return;
    global_available_registers = new_temporary_register_set();
    entry = region->rewrite_entry;
    for (record = (C2Expression *)entry->records->next;
         record != (C2Expression *)entry->last_record; record = next) {
        next = record->next;
        if (record->opcode != 0x131) continue;
        output = record->tail.right;
        if (!region_operand_is_invariant(output)) continue;
        if (record->left->kind == 7) update_global_available_registers(record);
        else add_register_to_set(region->global_registers, output->value.reg->number);
    }
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next)
            if (record->flags & 1) global_record_needs_rewrite(record);
    for (block = region->header; block != region->last->previous; block = block->previous)
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (record->opcode == 0x131 && record->tail.right->kind == 2 && record->left->kind == 2) {
                input_reg = record->left->value.reg;
                if (input_reg->rewrite_flags & 1) {
                    output_reg = record->tail.right->value.reg;
                    if (output_reg->rewrite_flags & 1) output_reg->flags |= 0x40;
                    else if (output_reg->global_expression)
                        merge_global_register_identity(input_reg, output_reg);
                }
            }
            if (record->precision_flags == 4) {
                update_global_record_registers(record, 1);
                record->precision_flags = 0;
            } else if (record->opcode == 0x131 && !record->value.reg)
                update_global_record_registers(record, 1);
        }
}

void __cdecl discard_region_hash_values(void)
{
    C2GlobalExpression **link, *expression;
    C2FunctionRegister *reg;
    C2GlobalDependency *dependency, **dependent;
    unsigned bucket;
    for (bucket = 0; bucket < 101; ++bucket) {
        link = &global_expression_buckets[bucket];
        while ((expression = *link) != NULL) {
            if (expression->opcode < 0x14d || expression->opcode > 0x150) {
                link = &expression->next;
                continue;
            }
            *link = expression->next;
            reg = expression->reg;
            reg->global_expression = NULL;
            while ((dependency = reg->global_dependencies) != NULL) {
                reg->global_dependencies = dependency->next_input;
                dependent = &dependency->input->global_dependents;
                while (*dependent != dependency) dependent = &(*dependent)->next_dependent;
                *dependent = dependency->next_dependent;
            }
        }
    }
}
void __fastcall region_candidate_pass_3edd3(C2FlowRegion *region)
{
    C2FunctionRegister *reg;
    unsigned number;
    discard_region_hash_values();
    for (number = next_optimizer_symbol(region_motion_sets[0]); number != (unsigned)-1;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        reg->region_motion_flags &= 0xfff9;
    }
    for (number = next_optimizer_symbol(region_motion_sets[9]); number != (unsigned)-1;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_function_register(number);
        reg->global_uses = NULL;
        reg->global_definitions = NULL;
    }
    clear_register_set(region_motion_sets[9]);
    clear_register_set(region_motion_sets[5]);
    clear_register_set(region_motion_sets[0]);
    clear_register_set(region_motion_sets[3]);
    clear_register_set(region_motion_sets[10]);
    clear_register_set(region_motion_sets[4]);
    clear_register_set(region_motion_sets[2]);
    clear_register_set(region_motion_sets[7]);
    clear_register_set(region_motion_sets[1]);
    clear_register_set(region_motion_sets[6]);
    clear_register_set(region_motion_sets[8]);
    reset_input_pool(3);
    if (region_iteration_counter) {
        release_small_node(region_iteration_counter);
        region_iteration_counter = NULL;
    }
}

void __fastcall finish_region_selection(C2Expression *record, C2BasicBlock *block)
{
    C2FlowEdge *outgoing = block->outgoing;
    C2Expression *left, *right, *consumer, *result, *inserted;
    C2BranchTarget *target;
    C2BasicBlock *destination;
    C2Record *anchor;
    unsigned opcode, condition;
    if (!outgoing) return;
    left = record->left;
    right = left->next;
    if ((((1U << (left->kind & 31)) & 6) && left->value.reg->value) ||
        (((1U << (right->kind & 31)) & 6) && right->value.reg->value)) return;
    consumer = follow_optimizer_result(record, &result);
    if (consumer->kind != 16 || next_global_code_record(record) != consumer ||
        !outgoing->next_outgoing) return;
    left = record->left;
    right = left->next;
    if (((1U << (left->kind & 31)) & 0x180) &&
        ((1U << (right->kind & 31)) & 0x180)) return;
    condition = consumer->convention;
    opcode = global_condition_opcode((unsigned char)condition);
    target = ((C2InputSymbol *)consumer->left->value.symbol)->branch_target;
    if (!target->block) target = get_symbol_branch_target(target, 24);
    destination = target->block;
    if (destination->incoming && !destination->incoming->next_incoming) {
        inserted = new_region_binary_expression(opcode, record->type, left, right,
                                                 insert_input_record, (C2Record *)target);
        inserted->precision_flags = 2;
    }
    /* Only the proved17-entry negation column is available here. */
    if (condition > 16) { __asm { int 3 } }
    opcode = global_condition_opcode(input_inverse_condition[condition]);
    destination = block->previous;
    anchor = destination->records;
    if (destination->incoming && !destination->incoming->next_incoming) {
        inserted = new_region_binary_expression(opcode, record->type, left, right,
                                                 insert_input_record, anchor);
        inserted->precision_flags = 2;
    }
}

const unsigned *__fastcall global_class1_opcodes(unsigned opcode)
{
    /* DWORD opcode lists, terminated by0. The numbered conditions use the
     * same opcode mapping as global_condition_opcode. */
    static const unsigned condition1[] = {0x152, 0};
    static const unsigned condition2[] = {0x153, 0x155, 0x154, 0};
    static const unsigned condition3[] = {0x154, 0};
    static const unsigned condition4[] = {0x155, 0};
    static const unsigned condition5[] = {0x156, 0x154, 0x152, 0};
    static const unsigned condition6[] = {0x157, 0x155, 0x152, 0};
    static const unsigned none[] = {0};
    switch (opcode) {
    case 0x152: return condition1;
    case 0x153: return condition2;
    case 0x154: return condition3;
    case 0x155: return condition4;
    case 0x156: return condition5;
    case 0x157: return condition6;
    default: return none;
    }
}
const unsigned *__fastcall global_class2_opcodes(unsigned opcode)
{
    /* The classifier's second DWORD-list column, with an explicit terminator. */
    static const unsigned condition1[] = {0x153, 0x155, 0x154, 0};
    static const unsigned condition2[] = {0x152, 0};
    static const unsigned condition3[] = {0x157, 0x155, 0x152, 0};
    static const unsigned condition4[] = {0x156, 0x154, 0x152, 0};
    static const unsigned condition5[] = {0x155, 0};
    static const unsigned condition6[] = {0x154, 0};
    static const unsigned none[] = {0};
    switch (opcode) {
    case 0x152: return condition1;
    case 0x153: return condition2;
    case 0x154: return condition3;
    case 0x155: return condition4;
    case 0x156: return condition5;
    case 0x157: return condition6;
    default: return none;
    }
}
C2FunctionRegister *__fastcall find_global_alternative_register(unsigned opcode,
    unsigned short type, C2Expression *left, C2Expression *right)
{
    C2GlobalExpression *expression;
    unsigned hash;
    set_global_expression_operands(2, left, right);
    hash = hash_global_expression(opcode, type);
    expression = find_global_expression(hash, opcode, type);
    return expression ? expression->reg : NULL;
}
int __fastcall classify_global_register(C2RegisterSet *available, C2FunctionRegister *reg)
{
    C2GlobalExpression *expression;
    C2Expression *left, *right;
    C2FunctionRegister *candidate;
    const unsigned *alternatives;
    unsigned short type;
    if (optimizer_set_contains(available, reg->number)) return 1;
    expression = reg->global_expression;
    left = expression->operands;
    right = left->next;
    type = expression->type;
    for (alternatives = global_class1_opcodes(expression->opcode); *alternatives; ++alternatives) {
        candidate = find_global_alternative_register(*alternatives, type, left, right);
        if (candidate && optimizer_set_contains(available, candidate->number)) return 1;
    }
    for (alternatives = global_class2_opcodes(expression->opcode); *alternatives; ++alternatives) {
        candidate = find_global_alternative_register(*alternatives, type, left, right);
        if (candidate && optimizer_set_contains(available, candidate->number)) return 2;
    }
    return 3;
}

void __fastcall finish_global_region(C2FlowRegion *region)
{
    C2FlowBlockLink *exit;
    collect_region_motion_definitions(region);
    for (exit = region->exits; exit; exit = exit->next) {
        intersect_optimizer_symbol_sets(exit->block->global_definitions,
                                        region->header->global_available_in);
        subtract_optimizer_symbol_sets(exit->block->global_definitions, global_motion_registers);
    }
}
