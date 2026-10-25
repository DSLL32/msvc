#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
/* Test-only allocation wrappers supply known initial bytes. This verifies
 * absence of zeroing without reading unspecified host allocation contents. */
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
static unsigned seed,allocations,zero_allocations;
static size_t expected_size;
void *__wrap_malloc(size_t size)
{
    void *memory=__real_malloc(size);
    if(seed) { assert(size==expected_size && memory);memset(memory,seed,size);++allocations; }
    return memory;
}
void *__wrap_calloc(size_t count,size_t size)
{
    if(seed)++zero_allocations;
    return __real_calloc(count,size);
}
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    static const DWORD sizes[]={1,11,65536,131072};
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,333}};
    unsigned row,index,n;int live=argc>1 && !strcmp(argv[1],"live");
    HLOCAL first,second;unsigned char before[108],after[108];
    if(argc>1 && !live) {
        if(!strcmp(argv[1],"flags")){recon_LocalAlloc(2,65536);return 42;}
        if(!strcmp(argv[1],"zero")){recon_LocalAlloc(0,0);return 42;}
        if(!strcmp(argv[1],"owner")){recon_LocalFree((HLOCAL)1);return 42;}
        first=recon_LocalAlloc(0,65536);
        if(!strcmp(argv[1],"interior")){recon_LocalFree((char *)first+1);return 42;}
        assert(!strcmp(argv[1],"twice") && !recon_LocalFree(first));
        recon_LocalFree(first);return 42;
    }
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(index=0;index<4;++index)for(row=0;row<3;++row) {
        expected_size=sizes[index];seed=0xa5;allocations=zero_allocations=0;
        errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
        fp(before);first=recon_LocalAlloc(LMEM_FIXED,sizes[index]);fp(after);seed=0;
        assert(first && !((uintptr_t)first&3) && allocations==1 && !zero_allocations);
        assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
        assert(!memcmp(before,after,108));
        seed=0x3c;second=recon_LocalAlloc(LMEM_FIXED,sizes[index]);seed=0;
        assert(second && second!=first && allocations==2 && !zero_allocations);
        for(n=0;n<sizes[index];++n)assert(((unsigned char *)first)[n]==0xa5 && ((unsigned char *)second)[n]==0x3c);
        memset(first,0x17,sizes[index]);memset(second,0xe9,sizes[index]);
        errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
        fp(before);assert(!recon_LocalFree(first) && !recon_LocalFree(second));fp(after);
        assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
        assert(!memcmp(before,after,108));
    }
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
