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

unsigned __stdcall asm_scale_encoding(unsigned scale);

/* Original0x7fb80..0x7fc40, stdcall12. A bracketed register times
   a constant selects an index scale; other products use the value evaluator. */
AsmExpression *__stdcall multiply_asm_expression(unsigned short operation,
    AsmExpression *left, AsmExpression *right)
{
    AsmAddress *address;
    unsigned scale;
    char *context;
    if (!asm_brackets_used || left->kind != 1 || right->kind != 0)
        return evaluate_asm_binary(operation,left,right);
    address = asm_operand->address;
    if (address->flags & 0xc0) {
        context = asm_error_context();
        report_syntax_error(430,context);
    }
    scale = asm_scale_encoding(right->value.integer);
    address->flags = (unsigned char)((scale << 6) | (address->flags & 0x3f));
    address = asm_operand->address;
    if (address->flags & 0xc0) {
        if (!address->first_register) {
            context = asm_error_context();
            report_syntax_error(431,context);
        } else if (!address->second_register) {
            address->second_register = address->first_register;
            asm_operand->address->first_register = 0;
        }
    }
    return left;
}

/* Original0x7f92d..0x7fae6;25 switch destinations0x7fae6..0x7fb4a.
   MASM comparisons return0xffff; signed division/shifts retain the target's
   32-bit representation. Untouched recovery result fields remain untouched. */
AsmExpression *__stdcall evaluate_asm_binary(unsigned short operation,
    AsmExpression *left, AsmExpression *right)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    AsmExpression *result;
    unsigned a, b;
    char *context;
    if (left->kind != 0 || right->kind != 0) {
        context = asm_error_context();
        report_syntax_error(425,asm_token_names[(short)operation],context);
        return left->kind != 0 ? right : left;
    }
    if (pool->remaining < (int)sizeof(AsmExpression)) refill_arena(&compiler_arena,pool);
    result = (AsmExpression *)pool->next;
    pool->remaining -= sizeof(AsmExpression);
    pool->next += sizeof(AsmExpression);
    result->kind = 0;
    a = left->value.integer;
    b = right->value.integer;
    switch ((short)operation) {
    case 1: result->value.integer = a == b ? 0xffff : 0; break;
    case 2: result->value.integer = (int)a >= (int)b ? 0xffff : 0; break;
    case 3: result->value.integer = (int)a > (int)b ? 0xffff : 0; break;
    case 4: result->value.integer = (int)a <= (int)b ? 0xffff : 0; break;
    case 5: result->value.integer = (int)a < (int)b ? 0xffff : 0; break;
    case 6: result->value.integer = a != b ? 0xffff : 0; break;
    case 7: result->value.integer = a & b; break;
    case 8:
        if (!b) {
            context = asm_error_context();
            report_syntax_error(419,context);
        } else result->value.integer = (int)a % (int)b;
        break;
    case 10: result->value.integer = a | b; break;
    case 11: result->value.integer = a << (b & 31); break;
    case 12: result->value.integer = (int)a >> (b & 31); break;
    case 13: result->value.integer = a ^ b; break;
    case 24: result->value.integer = a * b; break;
    case 25:
        if (!b) {
            context = asm_error_context();
            report_syntax_error(417,context);
        } else result->value.integer = (int)a / (int)b;
        break;
    }
    return result;
}
