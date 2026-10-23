#ifndef VALUE_SELECTION_H
#define VALUE_SELECTION_H
#include "temporaries.h"
#include "instruction-selection.h"
#include "instruction-rewrite.h"
#include <recon.h>
typedef struct FloatingOpcode {
    unsigned tree_opcode;
    char *description;
    unsigned opcode;
} FloatingOpcode;
typedef char FloatingOpcodeSizeCheck[sizeof(FloatingOpcode) == 12 ? 1 : -1];
extern const OpcodeMap signed_arithmetic_ops[], unsigned_shift_ops[];
extern const OpcodeMap signed_false_conditions[], unsigned_false_conditions[];
extern MachineOperand operand_st1;
extern Node *x87_stack_nodes[8];
void __fastcall record_x87_node(Node *node);
unsigned __fastcall lookup_floating_opcode(unsigned opcode, const FloatingOpcode *map);
void __fastcall arrange_x87_left(Node *node);
MachineOperand *__fastcall emit_register_barrier(MachineOperand *operand, unsigned type);
void __fastcall select_constant(Node *node, unsigned short reg);
unsigned short __fastcall select_conditional_value(Node *node);
void mark_x87_call_result(void);
MachineOperand *__fastcall frame_temporary_operand(SymbolHeader *temporary, unsigned offset);
MachineOperand *__fastcall immediate_operand(unsigned value);
MachineOperand *__fastcall symbol_operand(Node *reference, int indirect);
void __fastcall spill_x87_for_node(Node *node);
unsigned short __fastcall select_intrinsic(Node *node);
unsigned __fastcall aligned_value_size(Node *argument);

SymbolHeader *__fastcall floating_constant_symbol(Node *constant);
int __fastcall comparison_uses_intrinsic_flags(Node *node);
#endif
