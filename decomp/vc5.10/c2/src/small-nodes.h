#ifndef C2_SMALL_NODES_H
#define C2_SMALL_NODES_H
#include "function-reader.h"
typedef struct C2SmallNodeKnownSizes {
    unsigned unmodeled00, temporary_register, register_expression, implicit_register_expression, symbol, final_address, aggregate, integer, real_expression, register_set_expression, optimizer_index, binary_operation, block_copy, input_call_record, transfer, select_expression, call, intrinsic_expression, unmodeled12, input_expression, unmodeled14, control, function_entry, function_exit, statement_boundary, symbol_expression, register_set_record, symbol_use, unmodeled1c, statement_link;
} C2SmallNodeKnownSizes;
typedef union C2SmallNodeSizeView {
    C2SmallNodeKnownSizes known;
    unsigned values[30];
} C2SmallNodeSizeView;
extern const C2SmallNodeSizeView small_node_sizes;
#include <recon.h>

/* Common prefix only. Node-specific payload sizes come from the kind table. */
struct C2SmallNode {
    C2SmallNode *next;
    unsigned opcode;
    unsigned char kind;
};
typedef char C2SmallNodeKindCheck[offsetof(C2SmallNode, kind) == 8 ? 1 : -1];
#endif
