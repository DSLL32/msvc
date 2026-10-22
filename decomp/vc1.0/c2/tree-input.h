#ifndef TREE_INPUT_H
#define TREE_INPUT_H
#include "function-input.h"
/* Define opcode properties only as their uses are encountered. */
typedef enum NodeArity { NODE_BINARY, NODE_UNARY, NODE_LEAF, NODE_CHAIN } NodeArity;
#define NODE_ARITY 3
#define NODE_COMPLETE 4
#define NODE_HAS_SECOND 8
#define NODE_WRITES_LEFT 16
#define NODE_SIDE_EFFECT 32
#define NODE_ASSOCIATIVE 64
#define NODE_CAN_SWAP 128
#define NODE_CONSTANT_FOLD 256
#define NODE_ADDITIVE 512
typedef enum NodeShape {
    BINARY = NODE_BINARY | NODE_HAS_SECOND,
    UNARY = NODE_UNARY, LEAF = NODE_LEAF,
    CHAIN = NODE_CHAIN | NODE_HAS_SECOND,
    COMPLETE_BINARY = BINARY | NODE_COMPLETE,
    COMPLETE_UNARY = UNARY | NODE_COMPLETE,
    COMPLETE_LEAF = LEAF | NODE_COMPLETE,
    COMPLETE_CHAIN = CHAIN | NODE_COMPLETE,
    ASSIGNMENT = BINARY | NODE_WRITES_LEFT | NODE_SIDE_EFFECT,
    FOLDABLE_BINARY = BINARY | NODE_CONSTANT_FOLD,
    FOLDABLE_UNARY = UNARY | NODE_CONSTANT_FOLD,
    ASSOCIATIVE_BINARY = FOLDABLE_BINARY | NODE_ASSOCIATIVE | NODE_CAN_SWAP,
    ADDITIVE_BINARY = FOLDABLE_BINARY | NODE_ADDITIVE,
    ASSOCIATIVE_ADDITIVE = ASSOCIATIVE_BINARY | NODE_ADDITIVE,
    SWAPPABLE_BINARY = FOLDABLE_BINARY | NODE_CAN_SWAP,
    SWAPPABLE_ADDITIVE = ADDITIVE_BINARY | NODE_CAN_SWAP
} NodeShape;
#define NODE_OPCODE_COUNT 182
extern const unsigned short node_shapes[NODE_OPCODE_COUNT];
extern int expression_eof;
unsigned char input_expression_opcode(void);
Node *__fastcall decode_expression(unsigned short opcode, Function *function);
Node *__fastcall read_expression_record(Function *function);
#endif
