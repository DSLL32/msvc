/* Resource-era FILE encoding heuristics; cdecl in this LINK binary. */
#include <ctype.h>
#include "resources.h"
#include <recon.h>

int detect_resource_encoding(FILE *stream)
{
    char sample[40];
    int total=0,readable=0,even_zero=0,odd_zero=0;
    int count,i,limit,printable,result=0;
    int byte_order=resource_byte_order();
    long position=ftell(stream);
    if (position%2) fgetc(stream);
    do {
        count=fread(sample,1,sizeof(sample),stream);
        for (i=0;i<count;++i) {
            if (!sample[i]) {
                if (i%2) ++odd_zero;
                else ++even_zero;
            }
            printable=MB_CUR_MAX>1 ? _isctype((signed char)sample[i],0x157) :
                _pctype[(signed char)sample[i]]&0x157;
            if (printable || sample[i]=='\t' || sample[i]=='\n' || sample[i]=='\r')
                ++readable;
        }
        total+=count;
        limit=(total*4)/5;
        if (readable>limit) result=1;
        else if (odd_zero>limit/2) result=byte_order==1?2:3;
        else if (even_zero>limit/2) result=byte_order==1?3:2;
    } while (count==sizeof(sample) && !result);
    fseek(stream,position,SEEK_SET);
    return result;
}

/* Both branches of the original nominal endian probe return one. */
int resource_byte_order(void)
{
    return 1;
}
