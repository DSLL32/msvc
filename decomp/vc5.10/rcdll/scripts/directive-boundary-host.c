/* Native-only probe host, outside the reconstructed function graph. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <string.h>
#include "directives.h"
#include <recon.h>

int main(int count,char **arguments)
{
    WCHAR unavailable[2]={0x101,0};
    unsigned char result;
    if(count!=2) return 2;
    result=rc_directive_code(!strcmp(arguments[1],"line") ? L"line" : unavailable);
    printf("%u\n",result);
    return 0;
}
