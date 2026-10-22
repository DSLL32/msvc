/* Analysis-only ABI/edge-case check of the four original integer classifiers. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
typedef unsigned char (__stdcall *Classifier)(unsigned long);
int main(int argc, char **argv)
{
    static unsigned long edges[] = {0,1,9,32767,32768,65535,65536,
        0x7ffffffeUL,0x7fffffffUL,0x80000000UL,0xfffffffeUL,0xffffffffUL};
    static unsigned char expected[] = {7,7,8,9};
    HMODULE image;
    Classifier function;
    unsigned long value, seed, rva;
    unsigned i, j;
    if (argc != 6) return 2;
    image = LoadLibraryExA(argv[1], NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!image) { printf("load-error %lu\n",GetLastError()); return 3; }
    for (i = 0; i < 4; ++i) {
        rva = strtoul(argv[2+i], NULL, 16);
        function = (Classifier)((char *)image+rva);
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
    FreeLibrary(image);
    return 0;
}
