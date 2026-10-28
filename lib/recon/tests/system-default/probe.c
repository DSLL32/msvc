#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static int target_error;
int *recon_errno(void) {return &target_error;}
int main(int argc,char **argv)
{
    struct {DWORD before;SYSTEM_INFO info;DWORD after;} data;
    SYSTEM_INFO again;unsigned short cw,sw,cw2,sw2;unsigned char *base;
    assert(!recon_native_entry_active());memset(&data,0xa5,sizeof(data));
    if(argc>1) {
        if(!strcmp(argv[1],"null"))recon_GetSystemInfo(NULL);
        else if(!strcmp(argv[1],"wrap"))recon_GetSystemInfo((void *)0xfffffff0UL);
        else recon_GetSystemInfo((void *)((char *)&data.info+1));
        return 1;
    }
    errno=E2BIG;target_error=123;recon_doserrno=456;recon_SetLastError(789);
    __asm__ volatile("fnstcw %0;fnstsw %1":"=m"(cw),"=m"(sw));
    recon_GetSystemInfo(&data.info);recon_GetSystemInfo(&again);
    __asm__ volatile("fnstcw %0;fnstsw %1":"=m"(cw2),"=m"(sw2));
    assert(cw==cw2 && sw==sw2 && errno==E2BIG && target_error==123 && recon_doserrno==456 && recon_GetLastError()==789);
    assert(data.before==0xa5a5a5a5UL && data.after==0xa5a5a5a5UL);
    assert(data.info.dwPageSize==(DWORD)sysconf(_SC_PAGESIZE) && data.info.dwAllocationGranularity==65536);
    assert(again.dwPageSize==data.info.dwPageSize && again.dwAllocationGranularity==65536);
    assert(data.info.dwNumberOfProcessors>=1 && data.info.dwNumberOfProcessors<=32);
    assert((uintptr_t)data.info.lpMinimumApplicationAddress<(uintptr_t)data.info.lpMaximumApplicationAddress);
    base=recon_VirtualAlloc(NULL,0x4000000,MEM_RESERVE,PAGE_READWRITE);assert(base);
    assert(!((uintptr_t)base%(uintptr_t)data.info.dwPageSize));
    assert(recon_VirtualAlloc(base,data.info.dwPageSize,MEM_COMMIT,PAGE_READWRITE)==base);
    base[0]=0x31;base[data.info.dwPageSize-1]=0x7e;assert(base[0]==0x31 && base[data.info.dwPageSize-1]==0x7e);
    assert(recon_VirtualFree(base,0,MEM_RELEASE));return 0;
}
