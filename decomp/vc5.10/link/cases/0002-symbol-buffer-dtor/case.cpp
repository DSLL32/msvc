/* Internal cleanup probe at genuine main entry, after target CRT startup. */
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <new.h>
#include "../../src/symbol-buffer.h"
#include "../../src/initializers.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-tool.h>
#include <recon-native.h>
extern "C" {
#include "../../harness/startup.h"
}
#include "../../src/link.h"
#endif
#include <recon.h>

struct BufferLayout { void *data; unsigned used,capacity; };
typedef char BufferLayoutSize[(sizeof(BufferLayout)==12)?1:-1];
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *probe;
/* Single-inheritance nonvirtual member projection: the VC5 member pointer is
 * exactly one code DWORD. This is typed probe glue, never target source. */
class OriginalCleanup { public: void Invoke(); };
typedef void (OriginalCleanup::*OriginalMethod)();
typedef char MemberPointerSize[(sizeof(OriginalMethod)==4)?1:-1];
static void destroy(BufferLayout *object)
{
    union { DWORD address; OriginalMethod method; } target;
    target.address=(DWORD)RECON_ORIGINAL_ADDRESS(probe,0x331a0);
    (((OriginalCleanup *)object)->*target.method)();
}
#else
/* Retained native base-destructor entry; single nonvirtual base has the same
 * ECX/RET0 ABI. Bind the real symbol without creating a wrapper vertex. */
extern "C" void RECON_THISCALL native_buffer_cleanup(ScopedBuffer *)
    __asm__("_ZN12ScopedBufferD2Ev");
static void destroy(BufferLayout *object)
{
    native_buffer_cleanup((ScopedBuffer *)object);
}
#endif
static void fail(void)
{
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe,0);
#else
    recon_native_entry_finish(0);
#endif
}
static void check_cleanup(void)
{
    BufferLayout object,before;
    BufferBlock *owner;
    unsigned observed[8];
#ifndef RECON_ORIGINAL_DIRECT
    ReconCppAllocation *allocation;
    unsigned long long generation;
#endif
    object.data=NULL; object.used=0x12345678; object.capacity=0x23456789;
    before=object;
    destroy(&object);
    if(memcmp(&object,&before,sizeof(object)))fail();
    observed[0]=1; observed[1]=object.used; observed[2]=object.capacity;
#ifdef RECON_ORIGINAL_DIRECT
    owner=(BufferBlock *)RECON_ORIGINAL_ADDRESS(probe,0x809d0);
#else
    owner=&buffer12;
#endif
    if(!owner->base || owner->remaining!=1024 || owner->capacity!=1024)fail();
    object.data=owner->base; object.used=7; object.capacity=0x3000;
    before=object;
#ifndef RECON_ORIGINAL_DIRECT
    for(allocation=recon_cpp_allocations;allocation;allocation=allocation->next)
        if(allocation->base==object.data && allocation->live)break;
    if(!allocation || allocation->requested!=0x3000)fail();
    generation=allocation->generation;
#endif
    destroy(&object);
    /* Compare the pointer value and live object only, never freed payload.
     * Startup owner is deliberately not resumed or cleaned up after this probe. */
    if(memcmp(&object,&before,sizeof(object)) || owner->base!=before.data ||
       owner->remaining!=1024 || owner->capacity!=1024)fail();
#ifndef RECON_ORIGINAL_DIRECT
    if(allocation->live || allocation->generation!=generation)fail();
#endif
    observed[3]=1; observed[4]=object.used; observed[5]=object.capacity;
    observed[6]=owner->remaining; observed[7]=owner->capacity;
#ifdef RECON_ORIGINAL_DIRECT
    if(!recon_original_emit(probe,"buffer-cleanup.bin",observed,sizeof(observed)))fail();
    recon_original_finish(probe,1);
#else
    {
        unsigned expected[8];
        FILE *file=fopen("buffer-cleanup.bin","rb");
        if(!file || fread(expected,1,sizeof(expected),file)!=sizeof(expected) ||
           fgetc(file)!=EOF || fclose(file) || memcmp(expected,observed,sizeof(expected)))fail();
    }
    recon_native_entry_finish(1);
#endif
}
#ifdef RECON_ORIGINAL_DIRECT
extern "C" __declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    probe=context;
    if(!context || context->abi_version!=RECON_ORIGINAL_PROBE_VERSION ||
       context->struct_size!=sizeof(*context) || context->marker_rva!=0xde90 ||
       !context->image_base || !context->original_esp)fail();
    check_cleanup();
    fail();
}
#else
static int at_main(int argc,char **argv,char **environment)
{
    (void)argc;(void)argv;(void)environment;
    check_cleanup();
    return 0;
}
static void invoke(void *)
{
    char *args[]={ (char *)"LINK.EXE",NULL };
    recon_link_invoke(1,args);
}
static int direct(void)
{
    int passed=recon_native_at_entry((const void *)link_main,(const void *)at_main,invoke,NULL)==1;
    /* Match original probe completion: never resume target exit callbacks on
     * the deliberately consumed startup owner. Publish the full DWORD first. */
    recon_record_terminal_status(passed?0:1);
    _exit(passed?0:1);
}
static const uint32_t order[]={
    0x24e60,0x24e70,0x24ea0,0x31d70,0x31d80,0x31db0,
    0x31de0,0x31df0,0x31e20,0x44e60,0x44e70,0x44ea0,
    0xde90,0x331a0
};
RECON_EXPORT const ReconCase case_descriptor={
    RECON_ABI_VERSION,sizeof(ReconCase),"symbol-buffer-dtor",
    "Internal only: NULL and genuine startup-new allocation in a 12-byte scoped buffer; no ordinary CLI reach claim.",
    order,RECON_COUNT(order),NULL,0,RECON_DIRECT,RECON_EXIT,0,
    recon_link_setup,NULL,direct,NULL,0
};
#endif
