#include "expression-fields.h"
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
