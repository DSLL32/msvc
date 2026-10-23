/* External test driver. No reconstructed graph or application credit. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <string.h>
#include "../src/link.h"
#include <recon.h>
int *dispatcher_test_errno;
int __real_main(int,char **);
int __wrap_main(int argc,char **argv)
{
    dispatcher_test_errno=&errno;
    if(argc!=2)return 120;
    if(!strcmp(argv[1],"--direct"))return driver_usage();
    if(!strcmp(argv[1],"-helper")){
        /* Same caller-owned argv0 as the captured CreateProcess command. */
        argv[0]=recon_pgmptr;
        return __real_main(argc,argv);
    }
    return 120;
}
