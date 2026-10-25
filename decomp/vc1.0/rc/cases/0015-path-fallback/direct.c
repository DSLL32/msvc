/* Existence-only original PE alias. The shared captured-process boundary replies
 * before execution; no fixture runs this Windows executable. */
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <recon.h>
int path_setup(void)
{
    char child[PATH_MAX];
    if(recon_host_path("R:\\toolchains\\1.0-nt\\bin\\RCPP.EXE",child,sizeof(child)))
        return 0;
    if(mkdir("../launch",0700) || mkdir("../preprocessor bin",0700))return 0;
    return symlink(child,"../preprocessor bin/rcpp.exe")==0;
}
