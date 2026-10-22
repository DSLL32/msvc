/* Repeated tree normalization for the global optimization pipeline. */
#include "tree-rewrite.h"
#include "tree-input.h"
#include "records.h"
#include "options.h"
#include "expression-fields.h"
#include "algebra.h"
extern int floating_frontend, optimize_precision;
Node *__fastcall prepare_global_tree(Node *node);
Node *__fastcall factor_global_tree(Node *node);
Node *__fastcall finish_global_tree(Node *node);
Node *__fastcall prepare_global_node(Node *node);
Node *__fastcall factor_global_node(Node *node);
Node *__fastcall reduce_global_node(Node *node);
Node *__fastcall finish_global_node(Node *node);
Node *__fastcall narrow_binary_conversion(Node *node, unsigned short type);
Node *__fastcall push_unary_below_conversion(Node *node);
Node *__fastcall lift_both_unary(Node *node);
Node *__fastcall negate_tree(Node *node);
Node *__fastcall fold_bitwise_and(Node *node);
int __fastcall extract_reciprocal(Node **node);
int __fastcall extract_complement(Node **node);
Node *__fastcall reassociate_global_node(Node *node);
Node *__fastcall simplify_reciprocal(Node *node);
Node *__fastcall simplify_global_conversion(Node *node);
Node *__fastcall distribute_unary_tree(Node *node);
int __fastcall is_negatable_negative_constant(Node *node);
void __fastcall reduce_additive_chain(Node *node);
void __fastcall reduce_multiplicative_chain(Node *node);
void __fastcall reduce_bitwise_terms(Node *node, unsigned identity);
void __fastcall factor_bitwise_chain(Node *node, unsigned opcode);
Node *__fastcall fold_address_offset(Node *node);
void __fastcall combine_complementary_masks(Node *node);
void __fastcall cancel_unary_factors(Node *node, unsigned unary, unsigned combine);

/* Original [00053290,000533a0). Three successive fixed-point passes have
   separate iteration limits and different amounts of local re-folding. */
Node *__fastcall optimize_tree_global(Node *node)
{
    int iterations;
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF || node->opcode == 0x3c)
        return node;
    iterations = 0;
    do {
        if (++iterations > 100) compiler_report(1, "d:\\usr\\cuda\\src\\P2\\optimize.c", 0x6a);
        tree_changed = 0;
        order_tree_operands(node, 0);
        node = fold_local_tree(node);
        node = prepare_global_tree(node);
    } while (tree_changed);
    iterations = 0;
    for (;;) {
        node = factor_global_tree(node);
        if (!tree_changed) break;
        if (++iterations > 100) compiler_report(1, "d:\\usr\\cuda\\src\\P2\\optimize.c", 0x7a);
        tree_changed = 0;
        order_tree_operands(node, 0);
        node = fold_local_tree(node);
    }
    iterations = 0;
    for (;;) {
        node = finish_global_tree(node);
        if (!tree_changed) break;
        if (++iterations > 100) compiler_report(1, "d:\\usr\\cuda\\src\\P2\\optimize.c", 0x88);
        tree_changed = 0;
        order_tree_operands(node, 0);
    }
    return node;
}

/* Original [000534a0,00053500). Prepare the parent before visiting its
   resulting operands, then reassociate its arithmetic form. */
Node *__fastcall prepare_global_tree(Node *node)
{
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return node;
    node = prepare_global_node(node);
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF)
        node->left.node = prepare_global_tree(node->left.node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        node->second.node = prepare_global_tree(node->second.node);
    return reassociate_global_node(node);
}

/* Original [000535b0,00053a80), including the 35-entry opcode table.
   Normalize subtraction, reciprocal multiplication and comparison operands
   before the recursive global passes visit their resulting children. */
Node *__fastcall prepare_global_node(Node *node)
{
    Node *left = node->left.node, *right = node->second.node, *constant;
    unsigned short type;
    switch (node->opcode) {
    case 3:
        tree_changed = 1;
        if ((node->attributes.words.type & 0x300) && (left->attributes.words.type & 0xc00))
            build_tree("PPNYNTOL", left, right, 8, 2, node, 0x34, node);
        else build_tree("PPNYOL", left, right, 8, 2, node);
        return node;
    case 4:
        if (right->opcode == 0x33 && is_negatable_negative_constant(right)) {
            tree_changed = 1;
            return build_tree("PPNYFLNY", left, right, 8, node, 8);
        }
        if (left->opcode == 2 && !(left->left.node->attributes.words.type & 0xc00))
            distribute_constant(node, 2);
        break;
    case 5:
        if (right->opcode == 0x33 && is_negatable_negative_constant(right)) {
            tree_changed = 1;
            return build_tree("PPNYFLNY", left, right, 8, node, 8);
        }
        if ((node->attributes.words.type & 0x3000) && !optimize_precision) {
            tree_changed = 1;
            build_tree("PPNYOL", left, right, 0xaa, 4, node);
        }
        break;
    case 7:
        if (right->opcode == 8 && !optimize_precision) {
            tree_changed = 1;
            return build_tree("PPLOL", left, right->left.node, node, 0xaa, right);
        }
        if ((node->attributes.words.type & 0x3000) && right->opcode == 0x33 &&
            is_negatable_negative_constant(right) && !optimize_precision) {
            tree_changed = 1;
            return build_tree("PNYPNYFL", left, 0xaa, right, 8, node);
        }
        distribute_constant(node, 4);
        break;
    case 8:
        if (node_shapes[left->opcode] & NODE_ADDITIVE) return distribute_unary_tree(node);
        break;
    case 9:
        if (left->opcode == 2 && !(left->left.node->attributes.words.type & 0xc00)) {
            distribute_constant(node, 2);
            break;
        }
        /* fall through */
    case 0xa:
        if (right->opcode == 0x33 && left->opcode == 0xe) {
            constant = typed_integer_node(left, ~0U);
            build_tree("PPOL", left->left.node, constant, 0xd, left);
        }
        if (!distribute_constant(node, 0xb) && !distribute_constant(node, 0xc))
            distribute_constant(node, 0xd);
        break;
    case 0xb:
        if (!distribute_constant(node, 0xc)) distribute_constant(node, 0xd);
        break;
    case 0xc:
        if (left->opcode == 0xb && left->second.node->opcode != 0x33)
            distribute_constant(node, 0xb);
        break;
    case 0xd:
        if (right->opcode == 0x33 && (int)right->left.value < 0) {
            tree_changed = 1;
            return build_tree("PPNYFLNY", left, right, 0xe, node, 0xe);
        }
        break;
    case 0xe:
        if (left->opcode == 0xb || left->opcode == 0xc) {
            node = distribute_unary_tree(node);
            node->opcode = 0x17 - node->opcode;
        }
        break;
    case 0x1f: case 0x20:
comparison:
        type = node->attributes.words.type;
        if (type != 0x408 && !is_constant_value(right, 0) && !(type & 0x3000)) {
            tree_changed = 1;
            constant = intern_leaf(0x33, 0, 0x101, 0);
            build_tree("PPNYPLY", left, right, 3, constant, node);
        }
        break;
    case 0x21: case 0x22: case 0x23: case 0x24:
        if (!(node->attributes.words.type & 0xf00)) goto comparison;
        /* fall through */
    case 2:
        if (left->attributes.words.type == 0x408 && right->attributes.words.type == 0x408)
            build_tree("PNtPNtLY", left, 0x34, 0x404, right, 0x34, 0x404, node);
        break;
    case 0x34:
        return simplify_global_conversion(node);
    case 0xaa:
        return simplify_reciprocal(node);
    }
    return node;
}

/* Original [00053530,000535b0). Each associative class has its own
   cancellation pass; AND/OR also simplify the opposite bitwise operator. */
Node *__fastcall reassociate_global_node(Node *node)
{
    switch (node->opcode) {
    case 2:
        reduce_additive_chain(node);
        break;
    case 4:
        reduce_multiplicative_chain(node);
        break;
    case 0xb:
        reduce_bitwise_terms(node, ~0U);
        factor_bitwise_chain(node, 0xc);
        break;
    case 0xc:
        reduce_bitwise_terms(node, 0);
        /* fall through */
    case 0xd:
        factor_bitwise_chain(node, 0xb);
        break;
    }
    return node;
}

/* Original [00053450,000534a0). Factor children before the parent, then
   apply the same per-operator reassociation used by the first pass. */
Node *__fastcall factor_global_tree(Node *node)
{
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        node->left.node = factor_global_tree(node->left.node);
        if (node_shapes[node->opcode] & NODE_HAS_SECOND)
            node->second.node = factor_global_tree(node->second.node);
        node = factor_global_node(node);
        node = reassociate_global_node(node);
    }
    return node;
}

/* Original [00054590,00054710), including its 12-entry opcode table.
   Integer and pointer arithmetic use distinct compatible-factor callbacks. */
Node *__fastcall factor_global_node(Node *node)
{
    Node *result = node;
    int (__fastcall *factor)(Node *, unsigned, int);
    switch (node->opcode) {
    case 2:
        result = fold_address_offset(node);
        if (result != node) break;
        rewrite_shift_as_multiply(node->left.node);
        rewrite_shift_as_multiply(node->second.node);
        factor = (node->attributes.words.type & 0xc00) ||
            (node->left.node->attributes.words.type & 0xc00) ?
            factor_common_operator : factor_compatible_operator;
        (*factor)(node, 4, 8);
        combine_complementary_masks(node);
        break;
    case 4:
        factor_common_operator(node, 7, 0xaa);
        cancel_unary_factors(node, 0xaa, 4);
        break;
    case 7:
        if (node->left.node->opcode == 0xaa) return lift_left_unary(node);
        break;
    case 0xb:
        factor_compatible_operator(node, 0xc, 1);
        factor_common_operator(node, 9, 1);
        factor_common_operator(node, 0xa, 1);
        break;
    case 0xc:
        factor_common_operator(node, 0xb, 1);
        factor_common_operator(node, 9, 1);
        factor_common_operator(node, 0xa, 1);
        combine_complementary_masks(node);
        break;
    case 0xd:
        factor_compatible_operator(node, 0xb, 1);
        factor_common_operator(node, 9, 1);
        factor_common_operator(node, 0xa, 1);
        combine_complementary_masks(node);
        break;
    }
    return result;
}

/* Original [000533a0,00053400). The final recursive pass folds children,
   applies the local simplifier, and then performs its two global reductions. */
Node *__fastcall finish_global_tree(Node *node)
{
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        node->left.node = finish_global_tree(node->left.node);
        if (node_shapes[node->opcode] & NODE_HAS_SECOND)
            node->second.node = finish_global_tree(node->second.node);
        node = fold_constant_node(node);
        node = simplify_local_node(node);
        node = reduce_global_node(node);
        node = finish_global_node(node);
    }
    return node;
}

/* Original [00054bc0,00054db0), including its 13-entry opcode table.
   Move a widening left conversion above an operator when the narrower
   operands preserve the operation's value and signedness. */
Node *__fastcall reduce_global_node(Node *node)
{
    Node *left, *operand, *result;
    unsigned short source, target;
    unsigned source_bytes, target_bytes;
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return node;
    left = node->left.node;
    if (left->opcode != 0x34) return node;
    operand = left->left.node;
    source = operand->attributes.words.type;
    target = node->attributes.words.type;
    if ((source & 0x200) && operand->opcode == 0x34 &&
        (operand->left.node->attributes.words.type & 255) < (source & 255))
        source = operand->left.node->attributes.words.type;
    source_bytes = source & 255;
    target_bytes = target & 255;
    switch (node->opcode) {
    case 2: case 3: case 4:
        if (source_bytes == target_bytes && left->attributes.words.type == target &&
            node->second.node->attributes.words.type == source) return lift_left_unary(node);
        if (!(left->attributes.words.type & 0xc00) && (target & 0x300) &&
            (source & 0x300) && source_bytes == target_bytes)
            return narrow_binary_conversion(node, source);
        break;
    case 5: case 6: case 0xae: case 0xaf:
        if ((target & 0x300) && source_bytes < target_bytes)
            return narrow_binary_conversion(node, source);
        break;
    case 0xa: case 0xab:
        if (source_bytes < target_bytes) return lift_left_unary(node);
        break;
    case 0xb: case 0xc: case 0xd:
        if ((source & 0x300) && source_bytes <= target_bytes)
            return narrow_binary_conversion(node, source);
        break;
    case 0xe:
        if ((target & 0x100) && source < target) return push_unary_below_conversion(node);
        break;
    case 0x21: case 0x22: case 0x23: case 0x24:
        if (source_bytes == target_bytes) break;
        /* fall through */
    case 0x1f: case 0x20:
        if ((source & 0x300) && (target & 0x300) && source_bytes <= target_bytes) {
            result = narrow_binary_conversion(node, source);
            while (result != node) result = take_left_operand(result);
        }
        break;
    case 0x30: case 0x32:
        if (target == 0x408 && (source & 0xc00)) return remove_left_wrapper(node);
        break;
    }
    return node;
}

/* Original [00054710,00054bc0), including the 13-entry opcode table.
   Complete operator-specific rewrites after local and conversion folding. */
Node *__fastcall finish_global_node(Node *node)
{
    Node *left = node->left.node, *right = node->second.node;
    Node *result, *factor;
    int exponent;
    switch (node->opcode) {
    case 2:
        result = fold_address_offset(node);
        if (result != node) return result;
        if (left->opcode == 8) {
            if (right->opcode == 8) return lift_both_unary(node);
            return negate_tree(negate_tree(node));
        }
        if (extract_negative(&node->second.node)) node->opcode = 3;
        break;
    case 3:
        return fold_address_offset(node);
    case 4:
        if (left->opcode == 0xaa) {
            if (right->opcode == 0xaa) return lift_both_unary(node);
            result = remove_left_wrapper(node);
            build_tree("PPOL", right, result->left.node, 5, result);
            return result;
        }
        if (right->opcode == 0xaa) {
            node->opcode = 5;
            return remove_right_wrapper(node);
        }
        break;
    case 7:
        if (constant_as_integer(right, &exponent) && exponent >= 0 &&
            (!is_constant_value(left, 0) || exponent != 0) &&
            (!((node->attributes.words.type & 0x1000) || floating_frontend) || exponent <= 16) &&
            (!(node->attributes.words.type & 0x1000) || !optimize_precision) && can_repeat_tree(left)) {
            factor = copy_tree(left);
            result = typed_integer_node(factor, 1);
            while (exponent) {
                if (exponent & 1) result = build_tree("DPNY", factor, result, 4);
                factor = build_tree("DPNY", factor, factor, 4);
                if (exponent > 2) {
                    factor = build_tree("PNY", factor, 0x59);
                    factor->attributes.words.flags |= 0x4000;
                }
                exponent >>= 1;
            }
            release_tree(node);
            tree_changed = 1;
            return result;
        }
        if (left->opcode == 0xaa) return lift_left_unary(node);
        break;
    case 8:
        if (negate_tree_in_place(&node->left.node)) return take_left_operand(node);
        break;
    case 0xb:
        cancel_unary_factors(node, 0xe, 0xc);
        factor_compatible_operator(node, 9, 1);
        factor_compatible_operator(node, 0xa, 1);
        return fold_bitwise_and(node);
    case 0xc:
        cancel_unary_factors(node, 0xe, 0xb);
        /* fall through */
    case 0xd:
        factor_compatible_operator(node, 9, 1);
        factor_compatible_operator(node, 0xa, 1);
        break;
    case 0xe:
        if (extract_complement(&node->left.node)) return take_left_operand(node);
        break;
    case 0x21: case 0x22: case 0x23: case 0x24:
        if (node->attributes.words.type & 0xf00) break;
        /* fall through */
    case 0x1f: case 0x20:
        if (node->attributes.words.type == 0x408 ||
            (node->attributes.words.type & 0x3000) || !is_constant_value(right, 0)) break;
        while (left->opcode == 2) {
            if (negate_tree_in_place(&left->second.node)) {
                left->opcode = 3;
                break;
            }
            left = left->left.node;
        }
        if (left->opcode == 3) {
            result = remove_right_wrapper(node);
            result->second.node = left->second.node;
            left->opcode = 0x34;
            finish_canonical_node(result);
        }
        break;
    case 0x32: case 0x34:
        finish_canonical_node(node);
        break;
    case 0xaa:
        if (extract_reciprocal(&node->left.node)) return take_left_operand(node);
        break;
    case 0xae:
        cancel_unary_factors(node, 8, 0xaf);
        break;
    case 0xaf:
        cancel_unary_factors(node, 8, 0xae);
        break;
    }
    return node;
}
