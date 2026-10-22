/* Natural and irreducible regions in the global work graph. */
#include "flow-edges.h"
#include "bitsets.h"
#include <recon.h>
static WorkItem *region_candidate; /* 0007becc */
static WorkItem *irreducible_last; /* 0007bec8 */

/* Original [0005a360,0005a540). Discover irreducible ranges and natural
   loops from predecessor dominators. Natural loops receive an explicit
   preheader and a separated chain of back-edge predecessors. */
void __fastcall find_flow_regions(WorkItem *head)
{
    WorkItem *work, *last, *preheader, *member;
    FlowRegion *tail, *region, *parent, **children;
    FlowLink *predecessor, *backedges;
    Node *label, *old_label;
    tail = global_flow_regions = new_flow_region();
    for (work = head; work; work = work->next) work->region = 0;
    for (work = head; work; work = work->next) {
        last = find_irreducible_flow_end(work);
        if (last && last->number > work->number) {
            region = new_flow_region();
            region->flags |= 1;
            region->entry = region->header = work;
            region->last = last;
        } else {
            for (predecessor = work->predecessors; predecessor;
                 predecessor = predecessor->next)
                if (bitset_contains(predecessor->work->analysis.dominators,
                                    (int)work->number)) break;
            if (!predecessor) continue;
            backedges = partition_loop_predecessors(predecessor, work);
            label = new_flow_label_node();
            ((LabelSymbol *)label->right.symbol)->references = 0;
            preheader = build_work_lists("SNRnlTL", work->previous, label);
            old_label = work->body.expressions->tree;
            preheader->predecessors = work->predecessors;
            for (predecessor = work->predecessors; predecessor;
                 predecessor = predecessor->next) {
                if (predecessor->next == backedges) predecessor->next = 0;
                remove_flow_reference(&predecessor->work->successors, work);
                add_flow_successor(predecessor->work, preheader);
                redirect_flow_label(predecessor->work, old_label->right.symbol,
                                    label->right.symbol);
            }
            work->predecessors = backedges;
            connect_flow_blocks(preheader, work);
            preheader->flags |= work->flags & 1;
            region = new_flow_region();
            region->entry = preheader;
            region->header = work;
            region->last = backedges->work;
            for (predecessor = backedges; predecessor; predecessor = predecessor->next)
                if (region->last->number < predecessor->work->number)
                    region->last = predecessor->work;
        }
        parent = work->region;
        if (!parent) {
            tail->next = region;
            tail = region;
        } else {
            children = &parent->children;
            while (*children) children = &(*children)->next;
            *children = region;
            region->parent = parent;
        }
        if (region->flags & 1) {
            for (member = work;; member = member->next) {
                member->region = region;
                if (member == region->last) break;
            }
        } else assign_flow_region(region);
    }
    global_flow_regions = global_flow_regions->next;
}

/* Original [0005b780,0005b790). The allocator reserves 44 bytes, including
   analysis fields beyond the region prefix encountered so far. Arena two
   supplies zeroed storage. Keep the allocation extent independent of the
   current prefix's sizeof until those remaining fields are reconstructed. */
FlowRegion *new_flow_region(void)
{
    enum { REGION_ALLOCATION_BYTES = 44 };
    return arena_alloc(2, REGION_ALLOCATION_BYTES);
}

/* Original [0005b7d0,0005b800). Search from this candidate header and
   then clear the temporary reachability marks before the next candidate. */
WorkItem *__fastcall find_irreducible_flow_end(WorkItem *work)
{
    region_candidate = work;
    irreducible_last = 0;
    scan_irreducible_predecessors(work);
    clear_flow_visit_marks(work);
    return irreducible_last;
}

/* Original [0005b800,0005b890). An ordinary back-edge predecessor beyond
   the candidate which is not dominated by it makes an irreducible range.
   Record the furthest such predecessor while traversing successors once. */
void __fastcall scan_irreducible_predecessors(WorkItem *work)
{
    FlowLink *link;
    if (work->flags & 0x10) return;
    work->flags |= 0x10;
    for (link = region_candidate->predecessors; link; link = link->next)
        if (!(link->flags & 1) && link->work == work) break;
    if (link && work->number > region_candidate->number &&
        !bitset_contains(work->analysis.dominators, (int)region_candidate->number)) {
        if (!irreducible_last || irreducible_last->number < work->number)
            irreducible_last = work;
    }
    for (link = work->successors; link; link = link->next)
        scan_irreducible_predecessors(link->work);
}

/* Original [0005b890,0005b8c0). Clear only blocks marked by the preceding
   graph walk; removing the bit before recursion also handles cycles. */
void __fastcall clear_flow_visit_marks(WorkItem *work)
{
    FlowLink *link;
    if (!(work->flags & 0x10)) return;
    work->flags &= ~0x10;
    for (link = work->successors; link; link = link->next)
        clear_flow_visit_marks(link->work);
}
