#include <assert.h>
#include <sys/wait.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static void (__cdecl *truncate_name)(char *,const char *,size_t)=TruncStFromSz;
#else
#ifdef TruncStFromSz
#error MSPDB TruncStFromSz alias leaked outside explicit510
#endif
static void (__cdecl *truncate_name)(char *,const char *,size_t)=recon_mspdb_TruncStFromSz;
#endif
int main(void)
{
    char destination[260],source[]="owned";
    char *outputs[]={NULL,(char *)1,destination};
    const char *inputs[]={NULL,(const char *)1,source};
    unsigned i,j;pid_t child;int status;
    assert(sizeof(size_t)==4);
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    assert(truncate_name==recon_mspdb_TruncStFromSz);
#endif
    for(i=0;i<3;++i)for(j=0;j<3;++j) {
        child=fork();assert(child>=0);
        if(!child) {truncate_name(outputs[i],inputs[j],i?SIZE_MAX:0);_exit(42);}
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
    }
    return 0;
}
