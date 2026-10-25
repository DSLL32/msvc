#include <recon-tool.h>
#include <windows.h>
#include <stdlib.h>
#include <float.h>
#include "../src/link.h"
#include "../src/initializers.h"
#include "startup.h"
#include <recon.h>

Initializer application_initializers[] = {
    NULL,
    initialize_buffer12, initialize_buffer8, initialize_buffer4, initialize_timing
};

/* Verified MSVCRT startup operations; support only, no application credit. */
RECON_EXPORT int recon_link_setup(void)
{
 const char *profile = recon_host_getenv("RECON_SYSTEM_PROFILE");
 const char *reference = recon_host_getenv("RECON_SYSTEM_REFERENCE");
 if (profile || reference) recon_load_system_profile(profile,reference);
 return 1;
}
RECON_EXPORT int recon_link_invoke(int argc,char **argv)
{
 Initializer c_initializers[1];
 extern char **environ;
 c_initializers[0]=NULL;
 argv[0]=_pgmptr;
 _fmode=0; _commode=0;
 _controlfp(_PC_53,_MCW_PC);
 _initterm(c_initializers,c_initializers+1);
 _initterm(application_initializers,application_initializers+5);
 return link_main(argc,argv,environ);
}
