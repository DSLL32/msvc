/* Temporary expression nodes of the assembler grammar. */
#include "asm.h"
#include <recon.h>

/* Original0x7fe39..0x7fe73, stdcall4. Other node fields remain untouched. */
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

/* Original0x7fc40..0x7fd91, stdcall12. Addition combines displacement contributions
   while the active operand owns register slots and the single symbol. */
AsmExpression *__stdcall combine_asm_addresses(unsigned short operation,
    AsmExpression *left, AsmExpression *right)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    AsmExpression *result;
    char *context;
    if (operation == 23) {
        if (right->kind != 0) {
            context = asm_error_context();
            report_syntax_error(425,asm_token_names[(short)operation],context);
            return left;
        }
        if (pool->remaining < (int)sizeof(AsmExpression)) refill_arena(&compiler_arena,pool);
        result = (AsmExpression *)pool->next;
        pool->remaining -= sizeof(AsmExpression);
        pool->next += sizeof(AsmExpression);
        result->value.integer = left->value.integer - right->value.integer;
        result->kind = left->kind;
        return result;
    }
    if (operation != 22 && operation != 28 && operation != 35) {
#ifdef RECON_NATIVE_TEST
        /* Original7fd84 reads unwritten entryESP-4 after SUB4/three saves. */
        __asm { int 3 }
#endif
        return result;
    }
    if (left->kind == 3 || right->kind == 3 || (left->kind == 2 && right->kind == 2)) {
        context = asm_error_context();
        report_syntax_error(424,asm_token_names[(short)operation],context);
        return left;
    }
    if (pool->remaining < (int)sizeof(AsmExpression)) refill_arena(&compiler_arena,pool);
    result = (AsmExpression *)pool->next;
    pool->remaining -= sizeof(AsmExpression);
    pool->next += sizeof(AsmExpression);
    result->value.integer = left->value.integer + right->value.integer;
    result->kind = left->kind > right->kind ? left->kind : right->kind;
    return result;
}
