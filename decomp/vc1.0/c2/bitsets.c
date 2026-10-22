#include "bitsets.h"
/* Original [00066dc0,00066e20). The original one-bit table is the ordinary
   32-bit shift below; signed base comparisons match chunk ordering. */
int __fastcall bitset_contains(BitSet *set, unsigned member)
{
    int base = (int)(member & ~31U);
    while (set && set->base < base) set = set->next;
    return set && set->base == base && (set->bits & (1U << (member & 31))) != 0;
}
BitSet *free_bitset_nodes; /* 0007b994 */
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

/* Original [00066610,00066620). Callers pass a capacity in EDX, which
   this sparse representation ignores. Retain that two-register interface. */
BitSet *__fastcall ensure_bitset(BitSet *set, int capacity)
{
    return set ? set : new_bitset();
}

/* Original [00066740,000667e0). Replace the member chain by all values
   below count. Existing sentinel fields survive; chunks are allocated anew. */
BitSet *__fastcall fill_bitset(BitSet *set, int count)
{
    BitSet *tail, *chunk;
    unsigned full, i, remaining;
    if (!set) set = new_bitset();
    release_bitset(set->next);
    set->next = 0;
    if (!count) return set;
    tail = set;
    full = (unsigned)(count >> 5);
    for (i = 0; i < full; ++i) {
        chunk = new_bitset_node();
        tail->next = chunk;
        tail = chunk;
        chunk->base = (int)(i << 5);
        chunk->bits = ~0U;
    }
    remaining = (unsigned)count & 31;
    if (remaining) {
        chunk = new_bitset_node();
        tail->next = chunk;
        chunk->base = (int)(full << 5);
        chunk->bits = ~(~0U << remaining);
    }
    return set;
}

/* Original [00066a90,00066b30). Intersect sorted member chains in place,
   recycling removed destination chunks and retaining its sentinel. */
BitSet *__fastcall intersect_bitset(BitSet *destination, BitSet *source)
{
    BitSet *previous, *chunk, *member;
    if (!destination) return 0;
    if (!source) {
        release_bitset(destination->next);
        destination->next = 0;
        return destination;
    }
    previous = destination;
    member = source->next;
    while (member) {
        chunk = previous->next;
        if (!chunk) break;
        if (chunk->base == member->base) {
            chunk->bits &= member->bits;
            if (!chunk->bits) {
                previous->next = chunk->next;
                chunk->next = 0;
                release_bitset(chunk);
            } else previous = chunk;
            member = member->next;
        } else if (chunk->base < member->base) {
            previous->next = chunk->next;
            chunk->next = 0;
            release_bitset(chunk);
        } else member = member->next;
    }
    if (!member) {
        release_bitset(previous->next);
        previous->next = 0;
    }
    return destination;
}

/* Original [000668f0,00066950). Compare member chunks, not sentinel
   fields. A null pointer equals only a null pointer, not an empty sentinel. */
int __fastcall equal_bitsets(BitSet *left, BitSet *right)
{
    if (!left || !right) return left == right;
    left = left->next;
    right = right->next;
    while (left) {
        if (!right || left->base != right->base || left->bits != right->bits) return 0;
        left = left->next;
        right = right->next;
    }
    return right == 0;
}

/* Original [00066670,00066710). Reuse destination chunks, append a
   copied tail when needed and recycle any surplus destination chunks. */
BitSet *__fastcall assign_bitset(BitSet *destination, BitSet *source)
{
    BitSet **link, *chunk;
    if (!source) {
        if (!destination) return 0;
        release_bitset(destination->next);
        destination->next = 0;
        return destination;
    }
    if (!destination) return copy_bitset(source);
    link = &destination;
    for (;;) {
        chunk = *link;
        if (!source) {
            release_bitset(chunk);
            *link = 0;
            return destination;
        }
        if (!chunk) {
            *link = copy_bitset(source);
            return destination;
        }
        chunk->base = source->base;
        chunk->bits = source->bits;
        link = &chunk->next;
        source = source->next;
    }
}
