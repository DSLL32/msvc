#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "machine-analysis.h"
#include <recon.h>
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
typedef struct SchedulerUndo SchedulerUndo;
typedef struct SchedulerInstructionPair {
    MachineInstruction *first, *second;
} SchedulerInstructionPair;
/* Native current-pair index uses7c2bc+index*8; preceding history occupies
  7c2ac..7c2bc. Keep those aliases in one contextual typed object. */
typedef struct SchedulerPairs {
    SchedulerInstructionPair previous[2], candidates[3];
} SchedulerPairs;
typedef char SchedulerPairsSizeCheck[sizeof(SchedulerPairs) == 40 ? 1 : -1];
extern SchedulerUndo *scheduler_undo;
extern MachineInstruction *scheduler_cursor, *scheduler_last_float;
extern signed char scheduler_float_delay;
extern int scheduler_stop_window, scheduler_ready_count;
extern SchedulerPairs scheduler_pairs;
int __fastcall prepare_schedule_window(MachineInstruction *first, MachineInstruction *last);
MachineInstruction *__fastcall schedule_generic_instruction(MachineInstruction *instruction);
MachineInstruction *__fastcall schedule_pentium_instruction(MachineInstruction *instruction);
void __fastcall update_scheduled_registers(MachineInstruction *instruction);
MachineInstruction *__fastcall choose_pentium_instruction(MachineInstruction *instruction);
extern SchedulerRegionFrame *scheduler_regions;
extern MachineInstruction *scheduler_instruction;
extern unsigned short scheduler_register_capacity;
extern SchedulerRegisterState **scheduler_register_states;
extern int scheduler_track_registers, scheduler_after_label, scheduler_memory_run;
extern int scheduler_region_balance;
extern int scheduler_float_invalid, scheduler_float_depth, scheduler_has_float;
extern int scheduler_float_values[8], scheduler_float_serial;
extern unsigned scheduler_float_position_capacity;
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
