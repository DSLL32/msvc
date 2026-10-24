#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static void observe(uint64_t expected)
{
    uint64_t got;unsigned char before[108],after[108];
    errno=111;recon_doserrno=222;recon_SetLastError(333);
    memset(before,0,108);memset(after,0,108);
    __asm__ __volatile__("fld1\n\tfnsave %0\n\tfrstor %0":"=m"(before));
    got=recon_local_clock_call_count();
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(after));
    assert(!memcmp(before,after,108));
    __asm__ __volatile__("fstp %%st(0)":::"st");
    assert(got==expected && errno==111 && recon_doserrno==222 && recon_GetLastError()==333);
}
int main(int argc,char **argv)
{
    struct _timeb first,second,third;(void)argv;
    observe(0);
    setenv("TZ","UTC0",1);setenv("RECON_LOCAL_CLOCK","2026-10-24T00:05:27.848/823",1);
    memset(&first,0,sizeof(first));memset(&second,0,sizeof(second));
    recon_ftime(&first);observe(1);
    recon_ftime(&second);observe(2);observe(2);
    assert(first.time==1792800327 && first.millitm==848 && first.timezone==0 && first.dstflag==0);
    assert(second.time==1792800328 && second.millitm==671 && second.timezone==0 && second.dstflag==0);
    assert(((unsigned char *)&first)[10]==0 && ((unsigned char *)&first)[11]==0);
    assert(((unsigned char *)&second)[10]==0 && ((unsigned char *)&second)[11]==0);
    recon_ftime(&third);observe(3); /* Legacy clock remains unconstrained. */
    if(argc>1)return recon_local_clock_call_count()==2?0:42; /* Host finish rejects extra. */
    setenv("RECON_LOCAL_CLOCK","2026-10-24T00:05:27.848/0",1);
    recon_ftime(&third);observe(1);
    unsetenv("RECON_LOCAL_CLOCK");setenv("RECON_FIXED_TIME","1792800327",1);
    recon_ftime(&third);observe(0);
    puts("clock count read-only, sequence/reset/default/errors/FP pass");return 0;
}
