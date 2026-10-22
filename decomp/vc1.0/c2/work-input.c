#include "functions.h"
WorkItem *free_work_items; /* 00083300 */
/* Original [0004f690,0004f6e0). Arena allocations are initially clear;
   recycled records need all seventeen words cleared explicitly. */
WorkItem *new_work_item(void)
{
    WorkItem *work = free_work_items;
    unsigned *word, count;
    if (work) {
        free_work_items = work->next;
        word = (unsigned *)work;
        count = 17;
        while (count--) *word++ = 0;
    } else work = arena_alloc(5, sizeof(WorkItem));
    work->field3c = -1;
    work->field40 = -1;
    return work;
}
/* Original [0004f570,0004f690). Split expression chains at labels,
   procedure records and protected-region boundaries. */
WorkItem *__fastcall build_local_work(Expression *expression)
{
    WorkItem *head = 0, *work, **link = &head;
    Expression *next, *previous = 0;
    Node *node;
    int new_block = 1, region_depth = 0;
    while (expression) {
        next = expression->next;
        node = expression->tree;
        if (node->opcode == 0x3a) {
            new_block = 1;
            goto next_expression;
        }
        if (node->opcode == 0x46 || node->opcode == 0x48) {
            if (previous) previous->next = 0;
            work = new_work_item();
            *link = work;
            link = &work->next;
            work->expressions = expression;
            new_block = 1;
            goto next_expression;
        }
        if (node->opcode == 0x47 || node->opcode == 0x78) {
            if (region_depth) compiler_internal_error("@(#)trees.c:1.292", 0x209);
            goto split_block;
        }
        if (node->opcode == 0x29) goto split_block;
        if (node->opcode == 0x4f) {
            if (node->detail.control.code == 0x19) {
                ++region_depth;
                goto split_block;
            }
            if (node->detail.control.code == 0x1a) {
                --region_depth;
                new_block = 1;
            }
        }
        if (!new_block) goto next_expression;
split_block:
        new_block = 0;
        if (previous) previous->next = 0;
        work = new_work_item();
        *link = work;
        link = &work->next;
        if (region_depth) work->flags |= 2;
        work->expressions = expression;
next_expression:
        previous = expression;
        expression = next;
    }
    return head;
}
