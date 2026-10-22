#include "instruction-rewrite.h"
#include "tree-rewrite.h"
#include "register-use.h"
/* Original [000217c0,00021860). Determine whether an intervening instruction
   invalidates an operand before its use. Symbolic/immediate values are stable;
   a label ends availability for the other operand forms. */
int __fastcall operand_changed_between(MachineInstruction *first, MachineInstruction *last,
    MachineOperand *operand, unsigned bytes)
{
    MachineInstruction *instruction;
    unsigned kind = operand->flags & 15;
    if (kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS || kind == 7 || kind == OPERAND_SYMBOL)
        return 0;
    for (instruction = first->next; instruction != last; instruction = instruction->next) {
        kind = instruction->opcode & 15;
        if (kind == 1) return 1;
        if ((kind == 0 || kind == 8 || kind == 9 || kind == 10) &&
            instruction_modifies_operand(instruction, operand, bytes)) return 1;
    }
    return 0;
}

/* Original [00035760,00035840). Port, shift and extending-move operands
   have widths independent of the instruction's result width. */
unsigned __fastcall machine_operand_bytes(MachineInstruction *instruction, int index)
{
    unsigned operation = (instruction->opcode >> 8) & 255;
    if (operation == MC_IN && index == 1)
        return (instruction->operands[1].flags & 15) == OPERAND_REGISTER ? 2 : 1;
    if (operation == MC_OUT && index == 0)
        return (instruction->operands[0].flags & 15) == OPERAND_REGISTER ? 2 : 1;
    if (((operation >= MC_RCL && operation <= MC_SHR) ||
         operation == MC_MOVZX || operation == MC_MOVSX) && index == 1)
        return 1;
    if (operation == MC_FSTSW || operation == MC_FNSTSW)
        return 2;
    if ((operation == MC_MOVZXW || operation == MC_MOVSXW) && index == 1)
        return 2;
    return (instruction->opcode >> 4) & 15;
}

/* Original [00022740,000229d0), including its operation dispatch table.
   Check the x86 operand restrictions before changing the selected record. */
MachineInstruction *__fastcall replace_machine_operand(MachineInstruction *instruction, unsigned index,
    MachineOperand *operand, unsigned char apply)
{
    unsigned operation = (instruction->opcode >> 8) & 255;
    unsigned kind = operand->flags & 15;
    MachineOperand *destination;
    int check_general = 1;
    switch (operation) {
    case MC_IMUL: case MC_DIV: case MC_IDIV: case MC_MUL:
        if (kind == OPERAND_IMMEDIATE) return 0;
        break;
    case MC_IN: case MC_OUT:
        if (index != (unsigned)(operation == MC_IN) || kind != OPERAND_IMMEDIATE ||
            (operand->value.constant & 255) != operand->value.constant) return 0;
        check_general = 0;
        break;
    case MC_CALL:
        if (kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS || kind == 7) return 0;
        check_general = 0;
        break;
    case MC_RCL: case MC_RCR: case MC_ROL: case MC_ROR:
    case MC_SAR: case MC_SHL: case MC_SHR:
        if (index == 1 && kind != OPERAND_IMMEDIATE &&
            (kind != OPERAND_REGISTER || operand->value.reg.words.number != REG_ECX)) return 0;
        break;
    case MC_IMUL3: case MC_IMUL2:
        if (index == 0 && kind != OPERAND_REGISTER) return 0;
        break;
    case MC_MOVZX: case MC_MOVZXW: case MC_MOVSX: case MC_MOVSXW:
        if (index == 0) {
            if (kind != OPERAND_REGISTER) return 0;
        } else if (kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS || kind == 7)
            return 0;
        break;
    case MC_BT: case MC_BTS: case MC_BTR: case MC_BTC:
        if (index == 1 && kind != OPERAND_REGISTER && kind != OPERAND_IMMEDIATE) return 0;
        if (index == 0 && kind != OPERAND_REGISTER && kind != OPERAND_ABSOLUTE) return 0;
        break;
    }
    if (check_general && !operand_replacement_allowed(instruction, index, operand, apply)) return 0;
    if (apply & 1) {
        destination = &instruction->operands[index == 0 ? 0 : 1];
        release_operand_address(destination);
        *destination = *operand;
        tree_changed = 1;
    }
    return instruction;
}

/* Original [00022c40,00022d20). Prevent two memory operands and changes
   to a written destination unless the caller explicitly permits them. */
int __fastcall operand_replacement_allowed(MachineInstruction *instruction, unsigned index,
    MachineOperand *operand, unsigned char apply)
{
    int count = machine_operand_counts[(instruction->opcode >> 8) & 255];
    unsigned kind = operand->flags & 15;
    unsigned other;
    if (count == 1) {
        if (kind == OPERAND_REGISTER && !(apply & 2) && writes_first_operand(instruction)) return 0;
        return 1;
    }
    if (count != 2) return count;
    if (index == 1) {
        if (kind == OPERAND_LABEL_ADDRESS) return machine_operand_bytes(instruction, 1) == 4;
        return (instruction->operands[0].flags & 15) == OPERAND_REGISTER ||
            kind == OPERAND_REGISTER || kind == OPERAND_IMMEDIATE;
    }
    if (kind == OPERAND_REGISTER) {
        if (!(apply & 2) && writes_first_operand(instruction)) return 0;
        return 1;
    }
    if (kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS) return 0;
    other = instruction->operands[1].flags & 15;
    return other == OPERAND_REGISTER || other == OPERAND_IMMEDIATE || other == OPERAND_LABEL_ADDRESS;
}

/* Original [000224a0,00022660). Collapse fall-through branches, invert a
   conditional branch around a jump, and bypass a compatible target jump.
   The comparison conditions below express the encountered 82c38 relation. */
MachineInstruction *__fastcall simplify_branch(MachineInstruction *instruction)
{
    enum { CC_B = 2, CC_AE = 3, CC_E = 4, CC_NE = 5, CC_BE = 6,
           CC_L = 12, CC_GE = 13, CC_LE = 14, CC_G = 15 };
    MachineInstruction *next = instruction->next, *scan, *after;
    LabelSymbol *target, *replacement;
    unsigned operation, condition, target_condition;
    int compatible;
    if ((instruction->operands[0].flags & 15) != OPERAND_SYMBOL ||
        instruction->operands[0].value.symbol->kind != 3) return next;
    target = (LabelSymbol *)instruction->operands[0].value.symbol;
    if (branch_falls_through(instruction)) {
        tree_changed = 1;
        --target->references;
        remove_machine_instruction(instruction);
        return next;
    }
    for (scan = next; scan && ((scan->opcode & 15) == 7 || (scan->opcode & 15) == 4);
         scan = scan->next) ;
    if (scan && !(scan->opcode & 15) && ((scan->opcode >> 8) & 255) == MC_JMP &&
        (scan->operands[0].flags & 15) == OPERAND_SYMBOL) {
        replacement = (LabelSymbol *)scan->operands[0].value.symbol;
        if (target == replacement) {
            tree_changed = 1;
            --target->references;
            remove_machine_instruction(instruction);
            return next;
        }
        if (((instruction->opcode >> 8) & 255) == MC_JCC) {
            for (after = scan->next; after && ((after->opcode & 15) == 7 ||
                 (after->opcode & 15) == 4); after = after->next) ;
            if (after && (after->opcode & 15) == 1 && after->record.symbol == &target->symbol) {
                --target->references;
                instruction->operands[0].value.symbol = &replacement->symbol;
                instruction->operands[1].flags ^= 1;
                remove_machine_instruction(scan);
                tree_changed = 1;
                return instruction;
            }
        }
    }
    if (target->instruction) {
        scan = target->instruction->next;
        while (scan && ((scan->opcode & 15) == 4 || (scan->opcode & 15) == 7 ||
               (scan->opcode & 15) == 1)) scan = scan->next;
        if (scan && !(scan->opcode & 15)) {
            operation = (scan->opcode >> 8) & 255;
            compatible = operation == MC_JMP &&
                (scan->operands[0].flags & 15) == OPERAND_SYMBOL &&
                scan->operands[0].value.symbol != &target->symbol;
            if (operation == MC_JCC && ((instruction->opcode >> 8) & 255) == MC_JCC) {
                condition = instruction->operands[1].flags & 15;
                target_condition = scan->operands[1].flags & 15;
                compatible = condition == target_condition ||
                    (condition == CC_B && (target_condition == CC_NE || target_condition == CC_BE)) ||
                    (condition == CC_E && (target_condition == CC_AE || target_condition == CC_BE ||
                         target_condition == CC_GE || target_condition == CC_LE)) ||
                    (condition == CC_L && target_condition == CC_LE) ||
                    (condition == CC_G && target_condition == CC_GE);
            }
            if (compatible) {
                --target->references;
                replacement = (LabelSymbol *)scan->operands[0].value.symbol;
                ++replacement->references;
                instruction->operands[0].value.symbol = &replacement->symbol;
                tree_changed = 1;
                return instruction;
            }
        }
    }
    return next;
}

/* Original [00022ae0,00022b20). Only source/block markers and intervening
   labels may lie between a branch and its fall-through destination. */
int __fastcall branch_falls_through(MachineInstruction *instruction)
{
    MachineInstruction *scan;
    unsigned kind;
    for (scan = instruction->next; scan; scan = scan->next) {
        kind = scan->opcode & 15;
        if (kind == 1) {
            if (scan->record.symbol == instruction->operands[0].value.symbol) return 1;
        } else if (kind != 4 && kind != 7) return 0;
    }
    return 0;
}
