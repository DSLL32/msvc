#ifndef WORK_OPTIMIZER_H
#define WORK_OPTIMIZER_H
#include "global-work.h"
#include "bitsets.h"
typedef struct LoopTreeRecord LoopTreeRecord;
/* Native 24-byte memory-value record, including last local store and alias set. */
typedef struct WorkValue {
    struct WorkValue *hash_next, *previous;
    Node *canonical;
    unsigned identifier;
    Node *last_store;
    BitSet *aliases;
} WorkValue;
typedef char WorkValueSizeCheck[sizeof(WorkValue) == 24 ? 1 : -1];
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
typedef struct WorkReference {
    struct WorkReference *next;
    Node *node, *value;
} WorkReference;
enum { WORK_REFERENCE_BUCKETS = 32 };
typedef union WorkReferenceTable {
    WorkReference *buckets[WORK_REFERENCE_BUCKETS];
    union WorkReferenceTable *free_next;
} WorkReferenceTable;
typedef char WorkReferenceSizeCheck[sizeof(WorkReference) == 12 ? 1 : -1];
typedef char WorkReferenceTableSizeCheck[sizeof(WorkReferenceTable) == 128 ? 1 : -1];
extern WorkReferenceTable work_references;
extern WorkReferenceTable *free_work_reference_tables;
extern int next_work_reference;
extern BitSet *work_clobbered_values;
void __fastcall merge_predecessor_references(WorkItem *work);
void __fastcall analyze_block_references(WorkItem *work);
WorkReferenceTable *new_work_reference_table(void);
/* Encountered prefix of the 24-byte record linked through region+24.
   The current consumer reads next and tree only; field04 is uninterpreted. */
typedef struct RegionValue {
    struct RegionValue *next;
    unsigned field04;
    Node *tree;
} RegionValue;
extern BitSet *work_killed_values;
void __fastcall release_work_reference_table(WorkReferenceTable *table);
void flag_killed_work_references(void);
void __fastcall invalidate_work_reference(Node *node);
WorkReference *new_work_reference(void);
int __fastcall available_chain_operands(Node *node, BitSet *available);
typedef struct PendingWorkEffect {
    struct PendingWorkEffect *next;
    Node *tree;
} PendingWorkEffect;
extern PendingWorkEffect *pending_work_effects;
extern Expression *work_expression;
extern Node *enclosing_work_effect;
Node *__fastcall substitute_work_values(Node *node, int *changed);
Node *__fastcall analyze_reference_tree(Node *node, unsigned *effects, WorkReference **reference);
void __fastcall release_pending_work_effect(PendingWorkEffect *effect);
WorkReference *__fastcall lookup_work_reference(Node *node, int mode);
void __fastcall retarget_empty_work_branch(WorkItem *work, Node *node);
extern WorkReference *free_work_references;
/* A separate numbered-expression index, distinct from WorkValue's
   memory-value table. 0005dec0 constructs and caches temporary_load. */
typedef struct ExpressionUse {
    struct ExpressionUse *next;
    Node *tree;
} ExpressionUse;
typedef struct NumberedExpression {
    struct NumberedExpression *hash_next, *identifier_next;
    Node *canonical, *temporary_load;
    ExpressionUse *uses;
} NumberedExpression;
typedef char NumberedExpressionSizeCheck[sizeof(NumberedExpression) == 20 ? 1 : -1];
enum { NUMBERED_EXPRESSION_BUCKETS = 64 };
extern NumberedExpression *numbered_expressions[NUMBERED_EXPRESSION_BUCKETS];
extern NumberedExpression *numbered_expressions_by_id[NUMBERED_EXPRESSION_BUCKETS];
Node *__fastcall copy_numbered_expression(Node *node);
NumberedExpression *__fastcall intern_numbered_expression(Node *node);
void __fastcall add_expression_use(NumberedExpression *expression, Node *node);
int __fastcall equal_chain_value_ids(Node *left, Node *right);
extern Node *work_reference_parent;
PendingWorkEffect *new_pending_work_effect(void);
void __fastcall mark_work_reference(Node *node);
NumberedExpression *__fastcall find_numbered_expression(Node *node);
int __fastcall tree_uses_killed_value(Node *node);
extern PendingWorkEffect *free_pending_work_effects;
int __fastcall work_effect_invalidates_value(Node *effect, Node *value);
int __fastcall is_private_work_value(Node *node);
void build_work_alias_sets(void);
void __fastcall summarize_work_memory(WorkItem *work);
void __fastcall compute_work_memory_liveness(WorkItem *work);
void __fastcall invalidate_work_tree(Node *node);
int __fastcall has_no_later_work_store(Expression *expression);
enum {
    WORK_REFERENCE_AVAILABLE,
    WORK_MEMORY_READS,
    WORK_UNREAD_STORES,
    WORK_MEMORY_STORES,
    WORK_MEMORY_LIVE_IN,
    WORK_MEMORY_LIVE_OUT
};
extern BitSet *work_unread_stores, *work_stored_values, *work_upward_reads;
void __fastcall collect_work_memory_reads(Node *node);
WorkValue *__fastcall find_work_value(Node *node);
Node *__fastcall next_work_value_node(WorkValue **cursor);
#endif
