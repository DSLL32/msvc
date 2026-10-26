/* Internal20-byte cleanup-method probe at genuine main entry after startup. */
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <new.h>
#include "../../src/definition-buffer.h"
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

struct BufferLayout { void *data,*end; int capacity; DWORD callback,context; };
typedef char BufferLayoutSize[(sizeof(BufferLayout)==20)?1:-1];
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *probe;
/* Single-inheritance nonvirtual member projection: the VC5 member pointer is
 * exactly one code DWORD. This is typed probe glue, never target source. */
class OriginalCleanup { public: void Invoke(); int Set(BYTE *,BYTE *); };
typedef void (OriginalCleanup::*OriginalMethod)();
typedef char MemberPointerSize[(sizeof(OriginalMethod)==4)?1:-1];
static void destroy(BufferLayout *object)
{
    union { DWORD address; OriginalMethod method; } target;
    target.address=(DWORD)RECON_ORIGINAL_ADDRESS(probe,0x1f950);
    (((OriginalCleanup *)object)->*target.method)();
}
static int set_span(BufferLayout *object,BYTE *base,BYTE *end)
{
    typedef int (OriginalCleanup::*SetMethod)(BYTE *,BYTE *);
    union { DWORD address; SetMethod method; } target;
    target.address=(DWORD)RECON_ORIGINAL_ADDRESS(probe,0x1f910);
    return (((OriginalCleanup *)object)->*target.method)(base,end);
}
#else
/* Invoke the one real retained nonvirtual destructor outline. */
static void destroy(BufferLayout *object)
{
    ((DefinitionBuffer *)object)->~DefinitionStorage();
}
static int set_span(BufferLayout *object,BYTE *base,BYTE *end)
{
    return ((DefinitionBuffer *)object)->Set(base,end);
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
    BYTE old_span[16],new_span[24];
    int result;
    unsigned observed[18];
#ifndef RECON_ORIGINAL_DIRECT
    ReconCppAllocation *allocation;
    unsigned long long generation;
#endif
    object.data=NULL; object.end=(void *)0x12345678; object.capacity=0x23456789;
    object.callback=0x3456789a; object.context=0x456789ab;
    before=object;
    destroy(&object);
    if(memcmp(&object,&before,sizeof(object)))fail();
    observed[0]=1; observed[1]=(DWORD)object.end; observed[2]=object.capacity;
    observed[3]=object.callback; observed[4]=object.context;
#ifdef RECON_ORIGINAL_DIRECT
    owner=(BufferBlock *)RECON_ORIGINAL_ADDRESS(probe,0x809d0);
#else
    owner=&buffer12;
#endif
    if(!owner->base || owner->remaining!=1024 || owner->capacity!=1024)fail();
    object.data=owner->base; object.end=(BYTE *)object.data+7; object.capacity=0x3000;
    object.callback=0x55667788; object.context=0x66778899;
    before=object;
#ifndef RECON_ORIGINAL_DIRECT
    for(allocation=recon_cpp_allocations;allocation;allocation=allocation->next)
        if(allocation->base==object.data && allocation->live)break;
    if(!allocation || allocation->requested!=0x3000)fail();
    generation=allocation->generation;
#endif
    destroy(&object);
    before.capacity=0;
    /* Compare the live owner fields and pointer values, never freed payload. */
    if(memcmp(&object,&before,sizeof(object)) || owner->base!=before.data ||
       owner->remaining!=1024 || owner->capacity!=1024)fail();
#ifndef RECON_ORIGINAL_DIRECT
    if(allocation->live || allocation->generation!=generation)fail();
#endif
    observed[5]=1; observed[6]=object.capacity; observed[7]=object.callback;
    observed[8]=object.context; observed[9]=owner->remaining;
    /* Both spans remain live through the genuine Set invocations. This probe
     * exercises no relocation callback target; reject that domain beforehand. */
    memset(old_span,0x35,sizeof(old_span));
    memset(new_span,0x79,sizeof(new_span));
    object.data=old_span; object.end=old_span+9; object.capacity=0x13579bdf;
    object.callback=0; object.context=0x2468ace0;
    before=object;
    if(object.callback)fail();
    result=set_span(&object,NULL,NULL);
    if(result!=0 || memcmp(&object,&before,sizeof(object)))fail();
    observed[10]=result; observed[11]=1;
    if(object.callback)fail();
    result=set_span(&object,new_span,new_span+19);
    before.data=new_span; before.end=new_span+19;
    if(result!=1 || memcmp(&object,&before,sizeof(object)) ||
       old_span[0]!=0x35 || old_span[15]!=0x35 ||
       new_span[0]!=0x79 || new_span[23]!=0x79)fail();
    observed[12]=result; observed[13]=(BYTE *)object.end-(BYTE *)object.data;
    observed[14]=object.capacity; observed[15]=object.callback;
    observed[16]=object.context; observed[17]=1;
#ifdef RECON_ORIGINAL_DIRECT
    if(!recon_original_emit(probe,"definition-cleanup.bin",observed,sizeof(observed)))fail();
    recon_original_finish(probe,1);
#else
    {
        unsigned expected[18];
        FILE *file=fopen("definition-cleanup.bin","rb");
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
    0xde90,0x1f950,0x1f910
};
RECON_EXPORT const ReconCase case_descriptor={
    RECON_ABI_VERSION,sizeof(ReconCase),"definition-buffer-cleanup",
    "Internal 20-byte buffer: NULL/owned cleanup and Set NULL/live-span paths with callback NULL; no callback-target or ordinary CLI reach claim.",
    order,RECON_COUNT(order),NULL,0,RECON_DIRECT,RECON_EXIT,0,
    recon_link_setup,NULL,direct,NULL,0
};
#endif
