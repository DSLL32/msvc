#include "pack.h"
#include <recon.h>
unsigned public_scope_depth, discard_symbol_depth;
unsigned char *symbol_input;
int main(void)
{
    unsigned char record[12];
    unsigned i, pass;
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < 12; ++i) record[i] = (unsigned char)(0x40 + i);
        record[2] = 7; record[3] = 2;
        symbol_input = record; public_scope_depth = 7; discard_symbol_depth = pass ? 2 : 0;
        enter_symbol_scope();
        if (public_scope_depth != 8 || discard_symbol_depth != (pass ? 3 : 0)) return 1;
        if (record[2] != (pass ? 8 : 7) || record[3] != (pass ? 0 : 2)) return 2;
        for (i = 0; i < 12; ++i) if (i != 2 && i != 3 && record[i] != (unsigned char)(0x40 + i)) return 3;
    }
    return 0;
}
