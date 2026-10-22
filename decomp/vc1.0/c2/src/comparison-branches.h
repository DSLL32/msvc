#ifndef COMPARISON_BRANCHES_H
#define COMPARISON_BRANCHES_H
#include "machine.h"
#include <recon.h>
void __fastcall optimize_branch_comparisons(MachineContext *context);
int __fastcall machine_block_protected(MachineInstruction *instruction);
void __fastcall retarget_comparison_branch(MachineInstruction *target, MachineInstruction *branch,
    MachineInstruction *load, int mode);
int __fastcall swap_comparison_for_match(MachineInstruction *comparison, MachineInstruction *target,
    MachineInstruction *branch);
#endif
