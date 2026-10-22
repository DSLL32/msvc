#include "instruction-selection.h"
#include "backend-passes.h"
#include "tree-rewrite.h"
#include "function-input.h"
#include <recon.h>
extern int optimize_global;
MachineBlockHeader *selection_global_header; /* 0007c2a4 */
MachineBlockHeader *selection_end; /* 0007ce48 */
MachineBlockHeader *selected_block; /* 0007cb04 */
int temporary_operand_count; /* 0007c350 */
/* Original [00017cc0,00017cf0). Kind 7 stores the block's register-use
   sets, ordinal and work flags. */
MachineBlockHeader *__fastcall new_selection_block(BitSet *definitions, BitSet *uses,
    int number, unsigned short flags)
{
    MachineBlockHeader *block = (MachineBlockHeader *)append_machine_instruction(7);
    block->definitions = definitions;
    block->uses = uses;
    block->number = number;
    block->flags = flags;
    return block;
}
/* Original [000170c0,00017160). Consume the expression list after selecting
   each root. Instruction passes may replace the current block header. */
MachineBlockHeader *__fastcall select_work_item(WorkItem *work)
{
    MachineBlockHeader *block;
    Expression *expression, *next;
    current_source_location = 0;
    block = new_selection_block(work->pass.selection.definitions, work->pass.selection.uses, work->number, work->flags);
    if (!optimize_global) next_machine_register = 32;
    selected_block = block;
    expression = work->body.expressions;
    while (expression) {
        next = expression->next;
        if (expression->source_location && expression->source_location != current_source_location) {
            current_source_location = expression->source_location;
            emit_source_marker(current_source_location);
        }
        temporary_operand_count = 0;
        select_expression(expression->tree);
        release_tree(expression->tree);
        release_expression(expression);
        block = selected_block;
        work->pass.selection.uses = block->uses;
        work->pass.selection.definitions = block->definitions;
        expression = next;
    }
    selected_block = block;
    return block;
}
/* Original [00017000,000170c0). Select and finish each work block, keeping
   one end marker attached while the instruction passes process the block. */
void __fastcall generate_work(WorkItem *work)
{
    if (optimize_global) {
        selection_global_header = (MachineBlockHeader *)new_machine_instruction(7);
        selection_global_header->definitions = allocation_candidate_registers;
        selection_global_header->uses = allocation_candidate_registers;
    }
    selection_end = (MachineBlockHeader *)new_machine_record(7,
        (BitSet *)0, (BitSet *)0, -1, 0);
    for (; work; work = work->next) {
        work->body.block = select_work_item(work);
        machine_tail->next = (MachineInstruction *)selection_end;
        selection_end->previous = machine_tail;
        if (optimize_global) prepare_machine_context((MachineInstruction *)selected_block);
        finish_selected_block(selected_block, 1);
        if (selected_block->flags & 0x400) peephole_selected_block(selected_block, optimize_global);
        machine_tail = selection_end->previous;
    }
}
