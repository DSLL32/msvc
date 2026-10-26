#include <assert.h>
#include <sys/mman.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static void retired(void *pointer)
{
    unsigned char resident;long page=sysconf(_SC_PAGESIZE);
    errno=0;assert(mincore((void *)((uintptr_t)pointer&~((uintptr_t)page-1)),page,&resident)==-1 && errno==ENOMEM);
}
#endif
int main(int argc,char **argv)
{
    HANDLE (WINAPI *create)(DWORD,DWORD,DWORD)=HeapCreate;
    BOOL (WINAPI *destroy)(HANDLE)=HeapDestroy;
    HANDLE heap=create(1,0,0);
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    HANDLE other,empty;unsigned char *block,*large,*survivor,*freed;unsigned n;
    unsigned char before[108],after[108];
    if(argc>1 && !strcmp(argv[1],"null"))destroy(NULL);
    if(argc>1 && !strcmp(argv[1],"unknown"))destroy((HANDLE)1);
    if(argc>1 && !strcmp(argv[1],"interior"))destroy((char *)heap+1);
    other=create(1,0,0);empty=create(1,0,0);
    survivor=HeapAlloc(other,9,64);memset(survivor,0x5a,64);
    block=HeapAlloc(heap,9,16);large=HeapAlloc(heap,9,128008);freed=HeapAlloc(heap,9,528);
    assert(HeapFree(heap,1,freed));memset(block,0x33,16);memset(large,0x44,128008);
    if(argc>1 && !strcmp(argv[1],"live"))__asm__ __volatile__("fld1\n\tfldpi":::"st");
    errno=111;SetLastError(0x12345678);fp(before);assert(destroy(heap));fp(after);
    assert(errno==111 && GetLastError()==0x12345678 && !memcmp(before,after,108));
    if(argc>1 && !strcmp(argv[1],"dead"))destroy(heap);
    if(argc>1 && !strcmp(argv[1],"dead-alloc"))HeapAlloc(heap,9,16);
    retired(block);retired(large);retired(heap);
    for(n=0;n<64;++n)assert(survivor[n]==0x5a);
    /* Empty head and surviving tail owners exercise unlink without reuse. */
    assert(destroy(empty));assert(HeapFree(other,1,survivor));assert(destroy(other));
    if(argc>1 && !strcmp(argv[1],"live"))__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
#else
    (void)argc;(void)argv;destroy(heap);return 42;
#endif
}
