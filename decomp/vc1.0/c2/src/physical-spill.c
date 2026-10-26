/* Measure and apply memory spilling and deferred-value recreation. */
#include "physical-allocation.h"
#include "instruction-rewrite.h"
#include "register-use.h"
#include "function-input.h"
#include "temporaries.h"
#include "backend-passes.h"
#include "bitsets.h"
#include <recon.h>
extern int cpu_model, optimization_cost, optimize_speed;

/* Original [0002bff0,0002c7c0). Measure or apply spilling for one operand.
   Rematerialized definitions can disappear or be cloned at a use. Otherwise
   try a memory operand first, then place explicit loads/stores as required.
   Measurement and mutation deliberately share this original graph vertex. */
int __fastcall allocation_operand_cost(MachineInstruction *instruction, RegisterVariable *variable,
    int annotation, unsigned operand_index, int apply, unsigned *spill_bytes, WorkItem *work)
{
    MachineOperand *operand, spill;
    MachineInstruction *definition, *copy, *last, *previous, *move;
    MachineBlockHeader *rematerialization;
    MachineAddress *address;
    SymbolHeader *home;
    unsigned short number;
    unsigned opcode, operation, kind, width, storage;
    int cost, rematerialized_cost, high_byte;
    number = variable->number;
    if (!(number & 0xc000)) return 0;
    opcode = instruction->opcode;
    operation = (opcode >> 8) & 255;
    if ((opcode & 15) == 8 || (opcode & 15) == 9) {
        if (apply) remove_machine_instruction(instruction);
        return -rematerialization_cost_limit;
    }
    rematerialization = variable->rematerialization;
    if (!operand_index && (number & 0x4000) &&
        (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
        !(variable->flags & 0x100) && rematerialization &&
        !((rematerialization->next->opcode ^ opcode) & 0xff000000U) &&
        (operation == MC_MOV || operation == MC_LEA || operation == MC_MOVSX ||
         operation == MC_MOVZX || operation == MC_MOVSXW || operation == MC_MOVZXW)) {
        if (conservative_flag_analysis) return 0;
        if (apply) remove_machine_instruction(instruction);
        return -rematerialization->rematerialization_cost;
    }
    operand = &instruction->operands[operand_index ? 1 : 0];
    if (rematerialization && annotation) {
        rematerialized_cost = rematerialization->rematerialization_cost;
        definition = rematerialization->next;
        if (!apply) {
            if (((definition->opcode >> 8) & 255) == MC_LEA &&
                (definition->next->opcode & 15) == 7 &&
                fold_lea_address(definition, operand, 0)) {
                if (cpu_model == 4 && (operand->flags & 15) == OPERAND_BASED &&
                    (definition->operands[1].flags & 15) == OPERAND_INDEXED) return 1;
                return 0;
            }
            if (((definition->opcode >> 8) & 255) == MC_MOV &&
                (definition->next->opcode & 15) == 7) {
                kind = definition->operands[1].flags & 15;
                if ((kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS || kind == 7) &&
                    replace_machine_operand(instruction, operand_index, &definition->operands[1], 0)) {
                    kind = instruction->operands[0].flags & 15;
                    if (cpu_model > 3 && machine_operand_counts[operation] == 2 && operand_index == 1 &&
                        (kind == OPERAND_BASED || kind == OPERAND_INDEXED || kind == OPERAND_ABSOLUTE) &&
                        (instruction->operands[0].value.address->symbol ||
                         instruction->operands[0].value.address->first.displacement)) return 1;
                    return 0;
                }
            }
            return rematerialized_cost;
        }
        previous = instruction->previous;
        last = previous;
        while (!(definition->opcode & 15)) {
            if (work) record_rematerialized_registers(work, definition);
            copy = clone_machine_record(definition);
            copy->opcode = (copy->opcode & 0xffffffU) | (opcode & 0xff000000U);
            previous->next = copy;
            copy->previous = previous;
            definition = definition->next;
            previous = last = copy;
        }
        kind = operand->flags & 15;
        if (kind == OPERAND_REGISTER) operand->value.reg.words.flags = 0;
        else if (kind == OPERAND_INDEXED && operand->value.address->index.words.number == number)
            operand->value.address->index.words.flags = 0;
        else if (kind == OPERAND_INDEXED || kind == OPERAND_BASED)
            operand->value.address->base.words.flags = 0;
        last->next = instruction;
        instruction->previous = last;
        ++register_allocation_statistics.rematerialized_operands;
        rematerialization_changed = 1;
        return rematerialized_cost;
    }
    operand_for_spill(&spill, number, apply);
    kind = operand->flags & 15;
    if (kind == OPERAND_REGISTER && !(operation == MC_PUSH && optimize_speed && cpu_model > 3)) {
        if (!(number & 0x8000) &&
            (unsigned)(int)variable->storage_bytes < machine_operand_bytes(instruction, (int)operand_index)) {
            if (!operand_index && writes_first_operand(instruction)) goto explicit_spill;
            home = variable->home_symbol;
            storage = home->kind == 1 ? ((TemporarySymbol *)home)->attributes & 0x70 : 0;
            if ((home->kind != 1 || (storage != 0x10 && storage != 0x30)) && home->kind != 19)
                goto explicit_spill;
        }
        if (replace_machine_operand(instruction, operand_index, &spill, (unsigned char)(apply | 2))) {
            if (apply) {
                ++register_allocation_statistics.folded_spill_operands;
                if (!operand_index && spill_bytes && writes_first_operand(instruction)) {
                    width = (instruction->opcode >> 4) & 15;
                    if (*spill_bytes < width) *spill_bytes = width;
                    if (*spill_bytes == 1 && (instruction->opcode & 0x80000)) *spill_bytes = 2;
                }
            }
            return folded_spill_operand_cost(instruction, operand_index);
        }
    }
explicit_spill:
    cost = (cpu_model > 3 && optimization_cost != 3) ? rematerialization_cost_limit : 0;
    if (operand_index || kind != OPERAND_REGISTER || !overwrites_first_register(instruction, 0)) {
        if (apply) {
            high_byte = operand_index == 1 && kind == OPERAND_REGISTER && (instruction->opcode & 0x100000);
            move = new_machine_instruction(0);
            move->opcode = (move->opcode & ~0xff00U) | (MC_MOV << 8);
            width = kind == OPERAND_REGISTER && operand->value.reg.words.number == number ?
                (instruction->opcode >> 4) & 15 : 4;
            move->opcode = (move->opcode & ~0xf0U) | (width << 4);
            if (spill_bytes && *spill_bytes && *spill_bytes < width)
                move->opcode = (move->opcode & ~0xf0U) | ((*spill_bytes << 4) & 0xf0U);
            move->operands[0].flags = (move->operands[0].flags & ~15U) | OPERAND_REGISTER;
            move->operands[0].value.reg.words.number = number;
            move->operands[0].value.reg.words.flags = (unsigned short)annotation;
            move->operands[1] = spill;
            if (high_byte) move->opcode |= 0x180000;
            move->opcode = (move->opcode & 0xffffffU) | (instruction->opcode & 0xff000000U);
            insert_machine_after(instruction->previous, move);
            ++register_allocation_statistics.inserted_spill_loads;
        }
        cost += rematerialization_cost_limit;
    }
    if (!operand_index && (operand->flags & 15) == OPERAND_REGISTER &&
        writes_first_operand(instruction) && !(operand->flags & 0x400) &&
        (((instruction->opcode >> 8) & 255) != MC_MOV ||
         !operands_equal(&instruction->operands[1], &spill))) {
        if (apply) {
            opcode = instruction->opcode;
            high_byte = (opcode & 0xf0) == 0x10 && (opcode & 0x80000);
            move = new_machine_instruction(0);
            move->opcode = (move->opcode & ~0xff00U) | (MC_MOV << 8);
            width = 4;
            if (operand->value.reg.words.number == number) {
                width = (opcode >> 4) & 15;
                if ((number & 0x4000) && (unsigned)(int)variable->storage_bytes < width)
                    width = (unsigned)(int)variable->storage_bytes;
            }
            move->opcode = (move->opcode & ~0xf0U) | ((width << 4) & 0xf0U);
            if (spill_bytes) {
                if (*spill_bytes < width) *spill_bytes = width;
                if (*spill_bytes == 1 && high_byte) *spill_bytes = 2;
            }
            move->operands[1].flags = (move->operands[1].flags & ~15U) | OPERAND_REGISTER;
            move->operands[1].value.reg.words.number = number;
            move->operands[1].value.reg.words.flags = (unsigned short)annotation;
            move->operands[0] = spill;
            if (high_byte) move->opcode |= 0x180000;
            if (cost > 0) {
                address = new_machine_address();
                *address = *spill.value.address;
                move->operands[0].value.address = address;
            }
            move->opcode = (move->opcode & 0xffffffU) | (opcode & 0xff000000U);
            insert_machine_after(instruction, move);
            ++register_allocation_statistics.inserted_spill_stores;
        }
        cost += rematerialization_cost_limit;
    }
    return cost;
}

/* Original [0002cec0,0002d000). A dry run uses an EBP-relative placeholder.
   Applying a spill uses its existing variable home, or an arena4 hash entry
   with a newly reserved four-byte temporary for machine-class registers. */
MachineOperand *__fastcall operand_for_spill(MachineOperand *operand, unsigned short number, int apply)
{
    RegisterVariable *variable, *representative;
    SpillTemporary **bucket, *temporary;
    SymbolHeader *home;
    unsigned attributes;
    operand->flags = operand->value.constant = 0;
    if (!apply) {
        initialize_operand(operand, OPERAND_BASED, REG_EBP, (unsigned)-1, 0);
        return operand;
    }
    variable = lookup_register_variable(number, 1);
    if (variable->flags & 0x800) {
        representative = lookup_register_variable(variable->representative, 1);
        if (register_spill_homes_compatible(representative, variable)) number = representative->number;
    }
    if (number & 0x8000) {
        bucket = &spill_temporary_buckets[number & 31];
        for (temporary = *bucket; temporary; temporary = temporary->next)
            if (temporary->number == number) break;
        if (!temporary) {
            temporary = arena_alloc(4, sizeof(SpillTemporary));
            temporary->number = number;
            temporary->symbol = (SymbolHeader *)allocate_stack_temporary(4);
            temporary->next = *bucket;
            *bucket = temporary;
        }
        initialize_operand(operand, OPERAND_BASED, REG_EBP, 0, temporary->symbol);
    } else {
        home = variable->home_symbol;
        attributes = home->kind == 1 ? ((TemporarySymbol *)home)->attributes & 0x70 : 0;
        if ((home->kind == 1 && (attributes == 0x10 || attributes == 0x30)) || home->kind == 19)
            initialize_operand(operand, OPERAND_BASED, REG_EBP, variable->home_offset, home);
        else initialize_operand(operand, OPERAND_ABSOLUTE, 0, variable->home_offset, home);
    }
    return operand;
}

/* Original [0002cdc0,0002cec0). Cost a memory operand folded into the
   instruction. A memory destination with an immediate can incur another
   access; ADD by one/minus one retains its short increment/decrement form. */
int __fastcall folded_spill_operand_cost(MachineInstruction *instruction, unsigned operand_index)
{
    unsigned operation, source_kind;
    int penalty = 0;
    if ((cpu_model == 3 && optimization_cost == 0) || optimization_cost == 3) return 2;
    operation = (instruction->opcode >> 8) & 255;
    if (!operand_index && machine_operand_counts[operation] == 2) {
        source_kind = instruction->operands[1].flags & 15;
        if ((source_kind == OPERAND_IMMEDIATE || source_kind == OPERAND_LABEL_ADDRESS || source_kind == 7) &&
            !(operation == MC_ADD && source_kind == OPERAND_IMMEDIATE &&
              (instruction->operands[1].value.constant == 1 ||
               instruction->operands[1].value.constant == (unsigned)-1)))
            penalty = rematerialization_cost_limit;
    }
    if (operation == MC_MOV) return penalty ? penalty : 1;
    if (!writes_first_operand(instruction)) return penalty + rematerialization_cost_limit;
    if (operand_index) {
        if (operation == MC_MOVZX || operation == MC_MOVZXW ||
            operation == MC_MOVSX || operation == MC_MOVSXW) return 1;
        return rematerialization_cost_limit;
    }
    return penalty + 2 * rematerialization_cost_limit;
}

/* Original [0002b700,0002b87c). Mark the chosen spill list, rewrite each
   affected work item, then rerun selection only on blocks changed by spills. */
void __fastcall spill_register_variables(RegisterVariable *variables, int simplification)
{
    RegisterVariable *variable;
    WorkItem *work;
    BitSet *spilled = 0;
    unsigned flags;
    rematerialization_changed = 0;
    conservative_flag_analysis = 1;
    for (variable = variables; variable; variable = variable->work_next) {
        flags = variable->flags;
        if (flags & 0x40)
            compiler_internal_error("@(#)color.c:1.101", 0x2ec);
        if (!simplification && (flags & 0x800) && (variable->number & 0x8000))
            continue;
        variable->flags = flags | 0x40;
        variable->flags |= 0x400;
        if (variable->number & 0x4000) spill_variable_everywhere(variable);
    }
    if (rematerialization_changed) {
        rematerialization_changed = 0;
        finish_machine_blocks(deferred_instructions, 1);
    }
    prune_rematerialized_variables();
    conservative_flag_analysis = 0;
    for (variable = variables; variable; variable = variable->work_next) {
        if (!(variable->flags & 0x40)) continue;
        if ((variable->number & 0x4000) && variable->rematerialization &&
            !(variable->flags & 0x80)) {
            record_rematerialized_work_registers(current_work,
                variable->rematerialization->next, variable->number);
            variable->flags |= 0x80;
        }
        register_allocation_statistics.rewritten_instructions +=
            spill_variable_in_work(current_work, variable);
        if (variable->number & 0x4000)
            spilled = bitset_include(spilled, variable->number & 0x3fff);
        ++register_allocation_statistics.spilled_variables;
    }
    remove_spilled_work_registers(current_work, spilled);
    release_bitset(spilled);
    for (work = current_work; work; work = work->next) {
        if (work->flags & 0x100) {
            finish_selected_block(work->body.block, 1);
            work->flags &= ~0x100;
            (work->body.block)->flags &= ~0x100;
        }
    }
}

/* Original [0002cbd0,0002cc0a). The whole-stream pass includes ordinary
   and implicit-register instructions. Save next before rewriting the record. */
void __fastcall spill_variable_everywhere(RegisterVariable *variable)
{
    MachineInstruction *instruction, *next;
    unsigned kind;
    for (instruction = deferred_instructions; instruction; instruction = next) {
        next = instruction->next;
        kind = instruction->opcode & 15;
        if (!kind || kind == 8 || kind == 9)
            spill_instruction_variable(instruction, variable, 1, 0, 0);
    }
}

/* Original [0002c8f0,0002c9dc). Work reference sets contain tagged register
   numbers; tree-register block liveness instead uses the low14-bit number. */
int __fastcall spill_variable_in_work(WorkItem *work, RegisterVariable *variable)
{
    MachineInstruction *instruction, *next;
    unsigned spill_bytes = (unsigned)(int)variable->storage_bytes;
    unsigned short number;
    unsigned kind;
    int changed, result, total = 0;
    for (; work; work = work->next) {
        number = variable->number;
        if (number & 0x4000) {
            bitset_remove(work->body.block->uses, number & 0x3fff);
            bitset_remove(work->body.block->definitions, number & 0x3fff);
        }
        if (!bitset_contains(work->flow_sets[0], number)) continue;
        changed = 0;
        instruction = work->body.block->next;
        while ((instruction->opcode & 15) != 7) {
            next = instruction->next;
            kind = instruction->opcode & 15;
            if (!kind || kind == 8 || kind == 9) {
                result = spill_instruction_variable(instruction, variable, 1,
                    &spill_bytes, work);
                if (result) {
                    total += result;
                    changed = 1;
                }
            }
            instruction = next;
        }
        if (changed) {
            work->flags |= 0x100;
            work->body.block->flags |= 0x100;
            if (variable->number & 0x8000) return total;
        }
    }
    return total;
}

/* Original [0002cc10,0002cd25). Visit explicit operands in their original
   order. An index and base each carry independent signed use annotations. */
int __fastcall spill_instruction_variable(MachineInstruction *instruction,
    RegisterVariable *variable, int apply, unsigned *spill_bytes, WorkItem *work)
{
    MachineOperand *operand;
    MachineAddress *address;
    unsigned short number = variable->number;
    unsigned kind;
    int index, cost, total = 0;
    for (index = 0; index < machine_operand_counts[(instruction->opcode >> 8) & 255]; ++index) {
        operand = &instruction->operands[index];
        kind = operand->flags & 15;
        if (kind == OPERAND_REGISTER) {
            if (operand->value.reg.words.number == number) {
                cost = allocation_operand_cost(instruction, variable,
                    (short)operand->value.reg.words.flags, index, apply, spill_bytes, work);
                total += cost;
                if (cost < 0) return total;
            }
        } else if (kind == OPERAND_BASED || kind == OPERAND_INDEXED) {
            address = operand->value.address;
            if (kind == OPERAND_INDEXED && address->index.words.number == number)
                total += allocation_operand_cost(instruction, variable,
                    (short)address->index.words.flags, index, apply, spill_bytes, work);
            /* Applying an index spill may replace the address in the operand. */
            address = operand->value.address;
            if (address->base.words.number == number)
                total += allocation_operand_cost(instruction, variable,
                    (short)address->base.words.flags, index, apply, spill_bytes, work);
        }
    }
    return total;
}

/* Original [0002c8c0,0002c8ed). Selection liveness drops the spilled
   tree-register numbers after all affected instructions have been rewritten. */
void __fastcall remove_spilled_work_registers(WorkItem *work, BitSet *spilled)
{
    for (; work; work = work->next) {
        work->pass.selection.uses = subtract_bitset(work->pass.selection.uses, spilled);
        work->pass.selection.definitions = subtract_bitset(work->pass.selection.definitions, spilled);
    }
}

/* Original [0002c9e0,0002cbcc). A recreated value carries the registers
   used by its deferred sequence into each relevant work block's liveness.
   Widen dependency ranges before adding their numbers to those work sets. */
void __fastcall record_rematerialized_work_registers(WorkItem *work,
    MachineInstruction *definition, unsigned short number)
{
    MachineOperand *operand;
    MachineAddress *address;
    RegisterVariable *variable, *target;
    BitSet *dependencies = 0;
    unsigned kind, member = number & 0x3fff;
    unsigned short dependency;
    int count, bucket, affected;
    for (; definition && (definition->opcode & 15) != 7; definition = definition->next) {
        for (count = machine_operand_counts[(definition->opcode >> 8) & 255]; count > 0; --count) {
            operand = &definition->operands[count == 2 ? 1 : 0];
            kind = operand->flags & 15;
            switch (kind) {
            case OPERAND_REGISTER:
                dependency = operand->value.reg.words.number;
                if (dependency & 0x4000)
                    dependencies = bitset_include(dependencies, dependency & 0x3fff);
                break;
            case OPERAND_INDEXED:
                address = operand->value.address;
                dependency = address->index.words.number;
                if (dependency & 0x4000)
                    dependencies = bitset_include(dependencies, dependency & 0x3fff);
                /* Fall through: record the base independently. */
            case OPERAND_BASED:
                dependency = operand->value.address->base.words.number;
                if (dependency & 0x4000)
                    dependencies = bitset_include(dependencies, dependency & 0x3fff);
                break;
            }
        }
    }
    bitset_remove(dependencies, member);
    target = lookup_register_variable(number, 1);
    for (bucket = 0; bucket < REGISTER_VARIABLE_BUCKETS; ++bucket) {
        for (variable = register_variable_buckets[1][bucket]; variable; variable = variable->next) {
            dependency = variable->number & 0x3fff;
            if (!bitset_contains(dependencies, dependency)) continue;
            if (variable->flags & 0x40)
                dependencies = bitset_remove(dependencies, dependency);
            else {
                widen_register_block_interval(&variable->blocks, target->last_block);
                widen_register_block_interval(&variable->blocks, target->first_block);
            }
        }
    }
    for (; work; work = work->next) {
        if (!register_block_interval_contains(&target->blocks, work->number)) continue;
        affected = bitset_contains(work->pass.selection.definitions, member);
        if (affected)
            work->pass.selection.definitions = bitset_union(work->pass.selection.definitions, dependencies);
        if (bitset_contains(work->pass.selection.uses, member)) {
            affected = 1;
            work->pass.selection.uses = bitset_union(work->pass.selection.uses, dependencies);
        }
        if (affected) {
            work->flags |= 0x100;
            work->body.block->flags |= 0x100;
        }
    }
    release_bitset(dependencies);
}

/* Original [0002d060,0002d0a5). Ignore block-1; initialize an empty range
   or extend one signed endpoint, returning whether the interval changed. */
int __fastcall widen_register_block_interval(RegisterBlockInterval *range, int number)
{
    if (number == -1) return 0;
    if (range->first == -1) {
        range->last = (short)number;
        range->first = (short)number;
        return 1;
    }
    if ((int)range->first > number) {
        range->first = (short)number;
        return 1;
    }
    if ((int)range->last < number) {
        range->last = (short)number;
        return 1;
    }
    return 0;
}

/* Original [0002d0b0,0002d0cf). Block-1 always matches; other inputs use
   signed low-word comparisons against both endpoints. */
int __fastcall register_block_interval_contains(RegisterBlockInterval *range, int number)
{
    if (number == -1) return 1;
    return range->first <= (short)number && range->last >= (short)number;
}

/* Original [0002cd30,0002cdb1). Include full tagged register identities
   from each operand in the work's allocation-reference set. */
void __fastcall record_rematerialized_registers(WorkItem *work, MachineInstruction *instruction)
{
    MachineOperand *operand;
    unsigned index, kind;
    for (index = 0; (int)index < machine_operand_counts[(instruction->opcode >> 8) & 255]; ++index) {
        operand = &instruction->operands[index ? 1 : 0];
        kind = operand->flags & 15;
        switch (kind) {
        case OPERAND_REGISTER:
            work->flow_sets[0] = bitset_include(work->flow_sets[0], operand->value.reg.words.number);
            break;
        case OPERAND_INDEXED:
            work->flow_sets[0] = bitset_include(work->flow_sets[0], operand->value.address->index.words.number);
            /* Fall through: retain the base, including its native sentinel. */
        case OPERAND_BASED:
            work->flow_sets[0] = bitset_include(work->flow_sets[0], operand->value.address->base.words.number);
            break;
        }
    }
}

/* Original [0002d000,0002d036). A tree register with an observable or
   parameter memory home cannot share that home. The4000 bit belongs to
   the register number; the100/200 restrictions belong to its flags. */
int __fastcall register_spill_homes_compatible(RegisterVariable *left, RegisterVariable *right)
{
    if ((left->number & 0x4000) && (left->flags & 0x300)) return 0;
    if ((right->number & 0x4000) && (right->flags & 0x300)) return 0;
    return 1;
}

/* Original [0002d040,0002d059). Required lookup preserves variable
   identity; the existing interval helper owns extension and empty ranges. */
void __fastcall include_register_block_use(unsigned short number, int block)
{
    RegisterVariable *variable = lookup_register_variable(number, 1);
    widen_register_block_interval(&variable->blocks, block);
}
