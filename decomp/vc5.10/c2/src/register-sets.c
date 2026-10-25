#include "register-sets.h"
#include <recon.h>

#if defined(RECON_NATIVE_TEST) && defined(__linux__)
/* Host knowledge only: reset6, rather than a presumed original initializer,
 * makes this partial static slot available to the native reader. */
int register_set_pool_six_initialized;
#endif

/* Slot1 has a proved original NULL default. Slots4/6/12 are defined only by reset;
 * gaps0/2/3/5/7..11 are unmodeled storage, never original default claims. */
C2RegisterSet *free_register_sets[13];
C2RegisterSet *last_free_register_sets[13];
/* Separate node queues. Only pool1's two NULL pointer defaults are modeled. */
C2RegisterSetNode *free_register_set_nodes[13];
C2RegisterSetNode *last_free_register_set_nodes[13];

C2RegisterSet *__fastcall new_register_set(int pool)
{
    C2RegisterSet *set;
    set = free_register_sets[C2_REGISTER_SET_POOL(pool)];
    if (set)
        free_register_sets[C2_REGISTER_SET_POOL(pool)] = set->links.next_free;
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
