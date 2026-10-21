#include "tree-rewrite.h"
#include "tree-input.h"
Expression *current_expression;
/* Original [000516b0,000516e0). The first rewrite may replace the current
   expression, so reload that global before recording the returned tree. */
void __fastcall rewrite_expression(Expression *expression)
{
    TreeReference reference;
    Node *node;
    current_expression = expression;
    node = rewrite_subtree(expression->tree, 0, 0);
    current_expression->tree = node;
    analyze_tree_references(node, &reference);
}

/* Original [00051960,00051a40). Rewrite to a fixed point before walking
   the left operand, then apply the second rewrite and walk the right. */
Node *__fastcall rewrite_subtree(Node *node, Node *parent, unsigned flags)
{
    Node *previous;
    unsigned child_flags;
    if (node->opcode == 0x38 || node->opcode == 0x3b ||
        node->opcode == 0x3e || node->opcode == 0x41) flags |= 2;
    if ((node->attributes.words.type & 0xc00) == 0xc00) node->attributes.words.type = 0x408;
    do {
        previous = node;
        node = rewrite_before_children(node, parent, flags);
    } while (node != previous);
    if (node->opcode == 0x55 || (node->opcode == 0x34 &&
        (node->attributes.words.flags & 0x4000))) flags &= ~4U;
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        child_flags = flags;
        if (node_shapes[node->opcode] & NODE_WRITES_LEFT) child_flags |= 5;
        else if (node->opcode == 0x30 || node->opcode == 0x3e) child_flags |= 4;
        else if (node->opcode == 0x55) child_flags |= 8;
        node->left.node = rewrite_subtree(node->left.node, node, child_flags);
    }
    node = rewrite_after_left(node, parent, flags);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        node->second.node = rewrite_subtree(node->second.node, node, flags);
    return node;
}

extern void *output_state; /* 000831b0: default unknown-reference descriptor. */
/* Original [000516e0,00051960). Propagate a base symbol and the two
   reference flags, annotating loads and stores along the way. */
void __fastcall analyze_tree_references(Node *node, TreeReference *reference)
{
    TreeReference left_reference, right_reference;
    Node *left, *right;
    SymbolHeader *symbol;
    unsigned intrinsic;
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        left = node->left.node;
        analyze_tree_references(left, &left_reference);
    }
    if (node_shapes[node->opcode] & NODE_HAS_SECOND) {
        right = node->second.node;
        analyze_tree_references(right, &right_reference);
    }
    reference->symbol = output_state;
    reference->flags = 2;
    switch (node->opcode) {
    case 2: case 3:
        if (left->attributes.words.type & 0xc00) {
            reference->symbol = left_reference.symbol;
            if (right->opcode == 0x33) reference->flags = left_reference.flags;
            else reference->flags = left_reference.flags | right_reference.flags | 2;
        } else if (right->attributes.words.type & 0xc00) {
            reference->symbol = right_reference.symbol;
            if (left->opcode == 0x33) reference->flags = right_reference.flags;
            else reference->flags = left_reference.flags | right_reference.flags | 2;
        }
        break;
    case 0x26:
        symbol = node->right.symbol;
        reference->symbol = symbol;
        reference->flags = 1;
        if (symbol && symbol->kind != 19 && (((FunctionSymbol *)symbol)->field24 & 0x4000))
            reference->flags = 3;
        break;
    case 0x30: case 0x32:
        node->attributes.words.flags |= (unsigned short)left_reference.flags;
        if ((node->attributes.words.type & 0xff00) == 0x8000) node->attributes.words.flags |= 2;
        reference->flags = left_reference.flags & 2;
        node->right.symbol = reference->symbol = left_reference.symbol;
        node->detail.node = node;
        break;
    case 0x33:
        reference->flags = 0;
        break;
    case 0x34:
        *reference = left_reference;
        if (left->opcode != 0x33) reference->flags |= 2;
        break;
    case 0x40:
        intrinsic = node->left.node->left.value;
        if (intrinsic == 0xf0) analyze_tree_references(node->right.node, &right_reference);
        /* These four intrinsic descriptors have the operand-reference bit
           (80h at original 000839e4 + index*16); no other valid entry does. */
        if (intrinsic >= 0x98 && intrinsic <= 0x9b) *reference = right_reference;
        break;
    case 0x55:
        node->attributes.words.flags |= (unsigned short)(left_reference.flags | 2);
        node->right.symbol = left_reference.symbol;
        node->detail.node = node;
        /* fall through */
    case 8: case 0x0e: case 0x59: case 0x68:
        *reference = left_reference;
        break;
    }
}
