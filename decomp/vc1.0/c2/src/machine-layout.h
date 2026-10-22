#ifndef MACHINE_LAYOUT_H
#define MACHINE_LAYOUT_H
#include "machine.h"
unsigned __fastcall machine_instruction_size(MachineInstruction *instruction);
int __fastcall align_machine_location(int offset, MachineInstruction *instruction, unsigned apply);
MachineInstruction *__fastcall label_machine_instruction(LabelSymbol *label);
unsigned __fastcall label_has_fallthrough_predecessor(MachineInstruction *instruction);
MachineInstruction *__fastcall next_machine_operation(MachineInstruction *instruction);
int __fastcall alignment_lookahead(MachineInstruction *instruction);
int __fastcall extend_previous_instruction(MachineInstruction *instruction, unsigned bytes);
#endif
