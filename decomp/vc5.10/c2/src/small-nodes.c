#include "small-nodes.h"
#include <recon.h>

/* Encountered prefix view, not the original table's whole extent. The first
 * unrelated rows are UNMODELED backing, with no zero-default or equality claim.
 * Kinds4/21/22 are 28-byte symbol/control/entry nodes; kind24 is a24-byte boundary. */
typedef struct C2SmallNodeKnownSizes {
    unsigned unmodeled00[4], symbol, unmodeled05[16], control, function_entry, unmodeled23, statement_boundary;
} C2SmallNodeKnownSizes;
typedef union C2SmallNodeSizeView {
    C2SmallNodeKnownSizes known;
    unsigned values[25];
} C2SmallNodeSizeView;
static const C2SmallNodeSizeView small_node_sizes = { { {0}, 28, {0}, 28, 28, 0, 24 } };
typedef char C2SmallNodeSizeWidthCheck[sizeof(small_node_sizes.values[0]) == 4 ? 1 : -1];
typedef char C2SmallNodeSizeOffsetCheck[
    offsetof(C2SmallNodeKnownSizes, statement_boundary) == 96 ? 1 : -1];

C2SmallNode *__fastcall allocate_small_node(unsigned char kind)
{
    C2SmallNode *node;
    unsigned bytes;
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
    if (kind != 4 && kind != 21 && kind != 22 && kind != 24)
        recon_unsupported("unmodeled C2 small-node size row");
#endif
    bytes = small_node_sizes.values[kind];
    node = free_small_nodes[bytes];
    if (node) {
        free_small_nodes[bytes] = node->next;
        memset(node, 0, bytes);
    } else
        node = c2_pool_allocate(1, bytes);
    node->kind = kind;
    return node;
}
