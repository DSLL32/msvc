#ifndef TREE_INPUT_H
#define TREE_INPUT_H
#include "function-input.h"
/* Only the arity and self-contained-record properties encountered by the
   postfix reader are defined here. Other original opcode flags are separate. */
typedef enum NodeShape {
    LEAF, UNARY, BINARY, CHAIN,
    COMPLETE_LEAF, COMPLETE_UNARY, COMPLETE_BINARY, COMPLETE_CHAIN
} NodeShape;
#define NODE_ARITY 3
#define NODE_COMPLETE 4
#define NODE_OPCODE_COUNT 182
extern const unsigned char node_shapes[NODE_OPCODE_COUNT];
extern int expression_eof;
unsigned char input_expression_opcode(void);
Node *__fastcall decode_expression(unsigned short opcode, Function *function);
Node *__fastcall read_expression_record(Function *function);
#endif
