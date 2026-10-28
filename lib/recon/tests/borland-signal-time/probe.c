#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <assert.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static int error_slot,calls;
int *recon_errno(void) {return &error_slot;}
static void __cdecl first(int n) {(void)n;++calls;__builtin_trap();}
static void __cdecl second(int n) {(void)n;++calls;__builtin_trap();}
static ULONG WINAPI user(EXCEPTION_RECORD *a,void *b,CONTEXT *c,void *d) {(void)a;(void)b;(void)c;(void)d;__builtin_trap();}
static BOOL WINAPI blocker(DWORD n) {(void)n;return TRUE;}
extern ReconSignalHandler consumer_signal(int,ReconSignalHandler);
extern ReconSignalHandler (*consumer_binding(void))(int,ReconSignalHandler);
static void fp(unsigned char v[108]) {__asm__ volatile("fnsave %0;frstor %0":"=m"(*(unsigned char (*)[108])v));}
int main(int argc,char **argv)
{
    if(argc>1 && (!strcmp(argv[1],"date") || !strcmp(argv[1],"time"))) {
        if(argv[1][0]=='d')recon_borland_400_getdate((void *)1);else recon_borland_400_gettime((void *)1);return 1;
    }
    if(argc>1 && !strcmp(argv[1],"no-write")) {
        unsigned char *shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);int which,status;unsigned n;pid_t child;
        assert(shared!=MAP_FAILED);memset(shared,0xa5,4096);
        for(which=0;which<2;++which) {child=fork();assert(child>=0);if(!child) {if(which)recon_borland_400_gettime((void *)shared);else recon_borland_400_getdate((void *)shared);_exit(1);}assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);for(n=0;n<4096;++n)assert(shared[n]==0xa5);}
        return 0;
    }
#ifdef RECON_BORLAND_400_SIGNAL
    {
        struct sigaction before,registered,after;unsigned char a[108],b[108];ReconBorland400UserHandler old;
        assert(consumer_binding()==recon_borland_400_signal);
        signal(SIGINT,SIG_IGN);signal(SIGQUIT,SIG_IGN);assert(!sigaction(SIGINT,NULL,&before) && before.sa_handler==SIG_IGN);
        old=recon_borland_400_SetUserHandler(user);
        errno=E2BIG;error_slot=123;recon_doserrno=456;recon_SetLastError(789);__asm__ volatile("fld1;fldpi":::"st");fp(a);
        if(argc>1 && !strcmp(argv[1],"number")) {consumer_signal(21,first);return 1;}
        if(argc>1 && !strcmp(argv[1],"error-handler")) {consumer_signal(2,SIG_ERR);return 1;}
        assert(consumer_signal(2,first)==SIG_DFL);assert(!sigaction(SIGINT,NULL,&registered));assert(registered.sa_handler!=SIG_IGN && registered.sa_handler!=first);
        if(argc>1 && !strcmp(argv[1],"delivery")) {raise(SIGINT);return 1;}
        if(argc>1 && !strcmp(argv[1],"break-delivery")) {raise(SIGQUIT);return 1;}
        assert(recon_borland_400_signal(2,second)==first);assert(consumer_signal(2,second)==second);
        assert(consumer_signal(2,SIG_IGN)==second);assert(consumer_signal(2,SIG_DFL)==SIG_IGN);assert(consumer_signal(2,first)==SIG_DFL);
        assert(!sigaction(SIGINT,NULL,&after) && after.sa_handler==registered.sa_handler);
        assert(recon_borland_400_SetUserHandler(old)==user);
        fp(b);assert(!memcmp(a,b,2) && !memcmp(a+4,b+4,2) && !memcmp(a+8,b+8,2) && !memcmp(a+28,b+28,20));
        assert(!calls && errno==E2BIG && error_slot==123 && recon_doserrno==456 && recon_GetLastError()==789);
        /* Replace the console owner independently; a repeated signal call must
         * not attempt lazy registration again or overwrite that owner's state. */
        assert(recon_SetConsoleCtrlHandler(NULL,FALSE));assert(recon_SetConsoleCtrlHandler(blocker,TRUE));recon_SetLastError(777);
        assert(consumer_signal(2,second)==first && recon_GetLastError()==777);
        __asm__ volatile("fstp %%st(0);fstp %%st(0)":::"st");return 0;
    }
#else
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    assert(consumer_binding()==recon_watcom_signal);
#else
    assert(consumer_binding()==recon_signal);
#endif
    recon_borland_400_signal(2,first);return 1;
#endif
}
