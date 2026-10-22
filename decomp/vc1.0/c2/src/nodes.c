#include "expression-fields.h"
#include "tree-rewrite.h"
Node *free_nodes;
unsigned allocated_nodes;
/* Original [0004f740,0004f790). Only reused nodes are explicitly cleared. */
Node *__fastcall new_node(unsigned opcode)
{
    Node *node = free_nodes;
    if (node) {
        free_nodes = node->left.node;
        node->opcode = node->attributes.value = node->info.value = 0;
        node->left.value = node->second.value = node->right.value = node->detail.value = 0;
    } else {
        node = arena_alloc(5, sizeof(Node));
        ++allocated_nodes;
    }
    node->opcode = opcode;
    return node;
}

/* Original [0004f490,0004f4b0). Interned leaves and the shared sentinel
   remain owned by their tables; ordinary tree nodes return to the pool. */
void __fastcall release_node(Node *node)
{
    switch (node->opcode) {
    case 0x26: case 0x33: case 0x4c: case 0x69: case 0x9b: case 0xa2:
        return;
    }
    node->left.node = free_nodes;
    free_nodes = node;
}

Expression *free_expressions;
/* Original [0004fcf0,0004fd00); the adjacent work recycler is separate. */
void __fastcall release_expression(Expression *expression)
{
    expression->next = free_expressions;
    free_expressions = expression;
}
/* Original [0004fb90,0004fc70). Reclaim owned subtrees, retaining interned
   leaves. Switch ranges and the extra intrinsic chain have separate owners. */
void __fastcall release_tree(Node *node)
{
    unsigned opcode = node->opcode;
    unsigned shape = node_shapes[opcode], arity = shape & NODE_ARITY;
    Node *child, *next;
    if (arity == NODE_BINARY) {
        release_tree(node->second.node);
        release_tree(node->left.node);
        release_node(node);
    } else if (arity == NODE_UNARY) {
        release_tree(node->left.node);
        release_node(node);
    } else if (arity == NODE_LEAF) {
        if (shape & NODE_INTERNED) return;
        if (opcode == 0x3c) {
            child = node->second.node;
            while (child->opcode != 0x4c) {
                next = child->second.node;
                release_node(child);
                child = next;
            }
        }
        release_node(node);
    } else {
        child = node->second.node;
        while (child->opcode != 0x4c) {
            next = child->second.node;
            release_tree(child);
            child = next;
        }
        if (opcode == 0x40 && node->left.node->left.value == 0xf0) {
            child = node->right.node;
            while (child->opcode != 0x4c) {
                next = child->second.node;
                release_tree(child);
                child = next;
            }
        }
        release_tree(node->left.node);
        release_node(node);
    }
}
/* Original [0004f440,0004f490). */
Expression *new_expression(void)
{
    Expression *expression = free_expressions;
    if (expression) free_expressions = expression->next;
    else expression = arena_alloc(5, sizeof(Expression));
    expression->next = 0;
    expression->tree = 0;
    expression->field08 = expression->source_location = 0;
    return expression;
}

/* Original [00058030,00058060). Retain the shared end marker at the tail. */
Node *__fastcall reverse_node_list(Node *node)
{
    Node *first = node, *previous = 0, *next;
    if (node->opcode == 0x4c) return node;
    do {
        next = node->second.node;
        node->second.node = previous;
        previous = node;
        node = next;
    } while (node->opcode != 0x4c);
    first->second.node = node;
    return previous;
}

/* Original [000501c0,000501f0). Interned leaves retain identity; all
   other nodes receive a shallow copy in the ordinary node arena. */
Node *__fastcall clone_node(Node *node)
{
    Node *copy;
    if (node_shapes[node->opcode] & NODE_INTERNED) return node;
    copy = new_node(node->opcode);
    *copy = *node;
    return copy;
}
