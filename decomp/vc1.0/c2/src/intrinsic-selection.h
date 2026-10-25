#ifndef INTRINSIC_SELECTION_H
#define INTRINSIC_SELECTION_H
#include "value-selection.h"
#include <recon.h>
unsigned short __fastcall select_intrinsic_block(Node *node, unsigned short type, int fill);
unsigned short __fastcall select_intrinsic_a3_a4(Node *node, int mode);
unsigned short __fastcall select_intrinsic_a5(Node *node);
unsigned short __fastcall select_intrinsic_a6(Node *node);
unsigned short __fastcall select_intrinsic_string_length(Node *value, int mode, unsigned type);
unsigned short __fastcall select_intrinsic_aa_ab(Node *node, int mode);
MachineOperand *__fastcall zero_value_operand(unsigned short type);
unsigned short __fastcall select_runtime_intrinsic(Node *node);
MachineOperand *__fastcall compare_intrinsic_string_part(MachineOperand *left, MachineOperand *right, unsigned offset, unsigned type, MachineOperand *different);
int __fastcall intrinsic_float_cleanup_count(FunctionSymbol *symbol);
SymbolHeader *__fastcall lookup_external_name(char *name);
MachineOperand *__fastcall select_replicated_fill_operand(Node *value, unsigned short type, unsigned count);
void __fastcall select_block_operation(unsigned constant_count, MachineOperand *count,
    MachineOperand *source, MachineOperand *destination, int wide, unsigned short type,
    int fill, int remainder, int direction);
void __fastcall emit_explicit_block_transfer(int fill, MachineOperand *source,
    MachineOperand *destination, unsigned short type, unsigned offset);
#endif
