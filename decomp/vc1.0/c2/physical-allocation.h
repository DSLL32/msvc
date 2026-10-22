#ifndef PHYSICAL_ALLOCATION_H
#define PHYSICAL_ALLOCATION_H
#include "register-variables.h"
#include "instruction-selection.h"
/* Twelve active counters in the native clearing span7c520..7c634.
   No address of that span escapes. These contextual counters deliberately
   do not reproduce its57 clear-only words; see cases/allocation-state.txt. */
typedef struct RegisterAllocationStatistics {
    int rewritten_instructions, rounds, spilled_variables, simplify_candidates;
    int count_c534, count_c538, count_c53c, count_c540;
    int count_c544, count_c548;
    int strategies[2];
} RegisterAllocationStatistics;
extern RegisterAllocationStatistics register_allocation_statistics;
extern int available_integer_registers; /* 7c8b4 */
extern int rematerialization_cost_limit; /* 7c8bc */
extern int frame_register_available; /* 7c8c0 */
extern int frame_register_allocated; /* 7c8c4 */
/* 2cf58 allocates12 bytes for a temporary's hash entry; number&31 selects
   one of32 heads. Each of31 physical colors has a signed accumulated cost. */
typedef struct SpillTemporary {
    struct SpillTemporary *next;
    SymbolHeader *symbol;
    unsigned short number, reserved;
} SpillTemporary;
typedef char SpillTemporarySizeCheck[sizeof(SpillTemporary) == 12 ? 1 : -1];
extern SpillTemporary *spill_temporary_buckets[32]; /* 7c8d0..7c950 */
extern int physical_register_costs[31]; /* 7c6b4..7c730 */
typedef int (__fastcall *RegisterConflictVisitor)(RegisterVariable *, RegisterVariable *);
int frame_register_can_be_allocated(void);
int __fastcall function_allows_frame_register(Function *function);
void analyze_allocation_float_blocks(void);
void initialize_physical_variables(void);
void prune_rematerialized_variables(void);
int __fastcall machine_sequence_cost(MachineInstruction *instruction);
void initialize_allocation_round(void);
void __fastcall build_register_conflicts(WorkItem *work, RegisterConflictVisitor visitor);
int __fastcall add_register_conflict(RegisterVariable *left, RegisterVariable *right);
int simplify_unspillable_variables(void);
void build_allocation_constraints(void);
void finish_allocation_constraints(void);
RegisterVariable *__fastcall order_allocation_variables(int strategy);
int __fastcall count_register_conflicts(RegisterConflict *conflicts);
RegisterVariable *__fastcall color_register_variables(RegisterVariable **variables);
void __fastcall spill_register_variables(RegisterVariable *variables, int simplification);
void optimize_allocated_machine_regions(void);
void __fastcall assign_machine_registers(MachineInstruction *instructions);
int __fastcall frame_references_exceed_cost(WorkItem *work, int limit);
void record_debug_register_locations(void);
void __fastcall allocate_frame_symbols(WorkItem *work, int use_stack_pointer);
void __fastcall rewrite_stack_pointer_frame(MachineInstruction *instructions);
void finish_register_assignment(void);
#endif
