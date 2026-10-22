/* Final adjacent-operation peepholes after branch placement. */
#include "jump-threading.h"
#include "machine-layout.h"
#include "instruction-rewrite.h"
#include "register-use.h"
#include "frame.h"
#include <recon.h>
extern int cpu_model;

/* Original [000349e0,00034d00). Simplify adjacent CALL/RET, immediate
   arithmetic and stack-restoration sequences without crossing labels.
   A removed ESP adjustment makes the next LEAVE retain its stack reset. */
void __fastcall simplify_machine_pairs(MachineContext *context)
{
    MachineInstruction *instruction = next_machine_operation(context), *next, *removed, *created;
    MachineOperand frame_operand, stack_operand;
    unsigned operation, next_operation;
    int removed_stack_adjustment = 0;
    while (instruction) {
        next = instruction->next;
        if (!next) return;
        while (next && ((next->opcode & 15) == 4 || (next->opcode & 15) == 7 ||
                        (next->opcode & 15) == 11)) next = next->next;
        if (!next) return;
        if ((next->opcode & 15) != 0) {
            instruction = next_machine_operation(next);
            continue;
        }
        operation = (instruction->opcode >> 8) & 255;
        next_operation = (next->opcode >> 8) & 255;
        if (operation == MC_CALL) {
            if (next_operation == MC_RET && next->operands[0].value.constant == 0 &&
                instruction->operands[1].value.call->argument_bytes == 0 &&
                !machine_block_protected(instruction) && !machine_block_protected(next)) {
                instruction->opcode = (instruction->opcode & 0xffff10ffUL) | (MC_JMP << 8);
                removed = next;
                next = next_machine_operation(removed);
                remove_machine_instruction(removed);
            }
        } else if (operation == MC_ADD) {
            if (next_operation == MC_LEA) {
                if ((instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                    instruction->operands[0].value.reg.words.number == REG_ESP &&
                    (next->operands[0].flags & 15) == OPERAND_REGISTER &&
                    next->operands[0].value.reg.words.number == REG_ESP &&
                    (next->operands[1].flags & 15) == OPERAND_BASED &&
                    next->operands[1].value.address->base.words.number == REG_EBP &&
                    !machine_block_protected(instruction) && !machine_block_protected(next))
                    remove_machine_instruction(instruction);
            } else if (next_operation == MC_ADD || next_operation == MC_SUB) {
                if ((instruction->operands[1].flags & 15) == OPERAND_IMMEDIATE &&
                    (next->operands[1].flags & 15) == OPERAND_IMMEDIATE &&
                    operands_equal(&instruction->operands[0], &next->operands[0]) &&
                    !machine_block_protected(instruction) && !machine_block_protected(next)) {
                    if (next_operation == MC_ADD)
                        next->operands[1].value.constant += instruction->operands[1].value.constant;
                    else next->operands[1].value.constant -= instruction->operands[1].value.constant;
                    remove_machine_instruction(instruction);
                    if (!next->operands[1].value.constant) {
                        removed = next;
                        next = next_machine_operation(removed);
                        remove_machine_instruction(removed);
                    }
                }
            } else if (next_operation == MC_LEAVE &&
                       (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                       instruction->operands[0].value.reg.words.number == REG_ESP &&
                       !machine_block_protected(instruction) && !machine_block_protected(next)) {
                removed_stack_adjustment = 1;
                remove_machine_instruction(instruction);
            }
        } else if (operation == MC_LEAVE) {
            if (cpu_model > 3 && !machine_block_protected(instruction)) {
                initialize_operand(&frame_operand, OPERAND_REGISTER, REG_EBP, 0, 0);
                created = new_machine_record(0, MC_POP, 0x104, &frame_operand);
                insert_machine_after(instruction, created);
                if (removed_stack_adjustment || current_symbol->field30 || dynamic_stack_allocation) {
                    initialize_operand(&stack_operand, OPERAND_REGISTER, REG_ESP, 0, 0);
                    created = new_machine_record(0, MC_MOV, 0x104, &stack_operand, &frame_operand);
                    insert_machine_after(instruction, created);
                }
                remove_machine_instruction(instruction);
            }
            removed_stack_adjustment = 0;
        }
        instruction = next;
    }
}
