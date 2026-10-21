#include "tree-rewrite.h"
#include "control-input.h"
#include "records.h"
extern int default_integer_bytes;
unsigned expression_side_effects;
int expression_integer_bytes = 4;
int checking_expression;
/* Original [000501f0,00050300). Keep source markers, and diagnose ordinary
   expressions without effects after the recursive rewrite. */
Node *__fastcall canonicalize_tree(Function *function, Node *node)
{
    Node *child;
    expression_integer_bytes = default_integer_bytes;
    node = prepare_target_tree(node);
    expression_side_effects = 0;
    checking_expression = 1;
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
    checking_expression = 0;
    expression_integer_bytes = 1;
    return node;
}

/* Original [00026df0,00026e00): the i386 target's identity hook. */
Node *__fastcall prepare_target_tree(Node *node)
{
    return node;
}
