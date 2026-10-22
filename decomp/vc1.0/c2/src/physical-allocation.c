/* Physical register coloring and the decision to address a frame via ESP. */
#include "physical-allocation.h"
#include "backend-passes.h"
#include "bitsets.h"
#include "frame.h"
#include "instruction-rewrite.h"
#include <recon.h>
extern int cpu_model, optimization_cost, force_risc, optimization_risc;
extern int codeview_debug, omit_frame_pointer;
RegisterAllocationStatistics register_allocation_statistics;
int available_integer_registers, rematerialization_cost_limit;
int frame_register_available, frame_register_allocated;
int rematerialization_changed;
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
    register_allocation_statistics.rematerialized_operands = 0;
    register_allocation_statistics.folded_spill_operands = 0;
    register_allocation_statistics.inserted_spill_loads = 0;
    register_allocation_statistics.inserted_spill_stores = 0;
    register_allocation_statistics.eliminated_copies = 0;
    register_allocation_statistics.propagated_copies = 0;
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
                variable->representative = variable->number;
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

/* Original [0002bb50,0002bbd0). Negative-cost unassigned variables can be
   profitably simplified before coloring, except hard/combined constraints. */
int simplify_unspillable_variables(void)
{
    RegisterVariable *variable, *candidates = 0;
    unsigned group, bucket;
    for (group = 0; group < REGISTER_VARIABLE_CLASSES; ++group) {
        for (bucket = 0; bucket < REGISTER_VARIABLE_BUCKETS; ++bucket) {
            for (variable = register_variable_buckets[group][bucket]; variable; variable = variable->next) {
                if ((variable->flags & 63) != 32 || variable->spill_cost >= 0 ||
                    (variable->flags & 0x1840)) continue;
                variable->work_next = candidates;
                candidates = variable;
                ++register_allocation_statistics.simplify_candidates;
            }
        }
    }
    if (!candidates) return 0;
    spill_register_variables(candidates, 1);
    return 1;
}

/* Original [0002ba90,0002bb10). Two ordering strategies use total spill
   weight or cost per neighbor. Hard/isolated variables get the native large
   priority rather than a division by zero. Both virtual classes participate. */
void finish_allocation_constraints(void)
{
    RegisterVariable *variable;
    unsigned group, bucket;
    for (group = 1; group < REGISTER_VARIABLE_CLASSES; ++group) {
        for (bucket = 0; bucket < REGISTER_VARIABLE_BUCKETS; ++bucket) {
            for (variable = register_variable_buckets[group][bucket]; variable; variable = variable->next) {
                if (variable->spill_cost == 30000 || !variable->degree) {
                    variable->spill_priority = variable->spill_weight = 3000000;
                } else {
                    variable->spill_weight = variable->spill_cost * 100;
                    variable->spill_priority = variable->spill_weight / variable->degree;
                }
            }
        }
    }
}

/* Original [0002b9a0,0002ba80). Build a reverse removal order for greedy
   coloring. Prefer the highest degree still below the available-color count;
   when none is below it, remove the smallest selected spill-cost metric. */
RegisterVariable *__fastcall order_allocation_variables(int strategy)
{
    RegisterVariable *pending = 0, *ordered = 0, *variable;
    RegisterVariable **link, **best;
    RegisterConflict *edge;
    unsigned group, bucket;
    int degree, best_metric, metric;
    for (group = 1; group < REGISTER_VARIABLE_CLASSES; ++group)
        for (bucket = 0; bucket < REGISTER_VARIABLE_BUCKETS; ++bucket)
            for (variable = register_variable_buckets[group][bucket]; variable; variable = variable->next) {
                if (variable->flags & 0x800) continue;
                variable->work_next = pending;
                pending = variable;
            }
    while (pending) {
        best = 0;
        for (link = &pending; *link; link = &(*link)->work_next) {
            if (!best) best = link;
            else {
                degree = (*best)->degree;
                if (degree < available_integer_registers) {
                    if ((*link)->degree < available_integer_registers && degree < (*link)->degree)
                        best = link;
                } else if ((*link)->degree < available_integer_registers) best = link;
                else {
                    best_metric = strategy ? (*best)->spill_weight : (*best)->spill_priority;
                    metric = strategy ? (*link)->spill_weight : (*link)->spill_priority;
                    if (best_metric > metric) best = link;
                }
            }
        }
        variable = *best;
        *best = variable->work_next;
        variable->work_next = ordered;
        ordered = variable;
        for (edge = variable->conflicts; edge; edge = edge->next) --edge->variable->degree;
    }
    return ordered;
}

/* Original [0002ba80,0002ba90). Recompute degree from the unchanged
   adjacency links after the ordering pass used temporary decrement counts. */
int __fastcall count_register_conflicts(RegisterConflict *conflicts)
{
    int count = 0;
    for (; conflicts; conflicts = conflicts->next) ++count;
    return count;
}

/* Original [0002b880,0002b9a0). Seven contextual DWORD preferences at
   810f8..81114 end exactly before the affinity-list head: volatile registers
   first, then ESI/EDI/EBX, and finally EBP when its seventh slot is enabled. */
RegisterVariable *__fastcall color_register_variables(RegisterVariable **variables)
{
    static const unsigned preferred_registers[7] = {
        REG_EAX, REG_ECX, REG_EDX, REG_ESI, REG_EDI, REG_EBX, REG_EBP
    };
    RegisterVariable **link, *variable, *spills = 0;
    RegisterConflict *edge;
    unsigned used, neighbor_color;
    int index, color, cost;
    for (link = variables; (variable = *link) != 0; ) {
        used = 0;
        for (edge = variable->conflicts; edge; edge = edge->next) {
            neighbor_color = edge->variable->flags & 63;
            if (neighbor_color != 32) used |= 1U << (neighbor_color & 31);
        }
        color = -1;
        for (index = 0; index < available_integer_registers; ++index) {
            if (!(used & (1U << preferred_registers[index]))) {
                color = (int)preferred_registers[index];
                break;
            }
        }
        if (color == -1) {
            *link = variable->work_next;
            variable->work_next = spills;
            spills = variable;
        } else {
            variable->flags = (variable->flags & ~63U) | color;
            if (color == REG_EBP) frame_register_allocated = 1;
            cost = physical_register_costs[color] + variable->spill_cost;
            physical_register_costs[color] = cost > 30000 ? 30000 : cost;
            link = &variable->work_next;
        }
    }
    return spills;
}

/* Original [0002b250,0002b330). On386, retain EBP when weighted
   explicit frame references exceed the cost of omitting that register.
   486/Pentium selection bypasses this estimate. */
int __fastcall frame_references_exceed_cost(WorkItem *work, int limit)
{
    MachineInstruction *instruction;
    MachineOperand *operand;
    int total = 0, weight, i;
    unsigned kind;
    if (cpu_model > 3) return 0;
    for (; work; work = work->next) {
        weight = loop_depth_weight((unsigned)(short)work->region_depth);
        instruction = work->body.block->next;
        for (; (instruction->opcode & 15) != 7; instruction = instruction->next) {
            if ((instruction->opcode & 15) != 0) continue;
            for (i = machine_operand_counts[(instruction->opcode >> 8) & 255]; i > 0; --i) {
                operand = &instruction->operands[i == 2 ? 1 : 0];
                kind = operand->flags & 15;
                if ((kind == OPERAND_BASED || kind == OPERAND_INDEXED) &&
                    operand->value.address->base.words.number == REG_EBP) total += weight;
            }
            if (total > limit) return 1;
        }
    }
    return 0;
}

/* Original [0002ad60,0002adf0). Leave ordinary machine kinds after
   analysis-only markers and install physical colors in explicit operands.
   Block summaries gain physical live-in/definition bits through their helper. */
void finish_register_assignment(void)
{
    MachineInstruction *instruction;
    unsigned kind;
    int count;
    for (instruction = machine_instructions; instruction; instruction = instruction->next) {
        kind = instruction->opcode & 15;
        if (kind == 7) add_block_physical_registers((MachineBlockHeader *)instruction);
        else {
            if (kind == 8 || kind == 9) instruction->opcode &= ~15U;
            else if (kind != 0 && kind != 10) continue;
            instruction->opcode &= 0xffffff;
            count = machine_operand_counts[(instruction->opcode >> 8) & 255];
            if (count == 2) assign_operand_registers(instruction, &instruction->operands[1], 1);
            if (count == 1 || count == 2) assign_operand_registers(instruction, &instruction->operands[0], 0);
        }
    }
}

/* Original [0002d210,0002d300). Keep the logical live-in/definition
   members and add each canonical integer color. A temporary copy isolates
   iterator continuation state from mutations to the original sets. */
void __fastcall add_block_physical_registers(MachineBlockHeader *block)
{
    BitSet *copy;
    RegisterVariable *variable;
    unsigned flags;
    int member;
    copy = assign_bitset(0, block->uses);
    for (member = next_bitset_member(copy); member != -1; member = next_bitset_member(0)) {
        if (member > 31) {
            variable = lookup_register_variable((unsigned short)(member | 0x4000), 1);
            flags = variable->flags;
            if (flags & 0x800) flags = lookup_register_variable(variable->representative, 1)->flags;
            block->uses = bitset_include(block->uses, flags & 63);
        }
    }
    copy = assign_bitset(copy, block->definitions);
    for (member = next_bitset_member(copy); member != -1; member = next_bitset_member(0)) {
        if (member > 31) {
            variable = lookup_register_variable((unsigned short)(member | 0x4000), 1);
            flags = variable->flags;
            if (flags & 0x800) flags = lookup_register_variable(variable->representative, 1)->flags;
            block->definitions = bitset_include(block->definitions, flags & 63);
        }
    }
    release_bitset(copy);
}

/* Original [0002d0d0,0002d210). Resolve colored virtual/frame registers
   and discard their per-register analysis flags. Byte operands additionally
   require one of the four encodable byte-register colors. */
void __fastcall assign_operand_registers(MachineInstruction *instruction, MachineOperand *operand, int index)
{
    unsigned kind = operand->flags & 15;
    unsigned short number, color;
    RegisterVariable *variable;
    MachineAddress *address;
    if (kind == OPERAND_REGISTER) {
        operand->value.reg.words.flags = 0;
        number = operand->value.reg.words.number;
        if ((number & 0xc000) || number == 24) {
            variable = lookup_register_variable(number, 1);
            if (variable->flags & 0x800) variable = lookup_register_variable(variable->representative, 1);
            color = (unsigned short)(variable->flags & 63);
            if (machine_operand_bytes(instruction, index) <= 1 && !byte_register_available(color))
                compiler_internal_error("@(#)color.c:1.101", 0x5fd);
            operand->value.reg.words.number = color;
        }
        return;
    }
    if (kind != OPERAND_BASED && kind != OPERAND_INDEXED) return;
    address = operand->value.address;
    if (kind == OPERAND_INDEXED) {
        address->index.words.flags = 0;
        number = address->index.words.number;
        if ((number & 0xc000) || number == 24) {
            variable = lookup_register_variable(number, 1);
            if (variable->flags & 0x800) variable = lookup_register_variable(variable->representative, 1);
            address->index.words.number = (unsigned short)(variable->flags & 63);
        }
        if (address->base.words.number == 27) return;
    }
    address->base.words.flags = 0;
    number = address->base.words.number;
    if ((number & 0xc000) || number == 24) {
        variable = lookup_register_variable(number, 1);
        if (variable->flags & 0x800) variable = lookup_register_variable(variable->representative, 1);
        address->base.words.number = (unsigned short)(variable->flags & 63);
    }
}
