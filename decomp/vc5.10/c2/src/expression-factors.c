#include "optimizer.h"
#include "expression-order.h"
#include <recon.h>

typedef struct C2FactorEntry {
    struct C2FactorEntry *next;
    C2Expression *expression;
    unsigned count, weight;
} C2FactorEntry;
typedef char C2FactorEntrySizeCheck[sizeof(C2FactorEntry) == 16 ? 1 : -1];
typedef char C2FactorEntryExpressionCheck[offsetof(C2FactorEntry, expression) == 4 ? 1 : -1];
typedef char C2FactorEntryCountCheck[offsetof(C2FactorEntry, count) == 8 ? 1 : -1];
typedef char C2FactorEntryWeightCheck[offsetof(C2FactorEntry, weight) == 12 ? 1 : -1];
/* Original 499a20: sixteen pointer buckets. The real initializer writes all
 * sixty-four bytes before the factoring collectors consult any bucket. */
static C2FactorEntry *factor_buckets[16];

void initialize_factoring_storage(void)
{
    memset(factor_buckets, 0, sizeof(factor_buckets));
}

void __fastcall record_factor_operand(C2Expression *expression);

void __fastcall collect_factor_operands(C2Expression *expression, unsigned opcode, unsigned marker)
{
    C2Expression *operand, *value;
    for (operand = expression->left; operand; operand = operand->next) {
        value = operand;
        if (value->opcode == marker) value = value->left;
        if (value->opcode == opcode) collect_factor_operands(value, 0x11c, marker);
        else record_factor_operand(value);
    }
}

int __fastcall compare_expression_trees(C2Expression *left, C2Expression *right);

void __fastcall record_factor_operand(C2Expression *expression)
{
    unsigned key = expression->order_key;
    unsigned bucket = (key & 15) ^ ((key >> 8) & 15);
    C2FactorEntry *entry;
    for (entry = factor_buckets[bucket]; entry; entry = entry->next) {
        if (!compare_expression_trees(expression, entry->expression)) break;
    }
    if (entry) {
        ++entry->count;
    } else {
        entry = c2_pool_allocate(4, sizeof(C2FactorEntry));
        entry->next = factor_buckets[bucket];
        entry->expression = expression;
        entry->count = 1;
        entry->weight = ((key >> 16) & 255) + 1;
        factor_buckets[bucket] = entry;
    }
}

extern unsigned factoring_opcode, factoring_marker;
int __fastcall expression_can_flatten(C2Expression *expression);
/* The merge callback at409322 supplies ECX/EDX; actual441999 preserves
 * the nonvolatile registers and returns a full comparison in EAX, RET0. */
int __fastcall compare_factor_benefit(C2Expression *left, C2Expression *right);

void __fastcall arrange_factor_operands(C2Expression *expression, unsigned opcode, unsigned marker)
{
    C2Expression *operand, *value;
    for (operand = expression->left; operand; operand = operand->next) {
        value = operand;
        if (value->opcode == marker) value = value->left;
        if (value->opcode == opcode) arrange_factor_operands(value, 0x11c, marker);
    }
    factoring_marker = marker;
    factoring_opcode = opcode;
    if (expression_can_flatten(expression))
        expression->left = sort_expression_inputs(expression->left, compare_factor_benefit);
}

C2Expression *__fastcall get_factor_operand(C2Expression *expression);
C2FactorEntry *__fastcall lookup_factor_entry(C2Expression *expression);

int __fastcall compare_factor_benefit(C2Expression *left, C2Expression *right)
{
    C2Expression *a, *b;
    C2FactorEntry *first, *second;
    int left_score, right_score;
    a = get_factor_operand(left);
    b = get_factor_operand(right);
    first = lookup_factor_entry(a);
    second = lookup_factor_entry(b);
    /* Both IMULs in the original use the first entry's weight. Preserve
     * that asymmetric calculation and its signed DWORD comparison. */
    left_score = (int)((first->count - 1) * first->weight);
    right_score = (int)((second->count - 1) * first->weight);
    if (left_score > right_score) return -1;
    return left_score != right_score;
}

C2FactorEntry *__fastcall lookup_factor_entry(C2Expression *expression)
{
    unsigned key = expression->order_key;
    unsigned bucket = (key & 15) ^ ((key >> 8) & 15);
    C2FactorEntry *entry;
    for (entry = factor_buckets[bucket]; entry; entry = entry->next) {
        if (!compare_expression_trees(expression, entry->expression)) return entry;
    }
    return NULL;
}
