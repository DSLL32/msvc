/* Unconditional jump threading and machine block placement. */
#include "jump-threading.h"
#include "machine-layout.h"
#include "instruction-rewrite.h"
#include "register-use.h"
#include <recon.h>
extern int optimize_speed, optimization_jumps;

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

/* Original [00034520,00034910). Share equal instruction tails before a
   JMP, and hoist equal flag-preserving prefixes across a conditional jump. */
void __fastcall factor_machine_branch_code(MachineContext *context)
{
    MachineInstruction *branch, *label, *left, *right, *before, *scan, *next;
    MachineInstruction *new_record, *destination;
    LabelSymbol *symbol, *new_label;
    unsigned kind, operation;
    int matched;
    for (branch = context->next; branch; branch = branch->next) {
        kind = branch->opcode & 15;
        operation = (branch->opcode >> 8) & 255;
        if (kind == 0 && operation == MC_JMP &&
            (branch->operands[0].flags & 15) == OPERAND_SYMBOL &&
            branch->operands[0].value.symbol->kind == 3 && !machine_block_protected(branch)) {
            label = label_machine_instruction((LabelSymbol *)branch->operands[0].value.symbol);
            if (machine_block_protected(label)) continue;
            matched = 0;
            left = branch->previous;
            right = label->previous;
            for (;;) {
                kind = left->opcode & 15;
                if (kind == 4 || kind == 7 || kind == 5 ||
                    (kind == 1 && !machine_block_protected(left))) {
                    if (kind == 1) {
                        if (left == label) break;
                        before = left->previous;
                        before->next = left->next;
                        left->next->previous = before;
                        insert_machine_after(right, left);
                        left = before;
                    } else left = left->previous;
                    continue;
                }
                while ((right->opcode & 15) == 4 || (right->opcode & 15) == 7 ||
                       (right->opcode & 15) == 5 || (right->opcode & 15) == 1)
                    right = right->previous;
                if (right == branch ||
                    (optimize_speed && (left->opcode & 15) == 0 &&
                     ((left->opcode >> 8) & 255) == MC_JCC) ||
                    !machine_records_equal(left, right)) break;
                ++matched;
                left = left->previous;
                right = right->previous;
            }
            if (matched > 0 && !machine_block_protected(left) && !machine_block_protected(right)) {
                --((LabelSymbol *)label->record.symbol)->references;
                new_label = new_internal_label();
                new_record = new_machine_record(1, new_label);
                insert_machine_after(right, new_record);
                new_record->labels.next = context->labels.next;
                context->labels.next = new_record;
                new_label->instruction = new_record;
                for (scan = left->next; scan != branch; scan = next) {
                    next = scan->next;
                    operation = (scan->opcode >> 8) & 255;
                    if ((scan->opcode & 15) == 0 && (operation == MC_JCC || operation == MC_JMP) &&
                        (scan->operands[0].flags & 15) == OPERAND_SYMBOL)
                        --((LabelSymbol *)scan->operands[0].value.symbol)->references;
                    if ((scan->opcode & 15) == 1)
                        compiler_internal_error("@(#)pgoMD.c:1.49", 0x768);
                    remove_machine_instruction(scan);
                }
                ++new_label->references;
                branch->operands[0].value.symbol = &new_label->symbol;
            }
        } else if (kind == 0 && operation == MC_JCC &&
                   (branch->operands[0].flags & 15) == OPERAND_SYMBOL &&
                   branch->operands[0].value.symbol->kind == 3 && !machine_block_protected(branch)) {
            label = label_machine_instruction((LabelSymbol *)branch->operands[0].value.symbol);
            symbol = (LabelSymbol *)label->record.symbol;
            if (machine_block_protected(label) || symbol->references != 1) continue;
            before = previous_machine_operation(label);
            if (label_has_fallthrough_predecessor(label) ||
                (((before->opcode >> 8) & 255) == MC_JMP &&
                 (before->operands[0].flags & 15) == OPERAND_SYMBOL &&
                 before->operands[0].value.symbol == label->record.symbol)) continue;
            matched = 0;
            left = branch->next;
            right = label->next;
            for (;;) {
                while ((left->opcode & 15) == 4 || (left->opcode & 15) == 5 ||
                       ((left->opcode & 15) == 7 && !machine_block_protected(left)))
                    left = left->next;
                while ((right->opcode & 15) == 4 || (right->opcode & 15) == 5 ||
                       ((right->opcode & 15) == 7 && !machine_block_protected(right)))
                    right = right->next;
                if (right == branch || !machine_records_equal(left, right) ||
                    ((left->opcode & 15) == 0 &&
                     (machine_selection_properties[(left->opcode >> 8) & 255] & MC_WRITES_FLAGS))) break;
                ++matched;
                left = left->next;
                right = right->next;
            }
            if (matched > 0) {
                --symbol->references;
                if ((right->opcode & 15) == 1) {
                    new_label = (LabelSymbol *)right->record.symbol;
                    destination = right;
                } else {
                    new_label = new_internal_label();
                    destination = new_machine_record(1, new_label);
                    insert_machine_after(right->previous, destination);
                    destination->labels.next = context->labels.next;
                    context->labels.next = destination;
                    new_label->instruction = destination;
                }
                ++new_label->references;
                branch->operands[0].value.symbol = &new_label->symbol;
                for (scan = label->next; scan != destination; scan = next) {
                    next = scan->next;
                    adjust_machine_symbol_references(scan, -1);
                    remove_machine_instruction(scan);
                }
                branch->previous->next = branch->next;
                branch->next->previous = branch->previous;
                insert_machine_after(left->previous, branch);
            }
        }
    }
}

/* Original [00034910,000349e0). Discard records after unconditional
   transfers until an externally reachable label or the function end.
   Keep line/block markers and labels even while skipping an unreachable span. */
void __fastcall remove_unreachable_machine_code(MachineContext *context)
{
    MachineInstruction *terminator = find_block_terminator(context);
    MachineInstruction *scan, *next;
    LabelSymbol *label;
    unsigned kind, operand_kind;
    while (terminator && (terminator->opcode & 15) != 3) {
        operand_kind = terminator->operands[0].flags & 15;
        if (((terminator->opcode >> 8) & 255) == MC_JMP &&
            (operand_kind == 10 || operand_kind == OPERAND_LABEL_ADDRESS)) {
            next = terminator->next;
        } else {
            scan = terminator->next;
            for (;;) {
                next = scan->next;
                kind = scan->opcode & 15;
                if (kind == 7 || kind == 4) {
                    scan = next;
                    continue;
                }
                if (kind == 1) {
                    label = (LabelSymbol *)scan->record.symbol;
                    if (label->references <= 0 && !(label->label_kind & 4)) {
                        scan = next;
                        continue;
                    }
                    if (((terminator->opcode >> 8) & 255) == MC_JMP &&
                        (terminator->operands[0].flags & 15) == OPERAND_SYMBOL &&
                        terminator->operands[0].value.symbol->kind == 3 &&
                        scan->record.symbol == terminator->operands[0].value.symbol) {
                        adjust_machine_symbol_references(terminator, -1);
                        remove_machine_instruction(terminator);
                    }
                    break;
                }
                if (kind == 3) break;
                adjust_machine_symbol_references(scan, -1);
                remove_machine_instruction(scan);
                scan = next;
            }
        }
        terminator = find_block_terminator(next);
    }
}
/* A LEA, non-memory MOV or register XOR-self can move without a memory
   side effect. Keep this expression inline: it is part of each diamond arm. */
#define SIMPLE_DIAMOND_OPERATION(i) \
    ((((i)->opcode >> 8) & 255) == MC_LEA || \
     ((((i)->opcode >> 8) & 255) == MC_MOV && \
      ((i)->operands[1].flags & 15) != OPERAND_BASED && \
      ((i)->operands[1].flags & 15) != OPERAND_INDEXED && \
      ((i)->operands[1].flags & 15) != OPERAND_ABSOLUTE) || \
     ((((i)->opcode >> 8) & 255) == MC_XOR && \
      ((i)->operands[0].flags & 15) == OPERAND_REGISTER && \
      operands_equal(&(i)->operands[0], &(i)->operands[1])))

/* Original [00033870,00033ce8). Hoist one compatible arm operation into
   a conditional diamond and redirect its branch to the shared destination. */
int __fastcall merge_jump_diamond(MachineInstruction *branch, MachineInstruction *label,
    MachineInstruction *before_target)
{
    MachineInstruction *fallthrough, *scan, *source, *condition, *move;
    unsigned kind, operation;
    int count, target_complex, source_complex = 0;
    if (!optimization_jumps || !before_target) return 0;
    target_complex = !SIMPLE_DIAMOND_OPERATION(before_target);
    fallthrough = branch->next;
    while ((fallthrough->opcode & 15) == 4 || (fallthrough->opcode & 15) == 7)
        fallthrough = fallthrough->next;
    if ((fallthrough->opcode & 15) != 1 || machine_block_protected(fallthrough)) return 0;
    count = 0;
    scan = fallthrough->next;
    while ((scan->opcode & 15) != 1) {
        kind = scan->opcode & 15;
        if (kind == 3) break;
        if (!kind) {
            operation = (scan->opcode >> 8) & 255;
            if (operation == MC_JCC || operation == MC_JMP) return 0;
            ++count;
            if (scan == before_target) break;
        }
        scan = scan->next;
    }
    if (scan != before_target) return 0;
    if (count != 1) target_complex = 1;
    source = previous_machine_operation(branch);
    if (!source) return 0;
    if (!SIMPLE_DIAMOND_OPERATION(source)) {
        if (target_complex) return 0;
        source_complex = 1;
    }
    condition = source->previous;
    count = 1;
    while ((condition->opcode & 15) != 1) {
        kind = condition->opcode & 15;
        if (kind == 2) break;
        if (!kind) {
            operation = (condition->opcode >> 8) & 255;
            if (operation == MC_JCC || operation == MC_JMP) break;
            ++count;
        }
        condition = condition->previous;
    }
    if (count != 1) {
        if (target_complex) return 0;
        source_complex = 1;
    }
    if ((condition->opcode & 15) ||
        (condition->operands[0].flags & 15) != OPERAND_SYMBOL ||
        condition->operands[0].value.symbol != fallthrough->record.symbol ||
        (((LabelSymbol *)fallthrough->record.symbol)->references != 1 && source_complex) ||
        machine_block_protected(condition)) return 0;
    if (!source_complex) {
        if ((source->operands[0].flags & 15) == OPERAND_REGISTER) {
            if (diamond_register_constraint(source->operands[0].value.reg.words.number,
                fallthrough, before_target, (source->opcode >> 4) & 15)) {
                if (target_complex) return 0;
                source_complex = 1;
            }
        } else if (target_complex) return 0;
    }
    if (source_complex) {
        if ((before_target->operands[0].flags & 15) != OPERAND_REGISTER ||
            diamond_register_constraint(before_target->operands[0].value.reg.words.number,
                condition, source, (before_target->opcode >> 4) & 15)) return 0;
    }
    if (!target_complex && !source_complex) {
        if ((source->operands[0].flags & 0x8000) ||
            (source->operands[1].flags & 0x8000) ||
            ((source->opcode ^ before_target->opcode) & 0xf0) ||
            !operands_equal(&before_target->operands[0], &source->operands[0])) return 0;
    } else {
        if (target_complex && ((source->operands[0].flags & 0x8000) ||
            (source->operands[1].flags & 0x8000) ||
            !operand_changed_between(fallthrough, before_target->next,
                &source->operands[0], (source->opcode >> 4) & 15))) return 0;
        if (source_complex && ((before_target->operands[0].flags & 0x8000) ||
            (before_target->operands[1].flags & 0x8000) ||
            !operand_changed_between(condition, source->next,
                &before_target->operands[0], (before_target->opcode >> 4) & 15))) return 0;
    }
    if (source_complex) move = before_target;
    else {
        condition->operands[1].flags ^= 1;
        move = source;
    }
    if (((move->opcode >> 8) & 255) == MC_XOR) {
        move->opcode = (move->opcode & 0xffff01ffU) | (MC_MOV << 8);
        move->operands[1].flags = (move->operands[1].flags & 0xfffffff2U) | OPERAND_IMMEDIATE;
        move->operands[1].value.constant = 0;
    }
    move->previous->next = move->next;
    move->next->previous = move->previous;
    insert_machine_after(condition->previous, move);
    adjust_machine_symbol_references(condition, -1);
    condition->operands[0].value.symbol = label->record.symbol;
    remove_machine_instruction(branch);
    return 1;
}
#undef SIMPLE_DIAMOND_OPERATION
