#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
static void check_region(void *pointer,unsigned bytes)
{
    FILE *file=fopen("/proc/self/maps","r");char line[512],permissions[5];
    unsigned long start,end;int found=0;assert(file);
    while(fgets(line,sizeof(line),file)){
        if(sscanf(line,"%lx-%lx %4s",&start,&end,permissions)==3 &&
            start<=(uintptr_t)pointer && end>=(uintptr_t)pointer+bytes){
            assert(!strcmp(permissions,"---p"));found=1;break;
        }
    }
    fclose(file);assert(found);
}
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"good";
    DWORD sizes[3]={0x3200000,0x2980000,4096},bytes=sizes[0];
    DWORD kind=MEM_RESERVE|MEM_TOP_DOWN,protection=PAGE_READWRITE;
    void *address=NULL,*regions[3];unsigned i,j;unsigned char before[108],after[108];
    if(!strcmp(mode,"address"))address=(void *)0x10000;
    if(!strcmp(mode,"commit"))kind|=MEM_COMMIT;
    if(!strcmp(mode,"top-only"))kind=MEM_TOP_DOWN;
    if(!strcmp(mode,"unknown"))kind|=0x1000000;
    if(!strcmp(mode,"protection"))protection=PAGE_READONLY;
    if(!strcmp(mode,"zero"))bytes=0;
    if(!strcmp(mode,"short"))bytes=1;
    if(!strcmp(mode,"unaligned"))bytes++;
    if(!strcmp(mode,"overflow"))bytes=0xfffff000;
    if(strcmp(mode,"good")){
        recon_VirtualAlloc(address,bytes,kind,protection);return 42;
    }
    for(i=0;i<3;++i){
        errno=111;recon_doserrno=77;recon_SetLastError(0x12345678);
        __asm__ __volatile__("fld1\n\tfnsave %0\n\tfrstor %0":"=m"(before));
        regions[i]=recon_VirtualAlloc(NULL,sizes[i],kind,protection);
        __asm__ __volatile__("fnsave %0\n\tfrstor %0\n\tfstp %%st(0)":"=m"(after));
        assert(!memcmp(before,after,108));
        assert(errno==111 && recon_doserrno==77 && recon_GetLastError()==0x12345678);
        assert(regions[i] && !((uintptr_t)regions[i]&65535));
        for(j=0;j<i;++j)assert((uintptr_t)regions[i]+sizes[i]<=(uintptr_t)regions[j] ||
            (uintptr_t)regions[j]+sizes[j]<=(uintptr_t)regions[i]);
        check_region(regions[i],sizes[i]);
    }
    {
        unsigned char *old,*combined;
        old=recon_VirtualAlloc(NULL,0x500000,MEM_RESERVE,PAGE_READWRITE);assert(old);
        assert(recon_VirtualAlloc(old,0x4000,MEM_COMMIT,PAGE_READWRITE)==old);
        for(i=0;i<0x4000;++i)assert(old[i]==0);
        old[0]=0x5a;old[0x3fff]=0xa5;
        assert(recon_VirtualFree(old,0,MEM_RELEASE));
        combined=recon_VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(combined);
        for(i=0;i<4096;++i)assert(combined[i]==0);
        assert(recon_VirtualFree(combined,0,MEM_RELEASE));
    }
    for(i=0;i<3;++i){
        assert(recon_VirtualFree(regions[i],0,MEM_RELEASE));
        assert(!recon_VirtualFree(regions[i],0,MEM_RELEASE));
        assert(recon_GetLastError()==ERROR_INVALID_PARAMETER);
    }
    return 0;
}
