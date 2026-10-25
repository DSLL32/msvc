#include <stdint.h>
#include <stdlib.h>
/* External test driver. No reconstructed graph or application credit. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <string.h>
#include "../../src/link.h"
#include <recon.h>
int *dispatcher_test_errno;
int main(int,char **);
int probe_main(int argc,char **argv)
{
    dispatcher_test_errno=&errno;
    if(argc!=2)return 120;
    if(!strcmp(argv[1],"--direct"))return driver_usage();
    if(!strcmp(argv[1],"-helper")){
        /* Same caller-owned argv0 as the captured CreateProcess command. */
        argv[0]=recon_pgmptr;
        return main(argc,argv);
    }
    return 120;
}

/* Direct test completion follows the original external driver return/CRT exit. */
int run_probe(void)
{
    char *args[] = {"LINK.EXE","--direct",NULL};
    int code=probe_main(2,args);
    recon_record_terminal_status((uint32_t)code);exit(code);
    return 0;
}
