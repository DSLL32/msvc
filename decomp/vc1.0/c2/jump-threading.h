#ifndef JUMP_THREADING_H
#define JUMP_THREADING_H
#include "comparison-branches.h"
void __fastcall optimize_unconditional_jumps(MachineContext *context);
MachineInstruction *__fastcall find_function_end(MachineInstruction *instruction);
void __fastcall move_data_blocks_to_end(MachineContext *context);
MachineInstruction *__fastcall find_block_terminator(MachineInstruction *instruction);
void __fastcall register_machine_label(MachineInstruction *instruction);
int __fastcall merge_jump_diamond(MachineInstruction *branch, MachineInstruction *label,
    MachineInstruction *before_target);
#endif
