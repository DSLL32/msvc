#ifndef FLOAT_PEEPHOLES_H
#define FLOAT_PEEPHOLES_H
#include "machine.h"
#include "extended.h"
#include <recon.h>
/* Encountered six-operation ISA relationships shared by floating passes. */
typedef struct X87ArithmeticForms {
    unsigned normal, popping, right_popping, integer, right_integer, reversed;
} X87ArithmeticForms;
typedef char X87ArithmeticFormsSizeCheck[sizeof(X87ArithmeticForms) == 24 ? 1 : -1];
extern const X87ArithmeticForms x87_arithmetic_forms[6];
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
MachineInstruction *__fastcall rewrite_pentium_float_store(MachineInstruction *instruction);
MachineInstruction *__fastcall rewrite_i486_float_pop(MachineInstruction *instruction);
#endif
