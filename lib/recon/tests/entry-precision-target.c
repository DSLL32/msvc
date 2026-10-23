#include <stdio.h>
#include <recon.h>

int recon_precision_entry(int argc,char **argv,char **environment)
{
    (void)argc; (void)argv; (void)environment;
    printf("precision=%u\n",_controlfp(0,0)&0x30000);
    return 0;
}
