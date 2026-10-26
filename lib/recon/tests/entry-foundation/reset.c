#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
#include <stdint.h>

/* Authored linker-owned data/BSS interval; all loader/CRT/test bookkeeping is
 * deliberately outside it. A relocated pointer is part of the saved bytes. */
static int outside_value=73;
static struct {
    unsigned magic;
    int *relocated;
    unsigned char payload[13];
} reset_data __attribute__((section(".data.recon_reset")))={0x13572468,&outside_value,{1,2,3}};
static unsigned reset_bss[7] __attribute__((section(".bss.recon_reset")));
extern unsigned char recon_test_state_start[],recon_test_state_end[];
static unsigned char expected[2048],dirty[2048];
static size_t region_bytes;
static unsigned attaches,detaches,dep_attaches,dep_detaches,attempts;
static int fail_first,dep_writes;
static HMODULE reset_handle;
static int shared_fmode;
static const char identity[]="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
static HMODULE identity_module(const char *name,const char *path)
{
    ReconModuleSpec spec={name,path,identity,RECON_MODULE_NO_TLS};
    HMODULE result=recon_register_module(&spec);assert(result);return result;
}
static void dirty_state(unsigned char byte)
{
    memset(recon_test_state_start,byte,region_bytes);
    memcpy(dirty,recon_test_state_start,region_bytes);
}
static BOOL WINAPI dependency_entry(HINSTANCE module,DWORD reason,LPVOID reserved)
{
    (void)module;(void)reserved;
    if(reason==DLL_PROCESS_ATTACH) {
        ++dep_attaches;
        if(dep_writes) {
            /* Dependency can see fresh mapped state, then initialize its user. */
            assert(!memcmp(recon_test_state_start,expected,region_bytes));
            reset_data.payload[0]=0x59;
        }
    } else ++dep_detaches;
    return TRUE;
}
static BOOL WINAPI reset_entry(HINSTANCE module,DWORD reason,LPVOID reserved)
{
    (void)reserved;assert(module==reset_handle);
    assert(recon_fmode==shared_fmode && outside_value==73);
    if(reason==DLL_PROCESS_ATTACH) {
        unsigned char first=reset_data.payload[0];
        ++attaches;++attempts;
        if(dep_writes)reset_data.payload[0]=expected[(unsigned char *)reset_data.payload-recon_test_state_start];
        assert(!memcmp(recon_test_state_start,expected,region_bytes));
        assert(reset_data.magic==0x13572468 && reset_data.relocated==&outside_value);
        assert(!reset_bss[0] && !reset_bss[6]);
        if(dep_writes)assert(first==0x59);
        dirty_state(0xd3);
        return !(fail_first && attempts==1);
    }
    ++detaches;assert(!memcmp(recon_test_state_start,dirty,region_bytes));
    dirty_state(0xe4); /* Real cleanup sees dirty state; no early image reset. */
    return TRUE;
}
static void invalid_regions(HMODULE module)
{
    ReconModuleWritableRegion region;
    ReconStaticModuleSpec spec={NULL,0,NULL,NULL,0,&region};
    unsigned i;
    for(i=0;i<4;++i) {
        region.address=i==0?NULL:(i==1?(void *)4096:recon_test_state_start);
        region.bytes=i==0?1:(i==1?(size_t)-1:(i==2?0:region_bytes));
        /* Last case overlaps the already configured first module. */
        assert(!recon_configure_static_module(module,&spec));
        assert(recon_GetLastError()==ERROR_INVALID_PARAMETER);
    }
    region.address=(void *)(uintptr_t)(UINTPTR_MAX-7);region.bytes=16;
    assert(!recon_configure_static_module(module,&spec));
    assert(recon_GetLastError()==ERROR_INVALID_PARAMETER);
    spec.writable_region=NULL;
    assert(recon_configure_static_module(module,&spec)); /* Rejections did not commit. */
}
void entry_reset_checks(void)
{
    HMODULE dependency,other,one,two;ReconModuleWritableRegion region;
    ReconStaticModuleSpec spec;
    unsigned saved_attaches,saved_detaches;
    region_bytes=(size_t)(recon_test_state_end-recon_test_state_start);
    assert(region_bytes>sizeof(reset_data)+sizeof(reset_bss) && region_bytes<=sizeof(expected));
    memcpy(expected,recon_test_state_start,region_bytes);shared_fmode=recon_fmode;
    dependency=identity_module("resetdep.dll","C:\\apps\\resetdep.dll");
    reset_handle=identity_module("reset.dll","C:\\apps\\reset.dll");
    other=identity_module("invalidreset.dll","C:\\apps\\invalidreset.dll");
    memset(&spec,0,sizeof(spec));spec.entry=dependency_entry;
    assert(recon_configure_static_module(dependency,&spec));
    region.address=recon_test_state_start;region.bytes=region_bytes;
    spec.entry=reset_entry;spec.dependencies=&dependency;spec.dependency_count=1;spec.writable_region=&region;
    errno=E2BIG;assert(recon_configure_static_module(reset_handle,&spec));assert(errno==E2BIG);
    /* Descriptor is copied, not borrowed; original span lives with static bundle. */
    region.address=NULL;region.bytes=0;invalid_regions(other);
    dirty_state(0xa6); /* Snapshot is configure-time, not first-load sampling. */
    one=recon_LoadLibraryA("reset.dll");assert(one==reset_handle && attaches==1 && dep_attaches==1);
    recon_test_state_start[region_bytes-1]^=0x77;memcpy(dirty,recon_test_state_start,region_bytes);
    two=recon_LoadLibraryA("reset.dll");assert(two==one && attaches==1);
    assert(!memcmp(recon_test_state_start,dirty,region_bytes));
    assert(recon_FreeLibrary(one) && !detaches && !dep_detaches);
    assert(!memcmp(recon_test_state_start,dirty,region_bytes));
    assert(recon_FreeLibrary(two) && detaches==1 && dep_detaches==1);
    assert(recon_test_state_start[0]==0xe4); /* Detach writes remain until reload. */
    dep_writes=1;one=recon_LoadLibraryA("reset.dll");
    assert(one==reset_handle && attaches==2 && dep_attaches==2);
    assert(recon_FreeLibrary(one) && detaches==2 && dep_detaches==2);
    /* Failed attach cleanup remains dirty; retry restores exact initial bytes. */
    fail_first=1;attempts=0;saved_attaches=attaches;saved_detaches=detaches;
    assert(!recon_LoadLibraryA("reset.dll") && recon_GetLastError()==ERROR_DLL_INIT_FAILED);
    assert(attaches==saved_attaches+1 && detaches==saved_detaches+1 && recon_test_state_start[0]==0xe4);
    one=recon_LoadLibraryA("reset.dll");assert(one==reset_handle && attempts==2);
    assert(recon_FreeLibrary(one));
    assert(attaches==4 && detaches==4 && dep_attaches==4 && dep_detaches==4);
    /* Pin/direct dependency uses the same fresh attachment reset and final detach. */
    fail_first=0;assert(recon_attach_static_module(reset_handle));
    assert(attaches==5 && dep_attaches==5);
    assert(recon_attach_static_module(reset_handle) && attaches==5);
    one=recon_LoadLibraryA("reset.dll");assert(one==reset_handle && attaches==5);
    assert(recon_FreeLibrary(one) && detaches==4);
    recon_detach_process_modules();assert(detaches==5 && dep_detaches==5);
    assert(recon_fmode==shared_fmode && outside_value==73);
    puts("relocated data/BSS/padding/refcounts/dependencies/failure reset PASS");
}
