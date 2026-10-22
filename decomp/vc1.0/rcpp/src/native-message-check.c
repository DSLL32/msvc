/* Standalone diagnostic catalog/ABI probe, never part of RCPP's graph. */
#include "preprocessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>
typedef char *(__stdcall *MessageLookup)(unsigned);
int main(int argc, char **argv)
{
    MessageLookup lookup;
    FILE *output;
    unsigned code, length, alias;
    char *text;
    if (argc != 2) return 2;
    lookup = rc_message;
    output = fopen(argv[1],"wb");
    if (!output) return 4;
    for (code = 0; code < 65538; ++code) {
        unsigned argument = code == 65537 ? (unsigned)-1 : code;
        text = lookup(argument);
        if (text != lookup(argument)) return 5;
        length = strlen(text);
        fwrite(&argument,sizeof(argument),1,output);
        fwrite(&length,sizeof(length),1,output);
        fwrite(text,1,length,output);
    }
    alias = lookup(2045) == lookup(2412);
    fwrite(&alias,sizeof(alias),1,output);
    alias = lookup(0) == lookup((unsigned)-1);
    fwrite(&alias,sizeof(alias),1,output);
    fclose(output);
    return 0;
}
