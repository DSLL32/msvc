#include <process.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static int (__cdecl *compare)(const void *,const void *,size_t)=memicmp;
static char *(__cdecl *command)(char *)=getcmd;
#else
#ifdef memicmp
#error Watcom memicmp alias leaked
#endif
#ifdef getcmd
#error Watcom getcmd alias leaked
#endif
static int (__cdecl *compare)(const void *,const void *,size_t)=recon_watcom_memicmp;
static char *(__cdecl *command)(char *)=recon_watcom_getcmd;
#endif
int main(void)
{
    unsigned char *memory;unsigned n,mode;pid_t child;int status;
    memory=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    assert(memory!=MAP_FAILED);memset(memory,0xa5,4096);
    for(mode=0;mode<8;++mode) {
        child=fork();assert(child>=0);
        if(!child) {
            switch(mode) {
            case 0:compare(NULL,NULL,0);break;
            case 1:compare((void *)1,(void *)1,(size_t)-1);break;
            case 2:compare(memory,memory+64,4);break;
            case 3:compare("ABC","abc",3);break;
            case 4:command(NULL);break;
            case 5:command((char *)1);break;
            case 6:command((char *)memory);break;
            case 7:compare(memory,memory,0);break;
            }
            _exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert(memory[n]==0xa5);
    }
    assert(!munmap(memory,4096));return 0;
}
