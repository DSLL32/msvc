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

/* Original [00066710,00066740). Retain the sentinel and recycle only its
   member chunks; callers may reuse the returned empty set. */
BitSet *__fastcall clear_bitset(BitSet *set)
{
    if (!set) return 0;
    release_bitset(set->next);
    set->next = 0;
    return set;
}

/* Original [00066950,000669f0). Merge sorted member chunks into the
   destination sentinel; source-only chunks are copied rather than shared. */
BitSet *__fastcall bitset_union(BitSet *destination, BitSet *source)
{
    BitSet *previous, *chunk, *member, *inserted;
    if (!destination) return source ? copy_bitset(source) : 0;
    if (!source) return destination;
    previous = destination;
    member = source->next;
    while (member) {
        chunk = previous->next;
        if (!chunk) {
            previous->next = copy_bitset(member);
            break;
        }
        if (chunk->base == member->base) {
            chunk->bits |= member->bits;
            previous = chunk;
            member = member->next;
        } else if (chunk->base < member->base) previous = chunk;
        else {
            inserted = new_bitset_node();
            inserted->base = member->base;
            inserted->bits = member->bits;
            inserted->next = chunk;
            previous->next = inserted;
            previous = inserted;
            member = member->next;
        }
    }
    return destination;
}

/* Original [000665e0,00066610). Recycle the entire chain in one splice. */
void __fastcall release_bitset(BitSet *set)
{
    BitSet *tail;
    if (set) {
        for (tail = set; tail->next; tail = tail->next) ;
        tail->next = free_bitset_nodes;
        free_bitset_nodes = set;
    }
}
