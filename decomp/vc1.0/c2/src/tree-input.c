#include "tree-input.h"
#include "input.h"
#include "options.h"
#include <recon.h>
/* Original opcode classification at 000857b8. Only the encountered tree properties
   are represented; this is not the original packed flag table. */
const unsigned short node_shapes[NODE_OPCODE_COUNT] = {
    /* 00 */ COMPLETE_LEAF,
    /* 01 */ COMPLETE_LEAF,
    /* 02 */ ASSOCIATIVE_ADDITIVE | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 03 */ ADDITIVE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 04 */ ASSOCIATIVE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 05 */ FOLDABLE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 06 */ FOLDABLE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 07 */ FOLDABLE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 08 */ FOLDABLE_UNARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 09 */ FOLDABLE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 0a */ FOLDABLE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 0b */ ASSOCIATIVE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 0c */ ASSOCIATIVE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 0d */ ASSOCIATIVE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 0e */ FOLDABLE_UNARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 0f */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 10 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 11 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 12 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 13 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 14 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 15 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 16 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 17 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 18 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 19 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 1a */ FOLDABLE_UNARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 1b */ BINARY,
    /* 1c */ BINARY,
    /* 1d */ FOLDABLE_BINARY | NODE_REFERENCE_ROOT,
    /* 1e */ FOLDABLE_BINARY | NODE_REFERENCE_ROOT,
    /* 1f */ BINARY | NODE_CAN_SWAP | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 20 */ BINARY | NODE_CAN_SWAP | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 21 */ BINARY | NODE_CAN_SWAP | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 22 */ BINARY | NODE_CAN_SWAP | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 23 */ BINARY | NODE_CAN_SWAP | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 24 */ BINARY | NODE_CAN_SWAP | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 25 */ UNARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 26 */ INTERNED_LEAF | NODE_VALUE_EXPRESSION,
    /* 27 */ BINARY,
    /* 28 */ BINARY,
    /* 29 */ COMPLETE_LEAF,
    /* 2a */ COMPLETE_LEAF,
    /* 2b */ COMPLETE_LEAF,
    /* 2c */ UNARY,
    /* 2d */ COMPLETE_LEAF,
    /* 2e */ CHAIN | NODE_REFERENCE_ROOT,
    /* 2f */ CHAIN | NODE_REFERENCE_ROOT,
    /* 30 */ MEMORY_UNARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 31 */ UNARY,
    /* 32 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 33 */ INTERNED_LEAF | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 34 */ FOLDABLE_UNARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 35 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 36 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 37 */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 38 */ COMPLETE_UNARY | NODE_REFERENCE_ROOT,
    /* 39 */ COMPLETE_UNARY | NODE_REFERENCE_ROOT,
    /* 3a */ COMPLETE_LEAF,
    /* 3b */ COMPLETE_UNARY | NODE_REFERENCE_ROOT,
    /* 3c */ COMPLETE_LEAF,
    /* 3d */ COMPLETE_LEAF,
    /* 3e */ CHAIN | NODE_SIDE_EFFECT | NODE_REFERENCE_ROOT,
    /* 3f */ CHAIN | NODE_SIDE_EFFECT | NODE_REFERENCE_ROOT,
    /* 40 */ CHAIN | NODE_SIDE_EFFECT | NODE_REFERENCE_ROOT,
    /* 41 */ COMPLETE_UNARY | NODE_REFERENCE_ROOT,
    /* 42 */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 43 */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 44 */ BINARY,
    /* 45 */ COMPLETE_UNARY | NODE_REFERENCE_ROOT,
    /* 46 */ COMPLETE_CHAIN,
    /* 47 */ COMPLETE_LEAF,
    /* 48 */ COMPLETE_CHAIN,
    /* 49 */ COMPLETE_LEAF,
    /* 4a */ COMPLETE_CHAIN,
    /* 4b */ COMPLETE_UNARY,
    /* 4c */ COMPLETE_INTERNED_LEAF,
    /* 4d */ COMPLETE_LEAF,
    /* 4e */ LEAF,
    /* 4f */ COMPLETE_LEAF,
    /* 50 */ UNARY | NODE_REFERENCE_ROOT,
    /* 51 */ LEAF,
    /* 52 */ COMPLETE_LEAF,
    /* 53 */ COMPLETE_LEAF,
    /* 54 */ COMPLETE_LEAF,
    /* 55 */ COMPLETE_UNARY | NODE_HAS_SECOND | NODE_REFERENCE_ROOT,
    /* 56 */ COMPLETE_LEAF,
    /* 57 */ COMPLETE_UNARY | NODE_HAS_SECOND | NODE_REFERENCE_ROOT,
    /* 58 */ UNARY,
    /* 59 */ UNARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 5a */ MEMORY_UNARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 5b */ LEAF,
    /* 5c */ LEAF,
    /* 5d */ LEAF,
    /* 5e */ LEAF,
    /* 5f */ LEAF,
    /* 60 */ LEAF,
    /* 61 */ LEAF,
    /* 62 */ LEAF,
    /* 63 */ LEAF,
    /* 64 */ LEAF,
    /* 65 */ LEAF,
    /* 66 */ LEAF,
    /* 67 */ LEAF,
    /* 68 */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 69 */ INTERNED_LEAF | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 6a */ LEAF,
    /* 6b */ LEAF,
    /* 6c */ LEAF,
    /* 6d */ LEAF,
    /* 6e */ LEAF,
    /* 6f */ LEAF,
    /* 70 */ LEAF,
    /* 71 */ LEAF,
    /* 72 */ LEAF,
    /* 73 */ LEAF,
    /* 74 */ FOLDABLE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 75 */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 76 */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 77 */ BINARY,
    /* 78 */ COMPLETE_CHAIN | NODE_SIDE_EFFECT | NODE_REFERENCE_ROOT,
    /* 79 */ FOLDABLE_UNARY | NODE_REFERENCE_ROOT,
    /* 7a */ CHAIN | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 7b */ BINARY | NODE_REFERENCE_ROOT,
    /* 7c */ BINARY | NODE_REFERENCE_ROOT,
    /* 7d */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 7e */ CHAIN | NODE_SIDE_EFFECT | NODE_REFERENCE_ROOT,
    /* 7f */ COMPLETE_LEAF,
    /* 80 */ LEAF,
    /* 81 */ ASSOCIATIVE_ADDITIVE | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 82 */ SWAPPABLE_ADDITIVE | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 83 */ ASSOCIATIVE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 84 */ FOLDABLE_UNARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 85 */ LEAF,
    /* 86 */ UNARY,
    /* 87 */ ASSIGNMENT | NODE_REFERENCE_ROOT,
    /* 88 */ UNARY,
    /* 89 */ UNARY,
    /* 8a */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 8b */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 8c */ LEAF,
    /* 8d */ COMPLETE_UNARY | NODE_REFERENCE_ROOT,
    /* 8e */ COMPLETE_LEAF,
    /* 8f */ COMPLETE_LEAF,
    /* 90 */ COMPLETE_LEAF,
    /* 91 */ COMPLETE_LEAF,
    /* 92 */ COMPLETE_LEAF,
    /* 93 */ COMPLETE_LEAF,
    /* 94 */ COMPLETE_LEAF,
    /* 95 */ COMPLETE_LEAF,
    /* 96 */ COMPLETE_LEAF,
    /* 97 */ COMPLETE_LEAF,
    /* 98 */ COMPLETE_LEAF,
    /* 99 */ BINARY,
    /* 9a */ BINARY,
    /* 9b */ INTERNED_LEAF,
    /* 9c */ BINARY,
    /* 9d */ COMPLETE_CHAIN,
    /* 9e */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* 9f */ LEAF,
    /* a0 */ LEAF,
    /* a1 */ LEAF | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* a2 */ INTERNED_LEAF | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* a3 */ LEAF,
    /* a4 */ UNARY,
    /* a5 */ LEAF,
    /* a6 */ BINARY | NODE_REFERENCE_ROOT,
    /* a7 */ UNARY,
    /* a8 */ LEAF,
    /* a9 */ LEAF,
    /* aa */ FOLDABLE_UNARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* ab */ FOLDABLE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* ac */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* ad */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* ae */ SWAPPABLE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* af */ SWAPPABLE_BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* b0 */ UNARY,
    /* b1 */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* b2 */ BINARY | NODE_REFERENCE_ROOT | NODE_VALUE_EXPRESSION,
    /* b3 */ LEAF,
    /* b4 */ LEAF,
    /* b5 */ NODE_BINARY
};

/* Original [0004efd0,0004f100). Build operand trees from postfix records,
   with explicit chains for argument lists and their end marker (4c). */
Node *__fastcall read_expression(Function *function)
{
    Node *node, *child;
    unsigned short shape;
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
            if (node->left.node && (node_shapes[node->opcode] & NODE_ARITY) == NODE_UNARY) continue;
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
