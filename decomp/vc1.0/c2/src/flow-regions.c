/* Natural and irreducible regions in the global work graph. */
#include "flow-edges.h"
#include "bitsets.h"
#include "expression-fields.h"
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

/* Original [0005b790,0005b7cb). Partition the predecessor suffix by
   header dominance. Exchange only work pointers; link/edge flags stay put. */
FlowLink *__fastcall partition_loop_predecessors(FlowLink *predecessors, WorkItem *header)
{
    FlowLink *backedges = predecessors, *link;
    WorkItem *swap;
    int number;
    if (!predecessors) return 0;
    number = header->number;
    for (link = predecessors; link; link = link->next) {
        if (!bitset_contains(link->work->analysis.dominators, number)) {
            swap = backedges->work;
            backedges->work = link->work;
            link->work = swap;
            backedges = backedges->next;
        }
    }
    return backedges;
}

static FlowRegion *assigning_region; /* 0007bec4: recursive region assignment. */
void __fastcall mark_flow_region(WorkItem *work);
/* Original [0005b760,0005b77a). Mark the loop backwards from its final
   block, then attach the preheader which bounds that predecessor traversal. */
void __fastcall assign_flow_region(FlowRegion *region)
{
    assigning_region = region;
    mark_flow_region(region->last);
    assigning_region->entry->region = assigning_region;
}

/* Original [0005b8c0,0005b8f0). Assign each loop predecessor once,
   stopping at the separate preheader boundary. */
void __fastcall mark_flow_region(WorkItem *work)
{
    FlowLink *link;
    if (work == assigning_region->entry || work->region == assigning_region) return;
    work->region = assigning_region;
    for (link = work->predecessors; link; link = link->next) mark_flow_region(link->work);
}

unsigned __fastcall compact_flow_region(FlowRegion *region);
/* Original [0005a300,0005a32e). Compact child regions before their
   parents and accumulate every reported graph change. */
unsigned __fastcall simplify_flow_regions(FlowRegion *regions)
{
    unsigned changed = 0;
    for (; regions; regions = regions->next) {
        changed |= simplify_flow_regions(regions->children);
        changed |= compact_flow_region(regions);
    }
    return changed;
}

int __fastcall region_is_outside(FlowRegion *candidate, FlowRegion *region);
short __fastcall next_numbered_work(WorkItem *work);
/* Original [0005b8f0,0005ba05). Move intervening blocks outside this
   region after its last block, retaining explicit branches across every
   changed adjacency and the original ordering of numbered blocks. */
unsigned __fastcall compact_flow_region(FlowRegion *region)
{
    WorkItem *cursor = region->entry, *end = region->last;
    WorkItem *first, *last, *scan, *insertion, *after;
    short number;
    unsigned changed = 0;
    while (cursor != end) {
        if (region_is_outside(cursor->next->region, region) &&
            cursor->next->body.expressions->tree->opcode != 0x3a) {
            first = last = cursor->next;
            changed = 1;
            while (region_is_outside(last->next->region, region)) last = last->next;
            if (ensure_flow_branch(last, last->next)) last = last->next;
            cursor->next = last->next;
            if (cursor->next) cursor->next->previous = cursor;
            ensure_flow_branch(cursor, first);
            scan = end->next;
            number = first->number;
            while (scan) {
                if (scan->number >= number) break;
                if (scan->number < 0 && next_numbered_work(scan->next) > first->number) break;
                scan = scan->next;
            }
            insertion = scan ? scan->previous : end;
            after = insertion->next;
            last->next = after;
            if (after) after->previous = last;
            insertion->next = first;
            first->previous = insertion;
            ensure_flow_branch(insertion, after);
        }
        cursor = cursor->next;
    }
    return changed;
}

/* Original [0005ba30,0005ba48). A block in a descendant region belongs
   to this region too; an absent region is outside every enclosing loop. */
int __fastcall region_is_outside(FlowRegion *candidate, FlowRegion *region)
{
    for (; candidate; candidate = candidate->parent)
        if (candidate == region) return 0;
    return 1;
}

/* Original [0005a140,0005a2fb). Extend an enclosing region past its
   children, inserting a labeled continuation when its old terminal branch
   must still lead back to the enclosing header. Return the furthest block. */
WorkItem *__fastcall reduce_flow_regions(FlowRegion *regions)
{
    WorkItem *furthest = regions->last, *last, *old_last;
    Expression *expression, *header_expression;
    Node *branch, *label, *jump, *target;
    SymbolHeader *old_label;
    short number;
    for (; regions; regions = regions->next) {
        if (regions->children) {
            last = reduce_flow_regions(regions->children);
            if (regions->last->number < last->number) {
                old_last = regions->last;
                expression = old_last->body.expressions;
                while (expression) {
                    branch = expression->tree;
                    if (branch->opcode == 0x38 || branch->opcode == 0x3a ||
                        branch->opcode == 0x3c) break;
                    expression = expression->next;
                }
                if (!expression) regions->last = last;
                else {
                    label = new_flow_label_node();
                    jump = new_node(0x3a);
                    if (branch->opcode == 0x3c) {
                        for (header_expression = regions->header->body.expressions;
                             header_expression; header_expression = header_expression->next) {
                            if (header_expression->tree->opcode == 0x29) {
                                jump->right.symbol = header_expression->tree->right.symbol;
                                break;
                            }
                        }
                        old_label = jump->right.symbol;
                        if (branch->right.symbol == old_label) branch->right.symbol = label->right.symbol;
                        for (target = branch->second.node; target->opcode != 0x4c;
                             target = target->second.node)
                            if (target->right.symbol == old_label) target->right.symbol = label->right.symbol;
                    } else {
                        jump->right.symbol = branch->right.symbol;
                        branch->right.symbol = label->right.symbol;
                    }
                    number = last->number;
                    if (ensure_flow_branch(last, last->next)) last = last->next;
                    last = build_work_lists("SNRnlTLnl", last, label, jump);
                    last->region = regions;
                    last->number = number;
                    regions->last = last;
                    remove_flow_reference(&old_last->successors, regions->header);
                    remove_flow_reference(&regions->header->predecessors, old_last);
                    connect_flow_blocks(old_last, last);
                    connect_flow_blocks(last, regions->header);
                    last->flags |= old_last->flags & 1;
                    global_flow_changed = 1;
                }
            } else last = regions->last;
        } else last = regions->last;
        if (last->number > furthest->number) furthest = last;
    }
    return furthest;
}

/* Original [0005ba50,0005ba6b). Membership includes enclosing regions. */
int __fastcall work_in_flow_region(WorkItem *work, FlowRegion *region)
{
    FlowRegion *member;
    for (member = work->region; member; member = member->parent)
        if (member == region) return 1;
    return 0;
}

/* Original [0005b740,0005b753). Extend enclosing regions that share
   the same old final block with the region being updated. */
void __fastcall replace_flow_region_last(FlowRegion *region, WorkItem *replacement)
{
    WorkItem *old = region->last;
    do {
        region->last = replacement;
        region = region->parent;
    } while (region && region->last == old);
}

/* Original [0005a330,0005a354). The historical provisional name refers
   to refreshing each region's last block. Number comparisons are signed
   sixteen-bit comparisons; ties retain the existing last-block identity. */
void __fastcall refresh_region_entries(WorkItem *work)
{
    for (; work; work = work->next) {
        if (work->region && work->region->last->number < work->number)
            work->region->last = work;
    }
}

/* Original5ba10..5ba2f (31 bytes), ECX work chain/AX result/RET0.
   Skip newly inserted, negatively numbered work owners. A zero numbered
   owner is a valid result distinct from running off the chain only by identity.
   Dormant App helper: typed native unit qualification, no CLI reach claim. */
short __fastcall next_numbered_work(WorkItem *work)
{
    while (work && work->number < 0) work = work->next;
    return work ? work->number : 0;
}
