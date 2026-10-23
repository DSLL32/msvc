#include <stdio.h>
#include "listing.h"
#include <recon.h>
#undef printf
int main(void)
{
    unsigned widths[] = {0, 1, 2, 3, 4, 6, 8, 10};
    unsigned w, reg, limit;
    char *name;
    for (w = 0; w < sizeof(widths) / sizeof(*widths); ++w) {
        limit = !widths[w] ? 24 : widths[w] == 1 ? 8 : 14;
        for (reg = 0; reg < limit; ++reg) {
            name = machine_register_name(reg, widths[w]);
            printf("%u %u %s\n", widths[w], reg, name ? name : "NULL");
        }
    }
    return 0;
}
