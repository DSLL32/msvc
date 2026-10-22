/* Analysis-only ABI/edge-case check of the four original integer classifiers. */
#include "numbers.h"
#include <stdio.h>
#include <stdlib.h>
#include <recon.h>
typedef unsigned char (__stdcall *Classifier)(unsigned long);
int main(int argc, char **argv)
{
    static unsigned long edges[] = {0,1,9,32767,32768,65535,65536,
        0x7ffffffeUL,0x7fffffffUL,0x80000000UL,0xfffffffeUL,0xffffffffUL};
    static unsigned char expected[] = {7,7,8,9};
    Classifier function;
    Classifier functions[4];
    unsigned long value, seed, rva;
    unsigned i, j;
    (void)argc; (void)argv;
    functions[0] = rc_integer_nondecimal_kind;
    functions[1] = rc_integer_decimal_kind;
    functions[2] = rc_long_kind;
    functions[3] = rc_unsigned_kind;
    for (i = 0; i < 4; ++i) {
        function = functions[i];
        seed = 0x13579bdfUL;
        for (j = 0; j < 10012; ++j) {
            seed = seed * 1664525UL + 1013904223UL;
            value = j < 12 ? edges[j] : seed;
            if (function(value) != expected[i]) {
                printf("mismatch %u %08lx\n",i,value);
                return 1;
            }
        }
        printf("classifier %u: 10012 values returned %u\n",i,expected[i]);
    }
    return 0;
}
