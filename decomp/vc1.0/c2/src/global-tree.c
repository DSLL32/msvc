/* Repeated tree normalization for the global optimization pipeline. */
#include "tree-rewrite.h"
#include "tree-input.h"
#include "records.h"
#include "options.h"
#include "expression-fields.h"
#include "algebra.h"
#include "backend.h"
#include <recon.h>
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
Node *__fastcall copy_factor_exponent(Node *node);
void __fastcall reduce_bitwise_terms(Node *node, unsigned identity);
Node *__fastcall replace_bitwise_occurrences(Node *node, Node *value, unsigned identity);
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

/* Original [00055720,000559df). Search additive and multiplicative left
   spines for a common factor, allowing negation and an exact constant divisor.
   Rewrite one matching pair per call; the enclosing optimizer repeats passes. */
void __fastcall reduce_additive_chain(Node *node)
{
    Node **right_root, **right_sign, **right_factor = 0, **right_value;
    Node **left_root, **left_chain, **left_term, **left_sign;
    Node **left_factor = 0, **left_value, **saved_factor;
    Node *factor, *replacement;
    int right_divisor, left_divisor;
    while (node->opcode == 0x34) node = node->left.node;
    right_root = &node->second.node;
    right_sign = right_root;
    if ((*right_root)->opcode == 8) right_sign = &(*right_root)->left.node;
    for (;;) {
        if ((*right_sign)->opcode != 9) {
            right_factor = (*right_sign)->opcode == 4 ?
                &(*right_sign)->second.node : right_sign;
            right_value = right_factor;
            if ((*right_factor)->opcode == 7 &&
                constant_as_integer((*right_factor)->second.node, &right_divisor))
                right_value = &(*right_factor)->left.node;
            if ((*right_value)->opcode != 0x33) {
                left_root = &node->left.node;
                left_chain = left_root;
                for (;;) {
                    left_term = (*left_chain)->opcode == 2 ?
                        &(*left_chain)->second.node : left_chain;
                    left_sign = left_term;
                    if ((*left_term)->opcode == 8) left_sign = &(*left_term)->left.node;
                    for (;;) {
                        if ((*left_sign)->opcode != 9) {
                            left_factor = (*left_sign)->opcode == 4 ?
                                &(*left_sign)->second.node : left_sign;
                            left_value = left_factor;
                            if ((*left_factor)->opcode == 7 &&
                                constant_as_integer((*left_factor)->second.node, &left_divisor))
                                left_value = &(*left_factor)->left.node;
                            if (trees_equal(*left_value, *right_value, 1)) goto combine;
                        }
                        if (left_factor == left_sign) break;
                        left_sign = &(*left_sign)->left.node;
                    }
                    if (left_term == left_chain) break;
                    left_chain = &(*left_chain)->left.node;
                }
            }
        }
        if (right_factor == right_sign) return;
        right_sign = &(*right_sign)->left.node;
    }
combine:
    if (((*left_value)->attributes.words.type & 0xc00) &&
        ((left_sign != left_term) == (right_sign != right_root))) return;
    if (right_value != right_factor &&
        (left_value == left_factor || right_divisor > left_divisor)) {
        right_value = left_value;
        saved_factor = left_factor;
        left_factor = right_factor;
        right_factor = saved_factor;
    }
    factor = *right_factor;
    if (trees_equal(*left_factor, factor, 1)) {
        release_tree(*left_factor);
        replacement = typed_integer_node(factor, 1);
    } else {
        if (!(node_shapes[(*left_factor)->opcode] & NODE_HAS_SECOND)) return;
        if (right_value == right_factor) right_divisor = 1;
        else constant_as_integer((*right_factor)->second.node, &right_divisor);
        left_factor = &(*left_factor)->second.node;
        replacement = build_tree("PPNY", *left_factor,
            typed_integer_node(factor, (unsigned)right_divisor), 3);
    }
    *left_factor = replacement;
    *right_factor = typed_integer_node(factor, 1);
    build_tree("PPYPYNYPNYL", *left_root, *left_term, *right_root, 2, factor, 4, node);
    *left_term = typed_integer_node(factor, 0);
    if ((*right_sign)->opcode == 4) reduce_additive_chain((*right_root)->left.node);
    tree_changed = 1;
}

/* Original [00055ef0,00055fd6). Fold constant pointer displacements
   into symbol-address leaves, or subtract two addresses of the same symbol. */
Node *__fastcall fold_address_offset(Node *node)
{
    Node *left = node->left.node, *right = node->second.node, *value;
    unsigned displacement;
    if (right->opcode == 0x33) {
        if (left->opcode == 0x26) {
            displacement = right->left.value;
            if (node->opcode != 2) displacement = 0 - displacement;
            value = intern_leaf(0x26, left->attributes.words.flags,
                left->right.symbol, left->left.value + displacement, 0x404);
            release_tree(node);
            tree_changed = 1;
            return value;
        }
        if ((node_shapes[left->opcode] & NODE_ADDITIVE) &&
            left->second.node->opcode == 0x26) {
            node->opcode = node->opcode == left->opcode ? 2 : 3;
            build_tree("PPPLL", left->left.node, left->second.node, right, node, left);
            left->second.node = fold_address_offset(left->second.node);
            return left;
        }
    }
    if (node->opcode == 3 && left->opcode == 0x26 && right->opcode == 0x26 &&
        left->right.symbol == right->right.symbol) {
        value = integer_node(left->left.value - right->left.value, 0x104);
        release_tree(node);
        return value;
    }
    return node;
}

unsigned __fastcall combine_factor_constants(Node *operation, unsigned first, unsigned second);
Node *__fastcall operator_identity_node(Node *operation, Node *type_source);
/* Original [00055b40,00055d58). Factor compatible constant operators out
   of an associative left chain. An optional unary shell is ignored while
   matching, but remains represented in the rewritten tree. */
int __fastcall factor_common_operator(Node *node, unsigned opcode, int implicit)
{
    Node *right_root, *right_factor, *right_constant, *left_factor, *left_constant;
    Node **left_chain, **left_slot;
    unsigned right_bits, left_bits, combined;
    int right_integer, left_integer;
    while (node->opcode == 0x34) node = node->left.node;
    right_root = node->second.node;
    right_factor = right_root;
    if (right_factor->opcode == (unsigned)implicit) right_factor = right_factor->left.node;
    if (right_factor->opcode != opcode) return 0;
    right_constant = right_factor->second.node;
    if (!constant_as_integer(right_constant, &right_integer)) return 0;
    right_bits = (unsigned)right_integer;
    left_chain = &node->left.node;
    for (;;) {
        left_slot = (*left_chain)->opcode == node->opcode ?
            &(*left_chain)->second.node : left_chain;
        left_factor = *left_slot;
        if (left_factor->opcode == (unsigned)implicit) left_factor = left_factor->left.node;
        if (left_factor->opcode == opcode) {
            left_constant = left_factor->second.node;
            if (constant_as_integer(left_constant, &left_integer)) {
                left_bits = (unsigned)left_integer;
                if (can_combine_constants(right_factor, right_bits, left_bits)) {
                    build_tree("PPPNYPNYL", node->left.node, *left_slot, right_root,
                               node->opcode, left_constant, opcode, node);
                    combined = combine_factor_constants(right_factor, right_bits, left_bits);
                    right_factor->second.node = typed_integer_node(right_constant, combined);
                    left_factor->opcode = 0x34;
                    *left_slot = operator_identity_node(node, *left_slot);
                    release_node(right_constant);
                    factor_common_operator(node->second.node->left.node, opcode, implicit);
                    tree_changed = 1;
                    return 1;
                }
                if (can_combine_constants(right_factor, left_bits, right_bits)) {
                    build_tree("PPPNYPNYL", node->left.node, *left_slot, right_root,
                               node->opcode, right_constant, opcode, node);
                    combined = combine_factor_constants(right_factor, left_bits, right_bits);
                    left_factor->second.node = typed_integer_node(left_constant, combined);
                    right_factor->opcode = 0x34;
                    *left_slot = operator_identity_node(node, *left_slot);
                    release_node(left_constant);
                    factor_common_operator(node, opcode, implicit);
                    tree_changed = 1;
                    return 1;
                }
            }
        }
        if (left_slot == left_chain) return 0;
        left_chain = &(*left_chain)->left.node;
    }
}

int __fastcall adjust_factor_constant(Node *node, Node *factor, unsigned *value);
/* Original [00055d60,00055e09). Express a scalar constant through a
   matching left-side operator before applying ordinary common factoring. */
int __fastcall factor_compatible_operator(Node *node, unsigned opcode, int implicit)
{
    Node *chain, *candidate, *factor;
    unsigned adjusted;
    if (node->second.node->opcode == 0x33) {
        chain = node->left.node;
        for (;;) {
            candidate = chain->opcode == node->opcode ? chain->second.node : chain;
            factor = candidate;
            if (factor->opcode == (unsigned)implicit) factor = factor->left.node;
            if (factor->opcode == opcode && factor->second.node->opcode == 0x33 &&
                (factor->second.node->attributes.words.type & 0x300) &&
                adjust_factor_constant(node, factor, &adjusted)) {
                release_node(node->second.node);
                node->second.node = build_tree("PCNY", typed_integer_node(factor, adjusted),
                                              factor->second.node, opcode);
                break;
            }
            if (candidate == chain) break;
            chain = chain->left.node;
        }
    }
    return factor_common_operator(node, opcode, implicit);
}

Node *__fastcall push_unary_into_left(Node *node);
Node *__fastcall push_unary_into_right(Node *node);
/* Original [00054e50,00055070), including the13-entry opcode table.
   Distribute narrowing conversions where operand representations permit it;
   preserve pointer selectors and address leaves across conditional choices. */
Node *__fastcall simplify_global_conversion(Node *node)
{
    Node *operand, *inner, *choices;
    unsigned short target, source;
    for (;;) {
        operand = node->left.node;
        target = node->attributes.words.type;
        source = operand->attributes.words.type;
        switch (operand->opcode) {
        case 0x34:
            inner = operand->left.node;
            if (target == 0x404 && source == 0x408 &&
                inner->attributes.words.type == 0x808 &&
                (node_shapes[inner->opcode] & NODE_ADDITIVE)) {
                if (inner->left.node->attributes.words.type & 0xc00)
                    node->left.node = push_unary_into_left(operand);
                else node->left.node = push_unary_into_right(operand);
                continue;
            }
            break;
        case 2: case 3:
            if (target == 0x404 && source == 0x408) {
                if (operand->left.node->attributes.words.type & 0xc00)
                    return push_unary_into_left(node);
                return push_unary_into_right(node);
            }
            if ((target & 0x300) && (source & 0x300) &&
                (source & 255) >= (target & 255) &&
                (operand->left.node->attributes.words.type & 0x300))
                return distribute_unary_tree(node);
            break;
        case 4: case 7: case 0xb: case 0xc: case 0xd:
            if ((target & 0x300) && (source & 0x300) &&
                (source & 255) >= (target & 255)) return distribute_unary_tree(node);
            break;
        case 8:
            if ((source & 0xc00) && (source & 255) >= (target & 255))
                return push_unary_below_conversion(node);
            /* Fall through: ordinary integral unary conversion. */
        case 9: case 0xe:
            if ((target & 0x300) && (source & 0x300) &&
                (source & 255) >= (target & 255)) {
                if (operand->opcode == 9) return push_unary_into_left(node);
                return push_unary_below_conversion(node);
            }
            break;
        case 0x42: case 0x43:
            choices = operand->opcode == 0x42 ? operand->second.node : operand;
            if (choices->second.node->opcode == 0x26 || choices->left.node->opcode == 0x26)
                return node;
            if ((source & 255) >= (target & 255)) {
                if (operand->opcode == 0x42) return push_unary_into_right(node);
                return distribute_unary_tree(node);
            }
            break;
        case 0x68:
            if (target & 0xc00) {
                if (target == 0x404) return take_left_operand(remove_left_wrapper(node));
                operand->attributes.words.type = target;
                return take_left_operand(node);
            }
            break;
        }
        return node;
    }
}

int __fastcall constant_fits_type(Node *node, unsigned short type);
/* Original [00055240,000552c8). Narrow a binary operator beside a left
   conversion when the constant or converted right operand admits that width. */
Node *__fastcall narrow_binary_conversion(Node *node, unsigned short type)
{
    Node *right = node->second.node;
    if (right->opcode == 0x33) {
        if (node->opcode == 0xb && (node->attributes.words.type & 0x100) &&
            !constant_fits_type(right, type)) type = (unsigned short)((type & 255) | 0x200);
        if (constant_fits_type(right, type)) {
            node->second.node = convert_node_type(right, type);
            return lift_left_unary(node);
        }
    }
    if (right->opcode == 0x34) {
        right = right->left.node;
        if (right->attributes.words.type == type) return lift_both_unary(node);
        if (right->opcode == 0x34 && right->left.node->attributes.words.type == type)
            return lift_both_unary(node);
    }
    return node;
}

/* Original [00055330,0005540d). Test representability before narrowing
   integral constants, retaining their signed/unsigned32 interpretation. */
int __fastcall constant_fits_type(Node *node, unsigned short type)
{
    unsigned value = node->left.value;
    if (node->opcode != 0x33) return 0;
    switch (type) {
    case 0x101: return (unsigned)(int)(signed char)value == value;
    case 0x102: return (unsigned)(int)(short)value == value;
    case 0x104:
        return (int)value >= 0 || (node->attributes.words.type & 0x100) != 0;
    case 0x201: return (value & 255) == value;
    case 0x202: return (value & 0xffff) == value;
    case 0x204:
        return (int)value >= 0 || (node->attributes.words.type & 0x200) != 0;
    default:
        compiler_internal_error("@(#)optimize.c:1.144", 0x2e1);
        return compare_extended(1, (Extended *)((char *)node->left.pointer + 12), &extended_zero);
    }
}

int __fastcall is_subword_mask_pattern(Node *node, unsigned mask);
/* Original [00055a90,00055b40). Replace complementary masked choices
   with an XOR/AND selection, provided its repeated base can be evaluated twice. */
void __fastcall combine_complementary_masks(Node *node)
{
    Node *primary = node->left.node, *secondary = node->second.node, *swap, *base;
    unsigned mask;
    if (primary->opcode != 0xb || primary->second.node->opcode != 0x33 ||
        secondary->opcode != 0xb || secondary->second.node->opcode != 0x33) return;
    mask = primary->second.node->left.value ^ secondary->second.node->left.value;
    if (mask != normalize_integer_width(node, ~0U)) return;
    if (secondary->second.node->left.value > primary->second.node->left.value) {
        swap = primary; primary = secondary; secondary = swap;
    }
    if (!can_repeat_tree(primary) ||
        is_subword_mask_pattern(secondary, secondary->second.node->left.value)) return;
    release_node(primary->second.node);
    base = primary->left.node;
    build_tree("PPOLPLDOL", base, secondary->left.node, 0xd, primary,
               secondary->second.node, secondary, base, 0xd, node);
    tree_changed = 1;
}

/* Original [00055620,00055717). Find a repeatable factor along the left
   product spine, looking through power and reciprocal wrappers. Combine one
   matching pair by adding their exponents, then leave the next pass to fold. */
void __fastcall reduce_multiplicative_chain(Node *node)
{
    Node *right = node->second.node, *right_base = right, *left_base;
    Node *right_exponent, *left_exponent;
    Node **chain = &node->left.node, **factor;
    if (right_base->opcode == 7) right_base = right_base->left.node;
    if (right_base->opcode == 0xaa) right_base = right_base->left.node;
    for (;;) {
        factor = (*chain)->opcode == 4 ? &(*chain)->second.node : chain;
        if (node->opcode == 4 && (node->attributes.words.type & 0x1000) &&
            optimize_precision) return;
        left_base = *factor;
        if (left_base->opcode == 7) left_base = left_base->left.node;
        if (left_base->opcode == 0xaa) left_base = left_base->left.node;
        if (trees_equal(left_base, right_base, 1) && can_repeat_tree(left_base)) {
            right_exponent = copy_factor_exponent(right);
            left_exponent = copy_factor_exponent(*factor);
            build_tree("PDPPNYNYL", node->left.node, right_base, left_exponent,
                       right_exponent, 2, 7, node);
            release_tree(right);
            release_tree(*factor);
            *factor = typed_integer_node(node, 1);
            tree_changed = 1;
            return;
        }
        if (chain == factor) return;
        chain = &(*chain)->left.node;
    }
}

/* Original [00055e10,00055e8e). Combine matching unary factors across
   an associative left spine, replacing the removed wrapper by its identity. */
void __fastcall cancel_unary_factors(Node *node, unsigned unary, unsigned combine)
{
    Node *right = node->second.node, *left, *old;
    Node **chain, **factor;
    if (right->opcode != unary) return;
    chain = &node->left.node;
    for (;;) {
        left = *chain;
        factor = left->opcode == node->opcode ? &left->second.node : chain;
        if ((*factor)->opcode == unary) break;
        if (factor == chain) return;
        chain = &left->left.node;
    }
    if (operator_identity(node) == 0x2a) return;
    build_tree("PPNYL", (*factor)->left.node, right->left.node, combine, right);
    old = *factor;
    *factor = operator_identity_node(node, old);
    release_node(old);
    tree_changed = 1;
}

/* Original [00055450,000555ea). Extract a common operand from bitwise
   factor spines on either side of an associative expression. One match is
   rewritten per pass, retaining identities in the two original chains. */
void __fastcall factor_bitwise_chain(Node *node, unsigned opcode)
{
    Node **right_root, **right_chain, **right_factor;
    Node **left_root, **left_chain, **left_term, **left_factor;
    Node *right_group, *left_group, *scan, *common, *matched;
    unsigned operation, right_operation, left_operation;
    while (node->opcode == 0x34) node = node->left.node;
    operation = node->opcode;
    right_root = &node->second.node;
    right_chain = right_root;
    for (;;) {
        right_group = *right_chain;
        right_operation = right_group->opcode;
        right_factor = right_operation == opcode ? &right_group->second.node : right_chain;
        if ((*right_factor)->opcode != 0x33) {
            left_root = &node->left.node;
            left_chain = left_root;
            for (;;) {
                left_group = *left_chain;
                left_operation = left_group->opcode;
                left_term = left_operation == operation ? &left_group->second.node : left_chain;
                left_factor = left_term;
                for (;;) {
                    scan = *left_factor;
                    if (scan->opcode == opcode) left_factor = &scan->second.node;
                    if (trees_equal(*left_factor, *right_factor, 1)) goto combine;
                    if (scan->opcode != opcode) break;
                    left_factor = &scan->left.node;
                }
                if (left_operation != operation) break;
                left_chain = &left_group->left.node;
            }
        }
        if (right_operation != opcode) return;
        right_chain = &right_group->left.node;
    }
combine:
    common = *right_factor;
    matched = *left_factor;
    node->opcode = opcode;
    *left_factor = operator_identity_node(node, matched);
    release_tree(matched);
    *right_factor = operator_identity_node(node, common);
    left_term = left_operation == operation ? &left_group->second.node : left_chain;
    build_tree("PPPNYPNYOL", *left_root, *left_term, *right_root, operation,
               common, opcode, operation, node);
    *left_term = operator_identity_node(node, *left_term);
    if (right_factor != right_chain) factor_bitwise_chain((*right_root)->left.node, opcode);
    tree_changed = 1;
}

/* Original [00056e30,00056e4b). The initialized scratch node owns the
   slot used by the in-place negator, including an unchanged result on failure. */
Node *__fastcall negate_tree(Node *node)
{
    scratch_node->second.node = node;
    negate_tree_in_place(&scratch_node->second.node);
    return scratch_node->second.node;
}

/* Original [000555f0,00055619). Repeated right values in a bitwise tree
   become the operation's identity, complemented through an explicit NOT. */
void __fastcall reduce_bitwise_terms(Node *node, unsigned identity)
{
    Node *value = node->second.node;
    if (value->opcode == 0xe) {
        value = value->left.node;
        identity = ~identity;
    }
    identity = normalize_integer_width(node, identity);
    node->left.node = replace_bitwise_occurrences(node->left.node, value, identity);
}

/* Original [000562f0,0005636a). Visit only bitwise operator interiors;
   a matching terminal subtree is released and replaced by a typed identity. */
Node *__fastcall replace_bitwise_occurrences(Node *node, Node *value, unsigned identity)
{
    switch (node->opcode) {
    case 0xb: case 0xc: case 0xd:
        node->second.node = replace_bitwise_occurrences(node->second.node, value, identity);
        /* Fall through: the right subtree is visited first. */
    case 0xe:
        node->left.node = replace_bitwise_occurrences(node->left.node, value, identity);
        break;
    default:
        if (trees_equal(node, value, 1)) {
            release_tree(node);
            tree_changed = 1;
            return typed_integer_node(value, identity);
        }
        break;
    }
    return node;
}

/* Original [00056bf0,00056c50). A small constant mask permits wide AND
   operands to be narrowed to unsigned short before restoring the result type. */
Node *__fastcall fold_bitwise_and(Node *node)
{
    unsigned short type = node->attributes.words.type;
    Node *right = node->second.node, *left;
    if ((type & 255) > 4 && right->opcode == 0x33 && right->left.value <= 0xffff) {
        tree_changed = 1;
        right = convert_node_type(right, 0x202);
        left = convert_node_type(node->left.node, 0x202);
        build_tree("PPLY", left, right, node);
        return convert_node_type(node, type);
    }
    return node;
}
