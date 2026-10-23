/* Deep expression copies and their arena-specific owned payloads. */
#include "expression-preparation.h"
#include "floating.h"
#include <recon.h>

unsigned char *__stdcall copy_bytes_forward(unsigned char *destination,
    const unsigned char *source, unsigned bytes);
Extended *__stdcall save_extended_in_arena(Extended value, int arena);
void __stdcall collect_member_references(MemberReferenceGroup **groups, IrToken *expression);
ExpressionReference *__stdcall clone_expression_references(ExpressionReference *references, int arena);

/* Original 0x5cfe0..0x5d250. Mode six follows current expression
   allocation flags and retains the original type descriptor. Other modes
   intern the copied type/shape before recursively replacing owned operands.
   Keep all arena choices inline, preserving the original function graph. */
IrToken *__stdcall clone_expression(IrToken *expression, int arena)
{
    IrToken *copy, *operand;
    ArenaPool *pool;
    CType *type;
    TypeShape *shape;
    Extended *number;
    unsigned bytes;
    int selected_arena;
    MemberReferenceGroup *groups;
    if (arena == 6) {
        if (token_allocation_flags & 0x20000020UL)
            copy = allocate_permanent(sizeof(IrToken));
        else {
            pool = &compiler_arena.pools[2];
            if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
            copy = (IrToken *)pool->next;
            pool->remaining -= sizeof(IrToken);
            pool->next += sizeof(IrToken);
        }
    } else if (!arena) copy = allocate_permanent(sizeof(IrToken));
    else {
        selected_arena = arena == -1 ? 0 : arena;
        pool = &compiler_arena.pools[selected_arena];
        if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
        copy = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    *copy = *expression;
    if (arena != 6 && copy->type) {
        type = copy->type;
        if (type->related) type->related = intern_type_shape(expression->type->related);
        copy->type = intern_type((TypeDescription *)type);
    }
    switch (expression->flags & 7) {
    case 2:
        if (expression->value.pointer) {
            operand = clone_expression(expression->value.pointer, arena);
            copy->value.pointer = operand;
            operand->linkage.list_head = copy;
        }
        if (expression->tail.right) {
            operand = clone_expression(expression->tail.right, arena);
            copy->tail.right = operand;
            operand->linkage.list_head = copy;
        }
        break;
    case 1:
        operand = clone_expression(expression->value.pointer, arena);
        copy->value.pointer = operand;
        operand->linkage.list_head = copy;
        break;
    case 3:
        if (copy->type->flags & 0x100) {
            shape = copy->type->related;
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            if (shape) return copy;
            number = copy->value.pointer;
            copy->value.pointer = save_extended_in_arena(*number, arena);
        }
        break;
    case 5:
        bytes = (expression->tail.string.length + 3U) & ~3U;
        if (!arena) copy->value.pointer = allocate_permanent(bytes);
        else {
            if (arena == -1) arena = 0;
            /* Unlike the node allocation above, this payload branch uses
               its arena index directly, including mode six. */
            pool = &compiler_arena.pools[arena];
            if (pool->remaining < (int)bytes) refill_arena(&compiler_arena, pool);
            copy->value.pointer = pool->next;
            pool->remaining -= bytes;
            pool->next += bytes;
        }
        copy_bytes_forward(copy->value.pointer, expression->value.pointer,
            expression->tail.string.length);
        break;
    case 7:
        if (expression->kind == 0xca) {
            groups = 0;
            operand = clone_expression(expression->value.pointer, arena);
            copy->value.pointer = operand;
            operand->linkage.list_head = copy;
            collect_member_references(&groups, copy->value.pointer);
            copy->tail.reference_groups = groups;
        } else if (expression->kind == 0xc8 || expression->kind == 0xc9) {
            operand = clone_expression(expression->value.pointer, arena);
            copy->value.pointer = operand;
            operand->linkage.list_head = copy;
            copy->tail.references = clone_expression_references(expression->tail.references, arena);
        }
        break;
    }
    return copy;
}
