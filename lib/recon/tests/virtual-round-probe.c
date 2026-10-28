#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <recon.h>
static void fp(unsigned char state[108])
{__asm__ volatile("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state));}
static void permissions(void *address,unsigned bytes,const char *wanted)
{
    FILE *maps=fopen("/proc/self/maps","r");char line[256],mode[5];unsigned long begin,end,cursor=(uintptr_t)address;int found=0;
    assert(maps);while(fgets(line,sizeof(line),maps))if(sscanf(line,"%lx-%lx %4s",&begin,&end,mode)==3 && begin<=cursor && end>cursor) {assert(!strcmp(mode,wanted));cursor=end;if(cursor>=(uintptr_t)address+bytes){found=1;break;}}
    assert(!fclose(maps) && found);
}
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"normal";unsigned total=5*1024*1024,request=16379,rounded,n;unsigned char *base,*other,before[108],after[108];
    if(!strcmp(mode,"one"))request=1;
    if(!strcmp(mode,"4095"))request=4095;
    if(!strcmp(mode,"4096"))request=4096;
    if(!strcmp(mode,"4097"))request=4097;
    rounded=(request+4095)&~4095U;
    base=recon_VirtualAlloc(NULL,total,MEM_RESERVE,PAGE_READWRITE);assert(base);
    other=recon_VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(other);memset(other,0x61,4096);
    if(!strcmp(mode,"hole"))recon_VirtualAlloc(base+4096,request,MEM_COMMIT,PAGE_READWRITE);
    if(!strcmp(mode,"unaligned"))recon_VirtualAlloc(base+1,request,MEM_COMMIT,PAGE_READWRITE);
    if(!strcmp(mode,"readonly"))recon_VirtualAlloc(base,request,MEM_COMMIT,PAGE_READONLY);
    if(!strcmp(mode,"wrapped"))recon_VirtualAlloc(base,0xffffffffU,MEM_COMMIT,PAGE_READWRITE);
    if(!strcmp(mode,"foreign")){void *foreign=mmap(NULL,rounded,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(foreign!=MAP_FAILED);assert(!recon_VirtualAlloc(foreign,request,MEM_COMMIT,PAGE_READWRITE));assert(recon_GetLastError()==ERROR_INVALID_PARAMETER);assert(!munmap(foreign,rounded));return 0;}
    if(!strcmp(mode,"outside")){assert(!recon_VirtualAlloc(base,total+1,MEM_COMMIT,PAGE_READWRITE));assert(recon_GetLastError()==ERROR_INVALID_PARAMETER);return 0;}
    __asm__ volatile("fld1\n\tfldpi":::"st");errno=111;recon_doserrno=222;recon_SetLastError(333);fp(before);
    assert(recon_VirtualAlloc(base,request,MEM_COMMIT,PAGE_READWRITE)==base);fp(after);
    assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==333 && !memcmp(before,after,108));
    permissions(base,rounded,"rw-p");permissions(base+rounded,4096,"---p");
    for(n=0;n<rounded;++n)assert(!base[n]);memset(base,0x5a,rounded);
    assert(recon_VirtualAlloc(base+rounded,request,MEM_COMMIT,PAGE_READWRITE)==base+rounded);
    for(n=rounded;n<2*rounded;++n)assert(!base[n]);memset(base+rounded,0x39,rounded);
    assert(recon_VirtualAlloc(base+rounded-4096,1,MEM_COMMIT,PAGE_READWRITE)==base+rounded-4096);
    for(n=0;n<2*rounded;++n)assert(base[n]==(n<rounded?0x5a:0x39));
    if(!strcmp(mode,"last")){assert(recon_VirtualAlloc(base+2*rounded,total-2*rounded-5,MEM_COMMIT,PAGE_READWRITE)==base+2*rounded);base[total-1]=0x7a;assert(base[total-1]==0x7a);}
    if(!strcmp(mode,"recommit")){assert(recon_VirtualFree(base,0,MEM_DECOMMIT));recon_VirtualAlloc(base,1,MEM_COMMIT,PAGE_READWRITE);return 91;}
    permissions(base,(!strcmp(mode,"last")?total:2*rounded),"rw-p");
    for(n=0;n<4096;++n)assert(other[n]==0x61);
    assert(recon_VirtualFree(base,0,MEM_DECOMMIT));permissions(base,total,"---p");assert(recon_VirtualFree(base,0,MEM_RELEASE));
    assert(recon_VirtualFree(other,0,MEM_RELEASE));__asm__ volatile("fstp %%st(0)\n\tfstp %%st(0)":::"st");return 0;
}
