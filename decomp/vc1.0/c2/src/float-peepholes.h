#ifndef FLOAT_PEEPHOLES_H
#define FLOAT_PEEPHOLES_H
#include "machine.h"
#include "extended.h"
#include <recon.h>
const Extended *__fastcall floating_value_for_symbol(SymbolHeader *symbol);
void __fastcall split_float_memory_transfer(MachineInstruction *load, MachineInstruction *store, MachineInstruction *first, MachineInstruction *last, int reversed, MachineInstruction *comparison);
int __fastcall can_split_float_transfer(MachineInstruction *load, MachineInstruction *store);
MachineInstruction *__fastcall previous_float_load(MachineInstruction *instruction);
MachineInstruction *__fastcall find_float_transfer_match(MachineInstruction *instruction);
int __fastcall can_reuse_float_load(MachineInstruction *instruction);
int __fastcall can_reuse_float_load_width(MachineInstruction *instruction, unsigned bytes);
int __fastcall combine_float_store_load(MachineInstruction *first, MachineInstruction *last);
int __fastcall fold_float_compare_store(MachineInstruction *instruction, MachineInstruction *branch);
void __fastcall fold_previous_float_load(MachineInstruction *first, MachineInstruction *last);
#endif
