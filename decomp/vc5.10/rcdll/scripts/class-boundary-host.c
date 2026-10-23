/* Native-only probe host, outside the reconstructed function graph. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include "preprocess.h"
#include <recon.h>

int main(int count,char **arguments)
{
    unsigned short result;
    if(count!=2) return 2;
    result=rc_character_class((unsigned short)atoi(arguments[1]));
    printf("%u\n",result);
    return 0;
}
