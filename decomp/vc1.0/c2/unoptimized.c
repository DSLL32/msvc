/* Physical-register assignment for the unoptimized machine stream. */
#include "unoptimized.h"
#include "register-use.h"
#include "instruction-rewrite.h"
RegisterSlot unoptimized_registers[8]; /* 0007c358 */
unsigned unoptimized_live_mask; /* 0007c398 */
BitSet *unoptimized_live_temporaries; /* 0007c39c */
/* Original [00031d80,00031f30). Process sources before destinations, keep
   address base/index annotations separate, and discard per-block mappings. */
void lower_unoptimized_machine(void)
{
    MachineInstruction *instruction, *next;
    MachineOperand *operand;
    MachineAddress *address;
    unsigned kind, operation;
    int count, slot;
    for (instruction = machine_instructions; instruction; instruction = next) {
        next = instruction->next;
        kind = instruction->opcode & 15;
        if (kind == 0) {
            operation = (instruction->opcode >> 8) & 255;
            for (count = machine_operand_counts[operation]; count > 0; --count) {
                operand = &instruction->operands[count == 1 ? 0 : 1];
                kind = operand->flags & 15;
                if (kind == OPERAND_REGISTER) {
                    if (operand->value.reg.words.number & 0xc000) {
                        operation = (instruction->opcode >> 8) & 255;
                        if ((operation >= MC_FADD && operation <= MC_FLD1) ||
                            (operation >= MC_FUCOM && operation <= MC_FCOS))
                            operand->value.reg.words.number = REG_ST0;
                        else operand->value.reg.words.number = (unsigned short)assign_physical_register(
                            instruction, operand->value.reg.words.number, (operand->flags >> 10) & 1);
                    }
                } else if (kind == OPERAND_BASED) {
                    address = operand->value.address;
                    address->base.words.number = (unsigned short)assign_physical_register(
                        instruction, address->base.words.number, (operand->flags >> 10) & 1);
                } else if (kind == OPERAND_INDEXED) {
                    address = operand->value.address;
                    if (address->base.words.number != 0x1b)
                        address->base.words.number = (unsigned short)assign_physical_register(
                            instruction, address->base.words.number, (operand->flags >> 10) & 1);
                    address->index.words.number = (unsigned short)assign_physical_register(
                        instruction, address->index.words.number, (operand->flags >> 11) & 1);
                }
            }
        } else if (kind == 7) {
            if (((MachineBlockHeader *)instruction)->flags & 2)
                next = next_machine_block(instruction);
            else initialize_lowering_block((MachineBlockHeader *)instruction);
        }
    }
    for (slot = 0; slot < 8; ++slot) release_bitset(unoptimized_registers[slot].conflicts);
    for (slot = 0; slot < 8; ++slot) {
        unoptimized_registers[slot].virtual_register = 0;
        unoptimized_registers[slot].conflicts = 0;
    }
    release_bitset(unoptimized_live_temporaries);
    unoptimized_live_temporaries = 0;
}

/* Original [00031f30,000321a0). Build physical/virtual interference sets
   for one selected block before mapping its virtual register operands. */
void __fastcall initialize_lowering_block(MachineBlockHeader *block)
{
    MachineInstruction *instruction;
    MachineOperand *operand;
    MachineAddress *address;
    unsigned operation, kind, defined, last_use, bit, reg;
    int count;
    clear_bitset(unoptimized_live_temporaries);
    unoptimized_live_mask = 0;
    for (reg = 0; reg < 8; ++reg) {
        unoptimized_registers[reg].virtual_register = 0x1a;
        if (reg != REG_ESP && reg != REG_EBP) {
            clear_bitset(unoptimized_registers[reg].conflicts);
            if (block->uses && bitset_contains(block->uses, reg))
                unoptimized_live_mask |= 1U << reg;
        }
    }
    for (instruction = block->next; instruction; instruction = instruction->next) {
        kind = instruction->opcode & 15;
        if (kind == 7) return;
        if (kind == 0) {
            operation = (instruction->opcode >> 8) & 255;
            count = machine_operand_counts[operation];
            if (count == 2) {
                operand = &instruction->operands[1];
                kind = operand->flags & 15;
                if (kind == OPERAND_REGISTER) {
                    if (operand->flags & 0x400) mark_lowering_register_dead(operand->value.reg.words.number);
                } else if (kind == OPERAND_BASED) {
                    if (operand->flags & 0x400)
                        mark_lowering_register_dead(operand->value.address->base.words.number);
                } else if (kind == OPERAND_INDEXED) {
                    address = operand->value.address;
                    if (operand->flags & 0x800) mark_lowering_register_dead(address->index.words.number);
                    if (address->base.words.number != 0x1b && (operand->flags & 0x400))
                        mark_lowering_register_dead(address->base.words.number);
                }
            }
            if (count == 1 || count == 2) {
                operand = &instruction->operands[0];
                kind = operand->flags & 15;
                if (kind == OPERAND_REGISTER) {
                    if (writes_first_operand(instruction)) {
                        reg = operand->value.reg.words.number;
                        mark_lowering_register_live(reg);
                        record_lowering_conflicts(reg);
                    }
                    if (operand->flags & 0x400) mark_lowering_register_dead(operand->value.reg.words.number);
                } else if (kind == OPERAND_BASED) {
                    if (operand->flags & 0x400)
                        mark_lowering_register_dead(operand->value.address->base.words.number);
                } else if (kind == OPERAND_INDEXED) {
                    address = operand->value.address;
                    if (operand->flags & 0x800) mark_lowering_register_dead(address->index.words.number);
                    if (address->base.words.number != 0x1b && (operand->flags & 0x400))
                        mark_lowering_register_dead(address->base.words.number);
                }
            }
            record_byte_register_constraints(instruction, 0x1a);
            if (machine_selection_properties[operation] & MC_IMPLICIT_REGISTERS) {
                defined = implicit_register_mask(instruction, 2);
                last_use = instruction->operands[1].flags;
                for (reg = 0; reg < 8 && (last_use || defined); ++reg) {
                    bit = 1U << reg;
                    if (defined & bit) {
                        record_lowering_conflicts(reg);
                        defined &= ~bit;
                        if (last_use & bit) mark_lowering_register_dead(reg);
                        else mark_lowering_register_live(reg);
                    } else if (last_use & bit) mark_lowering_register_dead(reg);
                }
            }
        } else if (kind == 10 && (instruction->operands[1].flags & 0x400))
            mark_lowering_register_dead(instruction->operands[1].value.reg.words.number);
    }
}

/* Original [00032760,00032800). ESI and EDI have no byte-sized register
   view, so byte temporaries interfere with both physical choices. */
void __fastcall record_byte_register_constraints(MachineInstruction *instruction, unsigned short reg)
{
    int index;
    MachineOperand *operand;
    unsigned short temporary;
    for (index = 0; index < machine_operand_counts[(instruction->opcode >> 8) & 255]; ++index) {
        operand = &instruction->operands[index == 0 ? 0 : 1];
        temporary = operand->value.reg.words.number;
        if ((operand->flags & 15) == OPERAND_REGISTER && (temporary & 0xc000) &&
            (reg == 0x1a || reg == temporary) && machine_operand_bytes(instruction, index) == 1) {
            unoptimized_registers[6].conflicts = bitset_include(unoptimized_registers[6].conflicts, temporary & 0x3fff);
            unoptimized_registers[7].conflicts = bitset_include(unoptimized_registers[7].conflicts, temporary & 0x3fff);
        }
    }
}

/* Original [00032680,000326d0). Track only allocatable integer physical
   registers here; tagged temporary IDs have their own live set. */
void __fastcall mark_lowering_register_live(unsigned reg)
{
    if (reg & 0xc000)
        unoptimized_live_temporaries = bitset_include(unoptimized_live_temporaries, reg & 0xffff3fffU);
    else if (reg < 8 && reg != REG_EBP && reg != REG_ESP)
        unoptimized_live_mask |= 1U << reg;
}

/* Original [000326d0,00032760). A live physical register excludes all live
   temporaries; a temporary excludes each currently live physical register. */
void __fastcall record_lowering_conflicts(unsigned reg)
{
    unsigned physical, live;
    if (!(reg & 0xc000)) {
        if ((int)reg < 8 && reg != REG_EBP && reg != REG_ESP)
            unoptimized_registers[reg].conflicts = bitset_union(
                unoptimized_registers[reg].conflicts, unoptimized_live_temporaries);
    } else {
        live = unoptimized_live_mask;
        if (live) for (physical = 0; physical < 8; ++physical) {
            if (live & (1U << physical)) {
                unoptimized_registers[physical].conflicts = bitset_include(
                    unoptimized_registers[physical].conflicts, reg & 0xffff3fffU);
                live = unoptimized_live_mask;
            }
        }
    }
}

/* Original [00032630,00032680). End a physical or temporary live range. */
void __fastcall mark_lowering_register_dead(unsigned reg)
{
    if (reg & 0xc000)
        unoptimized_live_temporaries = bitset_remove(unoptimized_live_temporaries, reg & 0xffff3fffU);
    else if (reg < 8 && reg != REG_EBP && reg != REG_ESP)
        unoptimized_live_mask &= ~(1U << reg);
}
