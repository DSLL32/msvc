/* Temporary expression nodes of the assembler grammar. */
#include "asm.h"
#include <recon.h>

/* Original0x50308..0x50341. Other node fields remain untouched. */
AsmExpression *__stdcall make_asm_constant(unsigned value)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    AsmExpression *expression;
    if (pool->remaining < (int)sizeof(AsmExpression)) refill_arena(&compiler_arena,pool);
    expression = (AsmExpression *)pool->next;
    pool->remaining -= sizeof(AsmExpression);
    pool->next += sizeof(AsmExpression);
    expression->value.integer = value;
    expression->kind = 0;
    return expression;
}
