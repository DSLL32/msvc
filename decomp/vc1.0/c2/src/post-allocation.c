/* Copy propagation after physical coloring. */
#include "post-allocation.h"
#include "backend.h"
#include "instruction-rewrite.h"
#include "register-use.h"
#include "physical-allocation.h"
#include "physical-rewrite.h"
#include <recon.h>
#define MEMORY_COPY_OPERAND(p) (((p)->flags & 15) == OPERAND_ABSOLUTE || \
    ((p)->flags & 15) == OPERAND_BASED || ((p)->flags & 15) == OPERAND_INDEXED)
#define X87_COPY_OPERATION(op) (((op) >= MC_FADD && (op) <= MC_FLD1) || \
    ((op) >= MC_FUCOM && (op) <= MC_FCOS))
PhysicalRegisterCopy *physical_register_copies[31];
PhysicalRegisterCopy *free_physical_register_copies;

/* Original [0002e700,0002e8f0). Propagate physical copies within ordinary
   blocks, forgetting definitions, implicit clobbers and label boundaries.
   Thirty-one heads end exactly at the separate recycler word7c6b0. */
void __fastcall prepare_machine_blocks(WorkItem *work)
{
    MachineInstruction *instruction, *next;
    PhysicalRegisterCopy **copies;
    unsigned kind, operation, number, mask, i, protected_block = 0;
    for (i = 0; i < 31; ++i) physical_register_copies[i] = 0;
    free_physical_register_copies = 0;
    for (instruction = (MachineInstruction *)work->body.block; instruction; instruction = next) {
        next = instruction->next;
        kind = instruction->opcode & 15;
        if (kind == 1) {
            for (i = 0; i < 8; ++i) release_physical_copies(physical_register_copies[i]);
            for (i = 0; i < 31; ++i) physical_register_copies[i] = 0;
            continue;
        }
        if (kind == 7) {
            protected_block = ((MachineBlockHeader *)instruction)->flags & 2;
            continue;
        }
        if (kind != 0) continue;
        operation = (instruction->opcode >> 8) & 255;
        if (!protected_block) {
            if (operation == MC_JCC || operation == MC_JMP) {
                next = simplify_branch(instruction);
                continue;
            }
            /* Encountered native property1 is exactly the two named x87
               opcode ranges, already independently audited over231 entries. */
            if (!((operation >= MC_FADD && operation <= MC_FLD1) ||
                (operation >= MC_FUCOM && operation <= MC_FCOS)) &&
                propagate_physical_copy(instruction) == -1) continue;
        }
        if (writes_first_operand(instruction) &&
            machine_operand_counts[(instruction->opcode >> 8) & 255] > 0) {
            operation = (instruction->opcode >> 8) & 255;
            if ((instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                (number = instruction->operands[0].value.reg.words.number) < 8) {
                copies = &physical_register_copies[number];
                release_physical_copies(*copies);
                *copies = 0;
                if (!protected_block &&
                    ((operation == MC_MOV && !(instruction->operands[1].flags & 0x8000)) || operation == MC_LEA))
                    remember_physical_copy(copies, instruction);
            } else if (!protected_block && operation == MC_MOV &&
                (instruction->operands[1].flags & 15) == OPERAND_REGISTER &&
                !(instruction->operands[0].flags & 0x8000)) {
                number = instruction->operands[1].value.reg.words.number;
                remember_physical_copy(&physical_register_copies[number], instruction);
            }
        }
        operation = (instruction->opcode >> 8) & 255;
        if (machine_selection_properties[operation] & MC_IMPLICIT_REGISTERS) {
            mask = implicit_register_mask(instruction, 2);
            for (i = 0; i < 8 && mask; ++i) {
                if (mask & (1U << i)) {
                    release_physical_copies(physical_register_copies[i]);
                    physical_register_copies[i] = 0;
                    mask &= ~(1U << i);
                }
            }
        }
    }
    reset_arena(3);
}

/* Original [0002e8f0,0002edb0). Eliminate repeated loads/stores and LEAs,
   respecting byte lanes, changed memory and ESP-relative displacement.
   Return-1 means removed, positive means changed and zero means unchanged. */
int __fastcall propagate_physical_copy(MachineInstruction *instruction)
{
    MachineInstruction *previous, *consumer;
    MachineOperand *reg, *value, *prior_value;
    MachineAddress *address;
    PhysicalRegisterCopy *copy;
    unsigned operation, prior_operation, opcode, prior_opcode, width, prior_width;
    unsigned value_high, register_high, prior_value_high, prior_register_high, number;
    int store, prior_store, changed, count, compatible;
    operation = (instruction->opcode >> 8) & 255;
    if (operation == MC_MOV) {
        store = (instruction->operands[0].flags & 15) != OPERAND_REGISTER;
        reg = &instruction->operands[store ? 1 : 0];
        value = &instruction->operands[store ? 0 : 1];
        value_high = (instruction->opcode >> (store ? 19 : 20)) & 1;
        if ((reg->flags & 15) == OPERAND_REGISTER) {
            for (copy = physical_register_copies[reg->value.reg.words.number]; copy; copy = copy->next) {
                previous = copy->instruction;
                prior_opcode = previous->opcode;
                if (((prior_opcode >> 8) & 255) != MC_MOV) continue;
                prior_store = (previous->operands[0].flags & 15) != OPERAND_REGISTER;
                prior_value = &previous->operands[prior_store ? 0 : 1];
                opcode = instruction->opcode;
                width = (opcode >> 4) & 15;
                prior_width = (prior_opcode >> 4) & 15;
                if ((opcode & 0x200000) && MEMORY_COPY_OPERAND(prior_value) && prior_width == 4) {
                    consumer = instruction->next;
                    while ((consumer->opcode & 15) != 7) {
                        prior_operation = (consumer->opcode >> 8) & 255;
                        if (!(consumer->opcode & 15) && X87_COPY_OPERATION(prior_operation) &&
                            machine_operand_counts[prior_operation] == 2 &&
                            operands_equal(value, &consumer->operands[1])) break;
                        consumer = consumer->next;
                    }
                    if (!operand_mentions_register(prior_value, reg->value.reg.words.number) &&
                        !operand_changed_between(previous, consumer, prior_value, width)) {
                        consumer->operands[1] = *prior_value;
                        address = new_machine_address();
                        consumer->operands[1].value.address = address;
                        *address = *prior_value->value.address;
                        remove_machine_instruction(instruction);
                        ++register_allocation_statistics.eliminated_copies;
                        return -1;
                    }
                    continue;
                }
                register_high = (opcode >> (store ? 20 : 19)) & 1;
                prior_register_high = (prior_opcode >> (prior_store ? 20 : 19)) & 1;
                prior_value_high = (prior_opcode >> (prior_store ? 19 : 20)) & 1;
                compatible = (register_high == prior_register_high && value_high == prior_value_high) ||
                    (value_high == register_high && prior_width > 1);
                if (compatible && width <= prior_width &&
                    (((operands_equal(value, prior_value) &&
                       !operand_changed_between(previous, instruction, prior_value, width)) ||
                      ((current_function->field28 & 0x10) &&
                       equivalent_stack_copy_operands(previous, prior_value, instruction, value) &&
                       !stack_copy_operand_changed(previous, instruction, prior_value, width))) &&
                     !operand_mentions_register(value, reg->value.reg.words.number))) {
                    remove_machine_instruction(instruction);
                    previous->opcode &= ~0x400000U;
                    ++register_allocation_statistics.eliminated_copies;
                    return -1;
                }
            }
            if ((instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                (instruction->operands[1].flags & 15) == OPERAND_IMMEDIATE &&
                !instruction->operands[1].value.constant) return 0;
        }
    } else if (operation == MC_LEA) {
        for (number = 0; number < 8; ++number) {
            if (number == REG_EBP || number == REG_ESP) continue;
            for (copy = physical_register_copies[number]; copy; copy = copy->next) {
                previous = copy->instruction;
                if (((previous->opcode >> 8) & 255) != MC_LEA ||
                    ((previous->opcode ^ instruction->opcode) & 0xf0)) continue;
                if (operands_equal(&previous->operands[1], &instruction->operands[1]) &&
                    !address_changed_between(previous->previous, instruction->next, &instruction->operands[1])) {
                    if (instruction->operands[0].value.reg.words.number == number) {
                        previous->opcode &= ~0x400000U;
                        remove_machine_instruction(instruction);
                        ++register_allocation_statistics.eliminated_copies;
                        return -1;
                    }
                    instruction->opcode = (instruction->opcode & ~0xff00U) | (MC_MOV << 8);
                    release_operand_address(&instruction->operands[1]);
                    instruction->operands[1] = previous->operands[0];
                    ++register_allocation_statistics.propagated_copies;
                    return 1;
                }
            }
        }
        return 0;
    }
    count = machine_operand_counts[(instruction->opcode >> 8) & 255];
    changed = 0;
    if (count == 2) {
        if ((instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
            instruction->operands[0].value.reg.words.number == REG_ESP) return 0;
        changed = propagate_physical_operand(instruction, &instruction->operands[1], 1);
    } else if (count != 1) return 0;
    if (!writes_first_operand(instruction))
        changed += propagate_physical_operand(instruction, &instruction->operands[0], 0);
    return changed;
}

/* Original [0002e6e0,0002e700). Recycle the complete chain in native
   head-first order, retaining each saved next link before overwriting it. */
void __fastcall release_physical_copies(PhysicalRegisterCopy *copies)
{
    PhysicalRegisterCopy *next;
    while (copies) {
        next = copies->next;
        copies->next = free_physical_register_copies;
        free_physical_register_copies = copies;
        copies = next;
    }
}

/* Original [0002edb0,0002f020). Replace an equal nonregister value by
   an available recorded register copy, preserving byte lanes and widths.
   ESP is excluded; EBP is a candidate only when allocated as a value. */
int __fastcall propagate_physical_operand(MachineInstruction *instruction, MachineOperand *operand, unsigned index)
{
    PhysicalRegisterCopy *copy;
    MachineInstruction *previous;
    MachineOperand *value, *reg;
    unsigned high, bytes, number, opcode, previous_high, previous_bytes, mask, register_high;
    int store, compatible;
    if ((operand->flags & 15) == OPERAND_REGISTER) return 0;
    high = (instruction->opcode >> (index ? 20 : 19)) & 1;
    bytes = machine_operand_bytes(instruction, index);
    for (number = 0; number < 8; ++number) {
        if (number == REG_ESP || (number == REG_EBP && !(current_function->field28 & 0x20))) continue;
        if (bytes == 1 && !byte_register_available((unsigned short)number)) continue;
        for (copy = physical_register_copies[number]; copy; copy = copy->next) {
            previous = copy->instruction;
            opcode = previous->opcode;
            if (((opcode >> 8) & 255) != MC_MOV) continue;
            store = (previous->operands[0].flags & 15) != OPERAND_REGISTER;
            value = &previous->operands[store ? 0 : 1];
            reg = &previous->operands[store ? 1 : 0];
            previous_high = (opcode >> (store ? 19 : 20)) & 1;
            previous_bytes = (opcode >> 4) & 15;
            compatible = high == previous_high ? bytes <= previous_bytes : bytes < previous_bytes;
            if (compatible && operands_equal(operand, value) &&
                !operand_mentions_register(operand, (unsigned short)number) &&
                !operand_changed_between(previous, instruction, value, (instruction->opcode >> 4) & 15) &&
                replace_machine_operand(instruction, index, reg, (unsigned char)3)) {
                opcode = previous->opcode;
                if ((opcode & 0xf0) == 0x10 && high == ((opcode >> (store ? 19 : 20)) & 1)) {
                    register_high = (opcode >> (store ? 20 : 19)) & 1;
                    mask = index ? 0x100000U : 0x80000U;
                    instruction->opcode = (instruction->opcode & ~mask) | (register_high << (index ? 20 : 19));
                }
                previous->opcode = opcode & ~0x400000U;
                ++register_allocation_statistics.propagated_copies;
                return 1;
            }
        }
    }
    return 0;
}

/* Original [0002e6a0,0002e6e0). A copy source is one instruction plus
   the bucket's previous head; recycled nodes precede fresh arena3 storage. */
void __fastcall remember_physical_copy(PhysicalRegisterCopy **copies, MachineInstruction *instruction)
{
    PhysicalRegisterCopy *copy;
    if (free_physical_register_copies) {
        copy = free_physical_register_copies;
        free_physical_register_copies = copy->next;
    } else copy = arena_alloc(3, sizeof(PhysicalRegisterCopy));
    copy->instruction = instruction;
    copy->next = *copies;
    *copies = copy;
}

/* Original [0002f110,0002f16d). Compare ESP-relative copies after
   accounting for stack movement in the open instruction interval. */
int __fastcall equivalent_stack_copy_operands(MachineInstruction *first, MachineOperand *first_operand,
    MachineInstruction *last, MachineOperand *last_operand)
{
    unsigned kind = first_operand->flags & 15, displacement;
    if ((kind != OPERAND_BASED && kind != OPERAND_INDEXED) ||
        first_operand->value.address->base.words.number != REG_ESP ||
        !same_machine_operand_base(first_operand, last_operand)) return 0;
    displacement = (unsigned)last_operand->value.address->first.displacement -
        stack_adjustment_between(first, last);
    return displacement == (unsigned)first_operand->value.address->first.displacement;
}

/* Original [0002f170,0002f207). Net stack depth over the open interval.
   Only ordinary kind0 operations participate; PUSH/POP count four bytes
   and calls remove their recorded argument bytes. Arithmetic wraps as
   the original32-bit accumulator does. The caller supplies a linked end. */
unsigned __fastcall stack_adjustment_between(MachineInstruction *first,
    MachineInstruction *last)
{
    MachineInstruction *instruction;
    unsigned adjustment = 0, operation;
    for (instruction = first->next; instruction != last; instruction = instruction->next) {
        if (instruction->opcode & 15) continue;
        operation = (instruction->opcode >> 8) & 255;
        switch (operation) {
        case MC_POP: adjustment -= 4; break;
        case MC_PUSH: adjustment += 4; break;
        case MC_CALL: adjustment -= instruction->operands[1].value.call->argument_bytes; break;
        case MC_ADD: case MC_SUB:
            if ((instruction->operands[0].flags & 15) != OPERAND_REGISTER ||
                instruction->operands[0].value.reg.words.number != REG_ESP) break;
            if ((instruction->operands[1].flags & 15) != OPERAND_IMMEDIATE)
                compiler_internal_error("@(#)color.c:1.101", 0xd5f);
            if (operation == MC_SUB) adjustment += instruction->operands[1].value.constant;
            else adjustment -= instruction->operands[1].value.constant;
            break;
        }
    }
    return adjustment;
}

/* Original2f020..2f103, first ECX/last EDX/operand and byte width stack,
   RET8. Inspect the open interval with a private address copy, tracking ESP
   movement. Unsupported addressing and intervening labels are conservative. */
int __fastcall stack_copy_operand_changed(MachineInstruction *first,
    MachineInstruction *last, MachineOperand *operand, unsigned bytes)
{
    MachineOperand saved;
    MachineInstruction *instruction;
    unsigned kind = operand->flags & 15;
    if ((kind != OPERAND_BASED && kind != OPERAND_INDEXED) ||
        operand->value.address->base.words.number != REG_ESP || !operand->value.address->symbol) return 1;
    if (kind == OPERAND_INDEXED &&
        (register_reuse_barriers(first, last) &
         (1U << ((unsigned char)operand->value.address->index.words.number & 31)))) return 1;
    copy_machine_operand(operand, &saved);
    for (instruction = first->next; instruction != last; instruction = instruction->next) {
        if (!instruction) compiler_internal_error("@(#)color.c:1.101", 0xd7c);
        kind = instruction->opcode & 15;
        if (kind == 1) return 1;
        if ((kind == 0 || kind == 10) && stack_copy_instruction_changed(instruction, &saved, bytes)) return 1;
    }
    return 0;
}

/* Original2f210..2f3ea, instruction ECX/private operand EDX/width stack,
   RET4. Track the saved ESP-relative displacement before testing writes.
   A kind19 temporary is private; other symbols may expose their stack home
   through the original attributes bit0x2000. */
int __fastcall stack_copy_instruction_changed(MachineInstruction *instruction,
    MachineOperand *operand, unsigned bytes)
{
    MachineAddress *address = operand->value.address, *destination;
    SymbolHeader *symbol = address->symbol;
    unsigned operation = (instruction->opcode >> 8) & 255, kind, difference;
    switch (operation) {
    case MC_POP:
        address->first.displacement = (long)((unsigned)address->first.displacement - 4);
        break;
    case MC_PUSH:
        address->first.displacement = (long)((unsigned)address->first.displacement + 4);
        break;
    case MC_CALL:
        if (symbol->kind != 19 && (((BackendSymbol *)symbol)->attributes & 0x2000)) return 1;
        address->first.displacement = (long)((unsigned)address->first.displacement -
            instruction->operands[1].value.call->argument_bytes);
        break;
    case MC_ADD: case MC_SUB:
        if ((instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
            instruction->operands[0].value.reg.words.number == REG_ESP) {
            if (operation == MC_SUB)
                address->first.displacement = (long)((unsigned)address->first.displacement + instruction->operands[1].value.constant);
            else address->first.displacement = (long)((unsigned)address->first.displacement - instruction->operands[1].value.constant);
        }
        break;
    case MC_INS: case MC_MOVS: case MC_STOS:
        return 1;
    }
    if (machine_operand_counts[operation] <= 0 || !writes_first_operand(instruction)) return 0;
    kind = instruction->operands[0].flags & 15;
    if (kind != OPERAND_BASED && kind != OPERAND_INDEXED) return 0;
    destination = instruction->operands[0].value.address;
    if (!destination->symbol) {
        if (symbol->kind == 19 || !(((BackendSymbol *)symbol)->attributes & 0x2000)) return 0;
        return 1;
    }
    if (destination->symbol != symbol) return 0;
    if (destination->base.words.number != REG_ESP)
        compiler_internal_error("@(#)color.c:1.101", 0xdb0);
    difference = instruction->operands[0].flags ^ operand->flags;
    if (difference & 15) return 1;
    if ((operand->flags & 15) == OPERAND_INDEXED &&
        (address->index.words.number != destination->index.words.number || (difference & 0x300))) return 1;
    return memory_displacements_overlap(&instruction->operands[0],
        (instruction->opcode >> 4) & 15, operand, bytes);
}
