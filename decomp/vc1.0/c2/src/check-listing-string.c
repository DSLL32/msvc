/* Isolated listing probe: printf and ctype contracts are observed locally. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "listing.h"
#include <recon.h>
#undef printf
#undef sscanf
static char output[20000];
static unsigned used, classified;
static unsigned short classes[256];
unsigned short *recon_pctype = classes;
int recon_mb_cur_max;
int recon_isctype(int byte, int mask)
{
    if (byte < 0 || byte > 255 || mask != 0x157) abort();
    ++classified;
    return classes[byte] & mask;
}
int recon_printf(const char *format, ...)
{
    int count;
    va_list arguments;
    va_start(arguments, format);
    count = vsnprintf(output + used, sizeof(output) - used, format, arguments);
    va_end(arguments);
    if (count < 0 || used + count >= sizeof(output)) abort();
    used += count;
    return count;
}
void __fastcall write_listing_string(unsigned char *bytes, int count);
int main(int argc, char **argv)
{
    unsigned char bytes[1024];
    unsigned count, i, value;
    if (argc != 4 || strlen(argv[1]) > sizeof(bytes)*2 || strlen(argv[1])%2) return 2;
    count = strlen(argv[1])/2;
    for (i = 0; i < count; ++i) {
        if (sscanf(argv[1]+2*i, "%2x", &value) != 1) return 3;
        bytes[i] = value;
    }
    /* Only the tested printability mask is represented, for the C locale. */
    for (i = 32; i <= 126; ++i) classes[i] = 1;
    recon_mb_cur_max = atoi(argv[2]);
    listing_column = strtoul(argv[3], NULL, 10);
    write_listing_string(bytes, count);
    printf("%u %u\n", listing_column, classified);
    for (i = 0; i < used; ++i) printf("%02x", (unsigned char)output[i]);
    printf("\n");
    return 0;
}
