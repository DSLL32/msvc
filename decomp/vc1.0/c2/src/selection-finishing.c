#include "instruction-selection.h"
#include "backend-passes.h"
#include "function-input.h"
#include "tree-rewrite.h"
#include <recon.h>
/* Original [0001dca0,0001dd60). Reanalyze the block after any rewrite.
   The return value is the following block marker, including for skipped blocks. */
MachineInstruction *__fastcall finish_selected_block(MachineBlockHeader *block, int mode)
{
    MachineInstruction *instruction, *next;
    unsigned kind, flags;
    if (block->flags & 2) return next_machine_block((MachineInstruction *)block);
    do {
        tree_changed = 0;
        analyze_selected_block(block);
        instruction = block->next;
        while ((instruction->opcode & 15) != 7) {
            next = instruction->next;
            flags = instruction->opcode;
            kind = flags & 15;
            if (!kind) next = finish_machine_instruction(instruction, mode);
            else if (kind == 4) current_source_location = instruction->record.source_location;
            else if (kind == 8) {
                if (!instruction->record.next_use && (flags & 0x20000))
                    remove_machine_instruction(instruction);
                else instruction->opcode = flags & ~0x800000U;
            } else if (kind == 9) instruction->opcode = flags & ~0x800000U;
            instruction = next;
        }
    } while (tree_changed);
    return instruction;
}

/* Original [0001dc70,0001dc80). Preserve the separate whole-stream wrapper
   and its native tail edge into block finishing. */
void optimize_machine_flow(void)
{
    finish_machine_blocks(machine_instructions, 1);
}

/* Original [0001dc80,0001dca0). Each block finisher returns the next
   header; the final sentinel has no following instruction. */
void __fastcall finish_machine_blocks(MachineInstruction *instructions, int mode)
{
    while (instructions && instructions->next)
        instructions = finish_selected_block((MachineBlockHeader *)instructions, mode);
}
