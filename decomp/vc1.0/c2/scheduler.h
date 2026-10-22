#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "machine-analysis.h"
/* Constructor41dc0:44 zeroed bytes in arena3. Leading/1c fields await
   their dependency consumers; x87 values and ordering fields have live names. */
typedef struct InstructionSchedule {
    unsigned field00, field04;
    int float_inputs[2], float_result, float_arity, float_depth;
    unsigned field1c;
    int ordinal, priority;
    unsigned flags;
} InstructionSchedule;
typedef char InstructionScheduleSizeCheck[sizeof(InstructionSchedule) == 44 ? 1 : -1];
InstructionSchedule *__cdecl new_instruction_schedule(void);
void __fastcall prepare_float_schedule(MachineInstruction *instruction);
int __fastcall call_float_result_count(MachineInstruction *instruction);
/* Constructor use3f7d2: arena4 node, live only during scheduling. */
typedef struct SchedulerRegionFrame {
    MachineInstruction *header, *end;
    struct SchedulerRegionFrame *previous;
} SchedulerRegionFrame;
typedef char SchedulerRegionFrameSizeCheck[sizeof(SchedulerRegionFrame) == 12 ? 1 : -1];
typedef struct SchedulerRegisterState SchedulerRegisterState;
extern SchedulerRegionFrame *scheduler_regions;
extern MachineInstruction *scheduler_instruction;
extern unsigned short scheduler_register_capacity;
extern SchedulerRegisterState **scheduler_register_states;
extern int scheduler_track_registers, scheduler_after_label, scheduler_memory_run;
extern int scheduler_region_balance;
extern int scheduler_float_invalid, scheduler_float_depth, scheduler_has_float;
extern int scheduler_float_values[8], scheduler_float_serial;
extern int *scheduler_float_positions;
extern int scheduler_float_positions_owned;
void __fastcall schedule_machine_instructions(int track_registers);
void *__fastcall scheduler_allocate(unsigned bytes);
void __fastcall scheduler_release(void *memory);
int __fastcall instruction_accesses_memory(MachineInstruction *instruction);
void __fastcall prepare_scheduled_instruction(MachineInstruction *instruction, int ordinal);
void __fastcall schedule_window(MachineInstruction *first, MachineInstruction *last);
MachineInstruction *__fastcall recombine_scheduled_loads(MachineInstruction *first, MachineInstruction *last);
#endif
