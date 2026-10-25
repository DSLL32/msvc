/* Original-backed narrow integer boundaries; no new public reach claim. */
#include <recon-tool.h>
#include "c2.h"
#include <recon.h>
extern int __cdecl subtraction_overflows(unsigned,unsigned,int,unsigned);
extern unsigned __fastcall format_integer(unsigned,char *,unsigned);
static int integers(void)
{
    static const unsigned rows[][5] = {
        {0,1,0,1,0}, {0x80,1,0,1,1}, {0x7f,~0U,0,1,1},
        {0x8000,1,0,2,1}, {0x80000000,1,0,4,1},
        {0,1,1,4,0}, {~0U,0,1,1,0}
    };
    unsigned i;
    for(i=0;i<RECON_COUNT(rows);++i)
        if(subtraction_overflows(rows[i][0],rows[i][1],rows[i][2],rows[i][3])!=(int)rows[i][4])return 0;
    return 1;
}
static int integer_names(void)
{
    char text[40];
    if(format_integer(0x80000000U,text,10)!=11||strcmp(text,"-2147483648"))return 0;
    if(format_integer(~0U,text,16)!=8||strcmp(text,"ffffffff"))return 0;
    return format_integer(173,text,10)==3&&!strcmp(text,"173");
}

const ReconTest c2_tests[] = {
    { "subtraction original width/borrow boundaries", 1, integers },
    { "original decimal/hexadecimal digit contract", 1, integer_names },
};
