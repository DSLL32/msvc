/* Native-only fallback guard probe, outside the reconstructed function graph. */
#define RECON_SHIM_IMPL
#include <stdlib.h>
#include "command.h"
#include "directives.h"
#include <recon.h>
int main(int count,char **arguments)
{
    unsigned value=1,token;
    if(count!=2)return 2;
    token=(unsigned)strtoul(arguments[1],NULL,0);
    rc_heap=recon_HeapCreate(1,0,0);
    return rc_store_integer(token,&value)!=NULL;
}
