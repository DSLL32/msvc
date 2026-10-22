/* Place virtual-register transfer records before physical allocation. */
#include "machine-transfers.h"
#include "instruction-rewrite.h"
#include "register-use.h"
#include "bitsets.h"
#include "backend-passes.h"
#include "register-transfers.h"
#include "value-selection.h"

/* Original [00023e30,00023ee0). Place each virtual-register load/store
   at its nearest required instruction or call boundary. Preserve the saved
   traversal link while splicing records, and report whether any moved. */
int __fastcall prepare_machine_context(MachineInstruction *instructions)
{
    MachineInstruction *instruction, *next, *before;
    unsigned kind, flags;
    int changed = 0;
    for (instruction = instructions; instruction; instruction = next) {
        next = instruction->next;
        flags = instruction->opcode;
        kind = flags & 15;
        if (!((kind == 8 && (instruction->operands[0].value.reg.words.number & 0xc000)) || kind == 9) ||
            (flags & MACHINE_TRANSFER_PLACED)) continue;
        before = instruction;
        if (flags & MACHINE_TRANSFER_CALL_BOUNDARY)
            before = find_transfer_call_boundary(instruction);
        before = find_transfer_use_boundary(instruction, before);
        if ((instruction->opcode & 15) == 9) before = before->next;
        instruction->opcode |= MACHINE_TRANSFER_PLACED;
        if (before != instruction && before != instruction->next) {
            changed = 1;
            instruction->previous->next = instruction->next;
            instruction->next->previous = instruction->previous;
            instruction->previous = before->previous;
            instruction->next = before;
            before->previous->next = instruction;
            before->previous = instruction;
        }
    }
    return changed;
}

/* Original [0001a640,0001a730). Emit a MOV-shaped kind8 load or kind9
   store. Its memory operand comes from the register's canonical address;
   EAX/EDX retain their shared physical operand descriptors. */
void __fastcall emit_register_lifetime(unsigned short reg, unsigned type, int begin, unsigned flags)
{
    MachineInstruction *instruction;
    MachineOperand *home, *register_operand;
    instruction = append_machine_instruction(begin ? 8 : 9);
    instruction->opcode = (instruction->opcode & ~0xff00U) | (MC_MOV << 8);
    instruction->opcode = (instruction->opcode & ~0xf0U) | ((type << 4) & 0xf0U);
    if (reg & 0xc000) home = symbol_operand(register_memory_address(reg), 1);
    else if (!(reg & 0x3fff)) home = &operand_eax;
    else if ((reg & 0x3fff) == 2) home = &operand_edx;
    else {
        home = 0;
        compiler_internal_error("@(#)codegen.c:1.167", 0x81e);
    }
    if (begin) {
        register_operand = &instruction->operands[0];
        register_operand->flags = (register_operand->flags & ~15U) | OPERAND_REGISTER;
        register_operand->value.reg.words.number = reg;
        instruction->operands[1] = *home;
    } else {
        register_operand = &instruction->operands[1];
        register_operand->flags = (register_operand->flags & ~15U) | OPERAND_REGISTER;
        register_operand->value.reg.words.number = reg;
        instruction->operands[0] = *home;
    }
    if (flags) instruction->opcode |= MACHINE_TRANSFER_CALL_BOUNDARY;
    else instruction->opcode &= ~MACHINE_TRANSFER_CALL_BOUNDARY;
}

/* Original [00023c70,00023dc0). Move loads forward to their first use and
   stores backward to the last defining instruction. Stop at block or label
   boundaries; a store reaching a block head becomes an upward-exposed use. */
MachineInstruction *__fastcall find_transfer_use_boundary(MachineInstruction *transfer, MachineInstruction *start)
{
    MachineInstruction *candidate = start, *header;
    unsigned kind = transfer->opcode & 15, candidate_kind, operation;
    unsigned short number;
    if (kind == 8) number = transfer->operands[0].value.reg.words.number;
    else number = transfer->operands[1].value.reg.words.number;
    if (!(number & 0x4000)) compiler_internal_error("@(#)peeps.c:1.148", 0xdf8);
    for (;;) {
        candidate = kind == 8 ? candidate->next : candidate->previous;
        candidate_kind = candidate->opcode & 15;
        if (candidate_kind == 7 ||
            (candidate_kind == 1 && candidate->record.symbol->field0d != 1)) {
            if (kind != 9) return candidate;
            header = candidate;
            if (candidate_kind == 1) {
                header = header->previous;
                while ((header->opcode & 15) == 4) header = header->previous;
                if ((header->opcode & 15) != 7) continue;
            }
            ((MachineBlockHeader *)header)->uses = bitset_include(
                ((MachineBlockHeader *)header)->uses, transfer->operands[1].value.reg.words.number & 0x3fff);
            return candidate;
        }
        if (candidate_kind != 0 && candidate_kind != 9 && candidate_kind != 8) continue;
        operation = candidate->opcode & 0xff00;
        if (kind == 8 && (operation == (MC_JCC << 8) || operation == (MC_JMP << 8))) return candidate;
        if (!instruction_mentions_register(candidate, number)) continue;
        if (kind == 8) return candidate;
        if ((candidate->operands[0].flags & 15) == 1 &&
            candidate->operands[0].value.reg.words.number == number && writes_first_operand(candidate)) return candidate;
    }
}
