#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static void check(WCHAR *text,size_t units,WCHAR needle,int offset,int error,DWORD last)
{
    WCHAR copy[16],*result;unsigned char before[108],after[108];
    assert(units<=16);memcpy(copy,text,units*sizeof(WCHAR));
    errno=error;recon_doserrno=93;recon_SetLastError(last);
    memset(before,0,108);memset(after,0,108);
    __asm__ __volatile__("fld1\n\tfnsave %0\n\tfrstor %0":"=m"(before));
    result=recon_wcschr(text,needle);
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(after));
    assert(!memcmp(before,after,108));
    __asm__ __volatile__("fstp %%st(0)":::"st");
    assert(result==(offset<0?NULL:text+offset));
    assert(!memcmp(copy,text,units*sizeof(WCHAR)));
    assert(errno==error && recon_doserrno==93 && recon_GetLastError()==last);
}
int main(int argc,char **argv)
{
    WCHAR input[]={92,58,47,0,46,101,119,0,0x1234,0xabcd};
    WCHAR needles[]={92,58,47,46,101,119,0};
    int offsets[]={0,1,2,-1,-1,-1,3},errors[]={0,111};
    DWORD lasts[]={0,0x12345678};
    WCHAR repeated[]={0x1234,0xabcd,0x1234,0,0xabcd},empty[]={0,0xabcd};
    unsigned i,j,k;
    (void)argv;
    if(argc!=1){recon_wcschr(NULL,0);return 9;}
    assert(sizeof(WCHAR)==2);
    for(i=0;i<2;++i)for(j=0;j<2;++j)for(k=0;k<7;++k)
        check(input,10,needles[k],offsets[k],errors[j],lasts[i]);
    check(repeated,5,0x1234,0,111,0x12345678);
    check(repeated,5,0xabcd,1,111,0x12345678);
    check(repeated,5,0,3,111,0x12345678);
    check(repeated,5,0x5678,-1,111,0x12345678);
    check(empty,2,0,0,111,0x12345678);
    check(empty,2,0xabcd,-1,111,0x12345678);
    puts("28 canonical rows and six code-unit/termination controls passed");
    return 0;
}
