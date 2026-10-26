#include <assert.h>
#include <sys/wait.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static double (__cdecl *convert)(const char *,char **)=strtod;
#else
#ifdef strtod
#error Watcom strtod alias leaked
#endif
static double (__cdecl *convert)(const char *,char **)=recon_watcom_strtod;
#endif
static uint64_t value_bits(const char *text)
{ double value=convert(text,NULL);uint64_t bits;memcpy(&bits,&value,8);return bits; }
int main(void)
{
    const char *denied[]={"",".","-0.0","+1"," 1","1e0","1x","1..0","0.1","1.2","9007199254740993","100000000000000000","1.0000000000000001"};
    unsigned i,j;pid_t child;int status;
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    assert(convert==recon_watcom_strtod);
    errno=111;*recon_errno()=211;recon_doserrno=311;SetLastError(411);
    { unsigned short cw0,cw1,sw0,sw1;double result;
      __asm__ __volatile__("fld1\n\tfldpi\n\tfnstcw %0\n\tfnstsw %1":"=m"(cw0),"=m"(sw0)::"st");
      result=convert("1.25",NULL);assert(result==1.25);
      __asm__ __volatile__("fnstcw %0\n\tfnstsw %1":"=m"(cw1),"=m"(sw1));
      assert(cw0==cw1 && (sw0&0x383f)==(sw1&0x383f));
      __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
    assert(value_bits("1.25")==UINT64_C(0x3ff4000000000000));
    assert(value_bits("0.0")==0 && value_bits(".0")==0 && value_bits("00000")==0);
    assert(value_bits("9007199254740991")==UINT64_C(0x433fffffffffffff));
    assert(value_bits("9007199254740992")==UINT64_C(0x4340000000000000));
    /* Independent native division by powers of two gives exact arithmetic. */
    for(i=0;i<1024;++i)for(j=0;j<=7;++j) {
        unsigned denominator=1U<<j;uint64_t scaled=(uint64_t)i;unsigned k;
        char text[32];double expected=(double)i/(double)denominator;uint64_t bits;
        for(k=0;k<j;++k)scaled*=5;
        if(j)snprintf(text,sizeof(text),"%u.%0*llu",i/denominator,j,(unsigned long long)(scaled%((uint64_t[]){1,10,100,1000,10000,100000,1000000,10000000})[j]));
        else snprintf(text,sizeof(text),"%u",i);
        memcpy(&bits,&expected,8);assert(value_bits(text)==bits);
    }
    assert(errno==111 && *recon_errno()==211 && recon_doserrno==311 && GetLastError()==411);
#else
    (void)i;(void)j;(void)value_bits;
#endif
    for(i=0;i<sizeof(denied)/sizeof(*denied)+6;++i) {
        child=fork();assert(child>=0);if(!child) {
            const char *text=i<sizeof(denied)/sizeof(*denied)?denied[i]:"1";char *end=NULL;
            if(i==sizeof(denied)/sizeof(*denied))text=NULL;
            if(i==sizeof(denied)/sizeof(*denied)+1)text=(const char *)(UINTPTR_MAX-3);
            if(i==sizeof(denied)/sizeof(*denied)+2)end=(char *)1;
            if(i==sizeof(denied)/sizeof(*denied)+3)recon_lc_handle[1]=1;
            if(i==sizeof(denied)/sizeof(*denied)+4)recon_mb_cur_max=2;
            if(i==sizeof(denied)/sizeof(*denied)+5)recon_mb_codepage_gate=1;
            convert(text,end?&end:NULL);_exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
    }
    return 0;
}
