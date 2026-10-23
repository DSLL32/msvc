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
/* Native41e80 allocates8 bytes; the two byte flags follow the definition. */
typedef struct SchedulerRegisterState {
    MachineInstruction *definition;
    unsigned char recoverable, clobbered;
} SchedulerRegisterState;
typedef char SchedulerRegisterStateSizeCheck[sizeof(SchedulerRegisterState) == 8 ? 1 : -1];
typedef struct SchedulerUndo {
    MachineOperand *operand;
    long adjustment;
    unsigned short replacement;
    MachineInstruction *owner;
    struct SchedulerUndo *next;
} SchedulerUndo;
typedef char SchedulerUndoSizeCheck[sizeof(SchedulerUndo) == 20 ? 1 : -1];
extern int scheduler_address_penalty;
typedef struct SchedulerInstructionPair {
    MachineInstruction *first, *second;
} SchedulerInstructionPair;
/* Three history cycles occupy7c2ac..7c2c4. Native choice indices1/2
   relative to7c2bc select7c2c4/7c2cc: source selected[0/1]. Ready entries
   follow at7c2d4. Keeping these in one object preserves every physical slot. */
typedef struct SchedulerCandidate {
    MachineInstruction *instruction;
    SchedulerUndo *undo;
} SchedulerCandidate;
typedef char SchedulerCandidateSizeCheck[sizeof(SchedulerCandidate) == 8 ? 1 : -1];
typedef struct SchedulerPairs {
    SchedulerInstructionPair previous[3];
    SchedulerCandidate selected[2], ready[5];
} SchedulerPairs;
typedef char SchedulerPairsSizeCheck[sizeof(SchedulerPairs) == 80 ? 1 : -1];
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
void __fastcall prune_pentium_candidate_undos(MachineInstruction *instruction, int ready_count);
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
int __fastcall compare_scheduled_memory(MachineOperand *left, unsigned left_bytes, MachineOperand *right, unsigned right_bytes, int mode, int conservative, int *relation, SchedulerMemoryAccess *left_memory, SchedulerMemoryAccess *right_memory);
void __fastcall gather_pentium_candidates(MachineInstruction *instruction, int forward, int pipe_hint);
int __fastcall scan_schedule_interval(MachineInstruction *instruction, MachineInstruction *before, int forward, int flags, int force, int allow_backward, int restore);
int __fastcall can_schedule_pentium(MachineInstruction *instruction, int forward);
int __fastcall pentium_pair_independent(MachineInstruction *first, MachineInstruction *last);
void __fastcall remove_pentium_ready_pair(int first, int last);
int __fastcall scheduled_float_latency(MachineInstruction *instruction);
void __fastcall remove_pentium_candidate_undo(MachineInstruction *instruction, SchedulerCandidate *candidate);
void __cdecl apply_scheduler_undo(void);
void __fastcall record_scheduled_register(unsigned short reg, MachineInstruction *instruction, unsigned char clobbered);
int __fastcall scheduler_instruction_cycles(MachineInstruction *instruction);
extern int scheduler_has_indirect_branch, scheduler_branch_pipe;
void __fastcall set_scheduler_register_tracking(int enabled);
void __fastcall schedule_pentium_branch_edge(MachineInstruction *branch, MachineInstruction *candidate, MachineInstruction *middle, int taken, int crossed_label);
int __fastcall pair_pentium_fallthrough_jump(MachineInstruction *jump, MachineInstruction **middle);
int __fastcall pair_pentium_taken_edge(MachineInstruction *branch, MachineInstruction **middle, MachineInstruction *label, int *changed);
void __fastcall redirect_pentium_branch_labels(MachineInstruction *branch, MachineInstruction *middle);
void __fastcall assign_machine_operand(MachineOperand *destination, MachineOperand *source);
int __fastcall scheduled_address_changed_between(MachineInstruction *first, MachineInstruction *last,
    MachineOperand *operand, int forward);
#endif
