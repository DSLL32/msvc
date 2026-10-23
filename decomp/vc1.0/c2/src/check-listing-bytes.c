/* Isolated current C bodies with an explicit ASCII printf contract. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "listing.h"
#include <recon.h>
#undef printf
unsigned listing_column;
static char output[4096];
static unsigned used;
int recon_printf(const char *format, ...)
{
    int count;
    va_list arguments;
    va_start(arguments, format);
    count = vsnprintf(output + used, sizeof(output) - used, format, arguments);
    va_end(arguments);
    if (count < 0 || (unsigned)count >= sizeof(output) - used) abort();
    used += count;
    return count;
}
int main(int argc, char **argv)
{
    unsigned char bytes[20];
    unsigned i;
    if (argc != 5) return 2;
    for (i = 0; i < sizeof(bytes); ++i) bytes[i] = i * 13 + 7;
    listing_column = strtoul(argv[2], NULL, 0);
#ifdef CHECK_COMMENT_PREFIX
    if (write_listing_comment(atoi(argv[3])) != 0) abort();
#else
    if (atoi(argv[1]))
        write_listing_bytes(strtoul(argv[4], NULL, 0), atoi(argv[3]), bytes);
    else
        write_listing_spaces(atoi(argv[3]));
#endif
    printf("%u\n", listing_column);
    for (i = 0; i < used; ++i) printf("%02x", (unsigned char)output[i]);
    printf("\n");
    return 0;
}
