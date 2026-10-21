#include "tree-input.h"
#include "input.h"
/* Original opcode classification at 000857b8. Only the two shape properties
   used here are represented; this is not the original packed flag table. */
const unsigned char node_shapes[NODE_OPCODE_COUNT] = {
    /* 00 */ COMPLETE_BINARY, COMPLETE_BINARY, LEAF, LEAF, LEAF, LEAF, LEAF, LEAF,
    /* 08 */ UNARY, LEAF, LEAF, LEAF, LEAF, LEAF, UNARY, LEAF,
    /* 10 */ LEAF, LEAF, LEAF, LEAF, LEAF, LEAF, LEAF, LEAF,
    /* 18 */ LEAF, LEAF, UNARY, LEAF, LEAF, LEAF, LEAF, LEAF,
    /* 20 */ LEAF, LEAF, LEAF, LEAF, LEAF, UNARY, BINARY, LEAF,
    /* 28 */ LEAF, COMPLETE_BINARY, COMPLETE_BINARY, COMPLETE_BINARY, UNARY, COMPLETE_BINARY, CHAIN, CHAIN,
    /* 30 */ UNARY, UNARY, LEAF, BINARY, UNARY, LEAF, LEAF, LEAF,
    /* 38 */ COMPLETE_UNARY, COMPLETE_UNARY, COMPLETE_BINARY, COMPLETE_UNARY, COMPLETE_BINARY, COMPLETE_BINARY, CHAIN, CHAIN,
    /* 40 */ CHAIN, COMPLETE_UNARY, LEAF, LEAF, LEAF, COMPLETE_UNARY, COMPLETE_CHAIN, COMPLETE_BINARY,
    /* 48 */ COMPLETE_CHAIN, COMPLETE_BINARY, COMPLETE_CHAIN, COMPLETE_UNARY, COMPLETE_BINARY, COMPLETE_BINARY, BINARY, COMPLETE_BINARY,
    /* 50 */ UNARY, BINARY, COMPLETE_BINARY, COMPLETE_BINARY, COMPLETE_BINARY, COMPLETE_UNARY, COMPLETE_BINARY, COMPLETE_UNARY,
    /* 58 */ UNARY, UNARY, UNARY, BINARY, BINARY, BINARY, BINARY, BINARY,
    /* 60 */ BINARY, BINARY, BINARY, BINARY, BINARY, BINARY, BINARY, BINARY,
    /* 68 */ LEAF, BINARY, BINARY, BINARY, BINARY, BINARY, BINARY, BINARY,
    /* 70 */ BINARY, BINARY, BINARY, BINARY, LEAF, LEAF, LEAF, LEAF,
    /* 78 */ COMPLETE_CHAIN, UNARY, CHAIN, LEAF, LEAF, LEAF, CHAIN, COMPLETE_BINARY,
    /* 80 */ BINARY, LEAF, LEAF, LEAF, UNARY, BINARY, UNARY, LEAF,
    /* 88 */ UNARY, UNARY, LEAF, LEAF, BINARY, COMPLETE_UNARY, COMPLETE_BINARY, COMPLETE_BINARY,
    /* 90 */ COMPLETE_BINARY, COMPLETE_BINARY, COMPLETE_BINARY, COMPLETE_BINARY, COMPLETE_BINARY, COMPLETE_BINARY, COMPLETE_BINARY, COMPLETE_BINARY,
    /* 98 */ COMPLETE_BINARY, LEAF, LEAF, BINARY, LEAF, COMPLETE_CHAIN, LEAF, BINARY,
    /* a0 */ BINARY, BINARY, BINARY, BINARY, UNARY, BINARY, LEAF, UNARY,
    /* a8 */ BINARY, BINARY, UNARY, LEAF, LEAF, LEAF, LEAF, LEAF,
    /* b0 */ UNARY, LEAF, LEAF, BINARY, BINARY, LEAF
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
    if ((shape & NODE_ARITY) != BINARY) node->left.node = node->second.node = 0;
    if ((shape & NODE_ARITY) == BINARY && node->opcode != 0x4b && !(shape & NODE_COMPLETE)) {
        for (;;) {
            child = read_expression(function);
            shape = node_shapes[child->opcode];
            if ((shape & NODE_ARITY) != CHAIN && child->opcode != 0x3c) {
                if (!child->second.node && (shape & NODE_ARITY) != UNARY) child->second.node = node;
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
            if (node->left.node && (node_shapes[node->opcode] & NODE_ARITY) != UNARY) continue;
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
