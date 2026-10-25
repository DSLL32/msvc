/* Internal pass configuration on real driver state and App-owned Option nodes. */
#define RECON_SHIM_IMPL
#include <stddef.h>
#include "cl.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-native.h>
#endif
#include <recon.h>
typedef void (__cdecl *ChangePass)(Option *);
typedef Option *(__cdecl *MakeOption)(char *,char *,char *,unsigned);
typedef char PassSize[(sizeof(CompilerPass)==36)?1:-1];
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *probe;
#endif
static unsigned observed[45], used;
static void fail(void)
{
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe,0);
#else
    recon_native_entry_finish(0);
#endif
}

static void checked_call(ChangePass change,Option *option)
{
    unsigned stack0,stack1,regs0[4],regs1[4];
    unsigned short cw0,cw1;
    __asm {
        mov stack0,esp
        fnstcw cw0
        lea eax,regs0
        mov [eax],ebx
        mov [eax+4],esi
        mov [eax+8],edi
        mov [eax+12],ebp
    }
    change(option);
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
}


static void record(CompilerPass *passes,const CompilerPass *before)
{
    CompilerPass expected;
    int i;
    for(i=0;i<5;++i) {
        expected=before[i];
        expected.flags=passes[i].flags;
        if(memcmp(&expected,&passes[i],sizeof(expected)) ||
           (passes[i].flags&~1u)!=(before[i].flags&~1u) || used>=45) fail();
        observed[used++]=passes[i].flags;
    }
}
static Job *at_prepare(Driver *state,Job *head,PendingWork *pending)
{
    CompilerPass *passes,before[5];
    Driver saved;
    MakeOption make;
    ChangePass enable,disable,toggle;
    Option *off1,*off2,*on1,*on2,*change,*unknown,*missing,*self;
    (void)pending;
    if(!state || !head || !head->next || head->next->next ||
       state->inputs!=head->input || head->input->next!=head->next->input ||
       strcmp(head->input->name,"first.c") || strcmp(head->next->input->name,"second.c")) fail();
    saved=*state;
#ifdef RECON_ORIGINAL_DIRECT
    passes=(CompilerPass *)RECON_ORIGINAL_ADDRESS(probe,0x90a8);
    make=(MakeOption)RECON_ORIGINAL_ADDRESS(probe,0x1392);
    enable=(ChangePass)RECON_ORIGINAL_ADDRESS(probe,0x2432);
    disable=(ChangePass)RECON_ORIGINAL_ADDRESS(probe,0x2458);
    toggle=(ChangePass)RECON_ORIGINAL_ADDRESS(probe,0x2499);
#else
    passes=compiler_passes;make=new_option;
    enable=enable_pass;disable=disable_pass;toggle=toggle_pass;
#endif
    if(strcmp(passes[0].option_prefix,"D-") || strcmp(passes[1].option_prefix,"1-") ||
       strcmp(passes[2].option_prefix,"P-") || strcmp(passes[3].option_prefix,"2-") ||
       strcmp(passes[4].option_prefix,"C/")) fail();
    memcpy(before,passes,sizeof(before));
    off1=make("bd","1","D",0);off2=make("bd","2","D",0);
    on1=make("ba","1","D",0);on2=make("ba","2","D",0);
    change=make("bc","12","D",0);unknown=make("bc","?2","D",0);
    missing=make("bc","1?","D",0);self=make("bc","11","D",0);
    if(!off1 || !off2 || !on1 || !on2 || !change || !unknown || !missing || !self ||
       off1==off2 || change==unknown || change->value==unknown->value || ((unsigned)unknown&1)) fail();
    record(passes,before);
    checked_call(disable,off1);checked_call(disable,off2);record(passes,before);
    checked_call(toggle,change);record(passes,before); /* Disabled source: no change. */
    checked_call(enable,on1);record(passes,before);
    checked_call(toggle,change);record(passes,before); /* Enabled source enables target. */
    checked_call(disable,off2);checked_call(toggle,missing);record(passes,before);
    checked_call(toggle,unknown);record(passes,before); /* Unknown source retains the actual Option address bits. */
    checked_call(toggle,self);record(passes,before);
    checked_call(enable,on2);record(passes,before);
    if(used!=45 || memcmp(state,&saved,sizeof(saved)) ||
       strcmp(change->value,"12") || strcmp(unknown->value,"?2") ||
       !(passes[1].flags&1) || !(passes[3].flags&1)) fail();
#ifdef RECON_ORIGINAL_DIRECT
    if(!recon_original_emit(probe,"pass-toggle.bin",observed,sizeof(observed))) fail();
    recon_original_finish(probe,1);
#else
    {
        unsigned expected[45];FILE *file=fopen("pass-toggle.bin","rb");
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
int cl_pass_toggle_direct(void)
{
    return recon_native_at_entry((const void *)prepare_jobs,(const void *)at_prepare,invoke,NULL)==1;
}
#endif
