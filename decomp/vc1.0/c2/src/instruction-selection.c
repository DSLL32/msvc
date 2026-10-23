#include "instruction-selection.h"
#include "backend-passes.h"
#include "tree-rewrite.h"
#include "function-input.h"
#include "instruction-rewrite.h"
#include "register-variables.h"
#include "bitsets.h"
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
/* Original7c2a8: a process-wide generation for cloned rematerialization
   sequences. Zero is reserved in the high byte of a machine record. */
static unsigned rematerialization_generation;
/* Original [00019930,00019b6e). Select an assigned virtual register once,
   cloning its generated sequence into a deferred block when cheap to repeat. */
int __fastcall try_select_assignment(Node *node)
{
    MachineInstruction *before, *saved_tail, *saved_head, *first, *scan, *copy, *next;
    MachineBlockHeader *block;
    unsigned short number;
    unsigned member, tag;
    if (node->left.node->opcode != 0xa2) return 0;
    if (node->attributes.words.type & 0x3000) {
        node->attributes.words.flags &= ~0x20;
        select_expression(node);
        return 1;
    }
    number = (unsigned short)node->left.node->left.value;
    if (register_rematerialization(number)) return 0;
    node->attributes.words.flags &= ~0x20;
    before = machine_tail;
    if (!before) {
        compiler_internal_error("@(#)codegen.c:1.167", 0x5af);
        before = machine_tail;
    }
    select_expression(node);
    saved_tail = machine_tail;
    saved_head = machine_instructions;
    machine_tail = deferred_tail;
    machine_instructions = deferred_instructions;
    member = number & 0x3fff;
    new_selection_block(bitset_include(0, member), 0, 0, 0);
    block = (MachineBlockHeader *)machine_tail;
    ++rematerialization_generation;
    first = before->next;
    tag = rematerialization_generation << 24;
    first->opcode = (first->opcode & 0xffffffU) | tag;
    if (!(first->opcode & 0xff000000U)) ++rematerialization_generation;
    for (scan = first; scan; scan = scan->next) {
        copy = clone_machine_record(scan);
        machine_tail->next = copy;
        copy->previous = machine_tail;
        machine_tail = copy;
        tag = rematerialization_generation << 24;
        scan->opcode = (scan->opcode & 0xffffffU) | tag;
        copy->opcode = (copy->opcode & 0xffffffU) | (scan->opcode & 0xff000000U);
    }
    machine_tail->next = (MachineInstruction *)selection_global_header;
    selection_global_header->previous = machine_tail;
    conservative_flag_analysis = 1;
    finish_selected_block(block, 1);
    conservative_flag_analysis = 0;
    machine_tail = selection_global_header->previous;
    if (!rematerialization_is_cheap(block->next)) {
        machine_register_set = bitset_include(machine_register_set, member);
        scan = (MachineInstruction *)block;
        while (scan != (MachineInstruction *)selection_global_header) {
            next = scan->next;
            release_machine_instruction(scan);
            scan = next;
        }
        for (scan = first; scan; scan = scan->next) scan->opcode &= 0xffffffU;
        if (deferred_tail) {
            deferred_tail->next = (MachineInstruction *)selection_global_header;
            selection_global_header->previous = deferred_tail;
        }
    } else {
        set_register_rematerialization(number, block);
        deferred_tail = machine_tail;
        deferred_instructions = machine_instructions;
    }
    machine_tail = saved_tail;
    machine_instructions = saved_head;
    return 1;
}
