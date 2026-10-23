/* Integral constant evaluation, with the original width/overflow diagnostics. */
#include "tree-rewrite.h"
#include "tree-input.h"
#include "expression-fields.h"
#include "algebra.h"
#include "records.h"
#include "work-optimizer.h"
#include <recon.h>
int __cdecl addition_overflows(unsigned left, unsigned right, int is_unsigned, unsigned bytes);
int __cdecl subtraction_overflows(unsigned left, unsigned right, int is_unsigned, unsigned bytes);

/* Original [000572f0,000576a8), including13 opcode-table entries.
   Use unsigned storage for modulo32 arithmetic and explicit signed operations
   only where the original chooses IDIV/SAR or signed ordering. */
Node *__fastcall fold_integer_node(Node *node)
{
    unsigned arity = node_shapes[node->opcode] & NODE_ARITY;
    unsigned inherited = node->attributes.words.type & 0xc00;
    unsigned left = 0, right = 0, value = 0, normalized;
    unsigned short flags = 0;
    int is_unsigned = !(node->attributes.words.type & 0x100);
    int quiet;
    Node *constant;
    if (arity == NODE_BINARY) {
        right = node->second.node->left.value;
        inherited |= node->second.node->attributes.words.flags & 0x4000;
    }
    if (arity == NODE_BINARY || arity == NODE_UNARY) {
        left = node->left.node->left.value;
        inherited |= node->left.node->attributes.words.flags & 0x4000;
    }
    quiet = work_optimization_active || inherited;
    switch (node->opcode) {
    case 2:
        if (!quiet && addition_overflows(left, right, is_unsigned,
                                        node->attributes.words.type & 255))
            compiler_warning(2, 0x2f4);
        value = left + right;
        break;
    case 3:
        if (!quiet && subtraction_overflows(left, right, is_unsigned,
                                           node->attributes.words.type & 255))
            compiler_warning(2, 0x2f4);
        value = left - right;
        break;
    case 4:
        value = normalize_integer_width(node, left * right);
        if (!quiet && left) {
            if (is_unsigned) normalized = value / left;
            else normalized = (unsigned)((int)value / (int)left);
            if (normalized != right) compiler_warning(2, 0x2f4);
        }
        break;
    case 5:
        if (!right) {
            compiler_warning(3, 0x2d3);
            value = 0;
        } else if (!is_unsigned && right == ~0U && left == 0x80000000U)
            value = left;
        else if (is_unsigned) value = left / right;
        else value = (unsigned)((int)left / (int)right);
        break;
    case 6:
        if (!right) {
            compiler_warning(3, 0x2d4);
            value = 0;
        } else if (is_unsigned) value = left % right;
        else value = (unsigned)((int)left % (int)right);
        break;
    case 7:
        if ((int)right < 0 || (!right && !left)) return node;
        value = 1;
        while (right) {
            if (right & 1) value *= left;
            left *= left;
            right >>= 1;
        }
        break;
    case 8:
        value = 0 - left;
        if (is_unsigned) flags = 0x4000;
        break;
    case 9: value = left << (right & 31); break;
    case 0xa:
        if (is_unsigned) value = left >> (right & 31);
        else value = (unsigned)((int)left >> (right & 31));
        break;
    case 0xb: value = left & right; break;
    case 0xc: value = left | right; break;
    case 0xd: value = left ^ right; break;
    case 0xe: value = ~left; break;
    case 0x34: return fold_constant_conversion(node);
    case 0x84:
        value = left;
        if ((node->left.node->attributes.words.type & 0x100) && (int)left < 0)
            value = 0 - left;
        break;
    case 0xae:
        if (is_unsigned) value = right > left ? right : left;
        else value = (int)right > (int)left ? right : left;
        break;
    case 0xaf:
        if (is_unsigned) value = right < left ? right : left;
        else value = (int)right < (int)left ? right : left;
        break;
    default:
        compiler_internal_error("@(#)confold.c:1.144", 0xfe);
        break;
    }
    if (inherited) flags |= 0x4000;
    value = normalize_integer_width(node, value);
    constant = intern_leaf(0x33, flags, node->attributes.words.type, value);
    release_tree(node);
    return constant;
}

/* Original [000719fe,00071a3a). Match byte/word/dword ADD flags using
   unsigned arithmetic, so the checker itself never overflows signed C. */
int __cdecl addition_overflows(unsigned left, unsigned right, int is_unsigned, unsigned bytes)
{
    unsigned mask, sign, sum;
    if (bytes == 2) { mask = 0xffff; sign = 0x8000; }
    else if ((int)bytes < 2) { mask = 0xff; sign = 0x80; }
    else { mask = ~0U; sign = 0x80000000U; }
    left &= mask;
    right &= mask;
    sum = left + right;
    if (is_unsigned) return mask == ~0U ? sum < left : sum > mask;
    return (~(left ^ right) & (left ^ sum) & sign) != 0;
}

extern int default_integer_bytes;
/* Original [00057b00,00057bdc), including the six relation targets at
   57bc4..57bdc. Unsigned four-byte/address comparisons use signed ordering,
   exchanging the relation when the two sign bits differ. */
Node *__fastcall fold_integer_comparison(Node *node)
{
    unsigned left = node->left.node->left.value;
    unsigned right = node->second.node->left.value;
    unsigned operation = node->opcode;
    unsigned short type = node->attributes.words.type;
    unsigned value = left;
    Node *constant;
    if ((type == 0x204 || ((type & 0xc00) && (type & 255) == 4)) &&
        (int)(left ^ right) < 0)
        operation = map_comparison_opcode(operation, 0);
    switch (operation) {
    case 0x1f: value = left == right; break;
    case 0x20: value = left != right; break;
    case 0x21: value = (int)left <= (int)right; break;
    case 0x22: value = (int)left < (int)right; break;
    case 0x23: value = (int)left >= (int)right; break;
    case 0x24: value = (int)left > (int)right; break;
    }
    constant = intern_leaf(0x33, 0, default_integer_bytes | 0x100, value);
    release_tree(node);
    return constant;
}

/* Original [00071a3a,00071a76). Signed subtraction tests the byte,
   word or paired-word overflow flag. The original unsigned path ignores
   borrow and always returns zero; retain that behavior deliberately. */
int __cdecl subtraction_overflows(unsigned left, unsigned right, int is_unsigned, unsigned bytes)
{
    unsigned mask, sign, difference;
    if (bytes == 2) { mask = 0xffff; sign = 0x8000; }
    else if ((int)bytes < 2) { mask = 0xff; sign = 0x80; }
    else { mask = ~0U; sign = 0x80000000U; }
    left &= mask;
    right &= mask;
    difference = left - right;
    if (is_unsigned) return 0;
    return ((left ^ right) & (left ^ difference) & sign) != 0;
}
