#ifndef REGISTER_USE_H
#define REGISTER_USE_H
#include "machine.h"
typedef struct RegisterLifetime {
    struct RegisterLifetime *next;
    unsigned short reg;
    short operand_index;
    MachineInstruction *last_use, *definition, *previous_definition;
} RegisterLifetime;
typedef char RegisterLifetimeSizeCheck[sizeof(RegisterLifetime) == 20 ? 1 : -1];
extern BitSet *block_live_registers;
void __fastcall finish_block_registers(BitSet *live);
RegisterLifetime *__fastcall lookup_register_lifetime(unsigned short reg, int create);
void __fastcall finish_register_lifetime(RegisterLifetime *lifetime);
int __fastcall overwrites_first_register(MachineInstruction *instruction, int check_live);
int __fastcall overwrites_second_register(MachineInstruction *instruction, int check_live);
void __fastcall record_operand_uses(MachineInstruction *instruction, MachineOperand *operand, int index);
void __fastcall record_register_use(unsigned short reg, MachineInstruction *instruction, int index);
unsigned __fastcall register_definition_width(MachineInstruction *instruction, short reg, unsigned *byte_lane);
int __fastcall writes_first_operand(MachineInstruction *instruction);
unsigned __fastcall implicit_register_mask(MachineInstruction *instruction, int mode);
int __fastcall register_result_ignores_old_value(MachineInstruction *instruction);
int __fastcall register_partial_write_safe(unsigned short reg);
int __fastcall register_storage_bytes(unsigned short reg);
int __fastcall operands_equal(MachineOperand *left, MachineOperand *right);
int __fastcall instruction_has_side_effects(MachineInstruction *instruction);
#endif
