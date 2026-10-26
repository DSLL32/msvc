#include <assert.h>
#include <recon.h>
static unsigned char *range;static size_t range_count,range_width;static unsigned calls;
static int compare(const void *left,const void *right)
{
    uintptr_t a=(uintptr_t)left-(uintptr_t)range,b=(uintptr_t)right-(uintptr_t)range;
    assert(a<range_count*range_width && b<range_count*range_width && !(a%range_width) && !(b%range_width));
    ++calls;errno=100+calls%7;*recon_errno()=200+calls%7;recon_doserrno=300+calls%7;SetLastError(400+calls%7);
    return *(const unsigned char *)left-*(const unsigned char *)right;
}
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && defined(RECON_QSORT_WATCOM_1994)
static void run(size_t count,size_t width,unsigned pattern)
{
    unsigned char storage[240*8+2],original[240*8],seen[240],before[108],after[108];
    size_t n,byte;memset(storage,0x5a,sizeof(storage));memset(seen,0,sizeof(seen));range=storage+1;range_count=count;range_width=width;calls=0;
    for(n=0;n<count;++n) {
        range[n*width]=pattern==0?3:pattern==1?(unsigned char)n:pattern==2?(unsigned char)(count-1-n):(unsigned char)((n*17+3)%5);
        range[n*width+1]=(unsigned char)n;
        for(byte=2;byte<width;++byte)range[n*width+byte]=(unsigned char)(n+byte);
    }
    memcpy(original,range,count*width);fp(before);qsort(range,count,width,compare);fp(after);
    assert(!memcmp(before,after,108));
    for(n=0;n<count;++n) {
        unsigned identity=range[n*width+1];assert(identity<count && !seen[identity]);seen[identity]=1;
        assert(!memcmp(range+n*width,original+identity*width,width));
        if(n)assert(range[(n-1)*width]<=range[n*width]);
        if(!pattern)assert(identity==(count>2?(n==0?count/2:n==count/2?0:n):n));
    }
    if(!pattern)assert(calls==(count>2?count-1:count==2?1:0));
    if(calls)assert(errno==100+(int)(calls%7) && *recon_errno()==200+(int)(calls%7) && recon_doserrno==300+calls%7 && GetLastError()==400+calls%7);
    assert(storage[0]==0x5a);for(n=1+count*width;n<sizeof(storage);++n)assert(storage[n]==0x5a);
}
#endif
int main(int argc,char **argv)
{
    void (*sort)(void *,size_t,size_t,int (*)(const void *,const void *))=qsort;
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && defined(RECON_QSORT_WATCOM_1994)
    static const size_t counts[]={0,1,2,3,7,8,9,16,31,64,127,240};
    static const size_t widths[]={2,3,4,7,8};unsigned i,j,k;
    unsigned char small[]={1,0,0,1,1,2,0,3};
    assert(sort==recon_watcom_qsort_1994);
    if(argc>1) {
        if(!strcmp(argv[1],"null"))sort(NULL,2,4,compare);
        if(!strcmp(argv[1],"callback"))sort(small,2,4,NULL);
        if(!strcmp(argv[1],"zero"))sort(small,2,0,compare);
        if(!strcmp(argv[1],"overflow"))sort(small,(size_t)-1,4,compare);
        if(!strcmp(argv[1],"wrap"))sort((void *)0xfffffff0U,4,8,compare);
        return 42;
    }
    sort(NULL,0,0,NULL);sort(NULL,1,0,NULL);
    __asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(i=0;i<sizeof(counts)/sizeof(counts[0]);++i)for(j=0;j<sizeof(widths)/sizeof(widths[0]);++j)for(k=0;k<4;++k)run(counts[i],widths[j],k);
    /* A duplicate-key partition has the independently derived identities3,1,0,2. */
    range=small;range_count=4;range_width=2;calls=0;sort(small,4,2,compare);
    assert(small[1]==3 && small[3]==1 && small[5]==0 && small[7]==2);
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");return 0;
#else
    unsigned char old[]={0,0,0,1};(void)argc;(void)argv;(void)fp;
    assert(sort==recon_qsort);range=old;range_count=2;range_width=2;calls=0;sort(old,2,2,compare);
    assert(old[1]==1 && old[3]==0);recon_watcom_qsort_1994(NULL,0,0,NULL);return 42;
#endif
}
