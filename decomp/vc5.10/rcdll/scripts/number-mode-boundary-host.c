/* Native-only mode guard probe, outside the reconstructed function graph. */
#define RECON_SHIM_IMPL
#include <stdlib.h>
#include "directives.h"
#include <recon.h>
int main(int count,char **arguments)
{
    WCHAR source[]={L' ',0};
    if(count!=2)return 2;
    rc_source_cursor=source;
    rc_number_buffer_active=atoi(arguments[1]);
    return rc_read_number(L'1');
}
