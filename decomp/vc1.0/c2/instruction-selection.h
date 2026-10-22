#ifndef INSTRUCTION_SELECTION_H
#define INSTRUCTION_SELECTION_H
#include "machine.h"
extern BitSet *selection_anchor;
extern MachineBlockHeader *selection_global_header, *selection_end, *selected_block;
extern unsigned selection_flags;
MachineInstruction *__fastcall new_machine_instruction(unsigned kind);
MachineInstruction *__cdecl new_machine_record(unsigned kind, ...);
MachineBlockHeader *__fastcall select_work_item(WorkItem *work);
MachineInstruction *__fastcall finish_selected_block(MachineBlockHeader *block, int mode);
void __fastcall peephole_selected_block(MachineBlockHeader *block, int global);
MachineBlockHeader *__fastcall new_selection_block(BitSet *definitions, BitSet *uses,
    int number, unsigned short flags);
void __fastcall select_expression(Node *node);
void __fastcall emit_source_marker(unsigned source_location);
MachineInstruction *__fastcall append_machine_instruction(unsigned kind);
#endif
