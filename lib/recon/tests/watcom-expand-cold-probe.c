#include <malloc.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && !defined(RECON_SHIM_IMPL)
static void *(__cdecl *expand)(void *,size_t)=_expand;
static void *(__cdecl *near_expand)(void *,size_t)=_nexpand;
#else
#ifdef _expand
#error Watcom _expand alias leaked
#endif
#ifdef _nexpand
#error Watcom _nexpand alias leaked
#endif
static void *(__cdecl *expand)(void *,size_t)=recon_watcom_expand;
static void *(__cdecl *near_expand)(void *,size_t)=recon_watcom_expand;
#endif
int main(void)
{
    unsigned char *memory=(unsigned char *)mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    unsigned row,column,n;size_t sizes[3]={0,16408,(size_t)-1};pid_t child;int status;
    assert(expand==recon_watcom_expand && near_expand==recon_watcom_expand && memory!=MAP_FAILED);
    memset(memory,0xa5,4096);
    for(row=0;row<3;++row)for(column=0;column<3;++column) {
        child=fork();assert(child>=0);if(!child) {
            void *pointer=row==0?NULL:row==1?(void *)1:memory;
            if(column&1)near_expand(pointer,sizes[column]);else expand(pointer,sizes[column]);
            _exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert(memory[n]==0xa5);
    }
    assert(!munmap(memory,4096));return 0;
}
