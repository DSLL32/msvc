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
struct BasicInduction;
int __fastcall work_is_outside_child_regions(WorkItem *work);
int __fastcall is_region_invariant_value(Node *node);
struct BasicInduction *__fastcall new_basic_induction(Node *assignment, Node *step,
    WorkItem *work, struct BasicInduction *parent, Node *left, Node *right);
#endif
