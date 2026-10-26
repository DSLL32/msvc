#define RECON_SHIM_IMPL
#include <string.h>
#include "resources.h"
#include <recon-tool.h>
#include <recon.h>

static int cursor_width_owners(void)
{
    BYTE input[8]={255,255,0,0,0xaa,0xaa,0xcc,0xcc},saved[8],output[8];
    const BYTE expected[4]={255,0,0,0xaa};
    memcpy(saved,input,sizeof(input));memset(output,0x5a,sizeof(output));
    rc_reduce_cursor_width(input,output,4,2);
    if(memcmp(output,expected,4)||memcmp(input,saved,8)||
       output[4]!=0x5a||output[7]!=0x5a)return 0;
    rc_reduce_cursor_width(input,input,4,2);
    return !memcmp(input,expected,4)&&!memcmp(input+4,saved+4,4);
}
static int cursor_height_owners(void)
{
    WORD input[4]={0xffff,0xf0f0,0x0ff0,0x5555},saved[4],output[4];
    const WORD expected[2]={0xf0f0,0x0550};
    memcpy(saved,input,sizeof(input));memset(output,0xa5,sizeof(output));
    rc_reduce_cursor_height((BYTE *)input,(BYTE *)output,2,4,2);
    if(memcmp(output,expected,4)||memcmp(input,saved,8)||
       output[2]!=0xa5a5||output[3]!=0xa5a5)return 0;
    rc_reduce_cursor_height((BYTE *)input,(BYTE *)input,2,4,2);
    if(memcmp(input,expected,4)||memcmp(input+2,saved+2,4))return 0;
    rc_reduce_cursor_height((BYTE *)saved,(BYTE *)output,2,4,1);
    return !memcmp(output,saved,8);
}
const ReconTest compatibility_tests[]={
    {"cursor-width-source-destination",1,cursor_width_owners},
    {"cursor-height-source-destination",1,cursor_height_owners}
};
