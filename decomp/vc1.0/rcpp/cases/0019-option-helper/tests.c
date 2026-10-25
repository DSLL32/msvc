/* Analysis-only ABI/edge-case check of the four original integer classifiers. */
#include "numbers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preprocessor.h"
#include <recon.h>
typedef unsigned char (__stdcall *Classifier)(unsigned long);
int classifiers(void)
{
    static unsigned long edges[] = {0,1,9,32767,32768,65535,65536,
        0x7ffffffeUL,0x7fffffffUL,0x80000000UL,0xfffffffeUL,0xffffffffUL};
    static unsigned char expected[] = {7,7,8,9};
    Classifier function;
    Classifier functions[4];
    unsigned long value, seed, rva;
    unsigned i, j;
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
                return 0;
            }
        }
        printf("classifier %u: 10012 values returned %u\n",i,expected[i]);
    }
    return 1;
}

/* Compare the complete independently captured original catalog stream. */
int messages(void)
{
    FILE *expected;
    unsigned code, argument, length, alias, saved;
    char *text;
    unsigned char buffer[1024];
    expected = fopen("expected-messages.bin", "rb");
    if (!expected) return 0;
    for (code = 0; code < 65538; ++code) {
        argument = code == 65537 ? (unsigned)-1 : code;
        text = rc_message(argument);
        if (text != rc_message(argument)) return 0;
        length = strlen(text);
        if (fread(&saved,4,1,expected)!=1 || saved!=argument) return 0;
        if (fread(&saved,4,1,expected)!=1 || saved!=length || length>sizeof(buffer)) return 0;
        if (fread(buffer,1,length,expected)!=length || memcmp(buffer,text,length)) return 0;
    }
    alias = rc_message(2045)==rc_message(2412);
    if (fread(&saved,4,1,expected)!=1 || saved!=alias) return 0;
    alias = rc_message(0)==rc_message((unsigned)-1);
    if (fread(&saved,4,1,expected)!=1 || saved!=alias) return 0;
    if (fgetc(expected)!=EOF) return 0;
    fclose(expected);
    return 1;
}
