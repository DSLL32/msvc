#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static void inaccessible(void *address,unsigned bytes)
{
    FILE *file=fopen("/proc/self/maps","r");char line[512],mode[5];unsigned long begin,end;int found=0;
    assert(file);
    while(fgets(line,sizeof(line),file))if(sscanf(line,"%lx-%lx %4s",&begin,&end,mode)==3 &&
       begin<=(uintptr_t)address && end>=(uintptr_t)address+bytes) {
        assert(!strcmp(mode,"---p"));found=1;break;
    }
    assert(!fclose(file) && found);
}
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"normal";
    unsigned char *memory,*other,resident[2],before[108],after[108];unsigned n;
    DWORD kind=MEM_COMMIT|MEM_RESERVE,protection=PAGE_READWRITE;
    BOOL (WINAPI *decommit)(LPVOID,DWORD,DWORD)=recon_VirtualFree;
    assert(MEM_DECOMMIT==0x4000 && sysconf(_SC_PAGESIZE)==4096);
    if(!strcmp(mode,"reserve"))kind=MEM_RESERVE;
    if(!strcmp(mode,"topdown"))kind=MEM_RESERVE|MEM_TOP_DOWN;
    if(!strcmp(mode,"readonly"))protection=PAGE_READONLY;
    memory=recon_VirtualAlloc(NULL,8192,kind,protection);assert(memory);
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(!strcmp(mode,"null"))decommit(NULL,8192,MEM_DECOMMIT);
    if(!strcmp(mode,"foreign"))decommit(mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0),8192,MEM_DECOMMIT);
    if(!strcmp(mode,"interior"))decommit(memory+4096,4096,MEM_DECOMMIT);
    if(!strcmp(mode,"partial"))decommit(memory,4096,MEM_DECOMMIT);
    if(!strcmp(mode,"zero"))decommit(memory,0,MEM_DECOMMIT);
    if(strcmp(mode,"reserve") && strcmp(mode,"topdown") && strcmp(mode,"readonly"))memset(memory,0xa5,8192);
    other=recon_VirtualAlloc(NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);assert(other);memset(other,0x5a,4096);
    if(!strcmp(mode,"live"))__asm__ __volatile__("fld1\n\tfldpi":::"st");
    errno=111;recon_doserrno=222;recon_SetLastError(333);fp(before);
    assert(decommit(memory,8192,MEM_DECOMMIT));fp(after);
    assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==333 && !memcmp(before,after,108));
    if(!strcmp(mode,"repeat"))decommit(memory,8192,MEM_DECOMMIT);
    if(!strcmp(mode,"recommit"))recon_VirtualAlloc(memory,8192,MEM_COMMIT,PAGE_READWRITE);
    inaccessible(memory,8192);assert(!mincore(memory,8192,resident) && !(resident[0]&1) && !(resident[1]&1));
    for(n=0;n<4096;++n)assert(other[n]==0x5a);
    assert(decommit(memory,0,MEM_RELEASE));
    if(!strcmp(mode,"released"))decommit(memory,8192,MEM_DECOMMIT);
    errno=0;assert(mincore(memory,8192,resident)==-1 && errno==ENOMEM);
    assert(decommit(other,0,MEM_RELEASE));
    if(!strcmp(mode,"live"))__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
#else
    (void)n;(void)other;(void)resident;(void)before;(void)after;(void)inaccessible;(void)fp;
    assert(!decommit(memory,8192,MEM_DECOMMIT) && recon_GetLastError()==ERROR_NOT_SUPPORTED);
    assert(decommit(memory,0,MEM_RELEASE));
#endif
    return 0;
}
