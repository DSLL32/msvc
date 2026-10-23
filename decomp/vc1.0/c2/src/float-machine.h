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
/* Original88-byte arena4 stream-motion record. The constructor establishes
   the complete allocation extent; fields await their contextual consumers.
   Scheduling shares its boundary at1c through the same active-context alias. */
typedef struct FloatStreamBranch {
    LabelSymbol *label;
    MachineInstruction *instruction;
    int stack_depth;
    unsigned references;
    struct FloatStreamBranch *next;
    unsigned flags;
} FloatStreamBranch;
typedef char FloatStreamBranchSizeCheck[sizeof(FloatStreamBranch) == 24 ? 1 : -1];
typedef struct MemoryMotionContext {
    struct MemoryMotionContext *previous, *next;
    LabelSymbol *label;
    unsigned field0c, field10, field14, field18;
    MachineInstruction *boundary, *last;
    unsigned field24, field28, field2c;
    FloatStreamBranch *branches, *last_branch;
    unsigned field38, field3c;
    unsigned order, field44;
    int stack_depth, field4c, maximum_depth;
    unsigned flags;
} MemoryMotionContext;
typedef char MemoryMotionContextSizeCheck[sizeof(MemoryMotionContext) == 88 ? 1 : -1];
typedef struct FloatStreamUse FloatStreamUse;
extern MemoryMotionContext *memory_motion_context, *float_stream_context;
extern MemoryMotionContext *float_stream_first_context, *float_stream_last_context;
extern FloatStreamUse **float_stream_register_uses, **float_stream_sorted_uses;
extern FloatStreamUse *float_stream_uses;
extern int float_stream_register_count, float_stream_order;
extern MachineInstruction *float_stream_next, *float_stream_previous;
int initialize_float_stream_context(void);
void finish_float_stream_uses(void);
void __fastcall process_float_stream_label(MachineInstruction *instruction);
void __fastcall process_float_stream_protected_block(MachineInstruction *instruction);
void __fastcall process_float_stream_call(MachineInstruction *instruction);
void __fastcall process_float_stream_branch(MachineInstruction *instruction);
void __fastcall process_float_stream_return(MachineInstruction *instruction);
void __fastcall process_float_stream_operation(MachineInstruction *instruction);
MemoryMotionContext *__fastcall find_float_stream_label_context(LabelSymbol *label);
int __fastcall match_float_stream_branches(FloatStreamBranch *branches, MachineInstruction *instruction, MachineInstruction **last);
void close_float_stream_context(void);
void __fastcall start_float_stream_context(MachineInstruction *instruction, LabelSymbol *label);
void __fastcall analyze_float_motion_range(MachineInstruction *first, MachineInstruction *last, int mode);
void merge_float_stream_context(void);
int __fastcall resolve_float_stream_label(LabelSymbol *label, MachineInstruction *instruction);
#endif
