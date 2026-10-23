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
extern int float_machine_range_entry_depth, float_machine_maximum_depth;
extern int float_machine_conservative, float_machine_exit_depth, float_machine_stack_depth;
void initialize_float_register_homes(void);
void reset_float_register_stack(void);
int __fastcall scan_float_machine_range(MachineInstruction *first, MachineInstruction **end);
void __fastcall prepare_float_machine_instruction(MachineInstruction *instruction, MachineInstruction **next);
void __fastcall finish_float_machine_range(MachineInstruction *boundary);
int __fastcall call_preserves_float_stack(MachineInstruction *instruction);
void exclude_float_machine_registers(void);
void release_float_machine_data(void);
int __fastcall can_keep_float_store(MachineInstruction *instruction, unsigned short number, int use_summary);
int __fastcall float_store_value_dead(MachineInstruction *instruction);
int __fastcall previous_float_is_load(MachineInstruction *instruction);
void __fastcall initialize_float_register_home(MachineOperand *home, unsigned short number);
int __fastcall prepare_float_register_source(MachineInstruction *instruction, int number);
int __fastcall find_float_register_reload(MachineInstruction *instruction, unsigned short number, MachineInstruction **reload);
void __fastcall save_float_machine_register(MachineInstruction *instruction, FloatMachineRegister *reg, unsigned short number, MachineInstruction *reload);
void pop_float_machine_stack(void);
void __fastcall exchange_float_machine_stack(int first, int second);
void push_float_machine_stack(void);
void __fastcall float_register_heap_release(void *allocation);
#endif
