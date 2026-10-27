#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static size_t (__cdecl *typed)(const char *)=_mbstrlen;
typedef char Signature[__builtin_types_compatible_p(__typeof__(&_mbstrlen),size_t (__cdecl *)(const char *))?1:-1];
#else
#ifdef _mbstrlen
#error The unqualified original-name binding must remain unchanged.
#endif
static size_t (__cdecl *typed)(const char *)=recon_mbstrlen;
#endif
int main(void)
{
    char *owned;unsigned mode,n;pid_t child;int status;
    owned=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    assert(owned!=MAP_FAILED);memset(owned,0xa5,4096);
    for(mode=0;mode<3;++mode) {
        child=fork();assert(child>=0);
        if(!child) {typed(mode==0?NULL:mode==1?(const char *)1:owned);_exit(42);}
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert((unsigned char)owned[n]==0xa5);
    }
    assert(!munmap(owned,4096));return 0;
}
