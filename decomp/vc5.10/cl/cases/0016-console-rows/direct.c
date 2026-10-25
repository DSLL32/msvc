/* Internal utilities on genuine initialized driver state; no console setup. */
#define RECON_SHIM_IMPL
#include <stddef.h>
#include "cl.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-native.h>
#endif
#include <recon.h>
typedef int (__cdecl *ConsoleRows)(void);
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *probe;
#endif
typedef struct ConsoleResult {
    unsigned app[8];
    unsigned handle,open_error,open_errno,open_doserrno;
    unsigned query_attempted,query_result,query_error;
    unsigned close_result,close_error,close_errno,close_doserrno;
    CONSOLE_SCREEN_BUFFER_INFO screen;
} ConsoleResult;
static ConsoleResult observed;
static void fail(void)
{
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe,0);
#else
    recon_native_entry_finish(0);
#endif
}

static int checked_call(ConsoleRows rows)
{
    unsigned stack0,stack1,regs0[4],regs1[4];
    unsigned short cw0,cw1;
    int result;
    __asm {
        mov stack0,esp
        fnstcw cw0
        lea eax,regs0
        mov [eax],ebx
        mov [eax+4],esi
        mov [eax+8],edi
        mov [eax+12],ebp
    }
    result=rows();
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
    ConsoleRows rows;
    Driver saved;
    int *running;
    CompilerPass **active;
    int *error_slot;
    unsigned long *dos_slot;
    HANDLE handle;
#ifdef RECON_ORIGINAL_DIRECT
    typedef int *(__cdecl *ErrorSlot)(void);
    typedef unsigned long *(__cdecl *DosSlot)(void);
    HMODULE provider;
    ErrorSlot get_error;DosSlot get_dos;
#endif
    (void)pending;
    if(!state || !head || !head->next || head->next->next ||
       state->inputs!=head->input || strcmp(head->input->name,"first.c") ||
       strcmp(head->next->input->name,"second.c")) fail();
    saved=*state;
#ifdef RECON_ORIGINAL_DIRECT
    rows=(ConsoleRows)RECON_ORIGINAL_ADDRESS(probe,0x5816);
    running=(int *)RECON_ORIGINAL_ADDRESS(probe,0x97b0);
    active=(CompilerPass **)RECON_ORIGINAL_ADDRESS(probe,0x97ac);
    provider=GetModuleHandleA("MSVCRT.DLL");
    get_error=provider?(ErrorSlot)GetProcAddress(provider,"_errno"):NULL;
    get_dos=provider?(DosSlot)GetProcAddress(provider,"__doserrno"):NULL;
    if(!get_error || !get_dos) fail();
    error_slot=get_error();dos_slot=get_dos();
    if(!error_slot || !dos_slot) fail();
#else
    rows=console_rows;
    running=&pass_running;active=&active_pass;
    error_slot=&errno;dos_slot=&recon_doserrno;
#endif
    if(*running || *active) fail();
    memset(&observed,0,sizeof(observed));
    *error_slot=111;*dos_slot=222;
#ifdef RECON_ORIGINAL_DIRECT
    SetLastError(333);
#else
    recon_SetLastError(333);
#endif
    observed.app[0]=111;observed.app[1]=222;observed.app[2]=333;
    observed.app[3]=(unsigned)checked_call(rows);
#ifdef RECON_ORIGINAL_DIRECT
    observed.app[6]=GetLastError();
#else
    observed.app[6]=recon_GetLastError();
#endif
    observed.app[4]=(unsigned)*error_slot;observed.app[5]=*dos_slot;
    observed.app[7]=memcmp(state,&saved,sizeof(saved))==0 && !*running && !*active;
    if(!observed.app[7]) fail();
    /* Separate public API input test. This does not identify the earlier
     * unobserved App branch. Both calls receive the declared error tuple. */
    *error_slot=111;*dos_slot=222;
    memset(&observed.screen,0xa5,sizeof(observed.screen));
#ifdef RECON_ORIGINAL_DIRECT
    SetLastError(333);
    handle=CreateFileA("CONOUT$",GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    observed.open_error=GetLastError();
#else
    recon_SetLastError(333);
    handle=recon_CreateFileA("CONOUT$",GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    observed.open_error=recon_GetLastError();
#endif
    observed.handle=(unsigned)handle;
    observed.open_errno=(unsigned)*error_slot;observed.open_doserrno=*dos_slot;
    if(handle!=INVALID_HANDLE_VALUE) {
        observed.query_attempted=1;
#ifdef RECON_ORIGINAL_DIRECT
        observed.query_result=GetConsoleScreenBufferInfo(handle,&observed.screen);
        observed.query_error=GetLastError();
        observed.close_result=CloseHandle(handle);observed.close_error=GetLastError();
#else
        observed.query_result=recon_GetConsoleScreenBufferInfo(handle,&observed.screen);
        observed.query_error=recon_GetLastError();
        observed.close_result=recon_CloseHandle(handle);observed.close_error=recon_GetLastError();
#endif
        observed.close_errno=(unsigned)*error_slot;observed.close_doserrno=*dos_slot;
    }
#ifdef RECON_ORIGINAL_DIRECT
    if(!recon_original_emit(probe,"console-rows.bin",&observed,sizeof(observed))) fail();
#else
    {
        ConsoleResult expected;
        FILE *file=fopen("console-rows.bin","rb");
        if(!file || fread(&expected,1,sizeof(expected),file)!=sizeof(expected) ||
           fgetc(file)!=EOF || fclose(file)) fail();
        /* Live handles are opaque per-process identities; the exact returned
         * invalid sentinel and same-handle query/close roles remain strict. */
        if((expected.handle==(unsigned)INVALID_HANDLE_VALUE)!=
           (observed.handle==(unsigned)INVALID_HANDLE_VALUE) ||
           memcmp(&expected.app,&observed.app,sizeof(observed.app)) ||
           memcmp(&expected.open_error,&observed.open_error,
                  sizeof(observed)-offsetof(ConsoleResult,open_error))) fail();
    }
#endif
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe,1);
#else
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
int cl_console_rows_direct(void)
{
    return recon_native_at_entry((const void *)prepare_jobs,(const void *)at_prepare,invoke,NULL)==1;
}
#endif
