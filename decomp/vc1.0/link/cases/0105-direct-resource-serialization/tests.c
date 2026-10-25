/* Unsupported original input pass remains stopped before its undefined seek. */
#define RECON_SHIM_IMPL
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <recon.h>
extern int link_native_serialize_legacy_resources(int,int,unsigned long);
int test_undefined_resource_input_denied(void)
{
    int status;pid_t child=fork();
    if(child<0)return 0;
    if(!child){link_native_serialize_legacy_resources(-1,-1,1);_exit(121);}
    return waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==125;
}
