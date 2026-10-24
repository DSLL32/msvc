#include <assert.h>
#include <string.h>
#include <sys/mman.h>
static int (*host_compare)(const char *,const char *)=strcmp;
#include <recon.h>
#ifdef RECON_CRT_PROFILE_MSVCRT_510
#ifndef strcmp
#error Missing explicit510 strcmp remap
#endif
#else
#ifdef strcmp
#error Default strcmp must retain host exposure
#endif
#endif
static int (*compare)(const char *,const char *)=strcmp;
static void save_fp(unsigned char b[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])b)); }
static void check(const unsigned char *a,const unsigned char *b,int expected,unsigned offset_a,unsigned offset_b)
{
    unsigned char left[160],right[160],old_left[160],old_right[160],before[108],after[108];
    int result;size_t x=strlen((const char *)a)+1,y=strlen((const char *)b)+1;
    assert(x+offset_a+8<160 && y+offset_b+8<160);
    memset(left,0xd1,160);memset(right,0xe2,160);
    memcpy(left+8+offset_a,a,x);memcpy(right+8+offset_b,b,y);
    memcpy(old_left,left,160);memcpy(old_right,right,160);
    errno=111;recon_doserrno=222;recon_SetLastError(0x12345678);
    save_fp(before);result=compare((const char *)left+8+offset_a,(const char *)right+8+offset_b);save_fp(after);
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    assert(result==expected);
#else
    assert((result>0)-(result<0)==expected);
#endif
    assert(!memcmp(before,after,108) && errno==111 && recon_doserrno==222 && recon_GetLastError()==0x12345678);
    assert(!memcmp(left,old_left,160) && !memcmp(right,old_right,160));
}
int main(int argc,char **argv)
{
    unsigned char a[100],b[100],before[108],after[108];unsigned x,y,n,i,j;int result;char *page;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    assert(compare==recon_strcmp);
#else
    assert(compare==host_compare);
#endif
    if(argc>1 && !host_compare(argv[1],"null-left")) {recon_strcmp(NULL,"x");return 42;}
    if(argc>1 && !host_compare(argv[1],"null-right")) {recon_strcmp("x",NULL);return 42;}
    if(argc>1 && !host_compare(argv[1],"null-both")) {recon_strcmp(NULL,NULL);return 42;}
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    if(argc>1 && !host_compare(argv[1],"explicit-default")) {recon_strcmp("x","y");return 42;}
#endif
    if(argc>1) __asm__ __volatile__("fld1\n\tfldpi":::"st");
    check((const unsigned char *)"-Ze",(const unsigned char *)"-Gs4096",1,0,0);
    check((const unsigned char *)"-Gs4096",(const unsigned char *)"-Ze",-1,0,0);
    for(x=0;x<256;++x)for(y=0;y<256;++y) {
        a[0]=(unsigned char)x;b[0]=(unsigned char)y;a[1]=b[1]=0;
        check(a,b,(x>y)-(x<y),0,0);
    }
    for(n=0;n<64;++n) for(i=0;i<4;++i) for(j=0;j<4;++j) {
        memset(a,'q',n);memset(b,'q',n);a[n]=b[n]=0;
        check(a,b,0,i,j);b[n]='x';b[n+1]=0;check(a,b,-1,i,j);
        a[n]=255;a[n+1]=0;check(a,b,1,i,j);
    }
    /* Defined strings terminate at a guard page. No input allocation tail is
     * needed by this bytewise API contract; original allocation tails unclaimed. */
    page=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(page!=MAP_FAILED);
    assert(!mprotect(page+4096,4096,PROT_NONE));page[4094]='A';page[4095]=0;
    errno=2;recon_doserrno=2;recon_SetLastError(0);
    save_fp(before);result=compare(page+4094,"A");save_fp(after);
    assert(!result && !memcmp(before,after,108) && errno==2 && recon_doserrno==2 && !recon_GetLastError());
    munmap(page,8192);
    if(argc>1) __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    puts("65536 unsigned pairs +3072 prefix/alignment cases +actual callback/guardpage/errors/full108FP PASS");return 0;
}
