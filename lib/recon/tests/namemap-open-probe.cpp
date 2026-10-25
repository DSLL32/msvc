#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
struct PDB;
struct NameMap {
    static __declspec(dllimport) int __cdecl open(PDB *,int,NameMap **) RECON_MSPDB_NAMEMAP_OPEN;
};
static int (__cdecl *typed)(PDB *,int,NameMap **)=NameMap::open;
int main(void)
{
    unsigned mode,n;int status;pid_t child;NameMap **result;
    result=(NameMap **)mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    assert(result!=MAP_FAILED);memset(result,0xa5,4096);
    for(mode=0;mode<4;++mode) {
        child=fork();assert(child>=0);
        if(!child) {
            if(mode==0)NameMap::open(NULL,1,NULL);
            else if(mode==1)typed((PDB *)1,1,(NameMap **)1);
            else if(mode==2)NameMap::open((PDB *)1,1,result);
            else typed(NULL,-1,result);
            _exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert(((unsigned char *)result)[n]==0xa5);
    }
    assert(!munmap(result,4096));return 0;
}
