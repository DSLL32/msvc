/* Isolated native size/directive probe, excluded from the compiler image. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "listing.h"
#include <recon.h>
#undef printf
static unsigned diagnostics;
void __fastcall compiler_internal_error(char *source, unsigned code)
{
    if (strcmp(source, "@(#)listing.c:1.76") || code != 0x514) abort();
    ++diagnostics;
}
char *__fastcall listing_data_directive(long bytes);
int main(int argc, char **argv)
{
    char *result;
    if (argc != 2) return 2;
    result = listing_data_directive(strtol(argv[1], NULL, 10));
    printf("%u %s\n", diagnostics, result ? result : "NULL");
    return 0;
}
