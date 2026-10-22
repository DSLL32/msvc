#ifndef WORK_OPTIMIZER_H
#define WORK_OPTIMIZER_H
#include "global-work.h"
#include "bitsets.h"
typedef struct LoopTreeRecord LoopTreeRecord;
/* Encountered prefix of the native 24-byte value record. The final two
   words remain undefined until their uses are reconstructed. */
typedef struct WorkValue {
    struct WorkValue *hash_next, *previous;
    Node *canonical;
    unsigned identifier;
} WorkValue;
enum { WORK_VALUE_BUCKETS = 64 };
extern WorkValue *work_value_buckets[WORK_VALUE_BUCKETS];
extern BitSet *work_memory_values, *work_global_values, *work_direct_values, *work_indirect_values;
extern unsigned next_work_value;
extern int work_has_indirect_values;
extern int work_optimization_active, work_reference_first_pass;
extern LoopTreeRecord *free_loop_tree_records;
void __fastcall initialize_work_optimizer(WorkItem *work);
void __fastcall build_work_references(WorkItem *work);
void clear_work_expression_cache(void);
void __fastcall rewrite_work_values(WorkItem *work);
void __fastcall rewrite_work_expressions(WorkItem *work);
void __fastcall collect_region_values(FlowRegion *region);
void __fastcall hoist_region_values(FlowRegion *regions);
void __fastcall initialize_work_value_tree(Node *node, unsigned address_context, Node **unused_result);
WorkValue *__fastcall intern_work_value(Node *node);
extern WorkValue *last_work_value;
Node *__fastcall memory_value_node(Node *scratch, Node *node);
int __fastcall is_global_work_value(Node *node, int include_anonymous);
int __fastcall is_address_exposed_value(Node *node);
#endif
