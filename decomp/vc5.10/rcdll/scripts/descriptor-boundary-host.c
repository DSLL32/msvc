/* Native-only guard probe. The number dependency is deliberately controlled;
 * this does not reconstruct or accept the number parser. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include "directives.h"
#include <recon.h>

static unsigned char number_result;
unsigned char __stdcall rc_read_number(WCHAR first)
{
    if(first!=L'1')exit(3);
    return number_result;
}
int main(int count,char **arguments)
{
    WCHAR source[]={L'1',L' ',0};
    unsigned char result;
    if(count!=2)return 2;
    number_result=(unsigned char)atoi(arguments[1]);
    rc_source_cursor=source;
    rc_cpp_active=0;
    result=rc_next_token();
    printf("%u %u\n",result,rc_token_kind);
    return 0;
}
