/* Physical-register assignment for the unoptimized machine stream. */
#include "unoptimized.h"
#include "register-use.h"
#include "instruction-rewrite.h"
#include "temporaries.h"
#include <recon.h>
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

unsigned __fastcall spill_lowering_register(MachineInstruction *instruction, unsigned reg);
/* Original321a0..32276. Reuse an existing mapping first; otherwise choose
   a free nonconflicting integer register or spill one through the slow path. */
unsigned __fastcall assign_physical_register(MachineInstruction *instruction, unsigned reg, int last_use)
{
    unsigned slot;
    if (!(reg & 0xc000)) return reg;
    for (slot = 0; slot < 8; ++slot) {
        if (unoptimized_registers[slot].virtual_register == reg) {
            if (last_use) unoptimized_registers[slot].virtual_register = 0x1a;
            return slot;
        }
    }
    for (slot = 0; slot < 8; ++slot) {
        if (slot == REG_ESP || slot == REG_EBP) continue;
        if (unoptimized_registers[slot].virtual_register == 0x1a &&
            (!unoptimized_registers[slot].conflicts ||
             !bitset_contains(unoptimized_registers[slot].conflicts, reg & 0xffff3fffU))) {
            unoptimized_registers[slot].virtual_register = reg;
            return slot;
        }
    }
    slot = spill_lowering_register(instruction, reg);
    unoptimized_registers[slot].virtual_register = reg;
    return slot;
}

/* Original32280..32622, ECX current instruction/EDX tagged virtual register,
   RET0. Preserve an occupied physical register in a real stack temporary,
   reload its former virtual value at its next use, then extend conflicts
   through that value's remaining register/address lifetime. */
unsigned __fastcall spill_lowering_register(MachineInstruction *instruction, unsigned reg)
{
    MachineInstruction *before, *after, *store, *scan;
    MachineOperand memory, source;
    MachineOperand *operand;
    MachineAddress *address;
    TemporaryRegister *temporary;
    unsigned candidates, victim, physical, slot, operation, kind, flags, implicit;
    unsigned short number;
    int count, finished = 0;
    candidates = lowering_spill_candidates(reg);
    if (!candidates) compiler_internal_error("@(#)regasg.c:1.25", 0x1c1);
    victim = choose_lowering_spill(instruction, candidates);
    if (victim == 0x1a) compiler_internal_error("@(#)regasg.c:1.25", 0x1c5);
    physical = require_lowering_register(victim);
    memory.flags = memory.value.constant = 0;
    source.flags = source.value.constant = 0;
    temporary = allocate_stack_temporary(4);
    for (before = instruction->previous; before; before = before->previous)
        if (instruction_mentions_register(before, (unsigned short)physical)) break;
    initialize_operand(&memory, OPERAND_BASED, REG_EBP, 0, (SymbolHeader *)temporary);
    initialize_operand(&source, OPERAND_REGISTER, (unsigned short)physical, 0, 0);
    store = new_machine_record(0, MC_MOV, 4, &memory, &source);
    insert_machine_after(before, store);
    store->operands[1].flags |= 0x400;
    for (after = instruction->next; after; after = after->next)
        if (instruction_mentions_register(after, (unsigned short)victim)) break;
    initialize_operand(&memory, OPERAND_BASED, REG_EBP, 0, (SymbolHeader *)temporary);
    source.value.reg.words.number = (unsigned short)victim;
    insert_machine_after(after->previous, new_machine_record(0, MC_MOV, 4, &source, &memory));
    /* These sets have persistent sentinel owners. The original discards the
       remove return here; later includes explicitly publish their result. */
    for (slot = 0; slot < 8; ++slot)
        if (unoptimized_registers[slot].conflicts)
            bitset_remove(unoptimized_registers[slot].conflicts, victim & 0xffff3fffU);
#define ADD_SPILL_CONFLICT(number) do { \
    if (!((number) & 0xc000) && ((number) <= 7 || (number) > 31) && \
        (number) != REG_EBP && (number) != REG_ESP) \
        unoptimized_registers[number].conflicts = bitset_include( \
            unoptimized_registers[number].conflicts, victim & 0xffff3fffU); \
} while (0)
    /* Valid lowering operands identify physical0..7 or tagged temporaries;
       the original additionally skips the untagged8..31 special registers. */
    for (scan = after; ; scan = scan->next) {
        kind = scan->opcode & 15;
        if (kind == 7) break;
        if (kind) continue;
        operation = (scan->opcode >> 8) & 255;
        for (count = machine_operand_counts[operation]; count > 0; --count) {
            operand = &scan->operands[count == 2 ? 1 : 0];
            flags = operand->flags;
            switch (flags & 15) {
            case OPERAND_REGISTER:
                number = operand->value.reg.words.number;
                if (!(number & 0xc000) && (number <= 7 || number > 31) &&
                    number != REG_EBP && number != REG_ESP) {
                    ADD_SPILL_CONFLICT(number);
                } else if (number == (unsigned short)victim && (flags & 0x400)) finished = 1;
                break;
            case OPERAND_INDEXED:
                address = operand->value.address;
                number = address->index.words.number;
                ADD_SPILL_CONFLICT(number);
                if (number == (unsigned short)victim && (flags & 0x800)) finished = 1;
                /* Fall through: indexed operands also have a base lifetime. */
            case OPERAND_BASED:
                address = operand->value.address;
                number = address->base.words.number;
                ADD_SPILL_CONFLICT(number);
                if (number == (unsigned short)victim && (flags & 0x400)) finished = 1;
                break;
            }
        }
        record_byte_register_constraints(scan, (unsigned short)victim);
        if (machine_selection_properties[operation] & MC_IMPLICIT_REGISTERS) {
            implicit = implicit_register_mask(scan, 3);
            for (slot = 0; slot < 8 && implicit; ++slot)
                if (implicit & (1U << slot))
                    unoptimized_registers[slot].conflicts = bitset_include(
                        unoptimized_registers[slot].conflicts, victim & 0xffff3fffU);
        }
        if (finished) break;
    }
#undef ADD_SPILL_CONFLICT
    return physical;
}

/* Original32870..328bc, ECX requested virtual register/RET0. EBP and
   ESP are reserved; each remaining slot's conflict set excludes candidates. */
unsigned __fastcall lowering_spill_candidates(unsigned reg)
{
    unsigned slot, candidates = 0;
    for (slot = 0; slot < 8; ++slot)
        if (slot != REG_EBP && slot != REG_ESP &&
            !bitset_contains(unoptimized_registers[slot].conflicts, reg & 0xffff3fffU))
            candidates |= 1U << slot;
    return candidates;
}

/* Original32900..32a1e, ECX current instruction/EDX physical candidate mask,
   RET0. Eliminate candidates at their next virtual use and retain the last
   such virtual identifier, processing source operands before destinations. */
unsigned __fastcall choose_lowering_spill(MachineInstruction *instruction, unsigned candidates)
{
    MachineInstruction *scan;
    MachineOperand *operand;
    MachineAddress *address;
    unsigned victim = 0x1a, number;
    int count;
    for (scan = instruction->next; (scan->opcode & 15) != 7 && candidates; scan = scan->next) {
        if (scan->opcode & 15) continue;
        for (count = machine_operand_counts[(scan->opcode >> 8) & 255]; count > 0; --count) {
            operand = &scan->operands[count == 1 ? 0 : 1];
            switch (operand->flags & 15) {
            case OPERAND_REGISTER:
                number = operand->value.reg.words.number;
                if (lowering_register_is_candidate(number, candidates)) {
                    victim = number;
                    candidates &= ~(1U << require_lowering_register(number));
                }
                break;
            case OPERAND_INDEXED:
                address = operand->value.address;
                number = address->index.words.number;
                if (lowering_register_is_candidate(number, candidates)) {
                    victim = number;
                    candidates &= ~(1U << require_lowering_register(number));
                }
                if (address->base.words.number == 0x1b) break;
                /* Fall through: an indexed address may also use a candidate base. */
            case OPERAND_BASED:
                number = operand->value.address->base.words.number;
                if (lowering_register_is_candidate(number, candidates)) {
                    victim = number;
                    candidates &= ~(1U << require_lowering_register(number));
                }
                break;
            }
        }
    }
    return victim;
}

/* Original328c0..328f4, ECX virtual identifier/EDX candidate mask, RET0.
   A physical/special register is not a virtual eviction candidate. */
unsigned __fastcall lowering_register_is_candidate(unsigned reg, unsigned candidates)
{
    unsigned slot;
    if (!(reg & 0xc000)) return 0;
    slot = lookup_lowering_register(reg);
    return slot != 0x1a && (candidates & (1U << slot)) != 0;
}

/* Original32800..32839, ECX tagged virtual identifier/RET0. Search the
   eight current mappings;1a is the original unmapped sentinel. */
unsigned __fastcall lookup_lowering_register(unsigned reg)
{
    unsigned slot;
    if (!(reg & 0x8000)) compiler_internal_error("@(#)regasg.c:1.25", 0x158);
    for (slot = 0; slot < 8; ++slot)
        if (unoptimized_registers[slot].virtual_register == reg) return slot;
    return 0x1a;
}

/* Original32840..32861, ECX mapped virtual identifier/RET0. The caller
   requires an existing owner, retaining the original diagnostic boundary. */
unsigned __fastcall require_lowering_register(unsigned reg)
{
    unsigned slot = lookup_lowering_register(reg);
    if (slot == 0x1a) compiler_internal_error("@(#)regasg.c:1.25", 0x165);
    return slot;
}
