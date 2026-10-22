#include "bitsets.h"
/* Original [00066dc0,00066e20). The original one-bit table is the ordinary
   32-bit shift below; signed base comparisons match chunk ordering. */
int __fastcall bitset_contains(BitSet *set, unsigned member)
{
    int base = (int)(member & ~31U);
    while (set && set->base < base) set = set->next;
    return set && set->base == base && (set->bits & (1U << (member & 31))) != 0;
}
static BitSet *free_bitset_nodes; /* 0007b994 */
/* Original [00066590,000665d0). Reused chunks and arena allocations both
   leave the allocator with all three fields cleared. */
BitSet *new_bitset_node(void)
{
    BitSet *node = free_bitset_nodes;
    if (node) free_bitset_nodes = node->next;
    else node = arena_alloc(6, sizeof(BitSet));
    node->next = 0;
    node->base = 0;
    node->bits = 0;
    return node;
}
/* Original [000665d0,000665e0). */
BitSet *new_bitset(void)
{
    BitSet *set = new_bitset_node();
    set->base = -1;
    return set;
}
/* Original [00066cf0,00066d50). Each chunk stores a sorted aligned base
   and one bit per member. The head sentinel is retained for empty sets. */
BitSet *__fastcall bitset_include(BitSet *set, unsigned member)
{
    int base = (int)(member & ~31U);
    unsigned bit = 1U << (member & 31);
    BitSet *previous, *node;
    if (!set) set = new_bitset();
    previous = set;
    while (previous->next) {
        node = previous->next;
        if (base < node->base) break;
        if (base == node->base) {
            node->bits |= bit;
            return set;
        }
        previous = node;
    }
    node = new_bitset_node();
    node->base = base;
    node->bits = bit;
    node->next = previous->next;
    previous->next = node;
    return set;
}
