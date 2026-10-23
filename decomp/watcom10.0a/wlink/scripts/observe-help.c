/* Target-scoped UNMASKED diagnostic observer, not reconstructed WLINK logic.
 * Public debugger/RPM/WPM/owned-cleanup patterns were reviewed in immutable
 * WLIB v3 SHA bb2d7176ba54d76367d0de370220516657c8b26a59580eb644a58d53aed37eb0.
 * Persistent FF25 stepping and API frames below are NEW and require their own
 * authored Win32 probes. No WLIB guard code or CF protocol is incorporated. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "observer-state.h"
#include <recon.h>

#define MAX_THUNKS 64
#define MAX_FRAMES 32
#define MAX_OBJECT 1048576UL
#define MAX_BYTES 16777216UL
typedef struct Thunk {
    DWORD rva, iat, address, target, argc, no_return, calls;
    char name[64];
    int armed;
} Thunk;
typedef struct Frame {
    unsigned thunk;
    DWORD thread, esp, address, args[7];
    BYTE saved;
} Frame;
static HANDLE child, primary;
static DWORD process_id, thread_id, image_base, configured_base, image_size;
static DWORD entry_rva, main_rva, entry_byte, main_byte, main_kind;
static DWORD ntdll_base, ntdll_size, total_bytes, api_events;
static Thunk thunks[MAX_THUNKS];
static unsigned thunk_count, depth;
static Frame frames[MAX_FRAMES];
static ObStep step;
static unsigned step_thunk;
static FILE *events;
static DEBUG_EVENT pending;
static int event_pending, entry_seen, main_seen, loader_seen, exit_requested;
static DWORD exit_status;
static int real_exit_seen;
static DWORD real_exit_status;
#ifdef OBS_AUTHORED_PROBE
/* Compiled out of the real-target observer. Only a hash-bound authored child
 * may be run by the separate reviewed probe broker with these fault modes. */
static unsigned probe_fault;
static DWORD probe_alternate_iat;
static char probe_trigger[64];
static int probe_fault_used;
typedef struct ProbeMarker { DWORD rva,address; int dll; char name[64]; } ProbeMarker;
static ProbeMarker probe_markers[8];
static unsigned probe_marker_count;
static void read_probe_markers(const char *kind)
{
    unsigned i;
    DWORD value,got;
    for(i=0;i<probe_marker_count;++i) {
        if(probe_markers[i].address && ReadProcessMemory(child,(void *)probe_markers[i].address,&value,4,&got) && got==4)
            fprintf(events,"probe-marker kind=%s name=%s address=%08lx value=%08lx\n",kind,probe_markers[i].name,probe_markers[i].address,value);
        else fprintf(events,"probe-marker kind=%s name=%s UNAVAILABLE\n",kind,probe_markers[i].name);
    }
}
static int patch_word(DWORD address, DWORD value)
{
    DWORD old, ignored, wrote;
    int result;
    if(!VirtualProtectEx(child,(void *)address,4,PAGE_EXECUTE_READWRITE,&old)) return 0;
    result=WriteProcessMemory(child,(void *)address,&value,4,&wrote) && wrote==4;
    if(!FlushInstructionCache(child,(void *)address,4)) result=0;
    if(!VirtualProtectEx(child,(void *)address,4,old,&ignored)) result=0;
    return result;
}
#endif

static int memory(DWORD address, void *out, DWORD size)
{
    DWORD got;
    if (!size) return 1;
    if (address > 0xffffffffUL-size) return 0;
    return ReadProcessMemory(child,(void *)address,out,size,&got) && got==size;
}

static int patch(DWORD address, BYTE byte)
{
    DWORD old, ignored, wrote;
    int result;
    if (!VirtualProtectEx(child,(void *)address,1,PAGE_EXECUTE_READWRITE,&old)) return 0;
    result=WriteProcessMemory(child,(void *)address,&byte,1,&wrote) && wrote==1;
    if (!FlushInstructionCache(child,(void *)address,1)) result=0;
    if (!VirtualProtectEx(child,(void *)address,1,old,&ignored)) result=0;
    return result;
}

static int blob(const char *why, DWORD address, DWORD count)
{
    BYTE buffer[256];
    DWORD done=0, amount, i;
    if (count>MAX_OBJECT || total_bytes>MAX_BYTES-count) return 0;
    total_bytes+=count;
    fprintf(events,"bytes kind=%s address=%08lx count=%lu hex=",why,address,count);
    while (done<count) {
        amount=count-done; if(amount>sizeof(buffer)) amount=sizeof(buffer);
        if (!memory(address+done,buffer,amount)) return 0;
        for(i=0;i<amount;++i) fprintf(events,"%02x",buffer[i]);
        done+=amount;
    }
    fprintf(events,"\n"); return 1;
}

static int string(const char *why, DWORD address, int environment)
{
    DWORD length;
    BYTE value, last=1;
    if (!address) return 0;
    for(length=0;length<MAX_OBJECT;++length) {
        if (address>0xffffffffUL-length) return 0;
        if (!memory(address+length,&value,1)) return 0;
        if (!value && (!environment || !last)) return blob(why,address,length+1);
        last=value;
    }
    return 0;
}

static void regs(ObRegs *out, const CONTEXT *in)
{
    out->eip=in->Eip; out->esp=in->Esp; out->eax=in->Eax; out->ebx=in->Ebx;
    out->ecx=in->Ecx; out->edx=in->Edx; out->esi=in->Esi; out->edi=in->Edi;
    out->ebp=in->Ebp; out->flags=in->EFlags; out->cs=in->SegCs; out->ss=in->SegSs;
    out->ds=in->SegDs; out->es=in->SegEs; out->fs=in->SegFs; out->gs=in->SegGs;
}

static int context(CONTEXT *out)
{
    memset(out,0,sizeof(*out)); out->ContextFlags=CONTEXT_FULL;
    return GetThreadContext(primary,out);
}

static void log_context(const char *kind, const CONTEXT *c)
{
    fprintf(events,"context kind=%s thread=%lu eip=%08lx esp=%08lx eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx flags=%08lx cs=%04lx ss=%04lx ds=%04lx es=%04lx fs=%04lx gs=%04lx\n",
        kind,thread_id,c->Eip,c->Esp,c->Eax,c->Ebx,c->Ecx,c->Edx,c->Esi,c->Edi,c->Ebp,c->EFlags,c->SegCs,c->SegSs,c->SegDs,c->SegEs,c->SegFs,c->SegGs);
}

static int at_break(DWORD address, const CONTEXT *c)
{
    BYTE byte;
    return c->Eip==address+1 && memory(address,&byte,1) && byte==0xcc;
}

static int in_image(DWORD address)
{
    return address>=image_base && address-image_base<image_size;
}

static int pending_return(DWORD address)
{
    unsigned i;
    for(i=0;i<depth;++i) if(frames[i].address==address) return 1;
    return 0;
}

static int arm_thunk(Thunk *t)
{
    BYTE bytes[6];
    DWORD operand;
    if (t->armed || !memory(t->address,bytes,6) || bytes[0]!=0xff || bytes[1]!=0x25) return 0;
    memcpy(&operand,bytes+2,4);
    if (operand!=image_base+t->iat || !patch(t->address,0xcc)) return 0;
    t->armed=1; return 1;
}

static int log_inputs(const Thunk *t, const DWORD *a)
{
    if (!strcmp(t->name,"CreateFileA") || !strcmp(t->name,"DeleteFileA") ||
        !strcmp(t->name,"LoadLibraryA") || !strcmp(t->name,"FindFirstFileA")) {
        if (!string("actual-path",a[0],0)) return 0;
    } else if (!strcmp(t->name,"GetProcAddress")) {
        if ((a[1]&0xffff0000UL) && !string("actual-procedure-name",a[1],0)) return 0;
    } else if (!strcmp(t->name,"WriteFile")) {
        if (!blob("actual-WriteFile-input",a[1],a[2])) return 0;
    }
    if ((!strcmp(t->name,"ReadFile") || !strcmp(t->name,"WriteFile")) && a[3] &&
        !blob("raw-count-before-uncredited",a[3],4)) return 0;
    if (!strcmp(t->name,"GetConsoleMode") && !blob("raw-mode-before-uncredited",a[1],4)) return 0;
    if (!strcmp(t->name,"SetFilePointer") && a[2] && !blob("raw-high-before",a[2],4)) return 0;
    return 1;
}

static int log_outputs(const Thunk *t, const DWORD *a, DWORD result)
{
    DWORD count;
    if (!strcmp(t->name,"GetCommandLineA")) {
        if (!string("actual-command-line",result,0)) return 0;
    } else if (!strcmp(t->name,"GetEnvironmentStrings")) {
        if (!string("actual-Windows-environment-private",result,1)) return 0;
    } else if (!strcmp(t->name,"GetConsoleMode")) {
        if (!blob(result ? "defined-mode-after" : "raw-mode-after-uncredited",a[1],4)) return 0;
    } else if (!strcmp(t->name,"ReadFile") || !strcmp(t->name,"WriteFile")) {
        if (a[3]) {
            if (!memory(a[3],&count,4) || !blob(result ? "defined-count-after" : "raw-count-after-uncredited",a[3],4)) return 0;
            if (result && count>a[2]) return 0;
            if (result && !strcmp(t->name,"ReadFile") && !blob("defined-ReadFile-output",a[1],count)) return 0;
        } else if (result && !a[4]) return 0;
    } else if (!strcmp(t->name,"GetModuleFileNameA")) {
        if (result && result<a[2] && !blob("actual-module-name-result",a[1],result+1)) return 0;
    } else if (!strcmp(t->name,"GetLocalTime")) {
        if (!blob("defined-local-time",a[0],16)) return 0;
    } else if (!strcmp(t->name,"GetTimeZoneInformation")) {
        if (result!=0xffffffffUL && !blob("defined-timezone",a[0],172)) return 0;
    } else if (!strcmp(t->name,"SetFilePointer")) {
        if (a[2] && !blob("raw-high-after",a[2],4)) return 0;
    } else if (!strcmp(t->name,"GetProcAddress") && result) {
        fprintf(events,"HOLD successful-dynamic-resolution-needs-execution-coverage\n"); return 0;
    }
    return 1;
}

static int begin_api(unsigned index, CONTEXT *c)
{
    Thunk *t=&thunks[index];
    DWORD stack[8], target;
    unsigned i;
    Frame *f;
    ObRegs before, armed;
    CONTEXT readback;
    if (step.active || !t->armed || !at_break(t->address,c)) return 0;
    fprintf(events,"api-entry-candidate name=%s address=%08lx depth=%u thread=%lu esp=%08lx\n",t->name,t->address,depth,thread_id,c->Esp);
    if (depth ||
        !memory(c->Esp,stack,(t->argc+1)*4) || !memory(image_base+t->iat,&target,4) || !target ||
        api_events>=4096) return 0;
    ++api_events; ++t->calls;
    fprintf(events,"api-enter number=%lu name=%s thread=%lu thunk=%08lx iat=%08lx target=%08lx return=%08lx esp=%08lx argc=%lu args=",
        api_events,t->name,thread_id,t->address,image_base+t->iat,target,stack[0],c->Esp,t->argc);
    for(i=0;i<t->argc;++i) fprintf(events,"%08lx,",stack[i+1]);
    fprintf(events,"\n"); log_context("api-entry",c);
    if (!log_inputs(t,stack+1)) return 0;
    if (!strcmp(t->name,"RtlUnwind")) {
        fprintf(events,"HOLD nonlocal-RtlUnwind-boundary-needs-separate-review\n"); return 0;
    }
    if (t->no_return) {
        if (strcmp(t->name,"ExitProcess") || depth || exit_requested) return 0;
        exit_requested=1; exit_status=stack[1];
        fprintf(events,"actual-ExitProcess-request status=%lu\n",exit_status);
#ifdef OBS_AUTHORED_PROBE
        read_probe_markers("before-ExitProcess-JMP");
#endif
    } else {
        if (depth>=MAX_FRAMES || !in_image(stack[0]) || pending_return(stack[0]) ||
            stack[0]==image_base+entry_rva || stack[0]==image_base+main_rva) return 0;
        for(i=0;i<thunk_count;++i)
            if(stack[0]>=thunks[i].address && stack[0]-thunks[i].address<6) return 0;
        f=&frames[depth];
        f->thunk=index; f->thread=thread_id; f->esp=c->Esp; f->address=stack[0];
        memcpy(f->args,stack+1,t->argc*4);
        if (!memory(f->address,&f->saved,1) || f->saved==0xcc || !patch(f->address,0xcc)) return 0;
        ++depth;
    }
    regs(&before,c);
    if (ob_prepare(&step,thread_id,t->address,target,&before)!=OB_OK || !patch(t->address,0xff)) return 0;
    t->armed=0; t->target=target; step_thunk=index;
    c->Eip=t->address; c->EFlags|=0x100;
    if (!SetThreadContext(primary,c) || !context(&readback)) return 0;
    regs(&armed,&readback);
    log_context("JMP-TF-arm-readback",&readback);
    if (ob_armed(&step,thread_id,&armed)!=OB_OK) return 0;
#ifdef OBS_AUTHORED_PROBE
    if(probe_fault && !probe_fault_used && !strcmp(t->name,probe_trigger)) {
        DWORD replacement;
        if(!memory(image_base+probe_alternate_iat,&replacement,4) || !replacement || replacement==target) return 0;
        fprintf(events,"AUTHORED-PROBE-ONLY fault=%u trigger=%s alternate-iat=%08lx target=%08lx\n",probe_fault,t->name,image_base+probe_alternate_iat,replacement);
        if(probe_fault==1) {
            if(!patch_word(image_base+t->iat,replacement)) return 0;
        } else if(probe_fault==2) {
            if(!patch_word(t->address+2,image_base+probe_alternate_iat)) return 0;
        } else return 0;
        probe_fault_used=1;
    }
#endif
    return 1;
}

static int finish_step(CONTEXT *c)
{
    ObRegs actual, cleared;
    CONTEXT readback;
    DWORD target;
    int result;
    Thunk *t=&thunks[step_thunk];
    log_context("JMP-step-delivered",c);
    if (!memory(image_base+t->iat,&target,4)) {
        fprintf(events,"HOLD IAT-unreadable-during-step\n"); return 0;
    }
    if (target!=step.target) {
        fprintf(events,"HOLD IAT-changed-during-step iat=%08lx captured-target=%08lx actual-slot=%08lx actual-pc=%08lx\n",image_base+t->iat,step.target,target,c->Eip); return 0;
    }
    regs(&actual,c); result=ob_finish(&step,thread_id,&actual);
    fprintf(events,"JMP-step-result=%d site=%08lx expected-target=%08lx actual-target=%08lx\n",
        result,t->address,step.target,c->Eip);
    if (result!=OB_OK || !arm_thunk(t)) return 0;
    c->EFlags&=~0x100UL;
    if (!SetThreadContext(primary,c) || !context(&readback)) return 0;
    regs(&cleared,&readback);
    if (ob_clear(&step,thread_id,&cleared)!=OB_OK) return 0;
    fprintf(events,"JMP-rearmed name=%s address=%08lx\n",t->name,t->address);
    return 1;
}

static int finish_api(CONTEXT *c, DWORD address)
{
    Frame *f;
    Thunk *t;
    if (!depth || step.active) return 0;
    f=&frames[depth-1]; t=&thunks[f->thunk];
    fprintf(events,"api-return-candidate name=%s thread=%lu address=%08lx expected-address=%08lx entry-esp=%08lx actual-esp=%08lx argc=%lu stack-result=%d\n",t->name,thread_id,address,f->address,f->esp,c->Esp,t->argc,ob_return_stack(f->esp,t->argc,c->Esp));
    log_context("api-return-candidate",c);
    if (f->address!=address || f->thread!=thread_id || !at_break(address,c) ||
        ob_return_stack(f->esp,t->argc,c->Esp)!=OB_OK) return 0;
    fprintf(events,"api-return name=%s thread=%lu address=%08lx esp=%08lx eax=%08lx\n",
        t->name,thread_id,address,c->Esp,c->Eax);
    log_context("api-return",c);
    if (!log_outputs(t,f->args,c->Eax) || !patch(address,f->saved)) return 0;
    --depth; c->Eip=address;
    return SetThreadContext(primary,c);
}

static int observe_main(const CONTEXT *c)
{
    DWORD argc=c->Eax, argv=c->Edx, values[3], i, pointer;
    if (main_kind==2) {
        if (!memory(c->Esp,values,sizeof(values))) return 0;
        argc=values[1]; argv=values[2];
    }
    if (argc>4096 || argv>0xffffffffUL-argc*4) return 0;
    fprintf(events,"actual-main argc=%lu argv=%08lx arg-style=%lu\n",argc,argv,main_kind);
    for(i=0;i<argc;++i) {
        if (!memory(argv+i*4,&pointer,4) || !string("actual-main-argument",pointer,0)) return 0;
    }
    if (!memory(argv+argc*4,&pointer,4) || pointer) return 0;
    return blob("main-raw-stack",c->Esp,32);
}

static void module(DWORD base, HANDLE file)
{
    DWORD pe, directory, name, size;
    BY_HANDLE_FILE_INFORMATION info;
    BYTE byte;
    char text[256];
    unsigned i;
    fprintf(events,"module base=%08lx file=%08lx\n",base,(DWORD)file);
    if (file && GetFileInformationByHandle(file,&info))
        fprintf(events,"module-file-id volume=%08lx high=%08lx low=%08lx size=%08lx:%08lx\n",info.dwVolumeSerialNumber,info.nFileIndexHigh,info.nFileIndexLow,info.nFileSizeHigh,info.nFileSizeLow);
    if (!memory(base+0x3c,&pe,4) || pe>4096 ||
        !memory(base+pe+24+56,&size,4) || size<4096 || !memory(base+pe+24+96,&directory,4) || !directory ||
        !memory(base+directory+12,&name,4)) return;
#ifdef OBS_AUTHORED_PROBE
    if(base==image_base) for(i=0;i<probe_marker_count;++i)
        if(!probe_markers[i].dll && probe_markers[i].rva<=size-4) probe_markers[i].address=base+probe_markers[i].rva;
#endif
    for(i=0;i<sizeof(text);++i) {
        if (!memory(base+name+i,&byte,1)) return;
        text[i]=(char)byte;
        if (!byte) {
            fprintf(events,"module-export-name base=%08lx name=%s size=%08lx\n",base,text,size);
            if (!_stricmp(text,"ntdll.dll")) { ntdll_base=base; ntdll_size=size; }
#ifdef OBS_AUTHORED_PROBE
            if(!_stricmp(text,"observer-probe.dll")) {
                unsigned j;
                for(j=0;j<probe_marker_count;++j)
                    if(probe_markers[j].dll && probe_markers[j].rva<=size-4) probe_markers[j].address=base+probe_markers[j].rva;
            }
#endif
            return;
        }
    }
}

static int fail_owned(DWORD code, const char *why)
{
    DEBUG_EVENT e;
    DWORD began, result=STILL_ACTIVE;
    int seen=real_exit_seen;
    if(seen) result=real_exit_status;
    fprintf(events,"HOLD code=%lu reason=%s depth=%u step-active=%d\n",code,why,depth,step.active);
    if (child) {
#ifdef OBS_AUTHORED_PROBE
        {
            int stopped=event_pending && !seen;
            if(!event_pending && !seen) {
                DWORD previous=SuspendThread(primary);
                stopped=previous!=0xffffffffUL;
                fprintf(events,"probe-marker-timeout-suspend previous=%08lx succeeded=%d\n",previous,stopped);
            }
            fprintf(events,"probe-marker-snapshot stopped=%d pending-event=%d exit-seen=%d\n",stopped,event_pending,seen);
            if(stopped) read_probe_markers("before-owned-termination");
            else fprintf(events,"probe-markers-UNAVAILABLE-not-stopped\n");
        }
#endif
        if(event_pending && pending.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            seen=1; result=pending.u.ExitProcess.dwExitCode;
        }
        fprintf(events,"terminate-owned pid=%lu result=%d\n",process_id,TerminateProcess(child,code));
        if(event_pending) {
            ContinueDebugEvent(pending.dwProcessId,pending.dwThreadId,DBG_CONTINUE);
            event_pending=0;
        }
        began=GetTickCount();
        while(!seen && GetTickCount()-began<5000) {
            if(!WaitForDebugEvent(&e,100)) continue;
            if(e.dwProcessId!=process_id) break;
            if(e.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) { seen=1; result=e.u.ExitProcess.dwExitCode; }
            if(e.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT && e.u.LoadDll.hFile) CloseHandle(e.u.LoadDll.hFile);
            ContinueDebugEvent(e.dwProcessId,e.dwThreadId,DBG_CONTINUE);
            if(seen) break;
        }
        fprintf(events,"owned-cleanup exit-event=%d signaled=%d status=%lu\n",seen,WaitForSingleObject(child,0)==WAIT_OBJECT_0,result);
    }
    fflush(events); return (int)code;
}

static int read_config(const char *path)
{
    FILE *input=fopen(path,"rt");
    char line[256], extra;
    Thunk *t;
    if(!input) return 0;
    if(!fgets(line,sizeof(line),input) || sscanf(line,"image %lx %lx %lx %lx %lx %lx %lx %c",&configured_base,&image_size,&entry_rva,&main_rva,&entry_byte,&main_byte,&main_kind,&extra)!=7 ||
        image_size<4096 || configured_base>0xffffffffUL-image_size || entry_rva>=image_size || main_rva>=image_size || entry_rva==main_rva || entry_byte>255 || main_byte>255 || (main_kind!=1 && main_kind!=2)) { fclose(input); return 0; }
    while(fgets(line,sizeof(line),input)) {
#ifdef OBS_AUTHORED_PROBE
        if(!strncmp(line,"marker ",7)) {
            char scope[16];
            ProbeMarker *m;
            if(probe_marker_count==8) { fclose(input); return 0; }
            m=&probe_markers[probe_marker_count];
            if(sscanf(line,"marker %15s %lx %63s %c",scope,&m->rva,m->name,&extra)!=3 || (strcmp(scope,"image") && strcmp(scope,"dll"))) { fclose(input); return 0; }
            m->dll=!strcmp(scope,"dll"); ++probe_marker_count; continue;
        }
        if(!strncmp(line,"fault ",6)) {
            if(probe_fault || sscanf(line,"fault %u %lx %63s %c",&probe_fault,&probe_alternate_iat,probe_trigger,&extra)!=3 || probe_fault>2 || !probe_fault || probe_alternate_iat>image_size-4) { fclose(input); return 0; }
            continue;
        }
#endif
        if(thunk_count==MAX_THUNKS) { fclose(input); return 0; }
        t=&thunks[thunk_count];
        if(sscanf(line,"thunk %lx %lx %lu %lu %63s %c",&t->rva,&t->iat,&t->argc,&t->no_return,t->name,&extra)!=5 ||
            t->argc>7 || t->no_return>1 || t->rva>image_size-6 || t->iat>image_size-4) { fclose(input); return 0; }
        ++thunk_count;
    }
    fclose(input); return thunk_count!=0;
}

int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT e;
    CONTEXT c;
    DWORD began, deadline, status, code, address, i, count=0;
    BYTE byte;
    HANDLE input, output, error;
    char command[4096];
    if(argc!=10 || !read_config(argv[7])) return 120;
    deadline=strtoul(argv[8],NULL,10); if(!deadline || deadline>30000) return 120;
    events=fopen(argv[6],"wb"); if(!events) return 121;
    fprintf(events,"purpose=unmasked-target-diagnostic-only target=%s args=%s cwd=%s\n",argv[1],argv[2],argv[3]);
    memset(&security,0,sizeof(security)); security.nLength=sizeof(security); security.bInheritHandle=TRUE;
    input=CreateFileA(argv[9],GENERIC_READ,FILE_SHARE_READ,&security,OPEN_EXISTING,0,NULL);
    output=CreateFileA(argv[4],GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_NEW,0,NULL);
    error=CreateFileA(argv[5],GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_NEW,0,NULL);
    if(input==INVALID_HANDLE_VALUE || output==INVALID_HANDLE_VALUE || error==INVALID_HANDLE_VALUE) return 122;
    fprintf(events,"actual-inherited-stdio-types stdin=%lu stdout=%lu stderr=%lu\n",GetFileType(input),GetFileType(output),GetFileType(error));
    memset(&startup,0,sizeof(startup)); startup.cb=sizeof(startup); startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdInput=input; startup.hStdOutput=output; startup.hStdError=error;
    if(strlen(argv[1])+strlen(argv[2])+5>=sizeof(command)) return 120;
    sprintf(command,"\"%s\" %s",argv[1],argv[2]);
    if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[3],&startup,&process)) return 123;
    child=process.hProcess; primary=process.hThread; process_id=process.dwProcessId; thread_id=process.dwThreadId;
    CloseHandle(input); CloseHandle(output); CloseHandle(error);
    fprintf(events,"owned-child pid=%lu thread=%lu command=%s\n",process_id,thread_id,command);
    began=GetTickCount();
    for(;;) {
        if(GetTickCount()-began>=deadline || ++count>20000) {
            fprintf(events,"deadline-check elapsed=%lu budget=%lu polls=%lu\n",GetTickCount()-began,deadline,count);
            return fail_owned(124,"deadline-or-event-bound");
        }
        if(!WaitForDebugEvent(&e,100)) continue;
        pending=e; event_pending=1; status=DBG_CONTINUE;
        fprintf(events,"event kind=%lu pid=%lu tid=%lu elapsed=%lu\n",e.dwDebugEventCode,e.dwProcessId,e.dwThreadId,GetTickCount()-began);
        if(e.dwProcessId!=process_id) return fail_owned(125,"foreign-process-event");
        if(step.active && e.dwDebugEventCode!=EXCEPTION_DEBUG_EVENT) return fail_owned(125,"nonexception-event-during-owned-JMP");
        if(e.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) return fail_owned(126,"unexpected-target-thread");
        if(e.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            if(e.dwThreadId!=thread_id) return fail_owned(127,"wrong-primary-thread");
            image_base=(DWORD)e.u.CreateProcessInfo.lpBaseOfImage;
            if(image_base!=configured_base) return fail_owned(128,"unreviewed-image-relocation");
            module(image_base,e.u.CreateProcessInfo.hFile);
            if(e.u.CreateProcessInfo.hFile) CloseHandle(e.u.CreateProcessInfo.hFile);
            for(i=0;i<thunk_count;++i) { thunks[i].address=image_base+thunks[i].rva; if(!arm_thunk(&thunks[i])) return fail_owned(129,"thunk-preimage"); }
            if(!memory(image_base+entry_rva,&byte,1) || byte!=(BYTE)entry_byte || !patch(image_base+entry_rva,0xcc) ||
                !memory(image_base+main_rva,&byte,1) || byte!=(BYTE)main_byte || !patch(image_base+main_rva,0xcc)) return fail_owned(130,"entry-main-preimage");
        } else if(e.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            module((DWORD)e.u.LoadDll.lpBaseOfDll,e.u.LoadDll.hFile);
            if(e.u.LoadDll.hFile) CloseHandle(e.u.LoadDll.hFile);
        } else if(e.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            code=e.u.Exception.ExceptionRecord.ExceptionCode;
            address=(DWORD)e.u.Exception.ExceptionRecord.ExceptionAddress;
            if(e.dwThreadId!=thread_id || !context(&c) || !e.u.Exception.dwFirstChance) return fail_owned(131,"exception-owner-or-chance");
            if(step.active) {
                if(code!=EXCEPTION_SINGLE_STEP || address!=c.Eip || !finish_step(&c)) return fail_owned(132,"owned-JMP-step");
            } else if(code==EXCEPTION_BREAKPOINT && address==image_base+entry_rva && !entry_seen) {
                if(!at_break(address,&c) || !patch(address,(BYTE)entry_byte)) return fail_owned(133,"entry-break");
                log_context("PE-entry",&c); entry_seen=1; c.Eip=address;
                if(!SetThreadContext(primary,&c)) return fail_owned(133,"entry-context");
            } else if(code==EXCEPTION_BREAKPOINT && address==image_base+main_rva && !main_seen) {
                if(!entry_seen || !at_break(address,&c) || !observe_main(&c) || !patch(address,(BYTE)main_byte)) return fail_owned(134,"main-break");
                log_context("main",&c); main_seen=1; c.Eip=address;
                if(!SetThreadContext(primary,&c)) return fail_owned(134,"main-context");
            } else if(code==EXCEPTION_BREAKPOINT && pending_return(address)) {
                if(!finish_api(&c,address)) return fail_owned(135,"return-owner-stack-or-output");
            } else if(code==EXCEPTION_BREAKPOINT) {
                for(i=0;i<thunk_count;++i) if(address==thunks[i].address) break;
                if(i<thunk_count) { if(!begin_api(i,&c)) return fail_owned(136,"API-entry-frame-or-arm"); }
                else if(!loader_seen && !entry_seen && ntdll_base && address>=ntdll_base && address-ntdll_base<ntdll_size && memory(address,&byte,1) && byte==0xcc) {
                    loader_seen=1; fprintf(events,"initial-loader-INT3 ntdll-rva=%08lx\n",address-ntdll_base);
                } else return fail_owned(137,"unclassified-breakpoint");
            } else return fail_owned(138,"unclassified-exception");
        } else if(e.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            code=e.u.ExitProcess.dwExitCode;
            real_exit_seen=1; real_exit_status=code;
            fprintf(events,"owned-exit status=%lu requested=%lu pending-depth=%u step-active=%d\n",code,exit_status,depth,step.active);
            if(!entry_seen || !main_seen || !exit_requested || code!=exit_status || depth || step.active) return fail_owned(139,"exit-without-owned-noreturn-request");
            if(!ContinueDebugEvent(e.dwProcessId,e.dwThreadId,DBG_CONTINUE)) return fail_owned(142,"exit-continue");
            event_pending=0;
            if(WaitForSingleObject(child,5000)!=WAIT_OBJECT_0 || !GetExitCodeProcess(child,&address) || address!=code)
                return fail_owned(143,"exit-signaled-or-status");
            fprintf(events,"owned-success-confirmed exit-event=1 signaled=1 event-status=%lu actual-status=%lu\n",code,address);
            fprintf(events,"observer-complete API-events=%lu raw-bytes=%lu elapsed=%lu\n",api_events,total_bytes,GetTickCount()-began);
            fclose(events); CloseHandle(primary); CloseHandle(child); return 0;
        } else if(e.dwDebugEventCode==UNLOAD_DLL_DEBUG_EVENT) {
            fprintf(events,"module-unload base=%08lx\n",(DWORD)e.u.UnloadDll.lpBaseOfDll);
        } else if(e.dwDebugEventCode==OUTPUT_DEBUG_STRING_EVENT) {
            DWORD bytes=e.u.DebugString.nDebugStringLength;
            if(e.u.DebugString.fUnicode) bytes*=2;
            if(!blob("actual-debug-string",(DWORD)e.u.DebugString.lpDebugStringData,bytes)) return fail_owned(140,"debug-string-span");
        } else return fail_owned(140,"unsupported-debug-event");
        fflush(events);
        if(!ContinueDebugEvent(e.dwProcessId,e.dwThreadId,status)) return fail_owned(141,"continue-event");
        event_pending=0;
    }
}
