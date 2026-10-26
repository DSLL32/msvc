#include "register-sets.h"
#include <recon.h>

C2RegisterSetNode *optimizer_iterator_node; /* 48e224 */
unsigned optimizer_iterator_bits;          /* 48e2a8 */
C2RegisterSet *optimizer_iterator_set;     /* 48e2ac */

unsigned __fastcall next_optimizer_symbol(C2RegisterSet *set)
{
    unsigned bit, number;
    if (set) {
        optimizer_iterator_set = set;
        optimizer_iterator_node = set->links.first;
        optimizer_iterator_bits = optimizer_iterator_node ? optimizer_iterator_node->bits : 0;
    } else if (!optimizer_iterator_bits) {
        if (!optimizer_iterator_node) return ~0U;
        optimizer_iterator_node = optimizer_iterator_node->next;
        if (!optimizer_iterator_node) return ~0U;
        optimizer_iterator_bits = optimizer_iterator_node->bits;
    }
    /* A zero-bit group ends this call; the original does not skip ahead to
     * another group until a subsequent continuation call. */
    for (bit = 0; bit < 32; ++bit) {
        if (optimizer_iterator_bits & (1U << bit)) {
            number = optimizer_iterator_node->group + bit;
            if (number != ~0U) optimizer_iterator_bits &= ~(1U << (number & 31));
            return number;
        }
    }
    return ~0U;
}

unsigned __fastcall optimizer_set_contains(C2RegisterSet *set, unsigned number)
{
    C2RegisterSetNode *node = set->links.first;
    unsigned group = number & ~31U;
    while (node && node->group < group) node = node->next;
    if (!node || node->group != group) return 0;
    return node->bits & (1U << (number & 31));
}

#if defined(RECON_NATIVE_TEST) && defined(__linux__)
/* Host knowledge only: reset6, rather than a presumed original initializer,
 * makes this partial static slot available to the native reader. */
int register_set_pool_three_initialized;
int register_set_pool_four_initialized;
int register_set_pool_six_initialized;
int register_set_pool_twelve_initialized;
int register_set_pool_thirteen_initialized;
int register_set_pool_fourteen_initialized;
#endif

/* Header slots1/8 have proved NULL defaults. Slots3/4/6/12/13/14 are defined by reset;
 * gaps0/2/5/7/9..11 are unmodeled storage, never original default claims. */
C2RegisterSet *free_register_sets[15];
C2RegisterSet *last_free_register_sets[15];
/* Separate node queues. Pool0/1 defaults and real reset-written slots are
 * admitted only by their corresponding guarded reader; no gap defaults. */
C2RegisterSetNode *free_register_set_nodes[15];
C2RegisterSetNode *last_free_register_set_nodes[15];

/* Both sparse and dense sets use this 12-byte header. The separate sparse
 * release routine returns its list nodes before tail-transferring here. */
void __fastcall release_optimizer_set_header(C2RegisterSet *set)
{
    unsigned pool = C2_REGISTER_SET_HEADER_POOL(set->pool);
    if (free_register_sets[pool])
        last_free_register_sets[pool]->links.next_free = set;
    else
        free_register_sets[pool] = set;
    last_free_register_sets[pool] = set;
    set->links.next_free = NULL;
    set->state &= 0xff000000U;
}

C2RegisterSet *__fastcall new_register_set(int pool)
{
    C2RegisterSet *set;
    /* This is the common header constructor; sparse node allocation keeps
     * its separate, narrower pool guard. Pool8 headers are already proved. */
    set = free_register_sets[C2_REGISTER_SET_HEADER_POOL(pool)];
    if (set)
        free_register_sets[C2_REGISTER_SET_HEADER_POOL(pool)] = set->links.next_free;
    else
        set = c2_pool_allocate(pool, sizeof(C2RegisterSet));
    set->pool = (unsigned char)pool;
    set->links.first = NULL;
    set->state &= 0xff000000U;
    return set;
}

void __fastcall add_register_to_set(C2RegisterSet *set, unsigned number)
{
    C2RegisterSetNode **place, *node;
    unsigned group;

    group = number & ~31U;
    place = &set->links.first;
    node = *place;
    while (node && node->group < group) {
        place = &node->next;
        node = *place;
    }
    if (!node || node->group != group) {
        node = new_register_set_node(set);
        node->group = group;
        node->next = *place;
        *place = node;
    }
    node->bits |= 1U << (number & 31);
}

C2RegisterSetNode *__fastcall new_register_set_node(C2RegisterSet *set)
{
    C2RegisterSetNode *node, *next, *last;
    unsigned pool;

    pool = set->pool;
    node = free_register_set_nodes[C2_REGISTER_SET_POOL(pool)];
    if (node)
        free_register_set_nodes[C2_REGISTER_SET_POOL(pool)] = node->next;
    else {
        node = c2_pool_allocate(pool, 4 * sizeof(C2RegisterSetNode));
        next = node + 1;
        last = node + 3;
        if (!free_register_set_nodes[C2_REGISTER_SET_POOL(pool)]) {
            free_register_set_nodes[C2_REGISTER_SET_POOL(pool)] = next;
            last_free_register_set_nodes[C2_REGISTER_SET_POOL(pool)] = next;
        }
        while (next <= last) {
            last_free_register_set_nodes[C2_REGISTER_SET_POOL(pool)]->next = next;
            last_free_register_set_nodes[C2_REGISTER_SET_POOL(pool)] = next;
            ++next;
        }
    }
    node->group = 0;
    node->next = NULL;
    node->bits = 0;
    return node;
}

void __fastcall clear_register_set(C2RegisterSet *set)
{
    C2RegisterSetNode *node;
    unsigned pool;

    node = set->links.first;
    if (!node)
        return;
    pool = set->pool;
    if (free_register_set_nodes[C2_REGISTER_SET_POOL(pool)])
        last_free_register_set_nodes[C2_REGISTER_SET_POOL(pool)]->next = node;
    else
        free_register_set_nodes[C2_REGISTER_SET_POOL(pool)] = node;
    while (node->next)
        node = node->next;
    last_free_register_set_nodes[C2_REGISTER_SET_POOL(pool)] = node;
    set->links.first = NULL;
}

void __fastcall remove_register_set_node(C2RegisterSet *set, C2RegisterSetNode *node)
{
    C2RegisterSetNode *previous;
    unsigned pool;

    pool = C2_REGISTER_SET_POOL(set->pool);
    for (previous = set->links.first; previous && previous->next != node;
         previous = previous->next) ;
    if (previous) previous->next = node->next;
    else set->links.first = node->next;
    node->next = NULL;
    if (free_register_set_nodes[pool]) {
        last_free_register_set_nodes[pool]->next = node;
        last_free_register_set_nodes[pool] = node;
    } else {
        free_register_set_nodes[pool] = node;
        last_free_register_set_nodes[pool] = node;
    }
}

unsigned __fastcall test_and_remove_region_register(C2RegisterSet *set, unsigned number)
{
    C2RegisterSetNode *node;
    unsigned group = number & ~31U, mask, previous;
    for (node = set->links.first; node && node->group < group; node = node->next) ;
    if (!node || node->group != group) return 0;
    mask = 1U << (number & 31);
    previous = node->bits;
    node->bits = previous & ~mask;
    if (!node->bits) remove_register_set_node(set, node);
    return previous & mask;
}

unsigned __fastcall test_and_add_global_register(C2RegisterSet *set, unsigned number)
{
    C2RegisterSetNode *node, **link;
    unsigned group, mask, previous;

    group = number & ~31U;
    link = &set->links.first;
    for (node = *link; node && node->group < group; node = node->next)
        link = &node->next;
    if (!node || node->group != group) {
        node = new_register_set_node(set);
        node->group = group;
        node->next = *link;
        *link = node;
    }
    mask = 1U << (number & 31);
    previous = node->bits;
    node->bits |= mask;
    return previous & mask;
}

/* Compare sparse groups without changing either set or the enumeration cursor. */
int __fastcall optimizer_symbol_sets_intersect(C2RegisterSet *left, C2RegisterSet *right)
{
    C2RegisterSetNode *first = right->links.first, *match = NULL, *node;

    if (!first) return 0;
    for (node = left->links.first; node; node = node->next) {
        if (node->group >= first->group) {
            if (!match) match = first;
            while (match && match->group != node->group) match = match->next;
        } else match = NULL;
        if (match && (node->bits & match->bits)) return 1;
    }
    return 0;
}

unsigned __fastcall count_optimizer_symbols(C2RegisterSet *set)
{
    C2RegisterSetNode *node;
    unsigned count = 0, bits;

    for (node = set->links.first; node; node = node->next) {
        bits = node->bits;
        if (!bits) continue;
        bits = (bits & 0x55555555U) + ((bits >> 1) & 0x55555555U);
        bits = (bits & 0x33333333U) + ((bits >> 2) & 0x33333333U);
        bits = (bits & 0x0f0f0f0fU) + ((bits >> 4) & 0x0f0f0f0fU);
        bits = (bits & 0x00ff00ffU) + ((bits >> 8) & 0x00ff00ffU);
        count += (bits & 0xffff) + (bits >> 16);
    }
    return count;
}
