/* Region invariant and induction-value motion. */
#include "region-motion.h"
#include "expression-fields.h"
#include <recon.h>
FlowRegion *hoist_current_region; /* 0007bea8 */
WorkItem *hoist_entry_work;  /* 0007bea0 */
WorkItem *hoist_header_work; /* 0007be9c */
WorkItem *hoist_first_work;  /* 0007be98 */
WorkItem *hoist_last_work;   /* 0007be94 */
WorkItem *hoist_after_work;  /* 0007be90 */
/* Original000830d0: one cursor shared by expression and machine phases. */
TransformationCursor transformation_cursor;

/* Original [0005c020,0005c120). Process nested regions before their
   parent, establish expression anchors, and run induction/value-motion
   stages with the original early-exit and cleanup order. */
void __fastcall hoist_region_values(FlowRegion *regions)
{
    WorkItem *header;
    for (; regions; regions = regions->next) {
        if (regions->children) hoist_region_values(regions->children);
        if (regions->flags & 1) continue;
        hoist_current_region = regions;
        hoist_entry_work = regions->entry;
        if (!hoist_entry_work->body.expressions)
            hoist_entry_work->body.expressions = insert_tree_expression(0, 0, new_node(1));
        header = hoist_entry_work->next;
        if (!header->body.expressions)
            header->body.expressions = insert_tree_expression(0, 0, new_node(1));
        hoist_header_work = hoist_entry_work->next;
        transformation_cursor.expression = hoist_header_work->body.expressions;
        hoist_first_work = regions->entry->next->next;
        hoist_last_work = regions->last;
        hoist_after_work = hoist_last_work->next;
        hoist_invariant_region_stores();
        collect_basic_region_inductions();
        if (!prepare_region_exit_motion()) {
            collect_derived_region_inductions();
            combine_region_inductions();
            reduce_region_induction_strength();
            rewrite_region_inductions();
            simplify_region_exit();
            finish_region_motion();
            rebuild_region_header_references();
            release_region_motion();
        }
    }
}
