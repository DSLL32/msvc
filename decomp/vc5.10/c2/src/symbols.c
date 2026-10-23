#include "c2.h"
#include <recon.h>

unsigned c2_symbol_hash_count;
C2Symbol *c2_symbol_hash[1024];

void c2_reset_symbol_table(void)
{
    c2_symbol_hash_count = 0;
    memset(c2_symbol_hash, 0, sizeof(c2_symbol_hash));
}
