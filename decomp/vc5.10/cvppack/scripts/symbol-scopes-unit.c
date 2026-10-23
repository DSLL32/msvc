/* Native-only scope stack/list examples, not additional original oracles. */
#define RECON_SHIM_IMPL
#include "pack.h"
#include <recon.h>
#undef main

int main(void)
{
    unsigned char buffer[76];
    unsigned offsets[7] = { 4, 16, 32, 48, 52, 56, 72 };
    unsigned short lengths[7] = { 10, 14, 14, 2, 2, 14, 2 };
    unsigned short kinds[7] = { 5, 0x100a, 0x107, 6, 6, 0x100b, 6 };
    unsigned i;
    memset(buffer, 0, sizeof(buffer));
    for (i = 0; i < 7; ++i) {
        *(unsigned short *)(buffer + offsets[i]) = lengths[i];
        *(unsigned short *)(buffer + offsets[i] + 2) = kinds[i];
    }
    if (!link_symbol_scopes(buffer, sizeof(buffer)) ||
        *(unsigned *)(buffer + 8) != 16 ||
        *(unsigned *)(buffer + 20) || *(unsigned *)(buffer + 24) != 52 ||
        *(unsigned *)(buffer + 28) != 56 ||
        *(unsigned *)(buffer + 36) != 16 || *(unsigned *)(buffer + 40) != 48 ||
        *(unsigned *)(buffer + 60) || *(unsigned *)(buffer + 64) != 72) return 1;
    *(unsigned *)(buffer + 64) = 0;
    if (link_symbol_scopes(buffer, 72)) return 2;
    memset(buffer, 0, 8);
    *(unsigned short *)(buffer + 4) = 2;
    *(unsigned short *)(buffer + 6) = 6;
    if (!link_symbol_scopes(buffer, 8)) return 3;
    puts("nested next missing-end unmatched-end");
    return 0;
}
