/* Region invariant and induction-value motion. */
#include "region-motion.h"
#include "expression-fields.h"
#include "tree-input.h"
#include "function-input.h"
#include "work-prepare.h"
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

static BitSet *hoist_seen_memory_values; /* 0007be6c: reads seen during store scan. */
static BitSet *hoist_selected_values;    /* 0007be70: values selected for later rewriting. */
static BitSet *hoist_invariant_values;   /* 0007be74: invariant expression identifiers. */
int __fastcall work_dominates_region_exits(WorkItem *work);
int __fastcall work_dominates_region_backedges(WorkItem *work);
int __fastcall classify_region_invariants(Node *node);
int __fastcall can_hoist_region_store(Node *node);
void __fastcall remove_region_candidate(Node *node, int ancestors);
/* Original [0005f1c0,0005f2db). Mark invariant stores, then move a safe
   store to the region's insertion anchor only when every relevant exit/backedge
   passes through its block and no prior read or conflicting store prevents it. */
void hoist_invariant_region_stores(void)
{
    WorkItem *work;
    Expression *expression;
    Node *node;
    int blocked, identifier;
    hoist_seen_memory_values = 0;
    hoist_selected_values = 0;
    hoist_invariant_values = 0;
    for (work = hoist_first_work; work != hoist_after_work; work = work->next) {
        blocked = !(work_dominates_region_exits(work) && work_dominates_region_backedges(work));
        for (expression = work->body.expressions; expression; expression = expression->next) {
            node = expression->tree;
            node->attributes.words.flags &= ~0x400;
            if (classify_region_invariants(node) &&
                work->exception_scope == hoist_first_work->exception_scope && node->opcode == 0x32) {
                node->attributes.words.flags |= 0x400;
                if (!blocked && can_hoist_region_store(node) &&
                    !bitset_contains(hoist_seen_memory_values,
                        (short)node->detail.node->info.words.identifier)) {
                    insert_tree_expression(transformation_cursor.expression, 0, node);
                    expression->tree = new_node(1);
                    remove_region_candidate(node, 0);
                    identifier = (short)node->detail.node->info.words.identifier;
                    bitset_remove(work->flow_sets[WORK_MEMORY_STORES], identifier);
                    bitset_remove(work->flow_sets[WORK_UNREAD_STORES], identifier);
                }
            }
        }
    }
    release_bitset(hoist_seen_memory_values);
}

int __fastcall work_dominates_block(WorkItem *candidate, WorkItem *block);
/* Original [0005f3a0,0005f3fe). A movable store must dominate each
   block which can leave the region through a later-numbered successor. */
int __fastcall work_dominates_region_exits(WorkItem *work)
{
    WorkItem *block;
    FlowLink *successor;
    for (block = hoist_first_work; block != hoist_after_work; block = block->next)
        for (successor = block->successors; successor; successor = successor->next)
            if (successor->work->number > hoist_last_work->number &&
                !work_dominates_block(work, block)) return 0;
    return 1;
}

/* Original [0005f320,0005f32e). Distinct dominance-tail adapter. */
int __fastcall work_dominates_block(WorkItem *candidate, WorkItem *block)
{
    return bitset_contains(block->analysis.dominators, candidate->number);
}

int __fastcall work_dominates_target_predecessors(WorkItem *work, WorkItem *target);
/* Original [0005f330,0005f33b). Distinct adapter for region backedges. */
int __fastcall work_dominates_region_backedges(WorkItem *work)
{
    return work_dominates_target_predecessors(work, hoist_first_work);
}

/* Original [0005f340,0005f398). Check every in-region source of an
   incoming edge to the chosen target, including each loop backedge. */
int __fastcall work_dominates_target_predecessors(WorkItem *work, WorkItem *target)
{
    WorkItem *block;
    FlowLink *successor;
    for (block = hoist_first_work; block != hoist_after_work; block = block->next) {
        for (successor = block->successors; successor; successor = successor->next)
            if (successor->work == target) break;
        if (successor && !work_dominates_block(work, block)) return 0;
    }
    return 1;
}

/* Original [0005f800,0005f910). Classify invariant subtrees after their
   children, rejecting volatile or loop-clobbered loads and remembering all
   reads. Original shape0100 maps exactly to semantic NODE_VALUE_EXPRESSION. */
int __fastcall classify_region_invariants(Node *node)
{
    unsigned shape = node_shapes[node->opcode];
    int left, right = 1, eligible;
    RegionValue *candidate;
    if ((shape & NODE_ARITY) == NODE_LEAF) return 1;
    left = classify_region_invariants(node->left.node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        right = classify_region_invariants(node->second.node);
    shape = node_shapes[node->opcode];
    eligible = (shape & NODE_VALUE_EXPRESSION) != 0 || node->opcode == 0x32 || node->opcode == 0x55;
    if (node->opcode == 0x30) {
        if (node->attributes.words.flags & 0x800) eligible = 0;
        else {
            for (candidate = hoist_current_region->candidates; candidate; candidate = candidate->next)
                if (work_effect_invalidates_value(candidate->tree, node)) break;
            if (candidate) eligible = 0;
        }
        hoist_seen_memory_values = bitset_include(hoist_seen_memory_values,
            (short)node->detail.node->info.words.identifier);
    }
    if (eligible & left & right) {
        if (node_shapes[node->opcode] & NODE_VALUE_EXPRESSION) {
            hoist_invariant_values = bitset_include(hoist_invariant_values,
                (short)node->info.words.identifier);
            node->attributes.words.flags |= 0x10;
        }
        return 1;
    }
    node->attributes.words.flags &= ~0x10;
    return 0;
}

/* Original0007bc2c: head of arena-4 basic/derived induction records. */
static struct BasicInduction *basic_region_inductions;
/* Original [0005f070,0005f1be). Recognize canonical store updates by an
   invariant increment, excluding nested regions and restricted storage. */
void collect_basic_region_inductions(void)
{
    WorkItem *work;
    Expression *expression;
    Node *node, *address, *value;
    TemporarySymbol *symbol;
    unsigned attributes;
    basic_region_inductions = 0;
    for (work = hoist_first_work; work != hoist_after_work; work = work->next) {
        for (expression = work->body.expressions; expression; expression = expression->next) {
            node = expression->tree;
            if (node->opcode != 0x32) continue;
            symbol = (TemporarySymbol *)node->right.symbol;
            if (symbol && symbol->symbol.kind != 0x13) {
                attributes = symbol->attributes;
                if ((attributes & 0x2000) || (attributes & 15) == 6) continue;
                if (!(node->attributes.words.flags & 1) && (attributes & 0x4000)) continue;
            }
            if ((node->attributes.words.flags & 0x800) ||
                !work_is_outside_child_regions(work) || !can_hoist_region_store(node) ||
                !is_region_invariant_value(node->left.node) ||
                work->exception_scope != hoist_first_work->exception_scope) continue;
            address = node->left.node;
            value = node->second.node;
            if ((value->opcode == 2 || value->opcode == 3) &&
                matches_assignment_load(address, value->left.node) &&
                is_region_invariant_value(value->second.node))
                new_basic_induction(node, value->second.node, work, 0, 0, 0);
            if (value->opcode == 2 && matches_assignment_load(address, value->second.node) &&
                is_region_invariant_value(value->left.node))
                new_basic_induction(node, value->left.node, work, 0, 0, 0);
        }
    }
}

/* Original [0005f2e0,0005f313). Signed block ranges of immediate nested
   regions exclude their bodies from the enclosing basic-induction scan. */
int __fastcall work_is_outside_child_regions(WorkItem *work)
{
    FlowRegion *child;
    for (child = hoist_current_region->children; child; child = child->next)
        if (child->header->number <= work->number && child->last->number >= work->number)
            return 0;
    return 1;
}

/* Original [0005f7c0,0005f7fc). The store's own canonical candidate is
   permitted; a second candidate invalidating the same value prevents motion. */
int __fastcall can_hoist_region_store(Node *node)
{
    RegionValue *candidate;
    unsigned conflicts = 0;
    for (candidate = hoist_current_region->candidates; candidate; candidate = candidate->next)
        if (work_effect_invalidates_value(candidate->tree, node) && ++conflicts > 1)
            return 0;
    return 1;
}

/* Original [0005f580,0005f5b7). A numbered value is invariant when it
   is a leaf, or its expression identifier was classified invariant. */
int __fastcall is_region_invariant_value(Node *node)
{
    unsigned shape = node_shapes[node->opcode];
    short identifier = node->info.words.identifier;
    if (!(shape & NODE_VALUE_EXPRESSION) || !identifier) return 0;
    if ((shape & NODE_ARITY) == NODE_LEAF) return 1;
    return bitset_contains(hoist_invariant_values, identifier);
}
