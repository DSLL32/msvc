#include "small-nodes.h"
#include "code-input.h"
#include <recon.h>

/* Encountered prefix view, not the original table's whole extent. The first
 * unrelated rows are UNMODELED backing, with no zero-default or equality claim.
 * Kinds2/3/4/21/22 are 28-byte register/symbol/control/entry nodes; kind24
 * is a 24-byte boundary; integer kind7 is 32 bytes. */
typedef struct C2SmallNodeKnownSizes {
    unsigned unmodeled00[2], register_expression, implicit_register_expression, symbol, unmodeled05[2], integer, unmodeled08[13], control, function_entry, unmodeled23, statement_boundary;
} C2SmallNodeKnownSizes;
typedef union C2SmallNodeSizeView {
    C2SmallNodeKnownSizes known;
    unsigned values[25];
} C2SmallNodeSizeView;
static const C2SmallNodeSizeView small_node_sizes = { { {0}, 28, 28, 28, {0}, 32, {0}, 28, 28, 0, 24 } };
typedef char C2SmallNodeSizeWidthCheck[sizeof(small_node_sizes.values[0]) == 4 ? 1 : -1];
typedef char C2SmallNodeSizeOffsetCheck[
    offsetof(C2SmallNodeKnownSizes, statement_boundary) == 96 ? 1 : -1];

C2SmallNode *__fastcall allocate_small_node(unsigned char kind)
{
    C2SmallNode *node;
    unsigned bytes;
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
    if (kind != 2 && kind != 3 && kind != 4 && kind != 7 && kind != 21 && kind != 22 && kind != 24)
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

void __fastcall release_small_node(void *allocation)
{
    C2Expression *tree = allocation, *child, *next;
    C2SmallNode *node = allocation;
    C2InputUse *use;
    unsigned kind = node->kind, bytes;
    switch (kind) {
    case 5: case 6:
        if (tree->aggregate_value) release_small_node(tree->aggregate_value);
        if (tree->aggregate_second) release_small_node(tree->aggregate_second);
        break;
    case 11: case 12: case 13: case 14: case 15: case 17:
        goto release_children;
    case 16:
        if (tree->left->kind == 4) {
            use = ((C2InputSymbol *)tree->left->value.symbol)->input_uses;
            if (use) unlink_input_use(use, tree);
        }
        /* Fall through to the auxiliary-list and ordinary children. */
    case 20:
        release_small_node_list((C2SmallNode *)tree->detail.data);
        goto release_children;
    case 18:
        unlink_input_use(tree->detail.use, tree);
        release_small_node_list((C2SmallNode *)tree->aggregate_value);
        goto release_children;
    case 19:
        if (tree->opcode == 0x16d)
            release_input_exception_context(tree);
        else if (tree->opcode == 0x171)
            release_input_assembly_context(tree);
        goto release_children;
    case 22:
        release_small_node_list((C2SmallNode *)tree->left);
        break;
    case 25:
        release_small_node_list((C2SmallNode *)tree->tail.right);
        tree->declaration->input_operand = NULL;
        break;
    case 28:
        unlink_input_use(tree->value.use, tree);
        break;
    default:
        break;
    }
    goto recycle;
release_children:
    for (child = tree->left; child; child = next) {
        next = child->next;
        if (((1U << (child->kind & 31)) & 0x60U) &&
            !(child->location.attributes & 0x2000)) {
            child->aggregate_value = NULL;
            child->aggregate_second = NULL;
        }
        release_small_node(child);
    }
    for (child = tree->tail.right; child; child = next) {
        next = child->next;
        if (((1U << (child->kind & 31)) & 0x60U) &&
            !(child->location.attributes & 0x2000)) {
            child->aggregate_value = NULL;
            child->aggregate_second = NULL;
        }
        release_small_node(child);
    }
recycle:
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
    if (kind != 2 && kind != 3 && kind != 4 && kind != 7 && kind != 21 && kind != 22 && kind != 24)
        recon_unsupported("unmodeled C2 small-node size row");
#endif
    bytes = small_node_sizes.values[kind];
    node->next = free_small_nodes[bytes];
    free_small_nodes[bytes] = node;
}
