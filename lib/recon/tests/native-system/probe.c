#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
static DWORD saturated(unsigned long count,unsigned long unit) { return (uint64_t)count*unit>UINT32_MAX?UINT32_MAX:count*unit; }
int main(int argc,char **argv,char **env)
{
    ReconNativeEntry *entry;struct { SYSTEM_INFO value;DWORD tail; } system;
    struct { MEMORYSTATUS value;DWORD tail; } memory;
    struct sysinfo actual;unsigned short cw,sw,cw2,sw2;char path[MAX_PATH],name[MAX_PATH],cwd[4096];
    const char *call[4];char **child;unsigned n;int found=0;
    entry=recon_native_entry_initialize(argc,argv,env);
    if(argc>1 && !strcmp(argv[1],"child")) {
        assert(!strcmp(recon_getenv("LINK_REPRO_NAME"),"\\deflib.rsp"));
        assert(!strcmp(recon_getenv("TMP"),"."));assert(getcwd(cwd,sizeof(cwd)));
        assert(argc==3 && !strcmp(entry->current_directory,entry->argv[2]));
        assert(strstr(entry->command_line,"child"));
        return 0;
    }
    if(argc>1 && !strcmp(argv[1],"limit")) {struct rlimit limit={128*1024*1024,128*1024*1024};assert(!setrlimit(RLIMIT_AS,&limit));}
    if(argc>1 && !strcmp(argv[1],"null")) {recon_GetSystemInfo(NULL);return 1;}
    if(argc>1 && !strcmp(argv[1],"unaligned")) {recon_GlobalMemoryStatus((MEMORYSTATUS *)((char *)&memory+1));return 1;}
    if(argc>1 && !strcmp(argv[1],"wrap")) {recon_GetSystemInfo((SYSTEM_INFO *)0xfffffff0UL);return 1;}
    system.tail=memory.tail=0xdeadbeef;errno=E2BIG;recon_doserrno=73;recon_SetLastError(91);
    __asm__ volatile("fnstcw %0;fnstsw %1":"=m"(cw),"=m"(sw));
    recon_GetSystemInfo(&system.value);recon_GlobalMemoryStatus(&memory.value);recon_GetSystemInfo(&system.value);
    __asm__ volatile("fnstcw %0;fnstsw %1":"=m"(cw2),"=m"(sw2));
    assert(cw==cw2 && sw==sw2 && errno==E2BIG && recon_doserrno==73 && recon_GetLastError()==91);
    assert(system.tail==0xdeadbeef && memory.tail==0xdeadbeef);
    assert(system.value.dwPageSize==sysconf(_SC_PAGESIZE) && system.value.dwAllocationGranularity==65536);
    assert(system.value.dwNumberOfProcessors>=1 && system.value.dwNumberOfProcessors<=32);
    assert((uintptr_t)system.value.lpMinimumApplicationAddress<(uintptr_t)system.value.lpMaximumApplicationAddress);
    assert(!sysinfo(&actual) && memory.value.dwTotalPhys==saturated(actual.totalram,actual.mem_unit));
    assert(memory.value.dwLength==32 && memory.value.dwMemoryLoad<=100 && memory.value.dwAvailPhys<=memory.value.dwTotalPhys);
    assert(memory.value.dwAvailVirtual<=memory.value.dwTotalVirtual);
    if(argc>1) {assert(memory.value.dwTotalVirtual<=128*1024*1024);return 0;}
    assert(recon_GetTempPathA(sizeof(path),path));
    assert(recon_GetTempFileNameA(path,"ordinary",0,name));assert(recon_DeleteFileA(name));
    assert(!recon_getenv("LINK_REPRO_NAME"));
    assert(recon_SetEnvironmentVariableA("LINK_REPRO_NAME","\\deflib.rsp"));
    assert(!recon_getenv("LINK_REPRO_NAME"));child=recon_native_entry_spawn_environment();
    for(n=0;child[n];++n)if(!strcmp(child[n],"LINK_REPRO_NAME=\\deflib.rsp"))found=1;
    assert(found);call[0]=entry->program_path;call[1]="child";call[2]=entry->current_directory;call[3]=NULL;
    assert(!recon_spawnv(_P_WAIT,entry->program_path,call));return 0;
}
