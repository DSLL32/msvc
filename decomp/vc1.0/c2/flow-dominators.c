/* Dominator sets for globally optimized work blocks. */
#include "global-work.h"
#include "bitsets.h"

/* Original [00059610,00059700). Initialize non-entry blocks to the
   universal set, intersect predecessor dominators and add each block itself
   until no set changes. The entry's sole dominator is block zero. */
void __fastcall compute_work_dominators(WorkItem *head)
{
    WorkItem *work;
    FlowLink *predecessor;
    BitSet *scratch = 0;
    int changed;
    clear_bitset(head->analysis.dominators);
    head->analysis.dominators = ensure_bitset(head->analysis.dominators, global_work_capacity);
    bitset_include(head->analysis.dominators, 0);
    for (work = head->next; work; work = work->next) {
        work->analysis.dominators = ensure_bitset(work->analysis.dominators, global_work_capacity);
        fill_bitset(work->analysis.dominators, global_work_capacity);
    }
    do {
        changed = 0;
        for (work = head->next; work; work = work->next) {
            scratch = ensure_bitset(scratch, global_work_capacity);
            fill_bitset(scratch, global_work_capacity);
            for (predecessor = work->predecessors; predecessor; predecessor = predecessor->next)
                intersect_bitset(scratch, predecessor->work->analysis.dominators);
            bitset_include(scratch, (int)work->number);
            if (!equal_bitsets(scratch, work->analysis.dominators)) {
                assign_bitset(work->analysis.dominators, scratch);
                changed = 1;
            }
        }
    } while (changed);
    release_bitset(scratch);
}
