#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static unsigned *policy=&_amblksiz;
static char *project(char __far *pointer){return pointer;}
static char *(*flat)(char *)=project;
#else
#ifdef _amblksiz
#error Watcom allocator alias leaked
#endif
#ifdef __far
#error Watcom far annotation leaked
#endif
#endif
int main(void)
{
    typedef char Unsigned32[(sizeof(unsigned)==4)?1:-1];
    (void)sizeof(Unsigned32);
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    char buffer[128];unsigned char before[108],after[108];
    unsigned char *shared;pid_t child;int status;unsigned mode,n;
    int (*typed_mode)(FILE *,char *,int,size_t)=setvbuf;
    int (*typed_terminal)(int)=isatty;
    assert(policy==&recon_watcom_amblksiz && _amblksiz==16 && flat(buffer)==buffer);
    assert(stdaux==&recon_iob[3] && stdprn==&recon_iob[4] && _IOLBF==0x0200);
    assert(typed_terminal==recon_isatty && typed_mode==recon_watcom_setvbuf);
    shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(shared!=MAP_FAILED);memset(shared,0xa5,4096);
    for(mode=0;mode<4;++mode) {
        child=fork();assert(child>=0);
        if(!child) {
            typed_mode(mode==0?stdaux:mode==1?stdprn:mode==2?(FILE *)1:(FILE *)shared,(char *)1,_IOLBF,128);
            _exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert(shared[n]==0xa5);
    }
    assert(!munmap(shared,4096));
    errno=111;recon_doserrno=222;recon_SetLastError(333);
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])before));
    _amblksiz=32;assert(recon_watcom_amblksiz==32);_amblksiz=16;
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])after));
    assert(!memcmp(before,after,108) && errno==111 && recon_doserrno==222 && recon_GetLastError()==333);
#else
    assert(_IOLBF==0x40 && setvbuf==recon_setvbuf);
#ifdef stdaux
#error Watcom stdaux leaked
#endif
#ifdef stdprn
#error Watcom stdprn leaked
#endif
#ifdef isatty
#error Watcom isatty alias leaked
#endif
#endif
    return 0;
}
