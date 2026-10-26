/* Pentium branch-edge pairing and late instruction movement. */
#include "backend-passes.h"
#include "scheduler.h"
#include "machine-layout.h"
#include "instruction-selection.h"
#include "instruction-rewrite.h"
#include "jump-threading.h"
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

/* Original [00047d80,00047f61). Copy up to two fall-through operations
   before a jump, retaining a single pairable first operation as its middle.
   Split the destination label after the copied source prefix and retarget
   only this jump, preserving the old/new label reference counts. */
int __fastcall pair_pentium_fallthrough_jump(MachineInstruction *jump, MachineInstruction **middle)
{
    MachineInstruction *label, *block, *instruction, *last, *copy, *new_label;
    SymbolHeader *symbol;
    LabelSymbol *new_symbol;
    int count = 0, changed = 0;
    symbol = jump->operands[0].value.symbol;
    if (symbol->kind != 3) return 0;
    label = label_machine_instruction((LabelSymbol *)symbol);
    block = label;
    while (BRANCH_KIND(block) != 7) block = block->previous;
    if (!block || (((MachineBlockHeader *)block)->flags & 3)) return 0;
    instruction = label->next;
    fflush(stdout);
    while (instruction) {
        while (instruction && !EDGE_MACHINE(instruction) && BRANCH_KIND(instruction) != 1)
            instruction = instruction->next;
        if (!instruction || (EDGE_MACHINE(instruction) && EDGE_BLOCKS_SCAN(BRANCH_OPERATION(instruction))) ||
            BRANCH_KIND(instruction) == 1) break;
        last = instruction;
        copy = clone_machine_record(instruction);
        fflush(stdout);
        insert_machine_after(jump->previous, copy);
        ++count;
        if (count == 1 && !*middle) {
            scheduler_branch_pipe = 2;
            if (classify_pentium_pipe(instruction, &scheduler_branch_pipe)) {
                *middle = copy;
                instruction = instruction->next;
                continue;
            }
        }
        changed = 1;
        break;
    }
    if (!count) return 0;
    new_symbol = new_internal_label();
    new_label = new_machine_instruction(1);
    fflush(stdout);
    new_symbol->references = 1;
    new_label->record.symbol = &new_symbol->symbol;
    insert_machine_after(last, new_label);
    register_machine_label(new_label);
    --((LabelSymbol *)jump->operands[0].value.symbol)->references;
    jump->operands[0].value.symbol = &new_symbol->symbol;
    return changed;
}

/* Original [00047f70,00048036). Search for the next use/definition of
   condition flags, following one explicit branch destination. READ wins
   when an operation both reads and writes. An unresolved jump or a second
   jump after the destination conservatively retains the flags.
   The original reuses scan for both loops: if a valid label's suffix has
   no flag operation or jump before NULL, the outer increment dereferences
   NULL. Preserve that original list-shape assumption rather than adding
   a new result/guard for an unsupported compiler state. */
int __fastcall pentium_flags_live_after(MachineInstruction *instruction)
{
    MachineInstruction *scan, *label;
    unsigned operation, properties;
    for (scan = instruction->next; scan; scan = scan->next) {
        if ((scan->opcode & 15) != 0) continue;
        operation = (scan->opcode >> 8) & 255;
        properties = machine_selection_properties[operation];
        if (properties & MC_READS_FLAGS) return 1;
        if (properties & MC_WRITES_FLAGS) return 0;
        if (operation != MC_JMP) continue;
        if ((scan->operands[0].flags & 15) != 8) return 1;
        label = ((LabelSymbol *)scan->operands[0].value.symbol)->instruction;
        if (!label || (label->opcode & 15) != 1) return 1;
        for (scan = label->next; scan; scan = scan->next) {
            if ((scan->opcode & 15) != 0) continue;
            operation = (scan->opcode >> 8) & 255;
            properties = machine_selection_properties[operation];
            if (properties & MC_READS_FLAGS) return 1;
            if (properties & MC_WRITES_FLAGS) return 0;
            if (operation == MC_JMP) return 1;
        }
    }
    return 0;
}

/* Original47780..47841 (193 bytes), ECX branch/EDX paired middle, RET0.
   Keep the selected edge on its old label and redirect every other ordinary
   Jcc/JMP sharing that target to a new label after the moved instruction.
   The original does not subtract redirected edges from the old reference
   counter here. Its label-chain publication is inlined, not a helper edge.
   Dormant transform: constructor-based live native unit qualification only. */
void __fastcall redirect_pentium_branch_labels(MachineInstruction *branch,
    MachineInstruction *middle)
{
    LabelSymbol *symbol = new_internal_label();
    MachineInstruction *label = new_machine_instruction(1), *scan;
    SymbolHeader *old;
    symbol->instruction = label;
    fflush(stdout);
    symbol->references = 0;
    label->record.symbol = &symbol->symbol;
    label->labels.next = machine_context->labels.next;
    machine_context->labels.next = label;
    fflush(stdout);
    insert_machine_after(middle, label);
    old = branch->operands[0].value.symbol;
    scan = machine_context;
    fflush(stdout);
    for (; scan; scan = scan->next) {
        if (!(scan->opcode & 15) && scan != branch &&
            (BRANCH_OPERATION(scan) == MC_JCC || BRANCH_OPERATION(scan) == MC_JMP) &&
            scan->operands[0].value.symbol == old) {
            fflush(stdout);
            scan->operands[0].value.symbol = &symbol->symbol;
            ++symbol->references;
        }
    }
    fflush(stdout);
}
