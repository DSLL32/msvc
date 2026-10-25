/* CL-specific external-contract process. Started after the IO checkpoint.
 * Shared recon-tool still owns case execution, timeouts, capture and reaping. */
#define RECON_SHIM_IMPL
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <recon.h>

static pid_t validator;
static int descriptor=-1;
static void finish_provider(void)
{
    int status;pid_t result;
    if (descriptor<0 || validator<=0) recon_unsupported("CL missing dependency validator");
    close(descriptor);descriptor=-1;
    do result=waitpid(validator,&status,0);while(result<0 && errno==EINTR);
    if (result!=validator || !WIFEXITED(status) || WEXITSTATUS(status))
        recon_unsupported("CL dependency validator rejected/missing/extra calls");
}
void cl_start_provider(void)
{
    const char *kind=getenv("CL_PROVIDER_KIND"),*script=getenv("CL_PROVIDER_SCRIPT");
    int sockets[2];char value[32];
    if (!kind) return;
    if (!script || (strcmp(kind,"minimal") && strcmp(kind,"mre") && strcmp(kind,"link")) || descriptor>=0 ||
        socketpair(AF_UNIX,SOCK_STREAM,0,sockets)) recon_unsupported("CL unknown dependency host configuration");
    validator=fork();if(validator<0) recon_unsupported("CL validator fork");
    if (!validator) {
        close(sockets[0]);sprintf(value,"%d",sockets[1]);
        execl("/usr/bin/python3","python3","-E",script,value,(char*)0);
        _exit(125);
    }
    close(sockets[1]);descriptor=sockets[0];sprintf(value,"%d",descriptor);
    if (fcntl(descriptor,F_SETFD,FD_CLOEXEC) || atexit(finish_provider)) recon_unsupported("CL validator lifetime");
    if (!strcmp(kind,"minimal")) {
        if (setenv("CL_DEPENDENCY_FD",value,1) || setenv("RECON_SPAWN_FD",value,1)) recon_unsupported("CL C1/C2 binding");
    } else if (!strcmp(kind,"mre")) {
        if (setenv("CL_MRE_FD",value,1)) recon_unsupported("CL MRE binding");
    } else if (setenv("RECON_SPAWN_FD",value,1)) recon_unsupported("CL LINK binding");
}
