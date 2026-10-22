/* Unconditional jump threading and machine block placement. */
#include "jump-threading.h"
#include "machine-layout.h"
#include "instruction-rewrite.h"
extern int optimize_speed;

/* Original [00033550,00033870). Remove fall-through jumps, move isolated
   destination blocks, duplicate short tails and simplify branch diamonds. */
void __fastcall optimize_unconditional_jumps(MachineContext *context)
{
    MachineInstruction *instruction, *next, *label, *end, *before, *following;
    MachineInstruction *scan, *first, *tail, *copy, *last_copy, *new_label_record;
    LabelSymbol *symbol, *new_label;
    unsigned kind;
    int delta, duplicate;
    move_data_blocks_to_end(context);
    for (instruction = context; instruction; instruction = next) {
        next = instruction->next;
        kind = instruction->opcode & 15;
        if (kind == 7) {
            if (((MachineBlockHeader *)instruction)->flags & 2)
                next = next_machine_block(instruction);
            continue;
        }
        if (kind != 0 || ((instruction->opcode >> 8) & 255) != MC_JMP ||
            (instruction->operands[0].flags & 15) != OPERAND_SYMBOL ||
            instruction->operands[0].value.symbol->kind != 3) continue;
        if (branch_falls_through(instruction)) {
            adjust_machine_symbol_references(instruction, -1);
            remove_machine_instruction(instruction);
            continue;
        }
        label = label_machine_instruction((LabelSymbol *)instruction->operands[0].value.symbol);
        end = find_block_terminator(label);
        if ((end->opcode & 15) == 3 || end == instruction || machine_block_protected(label)) continue;
        before = previous_machine_operation(label);
        if (!label_has_fallthrough_predecessor(label) &&
            !(((before->opcode >> 8) & 255) == MC_JMP &&
              (before->operands[0].flags & 15) == OPERAND_SYMBOL &&
              before->operands[0].value.symbol == label->record.symbol) &&
            !(((end->opcode >> 8) & 255) == MC_JMP &&
              (end->operands[0].flags & 15) == OPERAND_SYMBOL && branch_falls_through(end))) {
            following = end->next;
            label->previous->next = following;
            following->previous = label->previous;
            next->previous->next = label;
            label->previous = next->previous;
            end->next = next;
            next->previous = end;
            if ((end->opcode & 15) == 0 && ((end->opcode >> 8) & 255) == MC_JMP) next = end;
            adjust_machine_symbol_references(instruction, -1);
            remove_machine_instruction(instruction);
            continue;
        }
        duplicate = 0;
        if (optimize_speed) {
            delta = -(int)machine_instruction_size(instruction);
            first = label->next;
            if (delta <= 6) {
                scan = first;
                do {
                    if ((scan->opcode & 15) == 0) delta += machine_instruction_size(scan);
                    if (scan == end) break;
                    scan = scan->next;
                } while (delta <= 6);
                if (delta <= 6) {
                    tail = instruction;
                    last_copy = 0;
                    /* end is a non-label terminator, so at least its copy is made. */
                    for (scan = first; ; scan = scan->next) {
                        if ((scan->opcode & 15) != 1) {
                            copy = clone_machine_record(scan);
                            tail->next = copy;
                            copy->previous = tail;
                            copy->next = 0;
                            tail = last_copy = copy;
                            if ((copy->opcode & 15) == 0) adjust_machine_symbol_references(copy, 1);
                        }
                        if (scan == end) break;
                    }
                    last_copy->next = next;
                    next->previous = last_copy;
                    if ((last_copy->opcode & 15) == 0 &&
                        ((last_copy->opcode >> 8) & 255) == MC_JMP) {
                        symbol = (LabelSymbol *)last_copy->operands[0].value.symbol;
                        if ((last_copy->operands[0].flags & 15) == OPERAND_SYMBOL &&
                            symbol->symbol.kind == 3 &&
                            instruction->operands[0].value.symbol == &symbol->symbol) {
                            new_label = new_internal_label();
                            new_label_record = new_machine_record(1, new_label);
                            --symbol->references;
                            insert_machine_after(instruction, new_label_record);
                            register_machine_label(new_label_record);
                            last_copy->operands[0].value.symbol = &new_label->symbol;
                            ++new_label->references;
                        } else next = last_copy;
                    }
                    adjust_machine_symbol_references(instruction, -1);
                    remove_machine_instruction(instruction);
                    duplicate = 1;
                }
            }
        }
        if (!duplicate && merge_jump_diamond(instruction, label, before)) next = label;
    }
}

/* Original [00033d70,00033e00). Keep each embedded-data label and its
   DATA records together, moving their range before the function-end marker. */
void __fastcall move_data_blocks_to_end(MachineContext *context)
{
    MachineInstruction *end = find_function_end(context);
    MachineInstruction *label, *scan, *last, *before, *following;
    for (label = context->labels.next; label; label = label->labels.next) {
        if (label->record.symbol->field0d != 1) continue;
        scan = label->next;
        while ((scan->opcode & 15) != 1 &&
               ((scan->opcode & 15) != 0 || ((scan->opcode >> 8) & 255) == MC_DATA))
            scan = scan->next;
        last = previous_machine_operation(scan);
        if ((last->opcode & 15) != 0 || ((last->opcode >> 8) & 255) != MC_DATA)
            compiler_internal_error("@(#)pgoMD.c:1.49", 0x62c);
        before = label->previous;
        following = last->next;
        following->previous = before;
        before->next = following;
        before = end->previous;
        label->previous = before;
        last->next = end;
        before->next = label;
        end->previous = last;
    }
}

/* Original [00033530,00033550). Function-end markers have record kind3. */
MachineInstruction *__fastcall find_function_end(MachineInstruction *instruction)
{
    while (instruction && (instruction->opcode & 15) != 3)
        instruction = instruction->next;
    return instruction;
}

/* Original [00033d30,00033d70). Conditional transfers can fall through;
   only JMP, RET or the function-end marker terminate this search. */
MachineInstruction *__fastcall find_block_terminator(MachineInstruction *instruction)
{
    unsigned kind, operation;
    for (; instruction; instruction = instruction->next) {
        kind = instruction->opcode & 15;
        if (kind == 3) return instruction;
        if (kind == 0) {
            operation = (instruction->opcode >> 8) & 255;
            if (operation == MC_JMP || operation == MC_RET) return instruction;
        }
    }
    return 0;
}

/* Original [00032f30,00032ff0). Determine whether control can fall into
   a label. DATA ranges after an INT remain reachable; RET/JMP terminate it. */
unsigned __fastcall label_has_fallthrough_predecessor(MachineInstruction *instruction)
{
    MachineInstruction *scan = instruction->previous;
    unsigned kind, operation;
    while (scan) {
        kind = scan->opcode & 15;
        if (!kind) break;
        if (kind == 1 || kind == 2) return 1;
        scan = scan->previous;
    }
    if (!scan) {
        compiler_internal_error("@(#)pgoMD.c:1.49", 0x4ae);
        return 0;
    }
    operation = (scan->opcode >> 8) & 255;
    if (operation == MC_RET || operation == MC_JMP) return 0;
    if (operation != MC_DATA) return 1;
    for (scan = scan->previous; scan; scan = scan->previous) {
        kind = scan->opcode & 15;
        if (kind == 0) {
            operation = (scan->opcode >> 8) & 255;
            if (operation != MC_DATA) return operation == MC_INT;
        } else if (kind != 4 && kind != 7) return 0;
    }
    compiler_internal_error("@(#)pgoMD.c:1.49", 0x4a8);
    return 1;
}
