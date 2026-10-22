/* Historical first-entry replay. This debugger is not target reconstruction. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { DWORD rva,size; unsigned char *bytes; } Chunk;
typedef struct {
    char *name; DWORD entries,chunks; DWORD *entry; Chunk *chunk;
} Function;
static FILE *events;
static PROCESS_INFORMATION process;
static DEBUG_EVENT pending;
static int pending_event,exited;
static DWORD base,image_size,index,step_count,started,timeout_ms;
static Function *steps,terminal;
static CONTEXT context;
static HANDLE child_process;

static void string_json(const char *text)
{
    const unsigned char *p=(const unsigned char *)text;
    fputc('"',events);
    for(;*p;++p) {
        if(*p=='"' || *p=='\\') { fputc('\\',events);fputc(*p,events); }
        else if(*p<32 || *p>=127) fprintf(events,"\\u%04x",*p);
        else fputc(*p,events);
    }
    fputc('"',events);
}
static void registers_json(void)
{
    fprintf(events,",\"registers\":{\"eip\":%lu,\"esp\":%lu,\"ebp\":%lu,"
            "\"eax\":%lu,\"ebx\":%lu,\"ecx\":%lu,\"edx\":%lu,"
            "\"esi\":%lu,\"edi\":%lu,\"eflags\":%lu}",
            context.Eip,context.Esp,context.Ebp,context.Eax,context.Ebx,
            context.Ecx,context.Edx,context.Esi,context.Edi,context.EFlags);
}
static void cleanup(void)
{
    DEBUG_EVENT ev;DWORD until=GetTickCount();
    if(child_process) TerminateProcess(child_process,99);
    if(process.hProcess && !exited) TerminateProcess(process.hProcess,99);
    if(pending_event) {
        ContinueDebugEvent(pending.dwProcessId,pending.dwThreadId,DBG_CONTINUE);
        pending_event=0;
    }
    while(process.hProcess && !exited && GetTickCount()-until<5000) {
        if(!WaitForDebugEvent(&ev,100)) continue;
        if(ev.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            TerminateProcess(ev.u.CreateProcessInfo.hProcess,99);
            if(ev.u.CreateProcessInfo.hFile)CloseHandle(ev.u.CreateProcessInfo.hFile);
        } else if(ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT &&
                  ev.dwProcessId==process.dwProcessId) exited=1;
        else if(ev.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT && ev.u.LoadDll.hFile)
            CloseHandle(ev.u.LoadDll.hFile);
        ContinueDebugEvent(ev.dwProcessId,ev.dwThreadId,DBG_CONTINUE);
    }
    if(process.hProcess) { WaitForSingleObject(process.hProcess,1000);CloseHandle(process.hProcess); }
    if(process.hThread) CloseHandle(process.hThread);
    if(child_process) CloseHandle(child_process);
}
static void fail(const char *reason)
{
    DWORD error=GetLastError();
    fprintf(events,"{\"event\":\"failure\",\"index\":%lu,\"reason\":",index);
    string_json(reason);
    fprintf(events,",\"winerror\":%lu,\"pid\":%lu,\"base\":%lu,\"expected\":",
            error,process.dwProcessId,base);
    string_json(steps && index<step_count && steps[index].name?
                steps[index].name:terminal.name?terminal.name:"");
    if(pending_event && pending.dwDebugEventCode==EXCEPTION_DEBUG_EVENT)
        fprintf(events,",\"thread\":%lu,\"exception\":%lu,\"address\":%lu",
                pending.dwThreadId,pending.u.Exception.ExceptionRecord.ExceptionCode,
                (DWORD)pending.u.Exception.ExceptionRecord.ExceptionAddress);
    registers_json();fputs("}\n",events);fflush(events);
    cleanup();fclose(events);exit(1);
}
static DWORD number(FILE *file)
{
    DWORD value;
    if(fread(&value,4,1,file)!=1)fail("truncated restore plan");
    return value;
}
static char *blob(FILE *file,DWORD *length)
{
    char *value;DWORD size=number(file);
    if(size>64*1024*1024)fail("oversized plan blob");
    value=(char *)malloc(size+1);
    if(!value || (size && fread(value,size,1,file)!=1))fail("plan blob read");
    value[size]=0;if(length)*length=size;return value;
}
static void load_function(FILE *file,Function *function,int restores)
{
    DWORD i,size;
    function->name=blob(file,&size);
    if(!size || size>1024)fail("invalid function name");
    function->entries=number(file);function->chunks=number(file);
    if(function->entries>65536 || function->chunks>65536 ||
       (!restores && function->chunks)) fail("invalid function plan");
    function->entry=(DWORD *)calloc(function->entries?function->entries:1,4);
    function->chunk=(Chunk *)calloc(function->chunks?function->chunks:1,sizeof(Chunk));
    if(!function->entry || !function->chunk)fail("function allocation");
    for(i=0;i<function->entries;++i) {
        function->entry[i]=number(file);
        if(function->entry[i]>=image_size)fail("entry outside image");
    }
    for(i=0;i<function->chunks;++i) {
        Chunk *chunk=function->chunk+i;
        chunk->rva=number(file);chunk->size=number(file);
        if(!chunk->size || chunk->rva>=image_size || chunk->size>image_size-chunk->rva)
            fail("restore range outside image");
        chunk->bytes=(unsigned char *)malloc(chunk->size);
        if(!chunk->bytes || fread(chunk->bytes,chunk->size,1,file)!=1)
            fail("restore range read");
    }
}
static int is_entry(Function *function,DWORD rva)
{
    DWORD i;
    for(i=0;i<function->entries;++i)if(function->entry[i]==rva)return 1;
    return 0;
}
static void restore(Function *function)
{
    DWORD i,written,old,discard,read;unsigned char *verify;
    for(i=0;i<function->chunks;++i) {
        Chunk *chunk=function->chunk+i;void *address=(void *)(base+chunk->rva);
        verify=(unsigned char *)malloc(chunk->size);
        if(!verify)fail("readback allocation");
        if(!VirtualProtectEx(process.hProcess,address,chunk->size,PAGE_EXECUTE_READWRITE,&old))
            fail("restore protection");
        if(!WriteProcessMemory(process.hProcess,address,chunk->bytes,chunk->size,&written) ||
           written!=chunk->size) fail("restore write");
        if(!ReadProcessMemory(process.hProcess,address,verify,chunk->size,&read) ||
           read!=chunk->size || memcmp(verify,chunk->bytes,chunk->size))fail("restore readback");
        free(verify);
        if(!FlushInstructionCache(process.hProcess,address,chunk->size))fail("instruction flush");
        if(!VirtualProtectEx(process.hProcess,address,chunk->size,old,&discard))fail("restore old protection");
    }
    context.Eip=(DWORD)pending.u.Exception.ExceptionRecord.ExceptionAddress;
    if(!SetThreadContext(process.hThread,&context))fail("rewind instruction pointer");
}
int main(int argc,char **argv)
{
    FILE *plan;DWORD preferred,terminal_kind,terminal_exit,i,length,code,address;
    char *application,*command,*cwd,*out_path,*err_path,*environment;
    STARTUPINFOA startup;SECURITY_ATTRIBUTES security;HANDLE out,err,input;
    int loader_break=0,terminal_reached=0;
    if(argc!=3)return 2;
    events=fopen(argv[2],"w");if(!events)return 3;
    plan=fopen(argv[1],"rb");if(!plan)fail("open restore plan");
    if(number(plan)!=0x32505246)fail("wrong restore plan version");
    preferred=number(plan);image_size=number(plan);timeout_ms=number(plan);
    step_count=number(plan);terminal_kind=number(plan);terminal_exit=number(plan);
    if(!image_size || !timeout_ms || step_count>100000 || terminal_kind>1)fail("invalid plan header");
    application=blob(plan,0);command=blob(plan,0);cwd=blob(plan,0);
    out_path=blob(plan,0);err_path=blob(plan,0);environment=blob(plan,&length);
    if(length && (length<2 || environment[length-1] || environment[length-2]))
        fail("environment missing double NUL");
    if(!length) { free(environment);environment=0; }
    load_function(plan,&terminal,0);
    steps=(Function *)calloc(step_count?step_count:1,sizeof(Function));
    if(!steps)fail("step allocation");
    for(i=0;i<step_count;++i)load_function(plan,steps+i,1);
    if(fgetc(plan)!=EOF)fail("trailing restore plan data");fclose(plan);
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    out=CreateFileA(out_path,GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    err=CreateFileA(err_path,GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    input=CreateFileA("NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,0);
    if(out==INVALID_HANDLE_VALUE || err==INVALID_HANDLE_VALUE || input==INVALID_HANDLE_VALUE)
        fail("open child standard handles");
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdInput=input;startup.hStdOutput=out;startup.hStdError=err;
    started=GetTickCount();
    if(!CreateProcessA(application,command,0,0,TRUE,DEBUG_PROCESS,environment,cwd,&startup,&process))
        fail("create target process");
    CloseHandle(input);CloseHandle(out);CloseHandle(err);
    fprintf(events,"{\"event\":\"start\",\"pid\":%lu,\"preferred_base\":%lu}\n",
            process.dwProcessId,preferred);fflush(events);
    for(;;) {
        if(GetTickCount()-started>=timeout_ms)fail("target timeout");
        if(!WaitForDebugEvent(&pending,1000)) {
            code=GetLastError();
            if(code==ERROR_SEM_TIMEOUT || code==WAIT_TIMEOUT)continue;
            fail("wait for debug event");
        }
        pending_event=1;
        if(pending.dwProcessId!=process.dwProcessId) {
            if(pending.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT)
                child_process=pending.u.CreateProcessInfo.hProcess;
            fail("unsupported child process");
        }
        if(pending.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            base=(DWORD)pending.u.CreateProcessInfo.lpBaseOfImage;
            if(pending.u.CreateProcessInfo.hFile)CloseHandle(pending.u.CreateProcessInfo.hFile);
            if(base!=preferred)fail("image relocated; raw restore requires preferred base");
        } else if(pending.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
            CloseHandle(pending.u.CreateThread.hThread);fail("unsupported application thread");
        } else if(pending.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            if(pending.u.LoadDll.hFile)CloseHandle(pending.u.LoadDll.hFile);
        } else if(pending.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            code=pending.u.Exception.ExceptionRecord.ExceptionCode;
            address=(DWORD)pending.u.Exception.ExceptionRecord.ExceptionAddress;
            if(code==EXCEPTION_BREAKPOINT && !loader_break && (address<base || address-base>=image_size))
                loader_break=1;
            else {
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                if(!GetThreadContext(process.hThread,&context))fail("read trapping thread context");
                if(pending.dwThreadId!=process.dwThreadId)fail("trap in unsupported thread");
                if(code!=EXCEPTION_BREAKPOINT || !pending.u.Exception.dwFirstChance)
                    fail("unexpected target exception");
                if(index<step_count) {
                    if(!is_entry(steps+index,address-base))fail("wrong frontier entry");
                    fprintf(events,"{\"event\":\"frontier\",\"index\":%lu,\"name\":",index);
                    string_json(steps[index].name);
                    fprintf(events,",\"rva\":%lu,\"thread\":%lu",address-base,pending.dwThreadId);
                    registers_json();fputs("}\n",events);fflush(events);
                    restore(steps+index);++index;
                } else {
                    if(!terminal_kind || !is_entry(&terminal,address-base))fail("wrong terminal frontier");
                    fputs("{\"event\":\"terminal\",\"name\":",events);string_json(terminal.name);
                    fprintf(events,",\"rva\":%lu,\"thread\":%lu",address-base,pending.dwThreadId);
                    registers_json();fputs("}\n",events);fflush(events);
                    terminal_reached=1;
                    if(!TerminateProcess(process.hProcess,99))fail("terminate at terminal frontier");
                }
            }
        } else if(pending.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            exited=1;
            if(index!=step_count)fail("premature target exit");
            if(!terminal_kind) {
                if(pending.u.ExitProcess.dwExitCode!=terminal_exit)fail("wrong target exit status");
                fputs("{\"event\":\"terminal\",\"name\":",events);string_json(terminal.name);
                fprintf(events,",\"exit_code\":%lu}\n",pending.u.ExitProcess.dwExitCode);
                terminal_reached=1;
            }
            if(!terminal_reached)fail("target exited before terminal frontier");
            if(!ContinueDebugEvent(pending.dwProcessId,pending.dwThreadId,DBG_CONTINUE))fail("continue exit");
            pending_event=0;
            fprintf(events,"{\"event\":\"pass\",\"frontiers\":%lu,\"milliseconds\":%lu}\n",
                    index,GetTickCount()-started);fflush(events);cleanup();fclose(events);return 0;
        }
        if(!ContinueDebugEvent(pending.dwProcessId,pending.dwThreadId,DBG_CONTINUE))fail("continue target");
        pending_event=0;
    }
}
