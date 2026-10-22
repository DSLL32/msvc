/* Per-function machine-stream and temporary-register state. */
#include "machine.h"
#include "machine-analysis.h"
#include "backend-passes.h"
#include "instruction-rewrite.h"
#include "global-work.h"
#include "frame.h"
MachineInstruction *machine_tail; /* 0007b94c */
MachineInstruction *free_machine_instructions; /* 0007b950 */
MachineInstruction *deferred_instructions, *deferred_tail; /* 0007b954, 0007b958 */
void *machine_unused_pool; /* 0007b95c: this build only clears the slot */
MachineAddress *free_machine_addresses; /* 0007b960 */
BitSet *machine_register_set; /* 0007b964 */
unsigned short next_machine_register = 32; /* 0008000c */
unsigned short next_tree_register = 32; /* 000838a8 */
/* Original [00068300,00068370). */
void reset_machine_state(void)
{
    machine_register_set = 0;
    free_machine_addresses = 0;
    machine_unused_pool = 0;
    machine_instructions = 0;
    deferred_tail = 0;
    deferred_instructions = 0;
    free_machine_instructions = 0;
    machine_tail = 0;
    next_machine_register = 32;
    next_tree_register = 32;
    reset_stack_frame();
}

/* Original [00068b70,00068c10). Rebuild the label chain and reference
   counts after selection. The first label of a region header receives the
   alignment flag used by the later layout pass. */
void __fastcall sync_machine_work(WorkItem *work, MachineContext *context)
{
    MachineInstruction *instruction, **link = &context->labels.next;
    unsigned kind;
    int seen_label;
    for (instruction = context->next; instruction; instruction = instruction->next) {
        if ((instruction->opcode & 15) == 1) {
            ((LabelSymbol *)instruction->record.symbol)->references = 0;
            *link = instruction;
            link = &instruction->labels.next;
        }
    }
    *link = 0;
    for (; work; work = work->next) {
        seen_label = 0;
        for (instruction = work->body.block->next; (instruction->opcode & 15) != 7;
             instruction = instruction->next) {
            kind = instruction->opcode & 15;
            if (!kind) adjust_machine_symbol_references(instruction, 1);
            else if (kind == 1) {
                if (!seen_label && work->region && work->region->header == work)
                    instruction->labels.flags |= 1;
                seen_label = 1;
            }
        }
    }
}

/* Original [000689f0,00068b70). Adjacent labels, separated only by source
   or block markers, share one canonical instruction. Keep the loop/attachment
   position, then redirect branch and data label operands with balanced counts. */
void __fastcall scan_machine_context(MachineContext *context)
{
    MachineInstruction *instruction, *scan, *next;
    MachineOperand *operand;
    MachineFlowRegion *attachment;
    SymbolHeader **slot;
    LabelSymbol *label;
    unsigned kind, operation;
    int aliased = 0, count, index;
    for (instruction = context; instruction; instruction = next) {
        next = instruction->next;
        if ((instruction->opcode & 15) != 1) continue;
        ((LabelSymbol *)instruction->record.symbol)->instruction = instruction;
        for (scan = next; scan; scan = scan->next) {
            kind = scan->opcode & 15;
            if (kind == 7 || kind == 4 || kind == 5) continue;
            if (kind != 1) break;
            aliased = 1;
            ((LabelSymbol *)scan->record.symbol)->instruction = instruction;
            instruction->labels.flags |= scan->labels.flags & 3;
            if (scan->labels.flags & 1) {
                instruction->previous->next = instruction->next;
                instruction->next->previous = instruction->previous;
                insert_machine_after(scan->previous, instruction);
            }
            attachment = scan->labels.attachment;
            if (attachment && !instruction->labels.attachment) {
                instruction->labels.attachment = attachment;
                attachment->label = instruction;
                if (scan != instruction->next) {
                    instruction->previous->next = instruction->next;
                    instruction->next->previous = instruction->previous;
                    insert_machine_after(scan->previous, instruction);
                }
            }
        }
        next = scan;
    }
    if (!aliased) return;
    for (instruction = context; instruction; instruction = instruction->next) {
        if (instruction->opcode & 15) continue;
        operation = (unsigned char)(instruction->opcode >> 8);
        if (operation != MC_JCC && operation != MC_JMP && operation != MC_DATA) continue;
        count = machine_operand_counts[operation];
        for (index = 0; index < count; ++index) {
            operand = &instruction->operands[index ? 1 : 0];
            kind = operand->flags & 15;
            if (kind == OPERAND_SYMBOL) slot = &operand->value.symbol;
            else if (kind == OPERAND_LABEL_ADDRESS) slot = &operand->value.address->symbol;
            else continue;
            label = (LabelSymbol *)*slot;
            if (label->symbol.kind == 3 && label->instruction->record.symbol != &label->symbol) {
                --label->references;
                ++((LabelSymbol *)label->instruction->record.symbol)->references;
                *slot = label->instruction->record.symbol;
            }
        }
    }
}

/* Original [000689b0,000689f0). Remove unused labels from both the machine
   stream and its side chain, retaining externally protected label kind4. */
void __fastcall compact_machine_context(MachineContext *context)
{
    MachineInstruction *previous = context, *instruction = context->labels.next, *next;
    LabelSymbol *symbol;
    while (instruction) {
        symbol = (LabelSymbol *)instruction->record.symbol;
        next = instruction->labels.next;
        if (!symbol->references && symbol->label_kind != 4) {
            remove_machine_instruction(instruction);
            previous->labels.next = next;
        } else previous = instruction;
        instruction = next;
    }
}
