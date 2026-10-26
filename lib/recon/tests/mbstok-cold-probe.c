#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static unsigned char *(__cdecl *tokenize)(unsigned char *,const unsigned char *)=_mbstok;
#else
#ifdef _mbstok
#error MSVCRT510 token alias leaked
#endif
static unsigned char *(__cdecl *tokenize)(unsigned char *,const unsigned char *)=recon_mbstok;
#endif
int main(void)
{
    unsigned char *memory=(unsigned char *)mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    unsigned a,b,n;pid_t child;int status;assert(memory!=MAP_FAILED);memset(memory,0xa5,4096);
    for(a=0;a<3;++a)for(b=0;b<3;++b){child=fork();assert(child>=0);
        if(!child){tokenize(a==0?NULL:a==1?(unsigned char *)1:memory,b==0?NULL:b==1?(const unsigned char *)1:memory+64);_exit(42);}
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert(memory[n]==0xa5);
    }
    assert(!munmap(memory,4096));return 0;
}
