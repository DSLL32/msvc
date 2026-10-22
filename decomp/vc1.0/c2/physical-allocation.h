#ifndef PHYSICAL_ALLOCATION_H
#define PHYSICAL_ALLOCATION_H
#include "register-variables.h"
#include "instruction-selection.h"
/* Twelve active counters in the native clearing span7c520..7c634.
   No address of that span escapes. These contextual counters deliberately
   do not reproduce its57 clear-only words; see cases/allocation-state.txt. */
typedef struct RegisterAllocationStatistics {
    int rewritten_instructions, rounds, spilled_variables, simplify_candidates;
    int rematerialized_operands, folded_spill_operands, inserted_spill_loads, inserted_spill_stores;
    int count_c544, count_c548;
    int strategies[2];
} RegisterAllocationStatistics;
extern RegisterAllocationStatistics register_allocation_statistics;
extern int available_integer_registers; /* 7c8b4 */
extern int rematerialization_cost_limit; /* 7c8bc */
extern int frame_register_available; /* 7c8c0 */
extern int rematerialization_changed; /* 7c8b8 */
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
extern int allocation_live_register_count; /* 7c8c8 */
extern RegisterConflict *allocation_live_registers; /* 7c8cc */
void __fastcall build_register_conflicts(WorkItem *work, RegisterConflictVisitor visitor);
void __fastcall initialize_allocation_live_registers(WorkItem *work);
void __fastcall record_operand_conflicts(WorkItem *work, MachineInstruction *instruction, int depth, RegisterConflictVisitor visitor);
void __fastcall record_special_register_conflicts(MachineInstruction *instruction, RegisterConflictVisitor visitor);
void __fastcall record_implicit_register_conflicts(MachineInstruction *instruction, RegisterConflictVisitor visitor);
int __fastcall add_allocation_live_register(RegisterConflict **list, unsigned short number);
int __fastcall remove_allocation_live_register(RegisterConflict **list, unsigned short number);
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
int __fastcall rewrite_stack_pointer_frame(MachineInstruction *instructions);
void finish_register_assignment(void);
void __fastcall record_allocation_operand(MachineInstruction *instruction, unsigned short number, int annotation, int operand_index, int depth);
void __fastcall conflict_with_live_registers(unsigned short number, unsigned short exclude, RegisterConflictVisitor visitor);
int __fastcall register_precedes_lea(MachineInstruction *instruction, unsigned short number);
int __fastcall register_follows_lea(MachineInstruction *instruction, unsigned short number);
int __fastcall allocation_operand_cost(MachineInstruction *instruction, RegisterVariable *variable, int annotation, unsigned operand_index, int apply, unsigned *spill_bytes, WorkItem *work);
int __fastcall loop_depth_weight(unsigned depth);
void __fastcall record_rematerialized_registers(WorkItem *work, MachineInstruction *instruction);
int __fastcall folded_spill_operand_cost(MachineInstruction *instruction, unsigned operand_index);
MachineOperand *__fastcall operand_for_spill(MachineOperand *operand, unsigned short number, int apply);
int __fastcall register_spill_homes_compatible(RegisterVariable *left, RegisterVariable *right);
RegisterConflict *__fastcall new_register_conflict(unsigned arena);
/* Native16-byte copy-affinity record, constructor2e5a0. The live list
   and recycler are shared by affinity sorting and coalescing. */
typedef struct RegisterAffinity {
    struct RegisterAffinity *next;
    RegisterVariable *left, *right;
    int benefit;
} RegisterAffinity;
typedef char RegisterAffinitySizeCheck[sizeof(RegisterAffinity) == 16 ? 1 : -1];
extern RegisterAffinity *register_affinities; /* 81114 */
extern RegisterAffinity *free_register_affinities; /* 81118 */
void __fastcall collect_register_affinities(WorkItem *work);
void sort_register_affinities(void);
int __fastcall coalesce_register_affinities(RegisterAffinity *affinity);
void __fastcall add_register_affinity(unsigned short left, unsigned short right, int benefit);
int __fastcall registers_conflict(RegisterVariable *left, RegisterVariable *right);
RegisterAffinity *__fastcall new_register_affinity(RegisterVariable *left, RegisterVariable *right);
RegisterAffinity *__fastcall merge_register_affinity(RegisterAffinity **list, RegisterAffinity *affinity);
void __fastcall release_register_affinity(RegisterAffinity *affinity);
void __fastcall insert_register_affinity(RegisterAffinity **list, RegisterAffinity *affinity);
int __fastcall count_shared_register_conflicts(RegisterVariable *left, RegisterVariable *right);
void __fastcall merge_register_graph(RegisterVariable *winner, RegisterVariable *loser);
void __fastcall remove_register_conflict(unsigned short left, unsigned short right);
RegisterAffinity *__fastcall unlink_register_affinity(RegisterAffinity *affinity);
void __fastcall add_block_physical_registers(MachineBlockHeader *block);
void __fastcall assign_operand_registers(MachineInstruction *instruction, MachineOperand *operand, int index);
#endif
