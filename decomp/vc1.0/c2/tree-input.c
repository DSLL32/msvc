#include "tree-input.h"
#include "input.h"
/* Original opcode classification at 000857b8. Only the encountered tree properties
   are represented; this is not the original packed flag table. */
const unsigned char node_shapes[NODE_OPCODE_COUNT] = {
    /* 00 */ COMPLETE_LEAF, COMPLETE_LEAF, BINARY, BINARY, BINARY, BINARY, BINARY, BINARY,
    /* 08 */ UNARY, BINARY, BINARY, BINARY, BINARY, BINARY, UNARY, ASSIGNMENT,
    /* 10 */ ASSIGNMENT, ASSIGNMENT, ASSIGNMENT, ASSIGNMENT, ASSIGNMENT, ASSIGNMENT, ASSIGNMENT, ASSIGNMENT,
    /* 18 */ ASSIGNMENT, ASSIGNMENT, UNARY, BINARY, BINARY, BINARY, BINARY, BINARY,
    /* 20 */ BINARY, BINARY, BINARY, BINARY, BINARY, UNARY, LEAF, BINARY,
    /* 28 */ BINARY, COMPLETE_LEAF, COMPLETE_LEAF, COMPLETE_LEAF, UNARY, COMPLETE_LEAF, CHAIN, CHAIN,
    /* 30 */ UNARY, UNARY, ASSIGNMENT, LEAF, UNARY, ASSIGNMENT, ASSIGNMENT, BINARY,
    /* 38 */ COMPLETE_UNARY, COMPLETE_UNARY, COMPLETE_LEAF, COMPLETE_UNARY, COMPLETE_LEAF, COMPLETE_LEAF, CHAIN, CHAIN,
    /* 40 */ CHAIN, COMPLETE_UNARY, BINARY, BINARY, BINARY, COMPLETE_UNARY, COMPLETE_CHAIN, COMPLETE_LEAF,
    /* 48 */ COMPLETE_CHAIN, COMPLETE_LEAF, COMPLETE_CHAIN, COMPLETE_UNARY, COMPLETE_LEAF, COMPLETE_LEAF, LEAF, COMPLETE_LEAF,
    /* 50 */ UNARY, LEAF, COMPLETE_LEAF, COMPLETE_LEAF, COMPLETE_LEAF, COMPLETE_UNARY | NODE_HAS_SECOND, COMPLETE_LEAF, COMPLETE_UNARY | NODE_HAS_SECOND,
    /* 58 */ UNARY, UNARY, UNARY, LEAF, LEAF, LEAF, LEAF, LEAF,
    /* 60 */ LEAF, LEAF, LEAF, LEAF, LEAF, LEAF, LEAF, LEAF,
    /* 68 */ BINARY, LEAF, LEAF, LEAF, LEAF, LEAF, LEAF, LEAF,
    /* 70 */ LEAF, LEAF, LEAF, LEAF, BINARY, BINARY, BINARY, BINARY,
    /* 78 */ COMPLETE_CHAIN, UNARY, CHAIN, BINARY, BINARY, ASSIGNMENT, CHAIN, COMPLETE_LEAF,
    /* 80 */ LEAF, BINARY, BINARY, BINARY, UNARY, LEAF, UNARY, ASSIGNMENT,
    /* 88 */ UNARY, UNARY, BINARY, BINARY, LEAF, COMPLETE_UNARY, COMPLETE_LEAF, COMPLETE_LEAF,
    /* 90 */ COMPLETE_LEAF, COMPLETE_LEAF, COMPLETE_LEAF, COMPLETE_LEAF, COMPLETE_LEAF, COMPLETE_LEAF, COMPLETE_LEAF, COMPLETE_LEAF,
    /* 98 */ COMPLETE_LEAF, BINARY, BINARY, LEAF, BINARY, COMPLETE_CHAIN, BINARY, LEAF,
    /* a0 */ LEAF, LEAF, LEAF, LEAF, UNARY, LEAF, BINARY, UNARY,
    /* a8 */ LEAF, LEAF, UNARY, BINARY, BINARY, BINARY, BINARY, BINARY,
    /* b0 */ UNARY, BINARY, BINARY, LEAF, LEAF, NODE_BINARY
};

/* Original [0004efd0,0004f100). Build operand trees from postfix records,
   with explicit chains for argument lists and their end marker (4c). */
Node *__fastcall read_expression(Function *function)
{
    Node *node, *child;
    unsigned char shape;
    node = read_expression_record(function);
    if (!node) return 0;
    shape = node_shapes[node->opcode];
    if ((shape & NODE_ARITY) != NODE_LEAF) node->left.node = node->second.node = 0;
    if ((shape & NODE_ARITY) == NODE_LEAF && node->opcode != 0x4b && !(shape & NODE_COMPLETE)) {
        for (;;) {
            child = read_expression(function);
            shape = node_shapes[child->opcode];
            if ((shape & NODE_ARITY) != NODE_CHAIN && child->opcode != 0x3c) {
                if (!child->second.node && (shape & NODE_ARITY) != NODE_UNARY) child->second.node = node;
                else child->left.node = node;
            } else {
                child->left.node = node;
                node = child;
                if (child->opcode == 0x3c) {
                    node->second.node = read_expression_record(function);
                    node = node->second.node;
                    while (node->opcode != 0x4c) {
                        node->second.node = read_expression_record(function);
                        node = node->second.node;
                    }
                    node->second.node = 0;
                } else {
                    do {
                        node->second.node = read_expression(function);
                        node = node->second.node;
                    } while (node->opcode != 0x4c);
                }
            }
            node = child;
            if (node->opcode == 0x4b || (node_shapes[node->opcode] & NODE_COMPLETE)) break;
            if (node->second.node && node->left.node) continue;
            if (node->left.node && (node_shapes[node->opcode] & NODE_ARITY) != NODE_UNARY) continue;
            break;
        }
    }
    return node;
}

int expression_eof;
/* Original [0004f100,0004f140). 4d terminates the expression stream. */
Node *__fastcall read_expression_record(Function *function)
{
    unsigned short opcode = input_expression_opcode();
    if (opcode == 0x4d) {
        expression_eof = 1;
        return 0;
    }
    return decode_expression(opcode, function);
}

/* Original [0004fcb0,0004fcf0). A bad byte is diagnosed before returning it. */
unsigned char input_expression_opcode(void)
{
    unsigned char opcode;
    if (expression_eof) return 0x4d;
    opcode = input_byte();
    if (opcode == 0xff) compiler_report(1, "d:\\usr\\cuda\\src\\P2\\trees.c", 0x3dd);
    return opcode;
}
