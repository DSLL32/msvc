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

/* Original0x5004f..0x5010e. Bracketed register multiplication selects an
   x86 index scale; ordinary constant multiplication uses the evaluator. */
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

/* Original0x50019..0x5004e. Valid scales1/2/4/8 map to two-bit exponents.
   Error recovery retains the original shift count for invalid values. */
unsigned __stdcall asm_scale_encoding(unsigned scale)
{
    unsigned encoding = 0;
    if (scale > 8 || (scale & (scale - 1))) report_syntax_error(423,scale);
    for (scale >>= 1; scale; scale >>= 1) ++encoding;
    return encoding;
}

/* Original0x5010f..0x5025f. Addition combines displacement contributions
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
            report_syntax_error(425,asm_operator_names[(short)operation],context);
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
    /* The grammar calls this helper only for these four operators. The
       original leaves its return slot unspecified for other operator IDs. */
    if (operation != 22 && operation != 28 && operation != 35) return result;
    if (left->kind == 3 || right->kind == 3 || (left->kind == 2 && right->kind == 2)) {
        context = asm_error_context();
        report_syntax_error(424,asm_operator_names[(short)operation],context);
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

/* Diagnostic vocabulary indexed by assembler operator IDs, original0x730a0.
   These are logical token names, shared by the expression error paths. */
char *asm_operator_names[36] = {
    "no token", "EQ", "GE", "GT", "LE", "LT", "NE", "AND", "MOD",
    "NOT", "OR", "SHL", "SHR", "XOR", "HIGH", "LENGTH", "LOW",
    "OFFSET", "PTR", "SEG", "SIZE", "TYPE", "+", "-", "*", "/",
    ":", ",", "[", "]", "(", ")", "{", ";", "newline", "."
};

/* Original0x4fdfc..0x4ffb4; switch destinations0x4ffb5..0x50018.
   MASM comparisons return0xffff. Arithmetic uses signed division/shifts
   and the original32-bit integer representation. */
AsmExpression *__stdcall evaluate_asm_binary(unsigned short operation,
    AsmExpression *left, AsmExpression *right)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    AsmExpression *result;
    unsigned a, b;
    char *context;
    if (left->kind != 0 || right->kind != 0) {
        context = asm_error_context();
        report_syntax_error(425,asm_operator_names[(short)operation],context);
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

enum AsmUnaryOperation {
    ASM_UNARY_NOT = 9, ASM_UNARY_HIGH = 14, ASM_UNARY_LOW = 16,
    ASM_UNARY_PLUS = 22, ASM_UNARY_MINUS = 23
};

/* Original 0x50260..0x50308, RET8. Constant-only assembler unary
   operators preserve all untouched expression fields and wrap negation. */
AsmExpression *__stdcall evaluate_asm_unary(short operation, AsmExpression *value)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    AsmExpression *result;
    char *context;
    if (value->kind) {
        context = asm_error_context();
        report_syntax_error(425, asm_operator_names[operation], context);
    }
    if (pool->remaining < (int)sizeof(AsmExpression)) refill_arena(&compiler_arena,pool);
    result = (AsmExpression *)pool->next;
    pool->remaining -= sizeof(AsmExpression);
    pool->next += sizeof(AsmExpression);
    result->kind = 0;
    switch (operation) {
    case ASM_UNARY_NOT:
        result->value.integer = ~value->value.integer;
        break;
    case ASM_UNARY_HIGH:
        result->value.integer = (value->value.integer >> 8) & 255;
        break;
    case ASM_UNARY_LOW:
        result->value.integer = value->value.integer & 255;
        break;
    case ASM_UNARY_PLUS:
        result->value.integer = value->value.integer;
        break;
    case ASM_UNARY_MINUS:
        result->value.integer = 0U - value->value.integer;
        break;
    }
    return result;
}
