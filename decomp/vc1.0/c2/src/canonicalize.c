#include "tree-rewrite.h"
#include "tree-input.h"
#include "control-input.h"
#include "records.h"
extern int default_integer_bytes;
unsigned expression_side_effects;
int expression_integer_bytes = 4;
/* Original [000501f0,00050300). Keep source markers, and diagnose ordinary
   expressions without effects after the recursive rewrite. */
Node *__fastcall canonicalize_tree(Function *function, Node *node)
{
    Node *child;
    expression_integer_bytes = default_integer_bytes;
    node = prepare_target_tree(node);
    expression_side_effects = 0;
    backend_stage = 1;
    switch (node->opcode) {
    case 1: case 0x29: case 0x2a: case 0x2b: case 0x3a: case 0x4f:
        break;
    case 0x56:
        current_source_filename = node->right.pointer;
        break;
    case 0x2c:
        child = node->left.node;
        release_node(node);
        node = child;
        goto rewrite;
    case 0x38: case 0x39: case 0x3b: case 0x3c: case 0x41:
    case 0x46: case 0x47: case 0x48: case 0x8d: case 0x8e:
    case 0x8f: case 0x90:
        expression_side_effects = 2;
        /* fall through */
    default:
rewrite:
        node = canonicalize_node(function, node, 0);
        if (!(expression_side_effects & 2)) compiler_warning(4, 0x2c1);
        break;
    }
    backend_stage = 0;
    expression_integer_bytes = 1;
    return node;
}

/* Original [00026df0,00026e00): the i386 target's identity hook. */
Node *__fastcall prepare_target_tree(Node *node)
{
    return node;
}

/* Original [00050300,000503b0). The third argument is the parent node.
   Argument-chain entries are rewritten for their effects, as in the original. */
Node *__fastcall canonicalize_node(Function *function, Node *node, Node *parent)
{
    Node *argument;
    node = canonicalize_before_children(function, node, parent);
    switch (node_shapes[node->opcode] & NODE_ARITY) {
    case NODE_BINARY:
        node->left.node = canonicalize_node(function, node->left.node, node);
        node->second.node = canonicalize_node(function, node->second.node, node);
        break;
    case NODE_UNARY:
        node->left.node = canonicalize_node(function, node->left.node, node);
        break;
    case NODE_CHAIN:
        node->left.node = canonicalize_node(function, node->left.node, node);
        argument = node->second.node;
        while (argument->opcode != 0x4c) {
            canonicalize_node(function, argument, node);
            argument = argument->second.node;
        }
        break;
    }
    node = canonicalize_after_children(node, parent);
    return finish_canonical_node(node);
}
