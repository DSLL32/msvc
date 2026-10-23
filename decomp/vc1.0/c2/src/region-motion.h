#ifndef REGION_MOTION_H
#define REGION_MOTION_H
#include "work-optimizer.h"
#include "transformation-cursor.h"
#include <recon.h>
extern FlowRegion *hoist_current_region;
extern WorkItem *hoist_entry_work, *hoist_header_work, *hoist_first_work;
extern WorkItem *hoist_last_work, *hoist_after_work;
void hoist_invariant_region_stores(void);
void collect_basic_region_inductions(void);
int prepare_region_exit_motion(void);
void collect_derived_region_inductions(void);
void combine_region_inductions(void);
void reduce_region_induction_strength(void);
void rewrite_region_inductions(void);
int simplify_region_exit(void);
void finish_region_motion(void);
void rebuild_region_header_references(void);
void release_region_motion(void);
/* Original36-byte arena-4 induction descriptor, encountered through exit motion.
   Unrecovered middle fields retain explicit offset names. */
typedef struct BasicInduction {
    struct BasicInduction *next;
    Node *assignment, *step;
    int dominates_region;
    int total_uses, derived_uses;
    Node *right, *left;
    struct BasicInduction *parent;
} BasicInduction;
typedef char BasicInductionSizeCheck[sizeof(BasicInduction) == 36 ? 1 : -1];
int __fastcall work_is_outside_child_regions(WorkItem *work);
int __fastcall is_region_invariant_value(Node *node);
struct BasicInduction *__fastcall new_basic_induction(Node *assignment, Node *step,
    WorkItem *work, struct BasicInduction *parent, Node *left, Node *right);
int __fastcall induction_is_dead_on_exit(Node *assignment);
void remove_region_blocks(void);
void __fastcall remove_region_block(WorkItem *work);
BasicInduction *__fastcall find_basic_induction_load(Node *node);
Node *__fastcall find_induction_initial_value(Node *assignment);
int __fastcall basic_induction_increment(BasicInduction *induction);
/* Encountered36-byte affine-value,16-byte use and grouping records. */
typedef struct DerivedInductionUse {
    struct DerivedInductionUse *next;
    Node *tree;
    Expression *expression;
    WorkItem *work;
} DerivedInductionUse;
typedef struct DerivedInduction {
    struct DerivedInduction *next;
    Node *tree;
    Node *parent;
    Node *factor;
    BasicInduction *basic;
    int negative;
    int uses, blocked;
    DerivedInductionUse *references;
} DerivedInduction;
typedef struct DerivedInductionGroup {
    struct DerivedInductionGroup *next;
    DerivedInduction *candidates;
    Node *representative;
    int blocked;
} DerivedInductionGroup;
typedef struct DerivedInductionRewrite {
    struct DerivedInductionRewrite *next;
    Node *value, *temporary_load;
} DerivedInductionRewrite;
typedef char DerivedInductionSizeCheck[sizeof(DerivedInduction) == 36 ? 1 : -1];
typedef char DerivedGroupSizeCheck[sizeof(DerivedInductionGroup) == 16 ? 1 : -1];
void __fastcall collect_derived_induction_tree(Node *node, Node *parent);
int __fastcall choose_derived_induction_representative(Node *difference, Node *left,
    Node *right, Node **representative);
int __fastcall derived_induction_is_profitable(Node *node);
void __fastcall rewrite_derived_induction_group(DerivedInduction *candidates, Node *representative);
BasicInduction *__fastcall find_basic_induction(Node *address);
void __fastcall add_derived_induction(Node *tree, Node *parent, BasicInduction *basic,
    Node *factor, int negative, int blocked);
void __fastcall collect_selected_region_values(Node *node);
Node *__fastcall rewrite_selected_region_values(Node *node);
Node *__fastcall materialize_region_value(Node *node);
typedef struct RegionMemoryOperation {
    struct RegionMemoryOperation *next;
    Node *assignment;
    unsigned reverse, opcode, bytes;
    BasicInduction *destination, *source;
    Node *original_assignment, *value;
    Expression *expression;
} RegionMemoryOperation;
typedef char RegionMemoryOperationSizeCheck[sizeof(RegionMemoryOperation) == 40 ? 1 : -1];
int supports_region_memory_operations(void);
int __fastcall work_dominates_region_paths(WorkItem *work);
BasicInduction *__fastcall find_region_memory_induction(Node *address, Node *assignment,
    unsigned bytes, unsigned *reverse);
void __fastcall exclude_region_memory_operations(Node *node);
void create_region_induction_counter(void);
void __fastcall insert_region_memory_operation(Expression *anchor, Node *value, Node *address,
    unsigned opcode, Node *counter, unsigned bytes, Node *assignment);
void __fastcall rewrite_region_induction_tree(Node *node);
void __fastcall remove_basic_induction(BasicInduction *induction);
void __fastcall rewrite_region_exit_counter(Node *count);
int __fastcall can_rewrite_induction_comparison(BasicInduction *induction, Node *bound, Node *adjustment);
/* Original60820 allocates20 bytes in arena2, with four cloned trees;
   the final tree is the saved trip count. Cleanup609f0 releases all four. */
struct LoopTreeRecord {
    struct LoopTreeRecord *next;
    Node *address, *assignment, *step, *trip_count;
};
typedef char LoopTreeRecordSizeCheck[sizeof(LoopTreeRecord) == 20 ? 1 : -1];
void __fastcall release_basic_induction(BasicInduction *induction);
void __fastcall release_loop_tree_record(LoopTreeRecord *record);
#endif
