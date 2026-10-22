#include "functions.h"
#include "function-input.h"
#include "bitsets.h"
#include "frame.h"
#include "optimizer-cache.h"
#include <recon.h>
extern unsigned next_leaf_identifier;
extern Node *floating_temporary_nodes;
void reset_leaf_tables(void);
WorkItem *free_work_items; /* 00083300 */
/* Original [0004fd90,0004fe10), ending with the leaf-table reset tail call.
   Clear pooled bitsets before releasing arena six, which owns their storage. */
void reset_work(void)
{
    dynamic_stack_allocation = 0;
    next_leaf_identifier = (unsigned)-1;
    lexical_depth = -1;
    procedure_depth = 0;
    free_bitset_nodes = NULL;
    floating_temporary_nodes = NULL;
    global_exit_work = 0;
    current_work = NULL;
    clear_optimizer_trees();
    reset_arena(2);
    reset_arena(1);
    reset_arena(6);
    reset_leaf_tables();
}
/* Original [0004fd00,0004fd10). */
void __fastcall free_work(WorkItem *work)
{
    work->next = free_work_items;
    free_work_items = work;
}
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
    work->number = -1;
    work->exception_scope = -1;
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
            work->body.expressions = expression;
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
        work->body.expressions = expression;
next_expression:
        previous = expression;
        expression = next;
    }
    return head;
}
