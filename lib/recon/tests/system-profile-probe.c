#include <assert.h>
#include <string.h>
#include <recon.h>
int main(int argc,char **argv)
{
    SYSTEM_INFO system; MEMORYSTATUS memory; const char *mode=argc>3?argv[3]:"good";
    assert(sizeof(system)==36 && sizeof(memory)==32 && offsetof(SYSTEM_INFO,dwPageSize)==4);
    assert(offsetof(MEMORYSTATUS,dwTotalPhys)==8 && offsetof(SYSTEM_INFO,wProcessorLevel)==32);
    errno=111; SetLastError(0x12345678);
    if(strcmp(mode,"unbound") && strcmp(mode,"unbound-memory")) recon_load_system_profile(argv[1],argv[2]);
    if(!strcmp(mode,"rebind")) recon_load_system_profile(argv[1],argv[2]);
    if(!strcmp(mode,"wrong-order") || !strcmp(mode,"unbound-memory")) GlobalMemoryStatus(&memory);
    memset(&system,0xa5,sizeof(system)); memset(&memory,0x5a,sizeof(memory));
    GetSystemInfo(!strcmp(mode,"null")?NULL:&system);
    if(!strcmp(mode,"unbound")) {
        assert(system.dwPageSize==(DWORD)sysconf(_SC_PAGESIZE) && system.dwAllocationGranularity==65536);
        assert(errno==111 && GetLastError()==0x12345678);return 0;
    }
    if(!strcmp(mode,"extra-system")) GetSystemInfo(&system);
    GlobalMemoryStatus(&memory);
    if(!strcmp(mode,"extra-memory")) GlobalMemoryStatus(&memory);
    assert(errno==111 && GetLastError()==0x12345678);
    fwrite(&system,1,sizeof(system),stdout); fwrite(&memory,1,sizeof(memory),stdout);
    return 0;
}
