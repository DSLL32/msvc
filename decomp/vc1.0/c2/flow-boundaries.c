/* Synthetic region and function boundaries in global work chains. */
#include "flow-edges.h"
#include "expression-fields.h"
extern int codeview_debug; /* 0008313c: optional CodeView boundary records */

/* Original [00059880,00059a30). Insert synthetic region-boundary
   records, move preheader blocks into the parent region and count the
   nesting depth of each block in the header-to-last interval. */
void __fastcall finish_flow_regions(FlowRegion *regions)
{
    FlowRegion *region;
    WorkItem *header, *before, *inserted, *work;
    Node *marker;
    unsigned i, count;
    for (region = regions; region; region = region->next) {
        finish_flow_regions(region->children);
        if (!(region->flags & 1)) {
            header = region->header;
            before = header->previous;
            count = region->entry == header ? 2 : 1;
            for (i = 0; i < count; ++i) {
                marker = new_node(0x4f);
                marker->detail.control.code = 1;
                marker->left.value = ~0U;
                inserted = build_work_lists("SNRnlTL", before, marker);
                inserted->region = region->parent;
                remove_flow_reference(&before->successors, header);
                connect_flow_blocks(before, inserted);
                remove_flow_reference(&header->predecessors, before);
                connect_flow_blocks(inserted, header);
                inserted->flags |= before->flags & 1;
                before = inserted;
            }
            region->entry = before->previous;
        }
        for (work = region->entry; work != region->header; work = work->next)
            if (work->region == region) work->region = region->parent;
        if (region->flags & 2) {
            marker = new_node(0x4f);
            marker->detail.control.code = 1;
            marker->left.value = ~0U;
            remove_flow_reference(&region->last->successors, region->last->next);
            remove_flow_reference(&region->last->next->predecessors, region->last);
            inserted = build_work_lists("SNRnlTL", region->last, marker);
            connect_flow_blocks(inserted->previous, inserted);
            connect_flow_blocks(inserted, inserted->next);
            inserted->flags |= inserted->next->flags & inserted->previous->flags & 1;
            inserted->region = inserted->next->region;
        }
        for (work = region->header;; work = work->next) {
            ++work->region_depth;
            if (work == region->last) break;
        }
    }
}

/* Original [00059720,00059880). Restore function begin/end control
   records after global flow edits, splitting the exit block when necessary. */
void __fastcall insert_function_boundaries(WorkItem *head)
{
    enum FunctionBoundaryCode {
        BOUNDARY_BEGIN = 0xf, BOUNDARY_END = 0x10,
        BOUNDARY_DEBUG_BEGIN = 0x11, BOUNDARY_DEBUG_END = 0x12
    };
    Node *begin, *end, *debug;
    WorkItem *work;
    Expression *expression, *first, **link;
    begin = new_node(0x4f);
    begin->detail.control.code = BOUNDARY_BEGIN;
    begin->left.value = ~0U;
    end = new_node(0x4f);
    end->detail.control.code = BOUNDARY_END;
    /* The native fallback restarts at head->next. Preserve that cursor
       behavior rather than changing the search for malformed entry chains. */
    for (work = head; work; work = head->next) {
        for (expression = work->body.expressions; expression; expression = expression->next)
            if (expression->tree->opcode == 0x46) break;
        if (!expression) continue;
        if (codeview_debug) {
            debug = new_node(0x4f);
            debug->detail.control.code = BOUNDARY_DEBUG_BEGIN;
            expression = build_work_lists("snrl", expression, debug);
        }
        build_work_lists("snl", expression, begin);
        break;
    }
    work = head;
    while (work->next) work = work->next;
    global_exit_work = work;
    link = &work->body.expressions;
    first = *link;
    if (!first) return;
    while ((*link)->tree->opcode != 0x47) {
        link = &(*link)->next;
        if (!*link) return;
    }
    if (link == &work->body.expressions ||
        (link == &first->next && first->tree->opcode == 0x29)) {
        *link = build_work_lists("nrlh", end, *link);
    } else {
        global_exit_work = build_work_lists("SnlhNRTL", work, end, *link);
        connect_flow_blocks(global_exit_work->previous, global_exit_work);
        *link = 0;
    }
    if (codeview_debug) {
        debug = new_node(0x4f);
        debug->detail.control.code = BOUNDARY_DEBUG_END;
        build_work_lists("snrl", global_exit_work->body.expressions, debug);
    }
}
