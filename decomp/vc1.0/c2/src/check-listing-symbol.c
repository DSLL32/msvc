/* Isolated symbol-listing formatting and output-flag probe. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "listing.h"
#include <recon.h>
#undef printf
#undef vsnprintf
static char output[256];
static unsigned used;
int recon_printf(const char *format, ...)
{
    int count;
    va_list args;
    va_start(args, format);
    count = vsnprintf(output + used, sizeof(output) - used, format, args);
    va_end(args);
    if (count < 0 || used + count >= sizeof(output)) abort();
    used += count;
    return count;
}
char *__fastcall symbol_display_name(SymbolHeader *symbol) { return "display"; }
int main(int argc, char **argv)
{
    BackendSymbol symbol;
    unsigned i;
    if (argc != 7) return 2;
    memset(&symbol, 0, sizeof(symbol));
    symbol.symbol.kind = atoi(argv[2]);
    symbol.output_flags = strtoul(argv[3], NULL, 10);
    symbol.attributes = strtoul(argv[4], NULL, 10);
    symbol.symbol.field04 = strtoul(argv[5], NULL, 10);
    listing_column = 5;
    if (atoi(argv[1])) write_local_listing(&symbol.symbol);
    else write_listing_symbol(atoi(argv[6]) ? NULL : &symbol.symbol);
    printf("%u %u\n", listing_column, symbol.output_flags);
    for (i = 0; i < used; ++i) printf("%02x", (unsigned char)output[i]);
    printf("\n");
    return 0;
}
