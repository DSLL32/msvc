#include "tree-rewrite.h"
#include "tree-input.h"
#include "expression-fields.h"
#include <recon.h>
int tree_changed; /* 00083154 */
extern int default_integer_bytes;
unsigned last_operand_key; /* 0007bf5c */
/* Original [00053500,00053530). Leaf records and expression-stack records
   require no local optimizer traversal. */
Node *__fastcall optimize_tree_local(Node *node)
{
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF && node->opcode != 0x3c) {
        order_tree_operands(node, 0);
        node = fold_local_tree(node);
    }
    return node;
}

/* Original [00053400,00053450). Simplification follows child folding. */
Node *__fastcall fold_local_tree(Node *node)
{
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        node->left.node = fold_local_tree(node->left.node);
        if (node_shapes[node->opcode] & NODE_HAS_SECOND)
            node->second.node = fold_local_tree(node->second.node);
        node = fold_constant_node(node);
        node = simplify_local_node(node);
    }
    return node;
}

/* Original [00056f60,000570a0). Dispatch scalar constants and combine
   adjacent constants without changing the original operator graph. */
Node *__fastcall fold_constant_node(Node *node)
{
    unsigned shape = node_shapes[node->opcode], arity = shape & NODE_ARITY;
    Node *left, *right, *value;
    unsigned left_shape;
    if (arity == NODE_LEAF) return node;
    if (!(shape & NODE_CONSTANT_FOLD)) return fold_special_node(node);
    if (arity == NODE_BINARY) {
        right = node->second.node;
        if (right->opcode != 0x33) return node;
    }
    left = node->left.node;
    if (left->opcode == 0x33) {
        if (node->attributes.words.type & 0xf00) value = fold_integer_node(node);
        else value = fold_floating_node(node);
        if (value != node) tree_changed = 1;
        return value;
    }
    left_shape = node_shapes[left->opcode];
    if (arity != NODE_UNARY && (left_shape & NODE_ARITY) == NODE_BINARY && left->second.node->opcode == 0x33) {
        if (left->opcode == node->opcode && (shape & NODE_ASSOCIATIVE))
            return build_tree("PPPLYFLY", left->left.node, left->second.node, right, left, node);
        if ((shape & NODE_ADDITIVE) && (left_shape & NODE_ADDITIVE))
            return build_tree("PPPOLYFLY", left->left.node, left->second.node, right,
                              left->opcode == node->opcode ? 2 : 3, node, left);
    }
    return node;
}

/* Original [000576b0,00057810). Branches, conditional values and
   comparisons have their own constant-folding rules. */
Node *__fastcall fold_special_node(Node *node)
{
    Node *left = node->left.node, *right = node->second.node, *value;
    unsigned opcode = node->opcode;
    if (opcode == 0x1f || opcode == 0x20) {
        if ((left->opcode == 0x26 && is_nonnull_address(left) && is_constant_value(right, 0)) ||
            (right->opcode == 0x26 && is_nonnull_address(right) && is_constant_value(left, 0))) {
            value = intern_leaf(0x33, 0, default_integer_bytes | 0x100, opcode == 0x20);
            release_tree(node);
            return value;
        }
    } else if (opcode == 0x38) {
        if (left->opcode == 0x33) {
            node->opcode = is_constant_value(left, 0) ? 0x3a : 1;
            release_node(left);
        }
        return node;
    } else if (opcode == 0x42) {
        if (left->opcode == 0x33) {
            value = copy_tree(is_constant_value(left, 0) ? right->second.node : right->left.node);
            release_tree(node);
            return value;
        }
        return node;
    }
    /* Original comparison-class property (0x10) consists of 1f..24. */
    if (opcode >= 0x1f && opcode <= 0x24 && left->opcode == right->opcode) {
        if (left->opcode == 0x26 && left->right.symbol == right->right.symbol)
            return fold_integer_comparison(node);
        if (left->opcode == 0x33) {
            if (node->attributes.words.type & 0xf00) return fold_integer_comparison(node);
            if (node->attributes.words.type & 0x3000) return fold_floating_comparison(node);
        }
    }
    return node;
}

/* Original [00057c20,00057cf0). Process the right operand first, retaining
   its final key across reassociation of the left chain. */
unsigned __fastcall order_tree_operands(Node *node, Node *parent)
{
    unsigned right_key = 0, left_key = 0, key, prior_key;
    if (node_shapes[node->opcode] & NODE_ASSOCIATIVE) reassociate_right_chain(node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        right_key = order_tree_operands(node->second.node, node);
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF)
        left_key = order_tree_operands(node->left.node, node);
    key = tree_order_key(node, left_key, right_key);
    if (node_shapes[node->opcode] & NODE_CAN_SWAP) {
        if ((node_shapes[node->opcode] & NODE_ASSOCIATIVE) &&
            rotate_left_chain(node, right_key, last_operand_key, parent && parent->opcode == 0x30)) {
            prior_key = last_operand_key;
            order_tree_operands(node->left.node, node);
            right_key = prior_key;
        } else right_key = sort_commutative_operands(node, left_key, right_key);
    }
    last_operand_key = right_key;
    return key;
}

/* Original [00057dc0,00057f60). The upper bits estimate subtree work;
   the low nibble gives a stable ordering among similarly sized operands. */
unsigned __fastcall tree_order_key(Node *node, unsigned left_key, unsigned right_key)
{
    unsigned opcode = node->opcode, arity = node_shapes[opcode] & NODE_ARITY;
    unsigned identifier, shifted;
    int number, upper, middle, folded;
    signed char byte_opcode;
    SymbolHeader *symbol;
    if (arity == NODE_BINARY) {
        if (opcode == 0x44) return right_key;
        return (((opcode >> 4) - opcode + right_key + left_key) & 15) |
               ((right_key + left_key + 0x20) & 0xfff0);
    }
    if (arity == NODE_UNARY) {
        if (opcode == 0x31 || (opcode == 0x34 && skip_node_conversions(node) != node)) return left_key;
        byte_opcode = (signed char)node->opcode;
        return ((((byte_opcode >> 4) - byte_opcode) + left_key) & 15) |
               ((left_key + (opcode == 0x30 ? 0x10 : 0x20)) & 0xfff0);
    }
    if (arity == NODE_LEAF) {
        if (opcode == 0x26) {
            symbol = node->right.symbol;
            if (!symbol || symbol->kind == 0x13) return 0x10;
            identifier = symbol->identifier;
            shifted = identifier >> 8;
            return ((identifier + (((shifted - identifier) >> 4) - shifted)) & 15) | 0x10;
        }
        if (opcode == 0x33) {
            if (node->attributes.words.type & 0x3000) number = *(int *)node->left.pointer;
            else number = (int)node->left.value;
            upper = number >> 16;
            middle = (int)((unsigned)upper - (unsigned)number) >> 8;
            folded = (int)((unsigned)number - (unsigned)upper + (unsigned)middle) >> 4;
            return ((unsigned)folded - (unsigned)middle - (unsigned)number + (unsigned)upper) & 15;
        }
        return (((opcode >> 4) - opcode) & 15) | 0x10;
    }
    if (arity == NODE_CHAIN)
        return (((opcode >> 4) - opcode + right_key + left_key) & 15) |
               ((right_key + left_key + 0x70) & 0xfff0);
    compiler_internal_error("@(#)sortnode.c:1.32", 0xa8);
}
