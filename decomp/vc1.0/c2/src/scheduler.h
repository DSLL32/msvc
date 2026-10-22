#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "machine-analysis.h"
#include <recon.h>
/* Dependency address record consumed by407b0/41f30. Registers14/16 are
   signed short physical-register IDs; -1 denotes an absent register. */
typedef struct SchedulerMemoryAccess {
    struct SchedulerMemoryAccess *next;
    MachineInstruction *instruction;
    long displacement;
    SymbolHeader *symbol;
    MachineOperand *operand;
    short base_register, index_register;
    unsigned width, flags;
} SchedulerMemoryAccess;
typedef char SchedulerMemoryAccessSizeCheck[sizeof(SchedulerMemoryAccess) == 32 ? 1 : -1];
/* Constructor41dc0:44 zeroed bytes in arena3; lazy dependency metadata. */
typedef struct InstructionSchedule {
    unsigned uses, defines;
    int float_inputs[2], float_result, float_arity, float_depth;
    SchedulerMemoryAccess *memory;
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
int __fastcall try_schedule_barrier(MachineInstruction *instruction, MachineInstruction **chosen, int *pipe_hint);
void __fastcall collect_pentium_candidates(MachineInstruction *instruction, int pipe_hint);
void __fastcall select_pentium_pair(int *candidate, int *count, int *wait, int *float_wait, int *dependent);
void __fastcall emit_pentium_pair(int candidate, int count, int dependent);
void __cdecl compact_pentium_candidates(void);
int __fastcall pentium_previous_memory_delay(MachineInstruction *instruction);
int __fastcall classify_pentium_pipe(MachineInstruction *instruction, int *pipe);
void __fastcall insert_pentium_stall(int reason);
void __fastcall remove_scheduled_float_exchange(MachineInstruction *instruction);
int __fastcall instruction_schedule_delay(MachineInstruction *instruction);
int __fastcall instruction_float_not_ready(MachineInstruction *instruction);
int __fastcall scheduler_move_conflict(MachineInstruction *first, MachineInstruction *last, int forward, int restore);
int __fastcall instruction_pair_conflict(MachineInstruction *first, MachineInstruction *last);
void __fastcall place_scheduled_instruction(MachineInstruction *first, MachineInstruction *last);
int __fastcall pentium_pair_dependency(MachineInstruction *first, MachineInstruction *last);
void __fastcall apply_scheduled_float(MachineInstruction *instruction);
void __fastcall advance_pentium_float_positions(MachineInstruction *instruction, int ready_count);
void __cdecl rotate_pentium_history(void);
int __fastcall pentium_prefix_instruction(MachineInstruction *instruction);
int __fastcall scheduled_float_conflict(MachineInstruction *first, MachineInstruction *last, int *adjustment);
void __fastcall analyze_scheduled_instruction(MachineInstruction *instruction);
void __fastcall resolve_scheduled_address(SchedulerMemoryAccess *memory, short reg);
int __fastcall scheduled_stack_overlap(MachineInstruction *first, MachineInstruction *last);
int __fastcall rewrite_scheduled_dependence(MachineInstruction *first, MachineInstruction *last, int forward, int reverse);
int __fastcall rewrite_scheduled_stack(MachineInstruction *first, MachineInstruction *last);
int __fastcall scheduled_memory_overlap(SchedulerMemoryAccess *first, SchedulerMemoryAccess *last, int forward);
int __fastcall scheduled_reads_first(unsigned operation);
void __fastcall analyze_scheduled_address(MachineInstruction *instruction, MachineOperand *operand, short *base, short *index, SchedulerMemoryAccess **memory);
#endif
