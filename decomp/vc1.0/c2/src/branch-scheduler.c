/* Pentium branch-edge pairing and late instruction movement. */
#include "backend-passes.h"
#include "scheduler.h"
#include "machine-layout.h"
#include "instruction-selection.h"
#include "instruction-rewrite.h"
#include <stdio.h>
#include <recon.h>
int scheduler_has_indirect_branch; /* 000830a8 */
int scheduler_branch_pipe; /* 0007e2a8: shared classifier in/out argument */
#define BRANCH_KIND(i) ((i)->opcode & 15)
#define BRANCH_OPERATION(i) (((i)->opcode >> 8) & 255)
#define IS_EDGE_BRANCH(i) (BRANCH_OPERATION(i) == MC_JCC || BRANCH_OPERATION(i) == MC_JMP)
/* Original [000471a0,00047461). Find a branch on either side of a JCC
   edge, allowing one pairable intervening instruction. Preserve label crossings
   separately for the fall-through and taken-edge movement helpers. */
void __fastcall schedule_i586_branches(MachineContext *context)
{
    MachineInstruction *branch, *next, *scan, *candidate, *middle, *label;
    int protected_block = 0, crossed_label;
    set_scheduler_register_tracking(0);
    scheduler_has_indirect_branch = 0;
    for (scan = context; scan; scan = scan->next)
        if (BRANCH_KIND(scan) == 0 && IS_EDGE_BRANCH(scan) &&
            (scan->operands[0].flags & 15) != OPERAND_SYMBOL) {
            scheduler_has_indirect_branch = 1;
            break;
        }
    for (branch = context; branch; branch = next) {
        next = branch->next;
        if (BRANCH_KIND(branch) == 7) protected_block = (((MachineBlockHeader *)branch)->flags & 3) != 0;
        if (BRANCH_KIND(branch) != 0 || protected_block || BRANCH_OPERATION(branch) != MC_JCC ||
            (branch->operands[0].flags & 15) != OPERAND_SYMBOL || branch->operands[0].value.symbol->kind != 3) continue;
        crossed_label = 0;
        for (scan = next; scan && BRANCH_KIND(scan) != 0; scan = scan->next)
            if (BRANCH_KIND(scan) == 1) crossed_label = 1;
        if (!scan) return;
        middle = 0;
        candidate = 0;
        if (IS_EDGE_BRANCH(scan)) candidate = scan;
        else {
            middle = scan;
            for (scan = scan->next; scan && BRANCH_KIND(scan) != 0; scan = scan->next)
                if (BRANCH_KIND(scan) == 1) crossed_label = 1;
            if (!scan) return;
            if (IS_EDGE_BRANCH(scan)) {
                scheduler_branch_pipe = 2;
                if (classify_pentium_pipe(middle, &scheduler_branch_pipe)) candidate = scan;
            }
        }
        if (candidate) {
            fflush(stdout);
            schedule_pentium_branch_edge(branch, candidate, middle, 0, crossed_label);
        }
        crossed_label = 0;
        label = label_machine_instruction((LabelSymbol *)branch->operands[0].value.symbol);
        for (scan = label->previous; scan && BRANCH_KIND(scan) != 0; scan = scan->previous)
            if (BRANCH_KIND(scan) == 1) crossed_label = 1;
        for (scan = label->next; scan && BRANCH_KIND(scan) != 0; scan = scan->next)
            if (BRANCH_KIND(scan) == 1) crossed_label = 1;
        if (!scan) return;
        candidate = 0;
        /* The original retains the first edge's middle pointer if this side
           begins directly with a branch; the child ignores it in that form. */
        if (IS_EDGE_BRANCH(scan)) candidate = scan;
        else {
            middle = scan;
            for (scan = scan->next; scan && BRANCH_KIND(scan) != 0; scan = scan->next)
                if (BRANCH_KIND(scan) == 1) crossed_label = 1;
            if (!scan) return;
            if (IS_EDGE_BRANCH(scan)) {
                scheduler_branch_pipe = 2;
                if (classify_pentium_pipe(middle, &scheduler_branch_pipe)) candidate = scan;
            }
        }
        if (candidate) {
            fflush(stdout);
            schedule_pentium_branch_edge(branch, candidate, middle, 1, crossed_label);
        }
    }
}
/* Original [00047470,00047773). Fill either side of a conditional edge,
   redirect shared targets after movement, or insert one/two pairing NOPs. */
void __fastcall schedule_pentium_branch_edge(MachineInstruction *branch,
    MachineInstruction *candidate, MachineInstruction *middle, int taken, int crossed_label)
{
    MachineInstruction *label, *prior, *nop;
    int moved = 0, changed = 0;
    unsigned kind;
    fflush(stdout);
    label = label_machine_instruction((LabelSymbol *)branch->operands[0].value.symbol);
    if (!taken && BRANCH_OPERATION(candidate) == MC_JMP &&
        (candidate->operands[0].flags & 15) == OPERAND_SYMBOL) {
        if (!crossed_label) moved = pair_pentium_fallthrough_jump(candidate, &middle);
        if (moved) return;
    }
    for (prior = label->previous; prior; prior = prior->previous) {
        kind = BRANCH_KIND(prior);
        if (kind == 0 || kind == 8 || kind == 9 || kind == 10) break;
    }
    if (!prior) return;
    if (BRANCH_OPERATION(prior) != MC_JMP ||
        (scheduler_has_indirect_branch && ((LabelSymbol *)branch->operands[0].value.symbol)->references != 1)) {
        if (taken || !(label->labels.flags & 1)) return;
    } else {
        if (!crossed_label) moved = pair_pentium_taken_edge(branch, &middle, label, &changed);
        if (changed && ((LabelSymbol *)branch->operands[0].value.symbol)->references != 1)
            redirect_pentium_branch_labels(branch, middle);
        if (moved || (!(label->labels.flags & (taken ? 2 : 1)) && !changed)) return;
    }
    nop = new_machine_record(0, MC_NOP, 1);
    nop->record.encoding_length = (nop->record.encoding_length & 0xfffffc01UL) | 1;
    insert_machine_after(candidate->previous, nop);
    if (!middle) {
        nop = new_machine_record(0, MC_NOP, 1);
        nop->record.encoding_length = (nop->record.encoding_length & 0xfffffc01UL) | 1;
        insert_machine_after(candidate->previous, nop);
    }
}

#define EDGE_MACHINE(i) (BRANCH_KIND(i) == 0 || (BRANCH_KIND(i) >= 8 && BRANCH_KIND(i) <= 10))
#define EDGE_BLOCKS_SCAN(op) ((op) == MC_JCC || (op) == MC_JMP || (op) == MC_LEAVE || (op) == MC_RET)
#define EDGE_CANDIDATE_MARK 0x200000UL
/* Original [00047850,00047d78). Fill taken-edge pairing slots by moving
   predecessors and cloning them after the target label. Stop the first scan
   at flag writers, then try at most six further candidates with flag liveness
   and dependence checks. Arena/undo/flush ordering is phase-owned state. */
int __fastcall pair_pentium_taken_edge(MachineInstruction *branch, MachineInstruction **middle,
    MachineInstruction *label, int *changed)
{
    MachineInstruction *block, *instruction, *previous, *copy;
    unsigned operation, kind;
    int count = 0, attempts;
    fflush(stdout);
    block = label;
    while (BRANCH_KIND(block) != 7) block = block->previous;
    if (!block || (((MachineBlockHeader *)block)->flags & 3)) return 0;
    instruction = branch->previous;
    scheduler_undo = 0;
    branch->record.scheduling = new_instruction_schedule();
    while (instruction) {
        kind = BRANCH_KIND(instruction);
        operation = BRANCH_OPERATION(instruction);
        if (EDGE_MACHINE(instruction) &&
            ((machine_selection_properties[operation] & MC_WRITES_FLAGS) || operation == MC_CALL)) break;
        if ((EDGE_MACHINE(instruction) && EDGE_BLOCKS_SCAN(operation)) || kind == 1) break;
        previous = instruction->previous;
        if (EDGE_MACHINE(instruction)) {
            ++count;
            if (count == 1) {
                instruction->record.scheduling = new_instruction_schedule();
                scheduler_branch_pipe = 2;
                if (!classify_pentium_pipe(instruction, &scheduler_branch_pipe) &&
                    !(instruction->opcode & EDGE_CANDIDATE_MARK)) {
                    fflush(stdout);
                    place_scheduled_instruction(branch, instruction);
                    copy = clone_machine_record(instruction);
                    ++*changed;
                    insert_machine_after(label, copy);
                    if (*changed == 1) *middle = copy;
                    reset_arena(3);
                    return 1;
                }
            } else if (!(instruction->opcode & EDGE_CANDIDATE_MARK)) {
                instruction->record.scheduling = new_instruction_schedule();
                scheduler_undo = 0;
                if (scan_schedule_interval(instruction, branch, 0, 0, 0, 0, 0)) {
                    fflush(stdout);
                    place_scheduled_instruction(branch, instruction);
                    instruction->opcode |= EDGE_CANDIDATE_MARK;
                    copy = clone_machine_record(instruction);
                    insert_machine_after(label, copy);
                    ++*changed;
                    scheduler_branch_pipe = 2;
                    if (*middle || !classify_pentium_pipe(copy, &scheduler_branch_pipe)) {
                        if (*changed == 1) *middle = copy;
                        reset_arena(3);
                        return 1;
                    }
                    if (*changed == 1) *middle = copy;
                }
            }
        }
        instruction = previous;
    }
    if (!instruction || (EDGE_MACHINE(instruction) && EDGE_BLOCKS_SCAN(BRANCH_OPERATION(instruction))) ||
        BRANCH_KIND(instruction) == 1) return 0;
    previous = instruction->previous;
    instruction->record.scheduling = new_instruction_schedule();
    instruction = previous;
    for (attempts = 0; attempts <= 5; ++attempts) {
        while (instruction && !EDGE_MACHINE(instruction) && BRANCH_KIND(instruction) != 1)
            instruction = instruction->previous;
        if (!instruction) return 0;
        kind = BRANCH_KIND(instruction);
        operation = BRANCH_OPERATION(instruction);
        if ((EDGE_MACHINE(instruction) && EDGE_BLOCKS_SCAN(operation)) || kind == 1 || operation == MC_CALL)
            return 0;
        previous = instruction->previous;
        if (!(instruction->opcode & EDGE_CANDIDATE_MARK) ||
            (((machine_selection_properties[operation] & MC_WRITES_FLAGS) || operation == MC_CALL) &&
             (pentium_flags_live_after(branch) || pentium_flags_live_after(label)))) {
            fflush(stdout);
            instruction->record.scheduling = new_instruction_schedule();
        } else {
            instruction->record.scheduling = new_instruction_schedule();
            scheduler_undo = 0;
            if (scan_schedule_interval(instruction, branch, 0, 0, 0, 0, 0)) {
                place_scheduled_instruction(branch, instruction);
                instruction->opcode |= EDGE_CANDIDATE_MARK;
                copy = clone_machine_record(instruction);
                fflush(stdout);
                insert_machine_after(label, copy);
                ++*changed;
                if (*middle || !classify_pentium_pipe(copy, &scheduler_branch_pipe)) {
                    if (*changed == 1) *middle = copy;
                    reset_arena(3);
                    return 1;
                }
                if (*changed == 1) *middle = copy;
            }
        }
        instruction = previous;
    }
    return 0;
}
