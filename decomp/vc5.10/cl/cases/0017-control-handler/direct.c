/* Internal inactive-control path; post-exit globals are not observed. */
#define RECON_SHIM_IMPL
#include "cl.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-native.h>
#endif
#include <recon.h>
typedef BOOL (WINAPI *ControlHandler)(DWORD);
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *probe;
#endif
static void fail(void)
{
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe,0);
#else
    recon_native_entry_finish(0);
#endif
}
static Job *at_prepare(Driver *state,Job *head,PendingWork *pending)
{
    ControlHandler handler;
    int *running,*was_interrupted;
    CompilerPass **active;
    (void)pending;
    if(!state || !head || !head->next || head->next->next ||
       state->inputs!=head->input || strcmp(head->input->name,"first.c") ||
       strcmp(head->next->input->name,"second.c")) fail();
#ifdef RECON_ORIGINAL_DIRECT
    handler=(ControlHandler)RECON_ORIGINAL_ADDRESS(probe,0x57c2);
    running=(int *)RECON_ORIGINAL_ADDRESS(probe,0x97b0);
    was_interrupted=(int *)RECON_ORIGINAL_ADDRESS(probe,0x97a8);
    active=(CompilerPass **)RECON_ORIGINAL_ADDRESS(probe,0x97ac);
#else
    handler=console_control_handler;
    running=&pass_running;was_interrupted=&interrupted;active=&active_pass;
#endif
    if(*was_interrupted || *running || *active) fail();
    handler(CTRL_C_EVENT); /* Genuine idle path exits4; it must not return. */
    fail();
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
int cl_control_handler_direct(void)
{
    return recon_native_at_entry((const void *)prepare_jobs,(const void *)at_prepare,invoke,NULL)==1;
}
#endif
