#ifndef C2_SMALL_NODES_H
#define C2_SMALL_NODES_H
#include "function-reader.h"
typedef struct C2SmallNodeKnownSizes {
    unsigned unmodeled00, temporary_register, register_expression, implicit_register_expression, symbol, unmodeled05[2], integer, unmodeled08[3], binary_operation, unmodeled0c[2], transfer, unmodeled0f, call, unmodeled11[4], control, function_entry, unmodeled23, statement_boundary, symbol_expression;
} C2SmallNodeKnownSizes;
typedef union C2SmallNodeSizeView {
    C2SmallNodeKnownSizes known;
    unsigned values[26];
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
