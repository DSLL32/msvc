/* Physical register coloring and the decision to address a frame via ESP. */
#include "physical-allocation.h"
#include "backend-passes.h"
#include "bitsets.h"
#include "frame.h"
extern int cpu_model, optimization_cost, force_risc, optimization_risc;
extern int codeview_debug, omit_frame_pointer;
RegisterAllocationStatistics register_allocation_statistics;
int available_integer_registers, rematerialization_cost_limit;
int frame_register_available, frame_register_allocated;
SpillTemporary *spill_temporary_buckets[32];
int physical_register_costs[31];

/* Original [0002aa80,0002ad60). Build conflicts, compare both ordering
   strategies by signed spill cost, rewrite spills, and retry. Arena4 owns
   the temporary conflict graph. EBP is color5; register24 is the logical
   frame reference whose eventual addressing form this pass selects. */
void allocate_machine_registers(void)
{
    RegisterVariable *variables, *spills, *variable;
    int strategy, best_strategy, spill_cost, best_cost;
    int group, bucket, use_stack_pointer;
    available_integer_registers = 6;
    register_allocation_statistics.rewritten_instructions = 0;
    register_allocation_statistics.rounds = 0;
    register_allocation_statistics.spilled_variables = 0;
    register_allocation_statistics.simplify_candidates = 0;
    register_allocation_statistics.count_c534 = 0;
    register_allocation_statistics.count_c538 = 0;
    register_allocation_statistics.count_c53c = 0;
    register_allocation_statistics.count_c540 = 0;
    register_allocation_statistics.count_c544 = 0;
    register_allocation_statistics.count_c548 = 0;
    register_allocation_statistics.strategies[0] = 0;
    register_allocation_statistics.strategies[1] = 0;
    frame_register_available = frame_register_can_be_allocated();
    if (frame_register_available) ++available_integer_registers;
    rematerialization_cost_limit =
        ((cpu_model == 3 && optimization_cost == 0) || optimization_cost == 3) ? 4 : 2;
    analyze_allocation_float_blocks();
    initialize_physical_variables();
    prune_rematerialized_variables();
    register_allocation_statistics.rounds = 0;
    for (;;) {
        if (++register_allocation_statistics.rounds >= 10)
            compiler_internal_error("@(#)color.c:1.101", 0x102);
        frame_register_allocated = 0;
        initialize_allocation_round();
        build_register_conflicts(current_work, add_register_conflict);
        if (register_allocation_statistics.rounds == 1 && simplify_unspillable_variables()) {
            reset_arena(4);
            continue;
        }
        build_allocation_constraints();
        finish_allocation_constraints();
        best_strategy = -1;
        for (strategy = 0; strategy < 2; ++strategy) {
            variables = order_allocation_variables(strategy);
            for (group = 0; group < REGISTER_VARIABLE_CLASSES; ++group)
                for (bucket = 0; bucket < REGISTER_VARIABLE_BUCKETS; ++bucket)
                    for (variable = register_variable_buckets[group][bucket]; variable; variable = variable->next)
                        variable->degree = (short)count_register_conflicts(variable->conflicts);
            spills = color_register_variables(&variables);
            if (!spills) {
                best_strategy = strategy;
                break;
            }
            spill_cost = 0;
            for (variable = spills; variable; variable = variable->work_next)
                spill_cost += variable->spill_cost;
            if (best_strategy == -1 || spill_cost < best_cost) {
                best_strategy = strategy;
                best_cost = spill_cost;
            }
            for (variable = variables; variable; variable = variable->work_next)
                variable->flags = (variable->flags & ~31U) | 32;
        }
        ++register_allocation_statistics.strategies[best_strategy];
        if (best_strategy != strategy) {
            variables = order_allocation_variables(best_strategy);
            spills = color_register_variables(&variables);
        }
        if (spills) {
            spill_register_variables(spills, 0);
            reset_arena(4);
            continue;
        }
        variable = create_register_variable(24);
        variable->flags = (variable->flags & 0xffffffc5U) | 5;
        if ((cpu_model == 5 || force_risc) && optimization_risc)
            optimize_allocated_machine_regions();
        assign_machine_registers(machine_instructions);
        use_stack_pointer = frame_register_allocated ||
            (frame_register_available && !frame_references_exceed_cost(current_work, 24));
        if (codeview_debug) record_debug_register_locations();
        allocate_frame_symbols(current_work, use_stack_pointer);
        if (use_stack_pointer) rewrite_stack_pointer_frame(machine_instructions);
        current_function->field28 = (current_function->field28 & ~0x10U) |
            ((unsigned)use_stack_pointer << 4);
        current_function->field28 = (current_function->field28 & ~0x20U) |
            (((unsigned)frame_register_allocated << 5) & 0x20U);
        finish_register_assignment();
        finish_machine_blocks(machine_instructions, 0);
        reset_arena(4);
        return;
    }
}

/* Original [0002bee0,0002bf40). A changed stack layout forbids reusing EBP.
   An inline assembly function needs an additional register-use inspection. */
int frame_register_can_be_allocated(void)
{
    if (stack_frame_extent || !omit_frame_pointer) return 0;
    if ((current_function->field28 & 1) &&
        !function_allows_frame_register(current_function)) return 0;
    return !(current_function->symbol->field0e & 0x1000);
}

/* Original [0002beb0,0002bee0). Revisit only blocks carrying the floating
   analysis bit, walking through the shared machine-block iterator. */
void analyze_allocation_float_blocks(void)
{
    MachineInstruction *instruction;
    for (instruction = machine_instructions; instruction; instruction = next_machine_block(instruction))
        if (((MachineBlockHeader *)instruction)->flags & 0x400)
            analyze_selected_block((MachineBlockHeader *)instruction);
}

/* Original [0002be90,0002beb0). Physical register variables0..13 are
   entered through the same bucket constructor as later virtual variables. */
void initialize_physical_variables(void)
{
    unsigned short number;
    for (number = 0; number < 14; ++number) create_register_variable(number);
}

/* Original [0002bde0,0002be90). Tree-class variables may have a deferred
   machine block for recreating their value. Keep it only when its cost is
   small and its number has not already been excluded from rematerializing. */
void prune_rematerialized_variables(void)
{
    RegisterVariable *variable;
    MachineBlockHeader *block;
    unsigned bucket;
    int cost, limit;
    limit = ((cpu_model == 3 && optimization_cost == 0) || optimization_cost == 3) ? 4 : 2;
    for (bucket = 0; bucket < REGISTER_VARIABLE_BUCKETS; ++bucket) {
        for (variable = register_variable_buckets[1][bucket]; variable; variable = variable->next) {
            block = variable->rematerialization;
            if (!block) continue;
            if (bitset_contains(machine_register_set, variable->number & 0x3fff)) {
                variable->rematerialization = 0;
            } else {
                cost = machine_sequence_cost(block->next);
                if (cost > limit) {
                    variable->rematerialization = 0;
                    machine_register_set = bitset_include(machine_register_set, variable->number & 0x3fff);
                } else {
                    variable->rematerialization->rematerialization_cost = cost;
                }
            }
        }
    }
}

/* Original [0002bd30,0002bde0). Discard temporary spill-home hashes,
   color costs and conflicts, retaining each variable's persistent home. */
void initialize_allocation_round(void)
{
    RegisterVariable *variable;
    unsigned i, group, bucket;
    for (i = 0; i < 32; ++i) spill_temporary_buckets[i] = 0;
    for (i = 0; i < 31; ++i) physical_register_costs[i] = 0;
    for (group = 0; group < REGISTER_VARIABLE_CLASSES; ++group) {
        for (bucket = 0; bucket < REGISTER_VARIABLE_BUCKETS; ++bucket) {
            for (variable = register_variable_buckets[group][bucket]; variable; variable = variable->next) {
                variable->field1a = variable->number;
                variable->flags &= ~0x1800U;
                variable->work_next = 0;
                variable->conflicts = 0;
                variable->degree = 0;
                variable->spill_weight = variable->spill_priority = 0;
                variable->spill_cost = (variable->flags & 0x40) ? 30000 : 0;
                if (variable->number & 0xc000)
                    variable->flags = (variable->flags & ~31U) | 32;
            }
        }
    }
}
