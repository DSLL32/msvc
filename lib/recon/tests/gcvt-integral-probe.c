#include <assert.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static char *(__cdecl *convert)(double,int,char *)=_gcvt;
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
struct SnapshotPair { unsigned char before[108],after[108]; };
static char *measure(const uint64_t *bits,char *output,struct SnapshotPair *state)
{
    char *result;
    /* A C double argument setup may itself use x87 FLD/FSTP. Push its exact
     * i386 cdecl words first, then snapshot only the public callee's effects. */
    __asm__ __volatile__("push %[out]\n\tpush $16\n\tpush 4(%[bits])\n\tpush (%[bits])\n\t"
        "fnsave (%[state])\n\tfrstor (%[state])\n\tcall *%[function]\n\t"
        "fnsave 108(%[state])\n\tfrstor 108(%[state])\n\tadd $16,%%esp"
        : "=a"(result) : [function]"0"(convert),[bits]"r"(bits),[out]"r"(output),[state]"r"(state)
        : "ecx","edx","memory","cc");
    return result;
}
int main(void)
{
    static const double values[]={0,1,7,-7,10,-10,1000000000,-1000000000,2147483647,-2147483648.0};
    static const char *expected[]={"0.","1.","7.","-7.","10.","-10.","1000000000.","-1000000000.","2147483647.","-2147483648."};
    union { uint64_t bits;double value; } rejected;
    char buffer[40],*output=buffer+9;unsigned n,row,index,pass,mode;
    struct SnapshotPair state;uint64_t argument;
    unsigned char *shared;int status;pid_t child;
    assert(sizeof(double)==8 && sizeof(int)==4 && convert==recon_gcvt);
    shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(shared!=MAP_FAILED);
    memset(shared,0xa5,4096);
#if !defined(RECON_CRT_PROFILE_MSVCRT_510) && !defined(RECON_CRT_PROFILE_WATCOM_100A)
    for(pass=0;pass<2;++pass) {
        if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(index=0;index<10;++index)for(row=0;row<3;++row) {
            memset(buffer,0xa5,sizeof(buffer));errno=111+row;recon_doserrno=211+row;SetLastError(311+row);
            memcpy(&argument,&values[index],sizeof(argument));
            assert(measure(&argument,output,&state)==output);
            assert(!strcmp(output,expected[index]) && !memcmp(state.before,state.after,108));
            assert(errno==111+(int)row && recon_doserrno==211+row && GetLastError()==311+row);
            for(n=0;n<9;++n)assert((unsigned char)buffer[n]==0xa5);
            for(n=9+strlen(expected[index])+1;n<sizeof(buffer);++n)assert((unsigned char)buffer[n]==0xa5);
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
#else
    (void)values;(void)expected;(void)buffer;(void)output;(void)row;(void)index;(void)pass;(void)state;(void)argument;(void)measure;
#endif
    (void)fp;
    for(mode=0;mode<17;++mode) {
        child=fork();assert(child>=0);
        if(!child) {
            double value=7;int digits=16;char *out=(char *)shared;
            switch(mode) {
            case 0:digits=15;break;
            case 1:out=NULL;break;
            case 2:out=(char *)1;break;
            case 3:out=(char *)(UINTPTR_MAX-5);break;
            case 4:value=1.25;break;
            case 5:rejected.bits=UINT64_C(0x8000000000000000);value=rejected.value;break;
            case 6:rejected.bits=1;value=rejected.value;break;
            case 7:rejected.bits=UINT64_C(0x7ff0000000000000);value=rejected.value;break;
            case 8:rejected.bits=UINT64_C(0x7ff8000000000000);value=rejected.value;break;
            case 9:value=2147483648.0;break;
            case 10:value=-2147483649.0;break;
            case 11:value=1e100;break;
            case 12:recon_lc_codepage=1252;break;
            case 13:recon_mb_cur_max=2;break;
            case 14:recon_mb_codepage=932;break;
            case 15:recon_mb_codepage_gate=1;break;
            case 16:recon_lc_handle[5]=1;break;
            }
            convert(value,digits,out);_exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert(shared[n]==0xa5);
    }
#if defined(RECON_CRT_PROFILE_MSVCRT_510) || defined(RECON_CRT_PROFILE_WATCOM_100A)
    child=fork();assert(child>=0);if(!child){convert(7,16,(char *)shared);_exit(42);}
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
    for(n=0;n<4096;++n)assert(shared[n]==0xa5);
#endif
    assert(!munmap(shared,4096));return 0;
}
