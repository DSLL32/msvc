#include "register-sets.h"
#include <recon.h>

/* Only slot1 is encountered and its original NULL default is proved.
 * Slot0 exists solely to preserve the original indexing/address relation. */
C2RegisterSet *free_register_sets[2];

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
