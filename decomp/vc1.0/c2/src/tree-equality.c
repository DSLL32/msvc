#include "tree-rewrite.h"
#include "tree-input.h"
#include "extended.h"
#include <recon.h>
unsigned __fastcall node_hash_key(Node *node);
int tree_comparison_flags; /* 0007e844: shared recursive equality mode */

/* Original [00058100,00058120). -1 selects the ordinary comparison mode. */
int __fastcall trees_equal(Node *left, Node *right, int flags)
{
    tree_comparison_flags = flags == -1 ? 0 : flags;
    return equal_tree_nodes(left, right);
}

/* Original [00058120,00058340). Compare address/value trees recursively.
   The ordinary mode also recognizes assignment values and loads of their
   target; the stricter mode excludes effects and distinct volatile loads. */
int __fastcall equal_tree_nodes(Node *left, Node *right)
{
    unsigned opcode, type;
    if (!left || !right) return 0;
    if (tree_comparison_flags && (node_shapes[left->opcode] & NODE_SIDE_EFFECT)) return 0;
    if (!tree_comparison_flags && left == right) return 1;
    type = left->attributes.words.type;
    if (type != right->attributes.words.type) return 0;
    opcode = left->opcode;
    if (opcode == right->opcode) {
        switch (opcode) {
        case 0x26:
            if (left->right.symbol == right->right.symbol && left->left.value == right->left.value)
                return 1;
            break;
        case 0x30:
            if (tree_comparison_flags && left != right &&
                ((left->attributes.words.flags | right->attributes.words.flags) & 0x800)) break;
            goto compare_operands;
        case 0x33:
            if ((type & 0x0f00) && left->left.value == right->left.value) return 1;
            if ((type & 0x1000) && compare_extended(1,
                (Extended *)left->left.pointer, (Extended *)right->left.pointer)) return 1;
            break;
        case 0x69:
            break;
        case 0xa1: case 0xa2:
            if (left->left.word == right->left.word) return 1;
            break;
        default:
compare_operands:
            if ((node_shapes[left->opcode] & NODE_ARITY) != NODE_LEAF &&
                !equal_tree_nodes(left->left.node, right->left.node)) break;
            if ((node_shapes[left->opcode] & NODE_HAS_SECOND) &&
                !equal_tree_nodes(left->second.node, right->second.node)) break;
            return 1;
        }
    }
    if (tree_comparison_flags) return 0;
    if (node_shapes[left->opcode] & NODE_WRITES_LEFT) {
        if (right->opcode == 0x30 && equal_tree_nodes(left->left.node, right->left.node)) return 1;
        if (left->opcode == 0x32 && equal_tree_nodes(left->second.node, right)) return 1;
    }
    if (node_shapes[right->opcode] & NODE_WRITES_LEFT) {
        if (left->opcode == 0x30 && equal_tree_nodes(left->left.node, right->left.node)) return 1;
        if (right->opcode == 0x32 && equal_tree_nodes(left, right->second.node)) return 1;
    }
    return 0;
}

/* Original [000580c0,00058100). Hash this node and its immediate operands;
   the key helper does not recurse, so depth below the operands is ignored. */
unsigned __fastcall shallow_tree_hash(Node *node)
{
    unsigned hash = node_hash_key(node);
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        hash += node_hash_key(node->left.node);
        if (node_shapes[node->opcode] & NODE_HAS_SECOND)
            hash += node_hash_key(node->second.node);
    }
    return hash;
}

/* Original [00058060,000580c0). Canonical addresses use their symbol's
   numeric identity; frame temporaries use their reserved stack/register slot.
   Integer constants hash their payload and floating constants hash to zero. */
unsigned __fastcall node_hash_key(Node *node)
{
    unsigned hash = (unsigned)node->attributes.words.type + node->opcode;
    SymbolHeader *symbol;
    if (node->opcode == 0x33) {
        if (node->attributes.words.type & 0x3000) return 0;
        return node->left.value;
    }
    symbol = node->right.symbol;
    if (node->opcode == 0x26) {
        if (symbol->kind == 19) hash = ((TemporaryRegister *)symbol)->number;
        else hash = node->left.value;
    }
    if (symbol && symbol->kind != 19) hash += symbol->identifier;
    return hash;
}
