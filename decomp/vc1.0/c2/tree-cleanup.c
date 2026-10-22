#include "tree-rewrite.h"
#include "tree-scratch.h"
/* Original [00058bd0,00058c80), including its record dispatch table. */
Node *__fastcall cleanup_expression_tree(Node *node)
{
    unsigned opcode = node->opcode;
    if ((opcode >= 0x45 && opcode <= 0x49) || opcode == 0x4f || opcode == 0x56 ||
        (opcode >= 0x8d && opcode <= 0x90) || opcode == 0x9d) return node;
    switch (opcode) {
    case 0x29: case 0x2a: case 0x2b: case 0x38: case 0x3a: case 0x3b: case 0x3c: case 0x41:
        return node;
    default:
        return cleanup_unused_tree(node);
    }
}
/* Original [00058b80,00058bd0). The tree cleanup may change the list
   cursor while splitting an expression, so reload it after every callback. */
void __fastcall cleanup_expressions(Expression **expressions)
{
    Expression *expression = *expressions;
    expression_cleanup_link = expressions;
    while (expression) {
        expression->tree = cleanup_expression_tree(expression->tree);
        if (expression->tree->opcode == 1) {
            *expression_cleanup_link = expression->next;
            release_tree(expression->tree);
            release_expression(expression);
        } else expression_cleanup_link = &expression->next;
        expression = *expression_cleanup_link;
    }
}
