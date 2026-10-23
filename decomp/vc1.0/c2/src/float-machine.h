#ifndef FLOAT_MACHINE_H
#define FLOAT_MACHINE_H
#include "machine.h"
#include <recon.h>
/* Original twenty-byte virtual x87 value record. The stack slot is -1
   while the value is absent; a home operand is materialized when needed. */
typedef struct FloatMachineRegister {
    int stack_slot;
    MachineOperand *home;
    MachineInstruction *last_store;
    int width;
    unsigned flags;
} FloatMachineRegister;
typedef char FloatMachineRegisterSizeCheck[sizeof(FloatMachineRegister) == 20 ? 1 : -1];
extern FloatMachineRegister *float_machine_registers;
extern int float_machine_register_count;
extern int float_machine_stack[8], float_machine_virtual_depth;
extern int float_machine_conservative, float_machine_depth, float_machine_incoming_depth;
void initialize_float_register_homes(void);
void reset_float_register_stack(void);
int __fastcall scan_float_machine_range(MachineInstruction *first, MachineInstruction **end);
MachineInstruction *__fastcall prepare_float_machine_instruction(MachineInstruction *instruction, MachineInstruction **next);
void __fastcall finish_float_machine_range(MachineInstruction *boundary);
int __fastcall call_preserves_float_stack(MachineInstruction *instruction);
void exclude_float_machine_registers(void);
void release_float_machine_data(void);
#endif
