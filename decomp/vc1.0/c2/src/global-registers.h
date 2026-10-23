#ifndef GLOBAL_REGISTERS_H
#define GLOBAL_REGISTERS_H
#include "global-work.h"
#include "bitsets.h"
#include <recon.h>
/* Original twelve-byte remembered floating-register declaration. Padding at
   offset2 is never initialized or read by record_float_register. */
typedef struct FloatRegisterDeclaration {
    unsigned short number, reserved;
    SymbolHeader *symbol;
    struct FloatRegisterDeclaration *next;
} FloatRegisterDeclaration;
typedef char FloatRegisterDeclarationSizeCheck[sizeof(FloatRegisterDeclaration) == 12 ? 1 : -1];
extern FloatRegisterDeclaration *float_register_declarations;
/* Native connected definition range. field06 is not consumed by this driver. */
typedef struct AllocationRange {
    struct AllocationRange *next;
    unsigned short number, field06;
    int first_block, last_block;
    BitSet *definitions;
    unsigned flags;
} AllocationRange;
typedef char AllocationRangeSizeCheck[sizeof(AllocationRange) == 24 ? 1 : -1];
/* Native 28-byte allocation value, keyed by its canonical address operand. */
typedef struct AllocationValue {
    struct AllocationValue *next;
    Node *node;
    BitSet *definitions, *registers;
    AllocationRange *ranges;
    unsigned global_load_count;
    short last_block;
    unsigned short diagnostic_flags;
} AllocationValue;
typedef char AllocationValueSizeCheck[sizeof(AllocationValue) == 28 ? 1 : -1];
enum { ALLOCATION_VALUE_BUCKETS = 32 };
enum AllocationDiagnosticFlags {
    ALLOCATION_CHECK_INITIALIZATION = 1,
    ALLOCATION_REPORTED_POSSIBLE_UNINITIALIZED = 2
};
/* Definition flow before connected ranges receive virtual registers. */
enum AllocationDefinitionFlow {
    ALLOCATION_DEFINITION_GEN = 2, ALLOCATION_DEFINITION_KILLS,
    ALLOCATION_DEFINITION_IN, ALLOCATION_DEFINITION_OUT
};
/* Register flow reuses the completed definition-flow slots. */
enum AllocationRegisterFlow {
    ALLOCATION_REGISTER_REFERENCES = 0,
    ALLOCATION_REGISTER_UPWARD_USES = 1,
    ALLOCATION_REGISTER_HISTORY_IN = 2,
    ALLOCATION_REGISTER_KILLS = 3,
    ALLOCATION_REGISTER_HISTORY_OUT = 1
};
extern AllocationValue *allocation_values[ALLOCATION_VALUE_BUCKETS];
extern BitSet *allocation_live_definitions, *allocation_candidate_registers;
extern BitSet *allocation_observable_registers, *allocation_initial_definitions;
extern BitSet *allocation_observable_definitions, *allocation_all_definitions;
extern int allocation_region_checked, allocation_last_definition;
void initialize_global_values(void);
void __fastcall record_float_register(unsigned short number, SymbolHeader *symbol);
void __fastcall collect_allocation_ranges(Node *node);
void __fastcall rewrite_allocation_registers(Node *node);
void __fastcall solve_allocation_liveness(WorkItem *work);
void __fastcall solve_allocation_history(WorkItem *work);
void __fastcall exclude_exception_registers(WorkItem *work);
void __fastcall restore_excluded_registers(Node *node);
extern BitSet *allocation_excluded_registers;
void assign_global_registers(void);
void extend_global_register_ranges(void);
void record_global_register_ranges(void);
extern AllocationRange *free_allocation_ranges;
extern unsigned next_allocation_definition;
void __fastcall solve_definition_reachability(WorkItem *work);
AllocationValue *__fastcall find_allocation_value(Node *node, int create);
BitSet *new_call_definitions(void);
int __fastcall allocation_expression_has_call(Node *node);
void __fastcall limit_global_allocation_values(WorkItem *work);
extern unsigned last_allocation_initial_definition;
unsigned __fastcall count_global_allocation_loads(Node *node);
int __cdecl compare_allocation_load_counts(const void *left, const void *right);
int __fastcall allocation_use_dominates_backedges(WorkItem *work);
AllocationRange *new_allocation_range(void);
void __fastcall release_allocation_range(AllocationRange *range);
#endif
