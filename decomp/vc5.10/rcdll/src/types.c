#include "directives.h"
#include <recon.h>

/* The interner indexes256 zero-initialized bucket pointers with hash &255. */
struct RcTypeEntry *rc_type_buckets[256];
