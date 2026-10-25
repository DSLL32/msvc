/* The original-valid LINNUM variant is an explicitly unsupported native route.
 * This control verifies the existing host-only stop before its stale consumer;
 * it neither delays free nor repairs target output. */
#define RECON_SHIM_IMPL
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <recon.h>
extern int probe_main(int,char **);
int test_stale_lines_denied(void)
{
    pid_t child=fork();int status;
    if(child<0)return 0;
    if(!child) {
        unsigned char bytes[2048];size_t n;FILE *input,*output;
        char *argv[]={"cvtomf","-g","-o","converted.obj","legacy.obj",NULL};
        input=fopen("unsupported-lines.obj","rb");output=fopen("legacy.obj","wb");
        if(!input||!output)_exit(120);
        while((n=fread(bytes,1,sizeof(bytes),input))!=0)
            if(fwrite(bytes,1,n,output)!=n)_exit(120);
        if(ferror(input)||fclose(input)||fclose(output))_exit(120);
        (void)probe_main(5,argv);
        _exit(121);
    }
    if(waitpid(child,&status,0)!=child)return 0;
    return WIFEXITED(status)&&WEXITSTATUS(status)==125;
}
