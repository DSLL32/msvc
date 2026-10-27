/* Fixture-only entry adapter. Canonical native-bin excludes this object.
 * The reusable XI/FILE lifecycle is shared with root entry.c. */
#include "startup.h"
#include <stdlib.h>
#include <alloca.h>
#include <recon.h>
extern char **environ;
extern int main(void);

int wcc_native_startup(int argc, char **argv)
{
    unsigned i,count;
    char **environment;
    char *empty_argv[1];
    if (argc<1 || !argv || !argv[0]) abort();
    for (count=0;environ[count];++count) if (count>=4096) abort();
    environment=(char **)alloca((count+1)*sizeof(char *));
    /* Actual cstart PUSHes each forward-scanned pointer, reversing order. */
    for (i=0;i<count;++i) environment[i]=environ[count-1-i];
    environment[count]=0;
    /* Existing fixture scope: original ___Argv remains unconsumed NULL;
     * the shared binder requires a nonnull, terminated vector. */
    empty_argv[0]=0;
    recon_crt_bind_arguments(0,empty_argv,environment);
    return wcc_startup_run(argv[0],environment,main);
}
