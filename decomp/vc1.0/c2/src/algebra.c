#include "algebra.h"
#include "tree-input.h"
#include "expression-fields.h"
#include "records.h"
#include <recon.h>
/* Original [00056230,000562f0), including its nine-entry opcode table.
   2a denotes an operator without a removable right-hand identity. */
int __fastcall operator_identity(Node *node)
{
    unsigned opcode = node->opcode;
    if (opcode <= 0xb) {
        switch (opcode) {
        case 2: case 3: case 9: case 10: return 0;
        case 4: case 5: case 7: return 1;
        case 0xb: return integer_type_limit(node, -1);
        default: return 0x2a;
        }
    }
    if (opcode == 0xc || opcode == 0xd) return 0;
    if (opcode == 0xae) {
        if (node->attributes.words.type & 0x1000) return 0x2a;
        if (node->attributes.words.type & 0x100) return integer_type_limit(node, 0x80000000);
        return 0;
    }
    if (opcode == 0xaf) {
        if (node->attributes.words.type & 0x1000) return 0x2a;
        if (node->attributes.words.type & 0x100) return integer_type_limit(node, 0x7fffffff);
        return integer_type_limit(node, -1);
    }
    return 0x2a;
}
/* Original [00053a80,00054590), including the 35-entry opcode table.
   Revisit a multiplication after converting it to a shift. Other rules
   return their replacement directly, leaving the outer tree walk in charge. */
Node *__fastcall simplify_local_node(Node *node)
{
    Node *left, *right, *value, *other, *candidate, *replacement, *alternatives;
    Node *chosen, *current;
    unsigned bits, mask, opcode;
    unsigned short type, flags;
    int identity, constant, invert;
restart:
    left = node->left.node;
    right = node->second.node;
    identity = operator_identity(node);
    if (identity != 0x2a && is_constant_value(right, identity)) {
        finish_canonical_node(node);
        release_node(right);
        node->opcode = 0x34;
        tree_changed = 1;
    }
    switch (node->opcode) {
    case 2:
        value = strip_integer_cast(left);
        if (value->opcode == 8) {
            other = strip_integer_cast(right);
            value = strip_integer_cast(left);
            value = strip_integer_cast(value->left.node);
            if (trees_equal(value, other, 2)) goto cancel_sum;
        }
        value = strip_integer_cast(right);
        if (value->opcode == 8) {
            other = strip_integer_cast(left);
            value = strip_integer_cast(right);
            value = strip_integer_cast(value->left.node);
            if (trees_equal(value, other, 2)) goto cancel_sum;
        }
        if (left->opcode == 2) {
            candidate = left->left.node;
            value = strip_integer_cast(candidate);
            if (value->opcode == 8) {
                other = strip_integer_cast(right);
                value = strip_integer_cast(candidate);
                value = strip_integer_cast(value->left.node);
                if (trees_equal(value, other, 2)) goto cancel_nested_sum;
            }
            candidate = left->second.node;
            value = strip_integer_cast(candidate);
            if (value->opcode == 8) {
                other = strip_integer_cast(right);
                value = strip_integer_cast(candidate);
                value = strip_integer_cast(value->left.node);
                if (trees_equal(value, other, 2)) goto cancel_nested_sum;
            }
        }
    case 3:
        if (node->attributes.words.type == 0x408) return lift_typed_sequence(node, 0x68);
        break;
    case 4:
        if (right->opcode == 0x33) {
            if (left->opcode == 9 && rewrite_shift_as_multiply(left)) return fold_constant_node(node);
            value = fold_absorbing_constant(node, 0);
            if (value != node) return value;
            if (reduce_constant_bits(left, node)) return value;
            if ((node->attributes.words.type & 0x300) && node->opcode == 4) {
                bits = right->left.value;
                if (!(bits & (bits - 1))) {
                    node->opcode = 9;
                    node->second.node = intern_leaf(0x33, 0, 0x101, integer_log2(bits));
                    release_node(right);
                    goto restart;
                }
            }
        }
        value = lift_typed_sequence(node, 9);
        if (value != node) return value;
        return lift_unary_operand(node, 8);
    case 5:
        if (right->opcode == 0x33) {
            if (is_constant_value(right, 0) && !((flags = right->attributes.words.flags) & 0x40)) {
                compiler_warning(3, 0x2d3);
                right->attributes.words.flags = flags | 0x40;
                return node;
            }
            bits = right->left.value;
            if (!(bits & (bits - 1)) &&
                ((node->attributes.words.type & 0x200) || is_nonnegative_tree(left))) {
                node->opcode = 10;
                node->second.node = intern_leaf(0x33, 0, 0x101, integer_log2(bits));
                release_node(right);
                tree_changed = 1;
                return node;
            }
            if (left->opcode == 4 && left->second.node->opcode == 0x33 &&
                (node->attributes.words.type & 0x300) &&
                can_combine_constants(left, left->second.node->left.value, bits)) {
                tree_changed = 1;
                return build_tree("PPPLYFLY", left->left.node, left->second.node, right, node, left);
            }
        }
        if (!(node->attributes.words.type & 0x200)) return lift_unary_operand(node, 8);
        if (left->opcode == 10) return lift_typed_sequence(node, 10);
        break;
    case 6:
        if (is_constant_value(right, 0) && !((flags = right->attributes.words.flags) & 0x40)) {
            compiler_warning(3, 0x2d4);
            right->attributes.words.flags = flags | 0x40;
            return node;
        }
        if ((node->attributes.words.type & 0x100) && extract_negative(&node->second.node)) return node;
        if ((node->attributes.words.type & 0x200) && right->opcode == 0x33) {
            bits = right->left.value;
            if (!(bits & (bits - 1))) {
                node->opcode = 0xb;
                node->second.node = typed_integer_node(right, bits - 1);
                release_node(right);
                tree_changed = 1;
            }
        }
        break;
    case 7:
        if (left->opcode == 8 && constant_as_integer(right, &constant))
            return constant & 1 ? lift_left_unary(node) : remove_left_wrapper(node);
        if (left->opcode == 7) {
            tree_changed = 1;
            build_tree("PPPOLL", left->left.node, left->second.node, right, 4, left, node);
            return node;
        }
        if (is_constant_value(right, 0) && !is_constant_value(left, 0)) {
            release_node(right);
            node->second.node = typed_integer_node(node, 1);
            return fold_absorbing_constant(node, 1);
        }
        break;
    case 8: case 0xe:
        if (left->opcode + node->opcode == 0x16) {
            node->opcode = node->opcode == 8 ? 2 : 3;
            node->second.node = typed_integer_node(left, 1);
            return remove_left_wrapper(node);
        }
        goto cancel_wrappers;
    case 9:
        if (left->opcode == 4 && left->second.node->opcode == 0x33 && rewrite_shift_as_multiply(node))
            return fold_constant_node(node);
        if (left->opcode == 8) return lift_left_unary(node);
    case 10:
        if (left->opcode == node->opcode) {
            if (right->opcode != 0x33) return node;
            if (left->second.node->opcode == 0x33) {
                tree_changed = 1;
                build_tree("PPPOLYL", left->left.node, left->second.node, right, 2, left, node);
                return node;
            }
        }
        if (right->opcode != 0x33 || reduce_constant_bits(left, node)) return node;
        mask = shift_discard_mask(node);
        node->left.node = simplify_masked_tree(left, mask);
        break;
    case 0xb:
        if (right->opcode != 0x33) return node;
        value = fold_absorbing_constant(node, 0);
        if (value != node) return value;
        node->left.node = simplify_masked_tree(left, right->left.value);
        return value;
    case 0xc:
        if (right->opcode != 0x33) return node;
        mask = normalize_integer_width(node, ~0U);
        value = fold_absorbing_constant(node, mask);
        if (value != node) return value;
        mask = normalize_integer_width(node, ~right->left.value);
        node->left.node = simplify_masked_tree(left, mask);
        return value;
    case 0xd:
        mask = normalize_integer_width(right, ~0U);
        if (is_constant_value(right, mask)) {
            node->opcode = 0xe;
            return remove_right_wrapper(node);
        }
        return lift_unary_operand(node, 0xe);
    case 0x1f: case 0x20:
        if (factor_common_operator(node, 0xb, 1)) {
            release_node(node->left.node);
            node->left.node = typed_integer_node(node, 0);
            node->second.node->left.node->opcode = 0xd;
            tree_changed = 1;
            return node;
        }
        goto simplify_comparison;
    case 0x21: case 0x22: case 0x23: case 0x24:
        if (left->opcode == 9 && (node->attributes.words.type & 0x100)) return node;
simplify_comparison:
        if (left->opcode == 8 && negate_tree_in_place(&node->second.node)) {
            tree_changed = 1;
            opcode = map_comparison_opcode(node->opcode, 0);
            build_tree("PPOL", left->left.node, node->second.node, opcode, node);
            release_node(left);
            return node;
        }
        if (!factor_compatible_operator(node, 10, 1) && !factor_compatible_operator(node, 9, 1)) return node;
        value = node->second.node;
        other = value->second.node;
        mask = shift_discard_mask(value);
        value->second.node = typed_integer_node(value, mask);
        value->opcode = 0xb;
        distribute_constant(value, node->opcode);
        release_node(node->left.node);
        release_node(node);
        release_node(other);
        return value;
    case 0x32:
        if (!(node->attributes.words.flags & 0x800) && right->opcode == 0x30 &&
            trees_equal(left, right->left.node, 1)) {
            release_node(node);
            release_tree(left);
            return right;
        }
        break;
    case 0x34:
        type = node->attributes.words.type;
        if (type == 0x408 && left->attributes.words.type == 0x404) {
            tree_changed = lower_far_pointer_cast(node);
            return node;
        }
        chosen = current = node;
        while (current->opcode == 0x34 || current->opcode == 0x68) {
            current = current->left.node;
            if (conversion_loses_value(current->attributes.words.type, type)) break;
            if ((type & 255) == (current->attributes.words.type & 255)) chosen = current;
        }
        if (chosen == node) return node;
        while (node->left.node != chosen) node = remove_left_wrapper(node);
        if (chosen->attributes.words.type == node->attributes.words.type) return take_left_operand(node);
        break;
    case 0x38:
        if (left->opcode < 0x1f || left->opcode > 0x20 || left->left.node->opcode != 0x42) return node;
        alternatives = left->left.node->second.node;
        if (trees_equal(left->second.node, alternatives->second.node, 1) &&
            alternatives->left.node->opcode == 0x33 &&
            !trees_equal(left->second.node, alternatives->left.node, 1)) invert = left->opcode == 0x1f;
        else {
            if (!trees_equal(left->second.node, alternatives->left.node, 1) ||
                alternatives->second.node->opcode != 0x33 ||
                trees_equal(left->second.node, alternatives->second.node, 1)) return node;
            invert = left->opcode == 0x20;
        }
        value = left->left.node->left.node;
        node->left.node = value;
        if (invert) value->opcode = map_comparison_opcode(value->opcode, 1);
        release_tree(alternatives);
        release_tree(left->second.node);
        release_node(left->left.node);
        release_node(left);
        break;
    case 0x59:
        if (left->opcode != 0x30 && left->opcode != 0x59 &&
            (node_shapes[left->opcode] & NODE_ARITY) != NODE_LEAF &&
            (!(node->attributes.words.type & 0xf00) || (node->attributes.words.flags & 0x4000))) return node;
        return take_left_operand(node);
    case 0x68:
        if (left->opcode == 0x68) {
            node->left.node = left->left.node;
            release_node(left->second.node);
            release_node(left);
        }
        break;
    case 0x84:
        extract_negative(&node->left.node);
        break;
    case 0xaa:
cancel_wrappers:
        if (left->opcode == node->opcode) return take_left_operand(remove_left_wrapper(node));
        break;
    case 0xae: case 0xaf:
        if (trees_equal(left, right, 1)) {
            release_node(node);
            release_tree(left);
            return right;
        }
        break;
    }
    return node;
cancel_sum:
    tree_changed = 1;
    value = typed_integer_node(node, 0);
    release_tree(node);
    return value;
cancel_nested_sum:
    tree_changed = 1;
    node->second.node = typed_integer_node(right, 0);
    release_tree(right);
    replacement = typed_integer_node(candidate, 0);
    if (left->left.node == candidate) left->left.node = replacement;
    else left->second.node = replacement;
    release_tree(candidate);
    return node;
}

/* Original [00058440,00058462). Start with an ordinary integer constant
   and apply the model's representation through the normal conversion graph. */
Node *__fastcall typed_integer_node(Node *model, unsigned value)
{
    unsigned short type = model->attributes.words.type;
    return convert_node_type(intern_leaf(0x33, 0, 0x104, value), type);
}

/* Original tables [00083650,00083668) and [00083668,00083680):
   complement a comparison or exchange its operands. Only comparison opcodes
   1f..24 index these six-entry maps; the original uses biased base pointers. */
enum ComparisonOpcode { COMPARE_EQUAL = 0x1f, COMPARE_NOT_EQUAL,
    COMPARE_LESS, COMPARE_LESS_EQUAL, COMPARE_GREATER, COMPARE_GREATER_EQUAL };
static const unsigned negated_comparison[] = {
    COMPARE_NOT_EQUAL, COMPARE_EQUAL, COMPARE_GREATER_EQUAL,
    COMPARE_GREATER, COMPARE_LESS_EQUAL, COMPARE_LESS
};
static const unsigned exchanged_comparison[] = {
    COMPARE_EQUAL, COMPARE_NOT_EQUAL, COMPARE_GREATER,
    COMPARE_GREATER_EQUAL, COMPARE_LESS, COMPARE_LESS_EQUAL
};

/* Original [00058000,00058018). */
unsigned __fastcall map_comparison_opcode(unsigned opcode, int negate)
{
    if (negate) return negated_comparison[opcode - COMPARE_EQUAL];
    return exchanged_comparison[opcode - COMPARE_EQUAL];
}

/* Original [00055070,000550ca). Integral/pointer constants carry their
   bits directly; floating constants compare through the compiler's extended
   representation after converting the requested signed integer value. */
int __fastcall is_constant_value(Node *node, int value)
{
    Extended number;
    if (node->opcode != 0x33) return 0;
    if (node->attributes.words.type & 0xf00)
        return node->left.value == (unsigned)value;
    integer_to_extended(&value, &number);
    return compare_extended(1, node->left.pointer, &number);
}

/* Original [00055410,0005544d). Test whether the candidate representation
   crosses the width or floating-format boundaries used by cast elimination. */
int __fastcall conversion_loses_value(unsigned short source, unsigned short target)
{
    if (source == target) return 0;
    if ((target & 255) > (source & 255)) return 1;
    if (source & 0x1000) {
        if ((target & 0xf00) && (target & 255) == (source & 255)) return 1;
        if (target & 0x1000) return 1;
    }
    return 0;
}

/* Original [000551b0,000551c7). Remove a redundant unary shell. */
Node *__fastcall take_left_operand(Node *node)
{
    Node *left;
    tree_changed = 1;
    left = node->left.node;
    release_node(node);
    return left;
}

/* Original [00056b80,00056ba3). Ignore one same-width integral/pointer
   cast while inspecting arithmetic; keep all floating representation casts. */
Node *__fastcall strip_integer_cast(Node *node)
{
    Node *operand;
    unsigned short type, operand_type;
    if (node->opcode == 0x34) {
        operand = node->left.node;
        type = node->attributes.words.type;
        operand_type = operand->attributes.words.type;
        if ((type & 255) == (operand_type & 255) &&
            (type & 0xf00) && (operand_type & 0xf00)) return operand;
    }
    return node;
}

/* Original [000552d0,000552f5). Lift a conversion from the left operand
   around the binary operation, retyping both levels through the builder. */
Node *__fastcall lift_left_unary(Node *node)
{
    Node *left;
    tree_changed = 1;
    left = node->left.node;
    return build_tree("PPLYLY", left->left.node, node->second.node, node, left);
}

/* Original [00055e90,00055ee1). Express a constant left shift as a
   multiply before common-factor analysis, looking through one negation. */
int __fastcall rewrite_shift_as_multiply(Node *node)
{
    Node *count;
    int shift;
    unsigned multiplier = 0;
    if (node->opcode == 8) node = node->left.node;
    if (node->opcode != 9 || node->second.node->opcode != 0x33) return 0;
    count = node->second.node;
    node->opcode = 4;
    shift = (int)count->left.value;
    if (shift < 32) multiplier = 1U << ((unsigned)shift & 31);
    node->second.node = typed_integer_node(node, multiplier);
    release_node(count);
    return 1;
}

int __fastcall is_negatable_negative_constant(Node *node);
/* Original [000567b0,000568aa). Extract one negative sign through
   conversions, shifts and reciprocal/product forms, mutating the owning slot. */
int __fastcall extract_negative(Node **slot)
{
    Node *node;
    unsigned value;
    for (;;) {
        node = *slot;
        switch (node->opcode) {
        case 0x34:
            if ((node->attributes.words.type & 0x300) &&
                (node->left.node->attributes.words.type & 255) <
                (node->attributes.words.type & 255)) return 0;
            /* Fall through: the sign can be inspected in the left operand. */
        case 9: case 0xaa:
            slot = &node->left.node;
            break;
        case 3:
            if (!extract_negative(&node->left.node)) return 0;
            node->opcode = 2;
            tree_changed = 1;
            return 1;
        case 4: case 5:
            return extract_negative(&node->left.node) || extract_negative(&node->second.node);
        case 8:
            return negate_tree_in_place(slot);
        case 0x33:
            if (is_negatable_negative_constant(node)) return negate_tree_in_place(slot);
            if (node->attributes.words.type & 0x200) {
                value = node->left.value;
                if (normalize_integer_width(node, 0 - value) < value)
                    return negate_tree_in_place(slot);
            }
            return 0;
        default:
            return 0;
        }
    }
}

/* Original [00055100,00055149). Prefer the shorter positive magnitude
   when negation changes a signed constant; floating signs use extended compare. */
int __fastcall is_negatable_negative_constant(Node *node)
{
    unsigned value;
    if (node->attributes.words.type & 0x100) {
        value = node->left.value;
        return normalize_integer_width(node, 0 - value) < value;
    }
    if (node->attributes.words.type & 0x3000)
        return compare_extended(4, node->left.pointer, &extended_zero) != 0;
    return 0;
}

int __fastcall simple_constant_operation_sign(Node *node);
Node *__fastcall reverse_subtraction_chain(Node *node, Node *right);
/* Original [000568b0,00056a18). Negate an expression through its owning
   slot, keeping conversion, unsigned division and constant-cost restrictions. */
int __fastcall negate_tree_in_place(Node **slot)
{
    Node *node;
    unsigned short type, source;
    for (;;) {
        node = *slot;
        switch (node->opcode) {
        case 0x34:
            type = node->attributes.words.type;
            source = node->left.node->attributes.words.type;
            if ((type & 0x300) &&
                (source == 0x408 || (source & 255) < (type & 255))) return 0;
            if ((type & 0x1000) && source == 0x204) return 0;
            /* Fall through: inspect a sign carried by the left operand. */
        case 9: case 0xaa:
            slot = &node->left.node;
            break;
        case 2: case 3:
            if (negate_tree_in_place(&node->left.node)) {
                node->opcode = 5 - node->opcode;
                tree_changed = 1;
                return 1;
            }
            if (node->opcode == 3 || negate_tree_in_place(&node->second.node)) {
                reverse_subtraction_chain(node, node->second.node);
                tree_changed = 1;
                return 1;
            }
            return 0;
        case 5:
            if (node->attributes.words.type & 0x200) return 0;
            /* Fall through: one negated factor reverses the result's sign. */
        case 4:
            if (negate_tree_in_place(&node->left.node)) return 1;
            if (simple_constant_operation_sign(node) <= 0 &&
                negate_tree_in_place(&node->second.node)) return 1;
            return 0;
        case 8:
            *slot = take_left_operand(node);
            tree_changed = 1;
            return 1;
        case 0x33:
            *slot = build_tree("PNYF", node, 8);
            tree_changed = 1;
            return 1;
        default:
            return 0;
        }
    }
}

/* Original [00056a40,00056a5f). Remove a redundant unary right operand. */
Node *__fastcall remove_right_wrapper(Node *node)
{
    Node *right;
    tree_changed = 1;
    right = node->second.node;
    node->second.node = right->left.node;
    release_node(right);
    return node;
}

/* Original [00058430,0005843f). Distinct typed integral-leaf adapter. */
Node *__fastcall integer_node(unsigned value, unsigned type)
{
    return intern_leaf(0x33, 0, type, value);
}

/* Original [00056370,000563cf). Lift a matching binary child around the
   current operator, reusing both operator nodes through the tree builder. */
Node *__fastcall lift_typed_sequence(Node *node, unsigned opcode)
{
    Node *left = node->left.node, *right;
    if (left->opcode == opcode) {
        tree_changed = 1;
        return build_tree("PPLYPLY", left->left.node, node->second.node, node,
                          left->second.node, left);
    }
    right = node->second.node;
    if (right->opcode == opcode) {
        tree_changed = 1;
        return build_tree("PPLYPLY", left, right->left.node, node,
                          right->second.node, right);
    }
    return node;
}

/* Original [00056a20,00056a3f). Lift a matching unary operand, retaining
   distinct left/right transformations and the unchanged-node result. */
Node *__fastcall lift_unary_operand(Node *node, unsigned opcode)
{
    if (node->left.node->opcode == opcode) return lift_left_unary(node);
    if (node->second.node->opcode == opcode) return lift_right_unary(node);
    return node;
}

/* Original [00056a60,00056aab). Preserve evaluation of the left operand
   when an absorbing right constant makes the arithmetic result known. */
Node *__fastcall fold_absorbing_constant(Node *node, int value)
{
    Node *right = node->second.node;
    if (is_constant_value(right, value)) {
        tree_changed = 1;
        if (can_repeat_tree(node)) {
            release_tree(node->left.node);
            release_node(node);
            return right;
        }
        node->opcode = 0x44;
    }
    return node;
}

/* Original [00056600,000567ab). Reduce consecutive constant shifts,
   or a shifted factor in a multiplication, retaining the discarded-bit mask. */
int __fastcall reduce_constant_bits(Node *node, Node *parent)
{
    Node *inner_constant, *outer_constant;
    unsigned operation = parent->opcode, count, outer, mask, difference, remainder;
    int outer_pattern, minimum;
    if (operation == 10 && (parent->attributes.words.type & 0x100)) return 0;
    if (operation != 10 && node->opcode == 4)
        return reduce_constant_bits(node->left.node, parent) ||
               reduce_constant_bits(node->second.node, parent);
    if (node->opcode != (operation == 10 ? 9U : 10U)) return 0;
    inner_constant = node->second.node;
    if (inner_constant->opcode != 0x33) return 0;
    count = inner_constant->left.value;
    outer_constant = parent->second.node;
    outer = outer_constant->left.value;
    mask = shift_discard_mask(node);
    if (operation == 4) {
        if (integer_log2((0U - outer) & outer) < (int)count) return 0;
        node->second.node = typed_integer_node(inner_constant, 0);
        remainder = (unsigned)((int)outer >> (count & 31));
    } else {
        difference = count - outer;
        if ((int)difference < 0) difference = 0U - difference;
        outer_pattern = is_subword_mask_pattern(parent, outer);
        if (outer_pattern != is_subword_mask_pattern(node, count) &&
            !is_subword_mask_pattern(node, difference)) {
            minimum = (int)outer < (int)count ? (int)outer : (int)count;
            if (minimum < (int)difference) return 0;
        }
        if ((int)outer > (int)count) node->opcode = parent->opcode;
        node->second.node = typed_integer_node(inner_constant, difference);
        remainder = 0;
    }
    parent->second.node = typed_integer_node(outer_constant, remainder);
    node->left.node = build_tree("PPNY", node->left.node, typed_integer_node(node, mask), 0xb);
    release_node(inner_constant);
    release_node(outer_constant);
    tree_changed = 1;
    return 1;
}

/* Original [00058020,00058030). Highest set-bit index; zero maps to zero. */
int __fastcall integer_log2(unsigned value)
{
    int power = 0;
    while (value >>= 1) ++power;
    return power;
}

/* Original [000565b0,000565fb). Determine source bits retained by a
   constant shift, narrowed to the expression's scalar representation. */
unsigned __fastcall shift_discard_mask(Node *node)
{
    int count = (int)node->second.node->left.value;
    if (node->opcode == 9 && count > 0)
        return (unsigned)(integer_type_limit(node, 0x7fffffff) >> (((unsigned)count - 1) & 31));
    if (count >= 32) return normalize_integer_width(node, 0);
    return normalize_integer_width(node, ~0U << ((unsigned)count & 31));
}

/* Original [000563d0,000565ac). Propagate an observed-bit mask through
   integer expressions and choose the cheaper equivalent constant bit pattern. */
Node *__fastcall simplify_masked_tree(Node *node, unsigned mask)
{
    Node *value, *constant, *replacement;
    unsigned low_mask, operation, bits, kept;
    int count;
    switch (node->opcode) {
    case 2: case 3:
        low_mask = (2U << ((unsigned)integer_log2(mask) & 31)) - 1;
        value = simplify_masked_tree(node->second.node, low_mask);
        node->second.node = value;
        operation = value->opcode;
        if (operation == 0xb || operation == 9) value = value->second.node;
        if (value->opcode == 0x33) {
            if (operation == 9) {
                if ((int)value->left.value < 32)
                    mask |= 1U << (value->left.value & 31);
            } else mask |= value->left.value;
            low_mask &= 0U - ((0U - mask) & mask);
        }
        node->left.node = simplify_masked_tree(node->left.node, low_mask);
        break;
    case 4:
        mask = (2U << ((unsigned)integer_log2(mask) & 31)) - 1;
        /* Fall through: both operands contribute to observed result bits. */
    case 0xb: case 0xc: case 0xd:
        node->second.node = simplify_masked_tree(node->second.node, mask);
        /* Fall through. */
    case 0xe:
        node->left.node = simplify_masked_tree(node->left.node, mask);
        break;
    case 9:
        constant = node->second.node;
        if (constant->opcode == 0x33) {
            bits = shift_discard_mask(node);
            bits &= mask >> (constant->left.value & 31);
            node->left.node = simplify_masked_tree(node->left.node, bits);
        }
        break;
    case 10:
        constant = node->second.node;
        if (constant->opcode == 0x33) {
            count = (int)constant->left.value;
            if (count >= 32) mask = 0;
            else mask <<= ((unsigned)count & 31);
            if (node->attributes.words.type & 0x100)
                mask |= (unsigned)integer_type_limit(node, 0x80000000U);
            mask = normalize_integer_width(node, mask);
            node->left.node = simplify_masked_tree(node->left.node, mask);
        }
        break;
    case 0x33:
        bits = node->left.value;
        kept = mask & bits;
        if (normalize_integer_width(node, ~bits & mask) < kept) bits |= ~mask;
        else bits = kept;
        if (normalize_integer_width(node, bits) != node->left.value) {
            replacement = typed_integer_node(node, bits);
            tree_changed = 1;
            release_node(node);
            return replacement;
        }
        break;
    }
    return node;
}

/* Original [00054db0,00054df9). Distribute a constant right operand over
   the requested binary operator, retaining node reuse and folding order. */
int __fastcall distribute_constant(Node *node, unsigned opcode)
{
    Node *left = node->left.node, *constant;
    unsigned operation;
    if (left->opcode != opcode) return 0;
    constant = node->second.node;
    if (constant->opcode != 0x33) return 0;
    operation = node->opcode;
    build_tree("PCOLYFPPNYFOL", left->left.node, constant, operation, left,
               left->second.node, constant, operation, opcode, node);
    tree_changed = 1;
    return 1;
}

/* Original [00055a00,00055a45). Accept integer-valued scalar constants,
   checking a floating conversion by comparing its exact original value. */
int __fastcall constant_as_integer(Node *node, int *value)
{
    if (node->opcode != 0x33) return 0;
    if (node->attributes.words.type & 0xf00) {
        *value = (int)node->left.value;
        return 1;
    }
    if (extended_to_integer(node->left.pointer, value)) return 0;
    return is_constant_value(node, *value);
}

/* Original [00056060,00056150), including its nine-entry dispatch table.
   Test whether factoring preserves an exact quotient, shift width or mask. */
int __fastcall can_combine_constants(Node *node, unsigned first, unsigned second)
{
    switch (node->opcode) {
    case 4: case 7:
        if (!second) return 0;
        if (node->attributes.words.type & 0x200) return first % second == 0;
        return (int)first % (int)second == 0;
    case 9: case 10:
        if ((int)second > (int)first) return 0;
        if (is_subword_mask_pattern(node, first) &&
            !is_subword_mask_pattern(node, first - second)) return 0;
        return 1;
    case 0xb:
        return (first & second) == first;
    case 0xc:
        return (first | second) == first;
    default:
        compiler_internal_error("@(#)optimize.c:1.144", 0x468);
        return 0;
    }
}

/* Original [00055fe0,00056054), including its nine-entry dispatch table.
   The compatibility query owns the guards for the factored operation. */
unsigned __fastcall combine_factor_constants(Node *operation, unsigned first, unsigned second)
{
    switch (operation->opcode) {
    case 4: case 7:
        if (!second) return 0;
        if (operation->attributes.words.type & 0x200) return first / second;
        return (unsigned)((int)first / (int)second);
    case 9: case 10:
        return first - second;
    case 0xc:
        return first ^ second;
    default:
        return first;
    }
}

/* Original [000559e0,000559f3). Represent the operation's identity using
   the separate model node's scalar type. */
Node *__fastcall operator_identity_node(Node *operation, Node *type_source)
{
    unsigned value = (unsigned)operator_identity(operation);
    return typed_integer_node(type_source, value);
}

/* Original [00056f20,00056f57). Reverse a left-associated sum into a
   subtraction chain with the supplied leading operand. */
Node *__fastcall reverse_subtraction_chain(Node *node, Node *right)
{
    Node *left = node->left.node;
    node->opcode = 3;
    if (left->opcode == 2) {
        node->second.node = left->second.node;
        reverse_subtraction_chain(left, right);
    } else {
        node->second.node = left;
        node->left.node = right;
    }
    return finish_canonical_node(node);
}

/* Original [000551d0,00055207). Drop one left-child wrapper, retaining
   its first operand and releasing the unused second operand of a binary form. */
Node *__fastcall remove_left_wrapper(Node *node)
{
    Node *left;
    tree_changed = 1;
    left = node->left.node;
    node->left.node = left->left.node;
    if ((node_shapes[left->opcode] & NODE_ARITY) == NODE_BINARY)
        release_tree(left->second.node);
    release_node(left);
    return node;
}
