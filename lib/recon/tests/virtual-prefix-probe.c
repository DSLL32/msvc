#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static void retired(void *address,unsigned bytes,int released)
{
    unsigned char resident[12800];FILE *file;char line[512],mode[5];unsigned long begin,end,cursor=(uintptr_t)address;int found=0;
    assert(bytes/4096<=sizeof(resident));errno=0;
    if(released){assert(mincore(address,bytes,resident)==-1 && errno==ENOMEM);return;}
    assert(!mincore(address,bytes,resident));
    {unsigned n;for(n=0;n<bytes/4096;++n)assert(!(resident[n]&1));}
    file=fopen("/proc/self/maps","r");assert(file);
    while(fgets(line,sizeof(line),file))if(sscanf(line,"%lx-%lx %4s",&begin,&end,mode)==3 &&
        begin<=cursor && end>cursor){assert(!strcmp(mode,"---p"));cursor=end;if(cursor>=(uintptr_t)address+bytes){found=1;break;}}
    assert(!fclose(file) && found);
}
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"normal";
    unsigned bytes=50*1024*1024,prefix=131072,n;unsigned char *memory,*other,before[108],after[108];
    DWORD kind=MEM_RESERVE|MEM_TOP_DOWN;
    if(!strcmp(mode,"whole")){kind=MEM_RESERVE;bytes=5*1024*1024;prefix=16384;}
    memory=recon_VirtualAlloc(NULL,bytes,kind,PAGE_READWRITE);assert(memory);
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(!strcmp(mode,"empty"))recon_VirtualFree(memory,bytes,MEM_DECOMMIT);
    if(!strcmp(mode,"hole"))recon_VirtualAlloc(memory+4096,4096,MEM_COMMIT,PAGE_READWRITE);
    if(!strcmp(mode,"readonly"))recon_VirtualAlloc(memory,4096,MEM_COMMIT,PAGE_READONLY);
    if(!strcmp(mode,"unaligned"))recon_VirtualAlloc(memory+1,4096,MEM_COMMIT,PAGE_READWRITE);
    assert(recon_VirtualAlloc(memory,prefix/2,MEM_COMMIT,PAGE_READWRITE)==memory);
    memset(memory,0xa5,prefix/2);
    assert(recon_VirtualAlloc(memory+prefix/2,prefix/2,MEM_COMMIT,PAGE_READWRITE)==memory+prefix/2);
    for(n=prefix/2;n<prefix;++n)assert(!memory[n]);
    memset(memory+prefix/2,0x5a,prefix/2);
    assert(recon_VirtualAlloc(memory+4096,4096,MEM_COMMIT,PAGE_READWRITE)==memory+4096);
    for(n=0;n<prefix;++n)assert(memory[n]==(n<prefix/2?0xa5:0x5a));
    if(!strcmp(mode,"partial"))recon_VirtualFree(memory,prefix-4096,MEM_DECOMMIT);
    if(!strcmp(mode,"interior"))recon_VirtualFree(memory+4096,prefix-4096,MEM_DECOMMIT);
    other=recon_VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(other);memset(other,0x33,4096);
    __asm__ __volatile__("fld1\n\tfldpi":::"st");errno=111;recon_doserrno=222;recon_SetLastError(333);fp(before);
    assert(recon_VirtualFree(memory,!strcmp(mode,"zero")?0:(!strcmp(mode,"whole")?bytes:prefix),MEM_DECOMMIT));fp(after);
    assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==333 && !memcmp(before,after,108));
    if(!strcmp(mode,"repeat"))recon_VirtualFree(memory,prefix,MEM_DECOMMIT);
    if(!strcmp(mode,"recommit"))recon_VirtualAlloc(memory,4096,MEM_COMMIT,PAGE_READWRITE);
    retired(memory,bytes,0);for(n=0;n<4096;++n)assert(other[n]==0x33);
    assert(recon_VirtualFree(memory,0,MEM_RELEASE));retired(memory,bytes,1);
    assert(recon_VirtualFree(other,4096,MEM_DECOMMIT));assert(recon_VirtualFree(other,0,MEM_RELEASE));
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
#else
    (void)prefix;(void)n;(void)other;(void)before;(void)after;(void)fp;(void)retired;
    assert(!recon_VirtualFree(memory,bytes,MEM_DECOMMIT) && recon_GetLastError()==ERROR_NOT_SUPPORTED);
    assert(recon_VirtualFree(memory,0,MEM_RELEASE));
#endif
    return 0;
}
