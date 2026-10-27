#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <recon.h>
struct PDB {
    static int __cdecl OpenValidate(char *,char *,char *,DWORD,DWORD,LONG *,char *,PDB **) RECON_MSPDB_PDB_OPENVALIDATE;
};
static int (__cdecl *pdb_typed)(char *,char *,char *,DWORD,DWORD,LONG *,char *,PDB **)=PDB::OpenValidate;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
extern "C" void __cdecl _adj_fdiv_r(void);
static void (__cdecl *divide_typed)(void)=_adj_fdiv_r;
#else
#ifdef _adj_fdiv_r
#error The original floating import must stay unmapped outside profile510.
#endif
static void (__cdecl *divide_typed)(void)=recon_adj_fdiv_r;
#endif
int main(void)
{
    unsigned char *owned;unsigned mode,n;pid_t child;int status;
    owned=(unsigned char *)mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    assert(owned!=MAP_FAILED);memset(owned,0xa5,4096);
    for(mode=0;mode<6;++mode) {
        child=fork();assert(child>=0);
        if(!child) {
            if(mode==0)PDB::OpenValidate(NULL,NULL,NULL,0,0,NULL,NULL,NULL);
            else if(mode==1)pdb_typed((char *)1,(char *)1,(char *)1,0xffffffff,0xffffffff,(LONG *)1,(char *)1,(PDB **)1);
            else if(mode==2)PDB::OpenValidate((char *)owned,(char *)owned,(char *)owned,7,9,(LONG *)(owned+256),(char *)(owned+512),(PDB **)(owned+1024));
            else if(mode==3)pdb_typed(NULL,NULL,NULL,0,0,(LONG *)owned,NULL,(PDB **)(owned+8));
            else if(mode==4)divide_typed();
            else {__asm__ __volatile__("fld1; fldpi":::"st");divide_typed();}
            _exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert(owned[n]==0xa5);
    }
    assert(!munmap(owned,4096));return 0;
}
