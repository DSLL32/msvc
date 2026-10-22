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
