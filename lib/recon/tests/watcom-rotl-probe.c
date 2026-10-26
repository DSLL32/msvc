#include <assert.h>
#include <limits.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static unsigned (__cdecl *rotate)(unsigned,unsigned)=_rotl;
#else
#ifdef _rotl
#error Watcom rotate alias leaked
#endif
static unsigned (__cdecl *rotate)(unsigned,unsigned)=recon_watcom_rotl;
#endif
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(void)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    static const unsigned values[]={0,1,0x80000000U,UINT_MAX,0x12345678U,0xf000};
    unsigned row,index,count,n,expected,pass;unsigned char before[108],after[108];
    assert(sizeof(unsigned)==4 && rotate==recon_watcom_rotl && rotate(0xf000,4)==0xf0000);
    for(pass=0;pass<2;++pass){
        if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(row=0;row<3;++row)for(index=0;index<6;++index)for(count=0;count<=256;++count){
            unsigned shift=count==256?UINT_MAX:count;
            expected=values[index];for(n=0;n<(shift&31);++n)expected=expected*2+(expected>>31);
            errno=111+row;*recon_errno()=211+row;recon_doserrno=311+row;SetLastError(411+row);fp(before);
            assert(rotate(values[index],shift)==expected);fp(after);
            assert(errno==111+(int)row && *recon_errno()==211+(int)row && recon_doserrno==311+row && GetLastError()==411+row && !memcmp(before,after,108));
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
    return 0;
#else
    (void)fp;rotate(0xf000,4);return 42;
#endif
}
