/* Internal-only probe: real source_jobs owners, no fabricated Job storage. */
#define RECON_SHIM_IMPL
#include <stddef.h>
#include "cl.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-native.h>
#endif
#include <recon.h>

typedef DeferredInput *(__cdecl *ConvertJobs)(Job *);
typedef char InputSize[(sizeof(Input)==16)?1:-1];
typedef char DeferredOffset[(offsetof(Job,deferred)==12)?1:-1];
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *probe;
#endif
static unsigned observed[8];
static void fail(void)
{
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe,0);
#else
    recon_native_entry_finish(0);
#endif
}

static DeferredInput *checked_call(ConvertJobs convert,Job *jobs)
{
    unsigned stack0,stack1,regs0[4],regs1[4];
    unsigned short cw0,cw1;
    DeferredInput *result;
    __asm {
        mov stack0,esp
        fnstcw cw0
        lea eax,regs0
        mov [eax],ebx
        mov [eax+4],esi
        mov [eax+8],edi
        mov [eax+12],ebp
    }
    result=convert(jobs);
    __asm {
        mov stack1,esp
        fnstcw cw1
        lea eax,regs1
        mov [eax],ebx
        mov [eax+4],esi
        mov [eax+8],edi
        mov [eax+12],ebp
    }
    if(stack0!=stack1 || cw0!=cw1 || memcmp(regs0,regs1,sizeof(regs0))) fail();
    return result;
}

static Job *at_prepare(Driver *state,Job *head,PendingWork *pending)
{
    Job *tail;
    Input *inputs;
    Input first_input,second_input;
    DeferredInput *result;
    ConvertJobs convert;
    (void)pending; /* Uninitialized caller storage: never read. */
    if(!state || !head || !(tail=head->next) || tail==head || tail->next ||
       !head->input || !tail->input || head->input==tail->input ||
       head->status || tail->status) fail();
    inputs=state->inputs;
    if(inputs!=head->input || inputs->next!=tail->input || tail->input->next ||
       strcmp(inputs->name,"first.c") || strcmp(tail->input->name,"second.c") ||
       inputs->kind!=0 || tail->input->kind!=0) fail();
    first_input=*inputs;
    second_input=*tail->input;
#ifdef RECON_ORIGINAL_DIRECT
    convert=(ConvertJobs)RECON_ORIGINAL_ADDRESS(probe,0x3b8e);
#else
    convert=deferred_from_jobs;
#endif
    observed[0]=2; /* Two independently allocated live owners. */
    result=checked_call(convert,NULL);
    observed[1]=result==NULL;
    if(!observed[1]) fail();
    result=checked_call(convert,tail);
    observed[2]=result==&tail->deferred?2:99;
    observed[3]=tail->deferred.next==NULL?0:99;
    if(observed[2]!=2 || observed[3]) fail();
    result=checked_call(convert,head);
    observed[4]=result==&head->deferred?1:99;
    observed[5]=head->deferred.next==&tail->deferred?2:99;
    observed[6]=tail->deferred.next==NULL?0:99;
    if(observed[4]!=1 || observed[5]!=2 || observed[6] ||
       state->inputs!=inputs || head->next!=tail || tail->next ||
       head->input!=inputs || tail->input!=inputs->next ||
       head->status || tail->status ||
       memcmp(inputs,&first_input,sizeof(Input)) ||
       memcmp(tail->input,&second_input,sizeof(Input))) fail();
    observed[7]=1;
    /* Only deferred.next is inspected after initialization by the real call.
     * Other DeferredInput fields and the Job allocation tails remain unread. */
#ifdef RECON_ORIGINAL_DIRECT
    if(!recon_original_emit(probe,"job-deferred.bin",observed,sizeof(observed))) fail();
    recon_original_finish(probe,1);
#else
    {
        unsigned expected[8];
        FILE *file=fopen("job-deferred.bin","rb");
        if(!file || fread(expected,1,sizeof(expected),file)!=sizeof(expected) ||
           fgetc(file)!=EOF || fclose(file) || memcmp(expected,observed,sizeof(expected))) fail();
    }
    recon_native_entry_finish(1);
#endif
    return NULL;
}

#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    unsigned *stack;
    probe=context;
    if(!context || context->abi_version!=RECON_ORIGINAL_PROBE_VERSION ||
       context->struct_size!=sizeof(*context) || context->marker_rva!=0x355e ||
       !context->image_base || !context->original_esp) fail();
    stack=(unsigned *)context->original_esp;
    /* Actual run_driver CALL at40334c, after source_jobs returned in EAX. */
    if(stack[0]!=(unsigned)RECON_ORIGINAL_ADDRESS(context,0x3351)) fail();
    at_prepare((Driver *)stack[1],(Job *)stack[2],(PendingWork *)stack[3]);
    fail();
}
#else
static void invoke(void *unused)
{
    char *args[]={"CL.EXE","/nologo","/Bz","/c","first.c","second.c",NULL};
    (void)unused;
    /* Existing qualified Win32 CRT precision boundary; no target state seed. */
    recon_controlfp(0x10000,0x30000);
    main(6,args,NULL);
}
int cl_job_direct(void)
{
    return recon_native_at_entry((const void *)prepare_jobs,(const void *)at_prepare,invoke,NULL)==1;
}
#endif
