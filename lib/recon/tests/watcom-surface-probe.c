#include <malloc.h>
#include <signal.h>
#include <setjmp.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <recon.h>

typedef char Buffer52[(sizeof(jmp_buf)==52)?1:-1];
typedef char UnsignedWords[__builtin_types_compatible_p(__typeof__(((jmp_buf *)0)[0][0]),unsigned int)?1:-1];
static void callback(int number) { (void)number; _exit(90); }
int main(void)
{
    unsigned int *buffer; int mode,status,i; pid_t child;
    ReconSignalHandler (*typed_signal)(int,ReconSignalHandler)=signal;
    int (*typed_jump)(unsigned int *)=_setjmp;
    int (__cdecl *typed_open)(const char *,int,...)=open;
    long (__cdecl *typed_seek)(int,long,int)=lseek;
    (void)typed_jump; (void)typed_signal;(void)typed_seek;
    assert(O_RDONLY==0 && O_BINARY==0x0200);
    assert(SIGABRT==1 && SIGFPE==2 && SIGILL==3 && SIGINT==4 && SIGSEGV==5);
    assert(SIGTERM==6 && SIGBREAK==7 && SIGUSR1==8 && SIGUSR2==9 && SIGUSR3==10);
    assert(SIGIDIVZ==11 && SIGIOVFL==12 && _SIGMIN==1 && _SIGMAX==12);
    assert((uintptr_t)SIG_IGN==1 && (uintptr_t)SIG_DFL==2 && (uintptr_t)SIG_ERR==3);
    assert(_HEAPOK==0 && _HEAPEMPTY==1 && _HEAPBADBEGIN==2 && _HEAPBADNODE==3 && _HEAPEND==4 && _HEAPBADPTR==5);
    buffer=mmap(NULL,52,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    assert(buffer!=MAP_FAILED); memset(buffer,0xa5,52);
    for (mode=0;mode!=9;++mode) {
        child=fork(); assert(child>=0);
        if (!child) {
            switch (mode) {
            case 0: (void)_heapchk(); break;
            case 1: (void)_nheapchk(); break;
            case 2: (void)signal(SIGFPE,callback); break;
            case 3: (void)_setjmp(buffer); break;
            case 4: (void)setjmp(buffer); break;
            case 5: longjmp(buffer,1); break;
            case 6: (void)raise(SIGINT); break;
            case 7: (void)typed_open(NULL,O_BINARY); break;
            case 8: (void)typed_open("unopened-*.image",O_RDONLY|O_BINARY); break;
            }
            _exit(91);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for (i=0;i!=13;++i) assert(buffer[i]==0xa5a5a5a5u);
    }
    assert(!munmap(buffer,52)); return 0;
}
