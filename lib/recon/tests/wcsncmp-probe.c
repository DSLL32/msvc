#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <recon.h>
typedef char CountIs32[(sizeof(size_t)==4)?1:-1];
static void fp(unsigned char b[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])b)); }
static void check(const WCHAR *left,const WCHAR *right,size_t count,int expected,int error,unsigned long dos,DWORD last)
{
    unsigned char before[108],after[108];int result;
    errno=error;recon_doserrno=dos;recon_SetLastError(last);
    fp(before);result=recon_wcsncmp(left,right,count);fp(after);
    assert(result==expected && errno==error && recon_doserrno==dos && recon_GetLastError()==last);
    assert(!memcmp(before,after,108));
}
int main(int argc,char **argv)
{
    WCHAR a[16],b[16],old_a[16],old_b[16],*guard_a,*guard_b;unsigned count,i,n,code;
    int expected,error;unsigned long dos,last;char *region;size_t page=(size_t)sysconf(_SC_PAGESIZE);
    if(argc>1 && !strcmp(argv[1],"null-left")){recon_wcsncmp(NULL,(const WCHAR *)1,1);return 9;}
    if(argc>1 && !strcmp(argv[1],"null-right")){recon_wcsncmp((const WCHAR *)1,NULL,1);return 9;}
    if(argc>1)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    while(scanf("%u %d %d %lu %lu",&count,&expected,&error,&dos,&last)==5){
        for(i=0;i<16;++i){assert(scanf("%x",&code)==1 && code<=65535);a[i]=(WCHAR)code;}
        for(i=0;i<16;++i){assert(scanf("%x",&code)==1 && code<=65535);b[i]=(WCHAR)code;}
        memcpy(old_a,a,sizeof(a));memcpy(old_b,b,sizeof(b));
        check(a,b,count,expected,error,dos,(DWORD)last);
        assert(!memcmp(a,old_a,sizeof(a)) && !memcmp(b,old_b,sizeof(b)));
    }
    /* Every unsigned code unit against a fixed high/low boundary checks full
     * return magnitude; errors and live/empty x87 are independently retained. */
    a[1]=b[1]=0;
    for(n=0;n<65536;++n){a[0]=(WCHAR)n;b[0]=(WCHAR)0xfffe;check(a,b,1,(int)n-65534,111,222,333);}
    region=mmap(NULL,page*4,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(region!=MAP_FAILED);
    assert(!mprotect(region+page,page,PROT_NONE) && !mprotect(region+page*3,page,PROT_NONE));
    guard_a=(WCHAR *)(region+page-2);guard_b=(WCHAR *)(region+page*3-2);
    *guard_a=*guard_b=0x8000;check(guard_a,guard_b,1,0,111,222,333);
    *guard_a=1;*guard_b=0xfffe;check(guard_a,guard_b,(size_t)-1,-65533,111,222,333);
    *guard_a=*guard_b=0;check(guard_a,guard_b,(size_t)-1,0,111,222,333);
    assert(!mprotect(region,page,PROT_READ) && !mprotect(region+page*2,page,PROT_READ));
    check(guard_a,guard_b,1,0,111,222,333);
    check((const WCHAR *)(region+page),(const WCHAR *)(region+page*3),0,0,111,222,333);
    check(NULL,NULL,0,0,111,222,333);check(NULL,guard_b,0,0,111,222,333);
    if(argc>1)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    assert(!munmap(region,page*4));
    puts("64 original rows/65536 unsigned returns/count0-guardpage-NUL/tails/errors/full108FP PASS");return 0;
}
