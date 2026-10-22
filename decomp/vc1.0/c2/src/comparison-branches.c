/* Sharing comparisons across conditional branches. */
#include "comparison-branches.h"
#include "machine-layout.h"
#include "instruction-rewrite.h"
#include "register-use.h"
extern int optimization_jumps, cpu_model;

/* Original [00033f50,00034360). Share a comparison already present at a
   conditional destination, including Pentium load/register-renaming cases. */
void __fastcall optimize_branch_comparisons(MachineContext *context)
{
    MachineInstruction *comparison, *branch, *target, *after_target, *before;
    MachineInstruction *copy;
    MachineOperand temporary;
    unsigned operation, target_opcode, kind;
    unsigned short saved_register, replacement_register;
    int matches, swapped;
    if (!optimization_jumps) return;
    for (comparison = next_machine_operation(context); comparison;
         comparison = next_machine_operation(comparison)) {
        operation = (comparison->opcode >> 8) & 255;
        if (operation != MC_CMP && operation != MC_TEST &&
            !(operation == MC_OR && operands_equal(&comparison->operands[0], &comparison->operands[1]) &&
              ((comparison->opcode & 0xf0) > 0x10 ||
               (((comparison->opcode >> 19) ^ (comparison->opcode >> 20)) & 1) == 0)))
            continue;
        if (machine_block_protected(comparison)) continue;
        branch = comparison->next;
        while ((branch->opcode & 15) == 4 || (branch->opcode & 15) == 7)
            branch = branch->next;
        if ((branch->opcode & 15) != 0 || ((branch->opcode >> 8) & 255) != MC_JCC ||
            (branch->operands[0].flags & 15) != OPERAND_SYMBOL ||
            branch->operands[0].value.symbol->kind != 3)
            continue;
        target = label_machine_instruction((LabelSymbol *)branch->operands[0].value.symbol);
        if (machine_block_protected(target)) continue;
        do {
            target = target->next;
        } while ((target->opcode & 15) == 4 || (target->opcode & 15) == 7);
        if (target != comparison &&
            (machine_records_equal(comparison, target) ||
             swap_comparison_for_match(comparison, target, branch))) {
            retarget_comparison_branch(target, branch, 0, 0);
            continue;
        }
        if (cpu_model != 5) continue;
        target_opcode = target->opcode;
        if ((target_opcode & 15) != 0 || ((target_opcode >> 8) & 255) != MC_MOV ||
            (target->operands[0].flags & 15) != OPERAND_REGISTER)
            continue;
        kind = target->operands[1].flags & 15;
        if (kind != OPERAND_BASED && kind != OPERAND_INDEXED && kind != OPERAND_ABSOLUTE)
            continue;
        after_target = target->next;
        while ((after_target->opcode & 15) == 4 || (after_target->opcode & 15) == 7)
            after_target = after_target->next;
        if ((after_target->opcode & 15) != 0 || after_target == comparison) continue;
        before = comparison->previous;
        while ((before->opcode & 15) == 4 || (before->opcode & 15) == 7)
            before = before->previous;
        if (machine_records_equal(before, target)) {
            if (!operand_mentions_register(&before->operands[1], before->operands[0].value.reg.words.number) &&
                (machine_records_equal(comparison, after_target) ||
                 swap_comparison_for_match(comparison, after_target, branch))) {
                retarget_comparison_branch(after_target, branch, target, 1);
                if (!(target->opcode & 0x400000UL)) before->opcode &= ~0x400000UL;
            }
            continue;
        }
        if ((before->opcode & 15) != 0 || ((before->opcode >> 8) & 255) != MC_MOV ||
            (before->operands[0].flags & 15) != OPERAND_REGISTER ||
            !operands_equal(&before->operands[1], &target->operands[1]) ||
            operand_mentions_register(&before->operands[1], before->operands[0].value.reg.words.number) ||
            !(target_opcode & 0x400000UL) ||
            !instruction_mentions_register(comparison, before->operands[0].value.reg.words.number))
            continue;
        saved_register = before->operands[0].value.reg.words.number;
        replacement_register = target->operands[0].value.reg.words.number;
        copy = clone_machine_record(comparison);
        before->operands[0].value.reg.words.number = replacement_register;
        if ((copy->operands[0].flags & 15) == OPERAND_REGISTER &&
            copy->operands[0].value.reg.words.number == saved_register)
            copy->operands[0].value.reg.words.number = replacement_register;
        if ((copy->operands[1].flags & 15) == OPERAND_REGISTER &&
            copy->operands[1].value.reg.words.number == saved_register)
            copy->operands[1].value.reg.words.number = replacement_register;
        matches = 0;
        swapped = 0;
        if (machine_records_equal(before, target)) {
            if (machine_records_equal(copy, after_target)) matches = 1;
            else if (swap_comparison_for_match(copy, after_target, branch)) {
                matches = 1;
                swapped = 1;
            }
        }
        release_machine_instruction(copy);
        before->operands[0].value.reg.words.number = saved_register;
        if (matches) {
            if (swapped) {
                temporary = comparison->operands[0];
                comparison->operands[0] = comparison->operands[1];
                comparison->operands[1] = temporary;
            }
            retarget_comparison_branch(after_target, branch, target, 2);
        }
    }
}

/* Original [00033cf0,00033d30). Locate this record's preceding block
   header; flag2 prohibits moving comparisons across it. */
int __fastcall machine_block_protected(MachineInstruction *instruction)
{
    while (instruction && (instruction->opcode & 15) != 7)
        instruction = instruction->previous;
    if (!instruction) compiler_internal_error("@(#)pgoMD.c:1.49", 0x70d);
    return (((MachineBlockHeader *)instruction)->flags & 2) != 0;
}

/* Original [00034440,00034520). Match the reversed operand order while
   preserving byte-register lanes and reversing the following CMP condition. */
int __fastcall swap_comparison_for_match(MachineInstruction *comparison,
    MachineInstruction *target, MachineInstruction *branch)
{
    enum { CC_O, CC_NO, CC_B, CC_AE, CC_E, CC_NE, CC_BE, CC_A,
           CC_S, CC_NS, CC_P, CC_NP, CC_L, CC_GE, CC_LE, CC_G };
    /* Original81160..81170: condition after exchanging comparison operands.
       Equality/parity remain unchanged; ordered relations exchange direction. */
    static const unsigned char exchanged_condition[16] = {
        CC_NO, CC_O, CC_A, CC_BE, CC_E, CC_NE, CC_AE, CC_B,
        CC_NS, CC_S, CC_P, CC_NP, CC_G, CC_LE, CC_GE, CC_L
    };
    unsigned target_opcode = target->opcode, opcode = comparison->opcode;
    MachineOperand temporary;
    if ((target_opcode & 15) != 0 || ((opcode ^ target_opcode) & 0xff00) ||
        !operands_equal(&comparison->operands[0], &target->operands[1]) ||
        !operands_equal(&comparison->operands[1], &target->operands[0]) ||
        ((opcode ^ target_opcode) & 0xf0)) return 0;
    if ((opcode & 0xf0) <= 0x10 &&
        ((((opcode >> 19) ^ (target_opcode >> 20)) & 1) ||
         (((target_opcode >> 19) ^ (opcode >> 20)) & 1))) return 0;
    temporary = comparison->operands[0];
    comparison->operands[0] = comparison->operands[1];
    comparison->operands[1] = temporary;
    if (((comparison->opcode >> 8) & 255) == MC_CMP)
        branch->operands[1].flags = (branch->operands[1].flags & ~15U) |
            exchanged_condition[branch->operands[1].flags & 15];
    return 1;
}
