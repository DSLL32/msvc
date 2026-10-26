#define RECON_WATCOM_SPRINTF_17E
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static int (__cdecl *format_value)(char *,const char *,...)=recon_watcom_sprintf_17e;
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static void check(double value,const char *expected)
{
    unsigned char output[64];unsigned i;int count;
    memset(output,0x5a,sizeof(output));count=format_value((char *)output,"%.17e",value);
    assert(count==(int)strlen(expected) && !memcmp(output,expected,count+1));
    for(i=count+1;i<sizeof(output);++i)assert(output[i]==0x5a);
}
static void exact_expected(char *output,unsigned numerator,unsigned fraction,int negative)
{
    uint64_t coefficient=numerator;char digits[32];unsigned n=0,i,used=0;
    int exponent;
    for(i=0;i<fraction;++i)coefficient*=5;
    do {digits[n++]=(char)('0'+coefficient%10);coefficient/=10;}while(coefficient);
    exponent=numerator?(int)n-(int)fraction-1:0;
    if(negative)output[used++]='-';
    output[used++]=digits[n-1];output[used++]='.';
    for(i=1;i<=17;++i)output[used++]=i<n?digits[n-1-i]:'0';
    output[used++]='e';output[used++]=exponent<0?'-':'+';
    if(exponent<0)exponent=-exponent;
    output[used++]=(char)('0'+exponent/100);
    output[used++]=(char)('0'+exponent/10%10);
    output[used++]=(char)('0'+exponent%10);output[used]=0;
}
#endif
int main(void)
{
    unsigned i,j;pid_t child;int status;
    unsigned char *shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    assert(shared!=MAP_FAILED);
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    assert(sprintf==recon_watcom_sprintf_17e);
    errno=111;*recon_errno()=211;recon_doserrno=311;SetLastError(411);
    check(.5,"5.00000000000000000e-001");
    check(1.25,"1.25000000000000000e+000");
    check(-2,"-2.00000000000000000e+000");
    check(0,"0.00000000000000000e+000");
    for(i=0;i<128;++i)for(j=0;j<=7;++j) {
        char expected[32];double value=(double)i/(double)(1U<<j);
        exact_expected(expected,i,j,0);check(value,expected);
        if(i){exact_expected(expected,i,j,1);check(-value,expected);}
    }
    {
        unsigned short cw0,cw1,sw0,sw1;double pi,one;
        __asm__ __volatile__("fld1\n\tfldpi\n\tfnstcw %0\n\tfnstsw %1":"=m"(cw0),"=m"(sw0)::"st");
        check(1.25,"1.25000000000000000e+000");
        __asm__ __volatile__("fnstcw %0\n\tfnstsw %1":"=m"(cw1),"=m"(sw1));
        assert(cw0==cw1 && (sw0&0x383f)==(sw1&0x383f));
        __asm__ __volatile__("fstpl %0\n\tfstpl %1":"=m"(pi),"=m"(one)::"st");
        assert(pi>3 && pi<4 && one==1);
    }
    assert(errno==111 && *recon_errno()==211 && recon_doserrno==311 && GetLastError()==411);
#else
    assert(sprintf!=recon_watcom_sprintf_17e);
#endif
    for(i=0;i<14;++i) {
        memset(shared,0x77,4096);child=fork();assert(child>=0);
        if(!child) {
            char *output=(char *)shared+256;const char *format="%.17e";double value=1.25;
            if(i==0)output=NULL;
            if(i==1)output=(char *)(UINTPTR_MAX-12);
            if(i==2)format=NULL;
            if(i==3)format="%d";
            if(i==4){format=(char *)shared;memcpy(shared,"%.17e",6);output=(char *)shared;}
            if(i==5)value=-0.0;
            if(i==6)value=.1;
            if(i==7){uint64_t raw=UINT64_C(0x7ff0000000000000);memcpy(&value,&raw,8);}
            if(i==8){uint64_t raw=1;memcpy(&value,&raw,8);}
            if(i==9)value=1e18;
            if(i==10)value=1.0/262144;
            if(i==11)recon_mb_cur_max=2;
            if(i==12)recon_mb_codepage_gate=1;
            if(i==13)recon_lc_handle[1]=1;
            format_value(output,format,value);_exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(j=0;j<4096;++j)assert(shared[j]==(i==4 && j<6?(unsigned char)"%.17e"[j]:0x77));
    }
    assert(!munmap(shared,4096));return 0;
}
