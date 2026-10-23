#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
int main(int argc,char **argv)
{
    struct {unsigned char before[16];SYSTEM_INFO system;unsigned char after[16];} data;
    unsigned char before[108],after[108];MEMORYSTATUS memory;
    const char *mode=argc>3?argv[3]:"good";unsigned i;
    assert(argc>=3 && sizeof(SYSTEM_INFO)==36);
    memset(&data,0xa5,sizeof(data));errno=111;recon_doserrno=77;recon_SetLastError(0x12345678);
    __asm__ __volatile__("fld1\n\tfnsave %0\n\tfrstor %0":"=m"(before));
    if(strcmp(mode,"unbound"))recon_load_system_info_profile(argv[1],argv[2]);
    if(!strcmp(mode,"rebind"))recon_load_system_info_profile(argv[1],argv[2]);
    if(!strcmp(mode,"combined-load"))recon_load_system_profile(argv[1],argv[2]);
    if(!strcmp(mode,"memory-first"))recon_GlobalMemoryStatus(&memory);
    recon_GetSystemInfo(!strcmp(mode,"null")?NULL:&data.system);
    if(!strcmp(mode,"repeat"))recon_GetSystemInfo(&data.system);
    if(!strcmp(mode,"memory"))recon_GlobalMemoryStatus(&memory);
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(after));
    assert(!memcmp(before,after,108));
    assert(errno==111 && recon_doserrno==77 && recon_GetLastError()==0x12345678);
    for(i=0;i<16;++i)assert(data.before[i]==0xa5 && data.after[i]==0xa5);
    assert(fwrite(&data.system,1,36,stdout)==36);
    return 0;
}
