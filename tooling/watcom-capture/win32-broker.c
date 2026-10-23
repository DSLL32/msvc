/* Public-API capture transport, not reconstructed application code.
   Authored basic transport probes passed; application admission is separate.
   Runtime policy/decoding remains in gate.py; this helper never free-runs a
   stopped application instruction. Initial OS-loader continuation requires a
   separately checked PE-entry trap. WCC's separately reviewed fixed loader
   prefix may stop at its launch CALL; that operation cannot enter CF code. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

enum { MAP_BYTES=16384, PAYLOAD=256, MAX_PAYLOAD=4096, MAX_SEQUENCE=1000000,
       MAX_TRAPS=64, MAX_EVENTS=10000, MAX_STEPS=10000, MAX_READ=512 };
enum { OP_EVENT=10, OP_CONTEXT=11, OP_READ=12, OP_TRAP=13, OP_CONSUME=14,
       OP_ARM=15, OP_STEP=16, OP_LOADER=17, OP_STOP=18, OP_SELECTORS=19, OP_REGION=20,
       OP_WCC_PREFIX=21, OP_WCC_ENTRY=22, OP_WCC_ENTRY_TRAP=23 };
typedef struct Trap { DWORD address; BYTE saved; int active; } Trap;
static BYTE *area;
static HANDLE map_file=INVALID_HANDLE_VALUE, map_handle, child, thread;
static HANDLE input=INVALID_HANDLE_VALUE, output=INVALID_HANDLE_VALUE, error_output=INVALID_HANDLE_VALUE;
static FILE *log_file;
static DEBUG_EVENT event;
static int pending, created, entry_reached, armed, exiting, child_exit_seen;
static int loader_break_seen;
static int wcc_prefix, wcc_stopped;
static int wcc_handoff, wcc_at_entry, wcc_trap_running, wcc_entry_stopped;
static unsigned wcc_instruction;
static DWORD wcc_cf_base, wcc_cf_entry;
static CONTEXT wcc_entry_context;
static DWORD child_pid, child_tid, image_base, image_size, entry_rva, entry_byte;
static DWORD teb, entry_address, ntdll_base, ntdll_size;
static DWORD events, steps, started, lease_tick, lease_value, seen;
static DWORD global_ms, lease_ms, expected_pc, expected_sp, expected_cs;
static CONTEXT armed_context;
static Trap traps[MAX_TRAPS];
static unsigned trap_count;
static DWORD startup_modules[64];
static int startup_module_active[64];
static unsigned startup_module_count;
typedef struct ExtraProcess { DWORD pid; HANDLE handle; int exited; } ExtraProcess;
static ExtraProcess extra[16];
static unsigned extra_count;
static int ownership_incomplete, teardown_ok;

static DWORD load(unsigned offset) { return *(volatile DWORD *)(area+offset); }
static void publish(unsigned offset,DWORD value)
{ InterlockedExchange((LONG *)(area+offset),(LONG)value); }
static int bounds(DWORD address,DWORD count)
{ return count && address<=0xffffffffUL-(count-1); }
static int memory(DWORD address,void *bytes,DWORD count)
{
    DWORD got=0;
    return child && bounds(address,count) && count<=MAX_READ &&
           ReadProcessMemory(child,(void *)address,bytes,count,&got) && got==count;
}
static int context(CONTEXT *value)
{
    if(!pending || !thread || event.dwThreadId!=child_tid) return 0;
    memset(value,0,sizeof(*value));value->ContextFlags=CONTEXT_FULL;
    return GetThreadContext(thread,value);
}
static void pack_context(DWORD *packet,const CONTEXT *value)
{
    packet[0]=value->Eip;packet[1]=value->Eax;packet[2]=value->Ecx;
    packet[3]=value->Edx;packet[4]=value->Ebx;packet[5]=value->Esp;
    packet[6]=value->Ebp;packet[7]=value->Esi;packet[8]=value->Edi;
    packet[9]=value->EFlags;packet[10]=value->SegCs;packet[11]=value->SegSs;
    packet[12]=value->SegDs;packet[13]=value->SegEs;packet[14]=value->SegFs;
    packet[15]=value->SegGs;
}
static int same_context(const CONTEXT *left,const CONTEXT *right)
{
    DWORD a[16],b[16];pack_context(a,left);pack_context(b,right);
    return !memcmp(a,b,sizeof(a));
}
static int patch(DWORD address,BYTE before,BYTE after)
{
    DWORD old,restored,wrote=0;BYTE observed;int ok;
    if(!memory(address,&observed,1) || observed!=before ||
       !VirtualProtectEx(child,(void *)address,1,PAGE_EXECUTE_READWRITE,&old)) return 0;
    ok=WriteProcessMemory(child,(void *)address,&after,1,&wrote) && wrote==1;
    if(!FlushInstructionCache(child,(void *)address,1)) ok=0;
    if(!VirtualProtectEx(child,(void *)address,1,old,&restored)) ok=0;
    if(!memory(address,&observed,1) || observed!=after) ok=0;
    fprintf(log_file,"patch address=%08lx before=%02x after=%02x wrote=%lu ok=%d\n",
            address,before,after,wrote,ok);fflush(log_file);
    return ok;
}
static int budget(void)
{
    DWORD now=GetTickCount(),lease=load(20);
    if(lease!=lease_value) {
        if(lease<=lease_value || lease>MAX_SEQUENCE) return 0;
        lease_value=lease;lease_tick=now;
    }
    return now-started<global_ms && now-lease_tick<lease_ms &&
           events<MAX_EVENTS && steps<MAX_STEPS;
}
static int in_image(DWORD address)
{ return address>=image_base && address-image_base<image_size; }
static int add_trap(DWORD address,DWORD expected,DWORD *index)
{
    unsigned i;
    if(!pending || armed || expected>255 || !in_image(address) || trap_count==MAX_TRAPS ||
       (!entry_reached && (address!=entry_address || expected!=entry_byte))) return 0;
    for(i=0;i<trap_count;i++) if(traps[i].address==address) return 0;
    if(!patch(address,(BYTE)expected,0xcc)) return 0;
    traps[trap_count].address=address;traps[trap_count].saved=(BYTE)expected;
    traps[trap_count].active=1;*index=trap_count++;
    return 1;
}
static int trap_ready(DWORD address)
{
    unsigned i;BYTE value;
    for(i=0;i<trap_count;i++) if(traps[i].active && traps[i].address==address)
        return memory(address,&value,1) && value==0xcc;
    return 0;
}
static void module_name(DWORD base)
{
    DWORD pe,signature,directory,name,size;WORD magic;char text[64];unsigned i;
    if(!bounds(base,0x40) || !memory(base,&magic,2) || magic!=0x5a4d || !memory(base+0x3c,&pe,4) ||
       pe>0x10000 || !bounds(base,pe+0x7c) || !memory(base+pe,&signature,4) ||
       signature!=0x4550 || !memory(base+pe+24+56,&size,4) || size<0x100 || pe+124>size ||
       !bounds(base,size) || !memory(base+pe+24+96,&directory,4) ||
       !directory || directory>size-40 || !memory(base+directory+12,&name,4) || name>=size) return;
    for(i=0;i<sizeof(text);i++) {
        if(name+i>=size || !memory(base+name+i,text+i,1)) return;
        if(!text[i]) {
            if(!_stricmp(text,"ntdll.dll")) {ntdll_base=base;ntdll_size=size;}
            fprintf(log_file,"module base=%08lx size=%08lx export-name=%s\n",base,size,text);
            return;
        }
    }
}
static int next_event(void)
{
    CONTEXT value;DWORD signature,pe,actual_entry,tls_rva,tls_size,directories,module;WORD magic;unsigned i;
    if(pending) return 0;
    while(budget()) {
        if(!WaitForDebugEvent(&event,10)) continue;
        pending=1;++events;
        fprintf(log_file,"event code=%lu pid=%lu tid=%lu tick=%lu\n",
                event.dwDebugEventCode,event.dwProcessId,event.dwThreadId,GetTickCount());fflush(log_file);
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT && event.dwProcessId!=child_pid) {
            if(extra_count<16) {
                extra[extra_count].pid=event.dwProcessId;
                extra[extra_count++].handle=event.u.CreateProcessInfo.hProcess;
            } else ownership_incomplete=1;
            if(event.u.CreateProcessInfo.hThread) CloseHandle(event.u.CreateProcessInfo.hThread);
            event.u.CreateProcessInfo.hThread=NULL;
            if(event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
            event.u.CreateProcessInfo.hFile=NULL;
            return 0; /* Remains stopped; teardown terminates it BEFORE continuing this event. */
        }
        if(event.dwProcessId!=child_pid || event.dwThreadId!=child_tid) return 0;
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            if(created || armed) return 0;
            created=1;image_base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            teb=(DWORD)event.u.CreateProcessInfo.lpThreadLocalBase;
            entry_address=(DWORD)event.u.CreateProcessInfo.lpStartAddress;
            if(event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
            event.u.CreateProcessInfo.hFile=NULL;
            /* PROCESS_INFORMATION owns the independent retained process/thread handles. */
            if(event.u.CreateProcessInfo.hProcess && event.u.CreateProcessInfo.hProcess!=child)
                CloseHandle(event.u.CreateProcessInfo.hProcess);
            if(event.u.CreateProcessInfo.hThread && event.u.CreateProcessInfo.hThread!=thread)
                CloseHandle(event.u.CreateProcessInfo.hThread);
            if(!bounds(image_base,0x40) || !memory(image_base,&magic,2) || magic!=0x5a4d ||
               !memory(image_base+0x3c,&pe,4) || pe>0x10000 || !bounds(image_base,pe+224) ||
               !memory(image_base+pe,&signature,4) || signature!=0x4550 ||
               !memory(image_base+pe+24,&magic,2) || magic!=0x10b ||
               !memory(image_base+pe+24+16,&actual_entry,4) || actual_entry!=entry_rva ||
               !memory(image_base+pe+24+92,&directories,4) || directories<10 ||
               !memory(image_base+pe+24+96+9*8,&tls_rva,4) ||
               !memory(image_base+pe+24+100+9*8,&tls_size,4) || tls_rva || tls_size ||
               !memory(image_base+pe+24+56,&image_size,4) || !image_size ||
               !bounds(image_base,image_size) || entry_rva>=image_size ||
               entry_address!=image_base+entry_rva) return 0;
        } else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            module=(DWORD)event.u.LoadDll.lpBaseOfDll;
            fprintf(log_file,"startup-load base=%08lx\n",module);
            module_name(module);
            if(event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
            event.u.LoadDll.hFile=NULL;
            if(armed || entry_reached || !module || startup_module_count==64) return 0;
            for(i=0;i<startup_module_count;i++)
                if(startup_module_active[i] && startup_modules[i]==module) return 0;
            startup_modules[startup_module_count]=module;
            startup_module_active[startup_module_count++]=1;
        } else if(event.dwDebugEventCode==UNLOAD_DLL_DEBUG_EVENT) {
            module=(DWORD)event.u.UnloadDll.lpBaseOfDll;
            for(i=0;i<startup_module_count;i++)
                if(startup_module_active[i] && startup_modules[i]==module) break;
            fprintf(log_file,"startup-unload base=%08lx known-active=%d\n",module,i<startup_module_count);
            if(armed || entry_reached || i==startup_module_count || !trap_ready(entry_address)) return 0;
            startup_module_active[i]=0;
            if(module==ntdll_base) {ntdll_base=0;ntdll_size=0;}
        } else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            if(!context(&value)) return 0;
            fprintf(log_file,"exception code=%08lx first=%lu address=%08lx pc=%08lx sp=%08lx flags=%08lx\n",
                    event.u.Exception.ExceptionRecord.ExceptionCode,event.u.Exception.dwFirstChance,
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress,value.Eip,value.Esp,value.EFlags);
            if(armed) {
                if(event.u.Exception.ExceptionRecord.ExceptionCode!=EXCEPTION_SINGLE_STEP ||
                   !event.u.Exception.dwFirstChance ||
                   (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress!=value.Eip ||
                   value.Eip!=expected_pc || value.Esp!=expected_sp || value.SegCs!=expected_cs) return 0;
                /* Raw full context is returned. Consumed TF is permitted only after
                   the recorded exact ARM readback; no other flags are normalized. */
                armed=0;
                if(wcc_handoff) {
                    if(++wcc_instruction==6) {
                        if(value.Eip!=wcc_cf_entry || !trap_ready(wcc_cf_entry)) return 0;
                        wcc_at_entry=1;wcc_entry_context=value;
                        fprintf(log_file,"wcc-cf-entry-db pc=%08lx cf-instructions=0\n",value.Eip);
                    }
                }
            } else if(wcc_trap_running && !wcc_entry_stopped &&
                      event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT &&
                      event.u.Exception.dwFirstChance &&
                      (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress==wcc_cf_entry &&
                      value.Eip==wcc_cf_entry+1 && trap_ready(wcc_cf_entry)) {
                CONTEXT expected=wcc_entry_context;
                expected.Eip=wcc_cf_entry+1;
                if(!same_context(&expected,&value)) return 0;
                wcc_entry_stopped=1;
                fprintf(log_file,"wcc-cf-entry-int3 pc=%08lx cf-instructions=0\n",value.Eip);
            } else if(wcc_prefix && !wcc_stopped &&
                      event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT &&
                      event.u.Exception.dwFirstChance &&
                      (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress==image_base+0x10763 &&
                      value.Eip==image_base+0x10764 && !(value.EFlags&0x100) &&
                      trap_ready(image_base+0x10763)) {
                wcc_stopped=1;
                fprintf(log_file,"wcc-launch-stopped pc=%08lx cf-instructions=0\n",value.Eip);
            } else if(event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT &&
                      event.u.Exception.dwFirstChance &&
                      (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress==entry_address &&
                      value.Eip==entry_address+1 && trap_ready(entry_address)) entry_reached=1;
            else if(entry_reached) return 0;
        } else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            child_exit_seen=1;return 0; /* Unexpected until explicit teardown. */
        } else return 0; /* Threads, callbacks, debug strings and unknown events HOLD. */
        fflush(log_file);return 1;
    }
    return 0;
}
static int continue_loader(void)
{
    CONTEXT value;DWORD address;BYTE opcode;
    if(!budget() || !pending || !created || armed || entry_reached || !trap_ready(entry_address)) return 0;
    if(event.dwDebugEventCode!=CREATE_PROCESS_DEBUG_EVENT && event.dwDebugEventCode!=LOAD_DLL_DEBUG_EVENT &&
       event.dwDebugEventCode!=UNLOAD_DLL_DEBUG_EVENT) {
        if(event.dwDebugEventCode!=EXCEPTION_DEBUG_EVENT ||
           event.u.Exception.ExceptionRecord.ExceptionCode!=EXCEPTION_BREAKPOINT ||
           !event.u.Exception.dwFirstChance || loader_break_seen || !context(&value)) return 0;
        address=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
        if(!ntdll_size || address<ntdll_base || address-ntdll_base>=ntdll_size ||
           value.Eip!=address+1 || (value.EFlags&0x100) ||
           !memory(address,&opcode,1) || opcode!=0xcc) return 0;
        loader_break_seen=1;
    }
    if(!budget() || !ContinueDebugEvent(child_pid,child_tid,DBG_CONTINUE)) return 0;
    pending=0;return 1;
}
static int consume(DWORD index)
{
    CONTEXT before,after,observed;Trap *point;
    if(!entry_reached || !pending || armed || index>=trap_count ||
       event.dwDebugEventCode!=EXCEPTION_DEBUG_EVENT ||
       event.u.Exception.ExceptionRecord.ExceptionCode!=EXCEPTION_BREAKPOINT ||
       !event.u.Exception.dwFirstChance || !context(&before)) return 0;
    point=traps+index;
    if(!point->active || (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress!=point->address ||
       before.Eip!=point->address+1 || !patch(point->address,0xcc,point->saved)) return 0;
    after=before;after.Eip=point->address;
    if(!SetThreadContext(thread,&after) || !context(&observed) || !same_context(&after,&observed)) return 0;
    point->active=0;return 1;
}
static int arm(const DWORD *packet)
{
    CONTEXT before,requested,observed;DWORD actual[16];BYTE opcode;
    if(!entry_reached || !pending || armed || !context(&before) || before.EFlags&0x100 ||
       !memory(before.Eip,&opcode,1) || opcode==0xcc) return 0;
    pack_context(actual,&before);
    if(memcmp(actual,packet,64)) return 0;
    if(wcc_handoff) {
        static const DWORD offsets[]={0x10763,0x10015,0x10016,0x10019,0x1001b,0x1001d};
        static const BYTE lengths[]={5,1,3,2,2,2};
        static const BYTE instructions[6][5]={{0xe8,0xad,0xf8,0xff,0xff},{0x0e},
            {0x66,0x8c,0xcb},{0xb4,2},{0x2b,0xc9},{0xff,0xd6}};
        BYTE bytes[5];DWORD next,sp;
        if(wcc_entry_stopped || wcc_instruction>=6 ||
           before.Eip!=image_base+offsets[wcc_instruction] ||
           !memory(before.Eip,bytes,lengths[wcc_instruction]) ||
           memcmp(bytes,instructions[wcc_instruction],lengths[wcc_instruction])) return 0;
        next=wcc_instruction==0?image_base+0x10015:
             wcc_instruction==5?wcc_cf_entry:before.Eip+lengths[wcc_instruction];
        sp=before.Esp;
        if(wcc_instruction==0 || wcc_instruction==1 || wcc_instruction==5) {
            if(sp<4) return 0;
            sp-=4;
        }
        if(packet[16]!=next || packet[17]!=sp || packet[18]!=before.SegCs ||
           (wcc_instruction==5 && (before.Esi!=wcc_cf_entry || !trap_ready(wcc_cf_entry)))) return 0;
    }
    expected_pc=packet[16];expected_sp=packet[17];expected_cs=packet[18];
    requested=before;requested.EFlags|=0x100;
    if(!SetThreadContext(thread,&requested) || !context(&observed) ||
       !same_context(&requested,&observed)) return 0;
    armed_context=observed;armed=1;
    fprintf(log_file,"arm pc=%08lx sp=%08lx before-flags=%08lx readback-flags=%08lx next-pc=%08lx next-sp=%08lx next-cs=%08lx\n",
            before.Eip,before.Esp,before.EFlags,observed.EFlags,expected_pc,expected_sp,expected_cs);
    fflush(log_file);return 1;
}
static int continue_wcc_prefix(DWORD *index)
{
    /* Only this immutable loader's initial prefix is admitted. The controller
       binds the file hash and compares its complete relocated code/data first.
       No CF heap byte is patched, and the launch CALL remains unexecuted. */
    static const BYTE entry_bytes[]={0xe9,0x75,0x06,0,0};
    static const BYTE invoke_bytes[]={0x0e,0x66,0x8c,0xcb,0xb4,2,0x2b,0xc9,0xff,0xd6,0xc3};
    static const BYTE launch_bytes[]={0xe8,0xad,0xf8,0xff,0xff};
    BYTE observed[11];CONTEXT value;
    if(!budget() || !pending || !entry_reached || armed || wcc_prefix ||
       entry_rva!=0x10010 || image_size!=0x60000 || !context(&value) ||
       value.Eip!=entry_address || value.EFlags&0x100 ||
       !memory(entry_address,observed,sizeof(entry_bytes)) ||
       memcmp(observed,entry_bytes,sizeof(entry_bytes)) ||
       !memory(image_base+0x10015,observed,sizeof(invoke_bytes)) ||
       memcmp(observed,invoke_bytes,sizeof(invoke_bytes)) ||
       !memory(image_base+0x10763,observed,sizeof(launch_bytes)) ||
       memcmp(observed,launch_bytes,sizeof(launch_bytes)) ||
       !add_trap(image_base+0x10763,0xe8,index)) return 0;
    wcc_prefix=1;
    fprintf(log_file,"wcc-prefix entry=%08lx launch=%08lx\n",entry_address,image_base+0x10763);
    fflush(log_file);
    if(!budget() || !ContinueDebugEvent(child_pid,child_tid,DBG_CONTINUE)) return 0;
    pending=0;return 1;
}
static int prepare_wcc_entry(DWORD *index)
{
    DWORD slots[3];CONTEXT value;MEMORY_BASIC_INFORMATION region;unsigned i;
    /* This is one verified loader-to-CF entry, not permission for a CF body.
       All initialized bytes/fixups must already match in the controller. */
    if(!pending || !wcc_stopped || wcc_handoff || armed || !context(&value) ||
       value.Eip!=image_base+0x10764 || value.EFlags&0x100 ||
       !memory(image_base+0x30000,slots,sizeof(slots)) || !slots[1] || slots[1]!=slots[2] ||
       !bounds(slots[1],0x91000) || slots[0]!=slots[1]+0x2fe00 || value.Esi!=slots[0] ||
       value.Ebx!=slots[1] || value.Edx!=image_base+0x10788 ||
       trap_count==MAX_TRAPS ||
       VirtualQueryEx(child,(void *)slots[0],&region,sizeof(region))!=sizeof(region) ||
       region.State!=MEM_COMMIT || region.Protect&(PAGE_GUARD|PAGE_NOACCESS) ||
       !patch(slots[0],0x88,0xcc)) return 0;
    traps[trap_count].address=slots[0];traps[trap_count].saved=0x88;
    traps[trap_count].active=1;*index=trap_count++;
    for(i=0;i<trap_count;i++) if(traps[i].address==image_base+0x10763 && traps[i].active) break;
    if(i==trap_count || !consume(i)) return 0;
    wcc_cf_base=slots[1];wcc_cf_entry=slots[0];wcc_handoff=1;
    fprintf(log_file,"wcc-entry-armed base=%08lx entry=%08lx original=88 masked=cc\n",wcc_cf_base,wcc_cf_entry);
    fflush(log_file);return 1;
}
static int continue_wcc_entry_trap(void)
{
    CONTEXT value;DWORD actual_return;
    if(!budget() || !pending || !wcc_at_entry || wcc_trap_running || wcc_entry_stopped || armed ||
       wcc_instruction!=6 || !context(&value) || !same_context(&value,&wcc_entry_context) ||
       value.Eip!=wcc_cf_entry || value.EFlags&0x100 || !trap_ready(wcc_cf_entry) ||
       event.dwDebugEventCode!=EXCEPTION_DEBUG_EVENT ||
       event.u.Exception.ExceptionRecord.ExceptionCode!=EXCEPTION_SINGLE_STEP ||
       !event.u.Exception.dwFirstChance ||
       !memory(value.Esp,&actual_return,4) || actual_return!=image_base+0x1001f) return 0;
    wcc_trap_running=1;
    if(!ContinueDebugEvent(child_pid,child_tid,DBG_CONTINUE)) return 0;
    pending=0;return 1;
}
static int teardown(DWORD code)
{
    DWORD began=GetTickCount(),actual=STILL_ACTIVE;DEBUG_EVENT drain;unsigned i;int extras_done;
    exiting=1;
    /* A failed termination request never licenses continuation of unknown code. */
    if(child && !child_exit_seen && WaitForSingleObject(child,0)!=WAIT_OBJECT_0 &&
       !TerminateProcess(child,code)) return 0;
    for(i=0;i<extra_count;i++) if(!extra[i].exited && WaitForSingleObject(extra[i].handle,0)!=WAIT_OBJECT_0 &&
                                 !TerminateProcess(extra[i].handle,code)) return 0;
    if(pending && event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT && event.dwProcessId!=child_pid) {
        for(i=0;i<extra_count;i++) if(extra[i].pid==event.dwProcessId) break;
        if(i==extra_count && !TerminateProcess(event.u.CreateProcessInfo.hProcess,code)) return 0;
    }
    if(pending && event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT && event.u.LoadDll.hFile)
        CloseHandle(event.u.LoadDll.hFile);
    if(pending && event.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT && event.u.CreateThread.hThread)
        CloseHandle(event.u.CreateThread.hThread);
    if(pending) {ContinueDebugEvent(event.dwProcessId,event.dwThreadId,DBG_CONTINUE);pending=0;}
    while(child && GetTickCount()-began<3000) {
        extras_done=1;for(i=0;i<extra_count;i++) if(!extra[i].exited) extras_done=0;
        if(child_exit_seen && extras_done) break;
        if(!WaitForDebugEvent(&drain,10)) continue;
        fprintf(log_file,"drain code=%lu pid=%lu tid=%lu\n",drain.dwDebugEventCode,drain.dwProcessId,drain.dwThreadId);
        if(drain.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT && drain.dwProcessId==child_pid) child_exit_seen=1;
        if(drain.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT)
            for(i=0;i<extra_count;i++) if(drain.dwProcessId==extra[i].pid) extra[i].exited=1;
        if(drain.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT && drain.dwProcessId!=child_pid) {
            if(!TerminateProcess(drain.u.CreateProcessInfo.hProcess,code)) return 0;
            if(extra_count<16) {
                extra[extra_count].pid=drain.dwProcessId;
                extra[extra_count++].handle=drain.u.CreateProcessInfo.hProcess;
            } else ownership_incomplete=1;
            if(drain.u.CreateProcessInfo.hThread) CloseHandle(drain.u.CreateProcessInfo.hThread);
            if(drain.u.CreateProcessInfo.hFile) CloseHandle(drain.u.CreateProcessInfo.hFile);
        }
        if(drain.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT && drain.u.CreateThread.hThread)
            CloseHandle(drain.u.CreateThread.hThread);
        if(drain.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT && drain.u.LoadDll.hFile) CloseHandle(drain.u.LoadDll.hFile);
        ContinueDebugEvent(drain.dwProcessId,drain.dwThreadId,DBG_CONTINUE);
    }
    if(child) GetExitCodeProcess(child,&actual);
    fprintf(log_file,"owned-teardown exit-event=%d signaled=%d status=%lu\n",child_exit_seen,
            child && WaitForSingleObject(child,0)==WAIT_OBJECT_0,actual);fflush(log_file);
    extras_done=1;
    for(i=0;i<extra_count;i++) {
        if(!extra[i].exited || WaitForSingleObject(extra[i].handle,0)!=WAIT_OBJECT_0) extras_done=0;
        fprintf(log_file,"extra-owned-teardown pid=%lu exit-event=%d signaled=%d\n",extra[i].pid,
                extra[i].exited,WaitForSingleObject(extra[i].handle,0)==WAIT_OBJECT_0);
    }
    teardown_ok=child && child_exit_seen && extras_done && !ownership_incomplete &&
                actual!=STILL_ACTIVE && WaitForSingleObject(child,0)==WAIT_OBJECT_0;
    return teardown_ok;
}
static int dispatch(DWORD operation,DWORD length,DWORD *reply_length)
{
    DWORD packet[24],index,selectors[4];CONTEXT value;LDT_ENTRY descriptor;
    MEMORY_BASIC_INFORMATION region;unsigned i;
    *reply_length=0;
    if(length>sizeof(packet)) return 0;
    /* Default prefix capture is permanently stopped. Explicit OP22 admits only
       six byte-bound loader instructions, then inspection/STOP at CF entry.
       No RPC can admit the first CF body byte or a different transfer target. */
    if(wcc_prefix && operation!=OP_CONTEXT && operation!=OP_READ &&
       operation!=OP_SELECTORS && operation!=OP_REGION && operation!=OP_STOP &&
       !(operation==OP_EVENT && (!wcc_stopped || (wcc_handoff && !wcc_at_entry) ||
                                (wcc_trap_running && !wcc_entry_stopped))) &&
       !(operation==OP_WCC_ENTRY && wcc_stopped && !wcc_handoff) &&
       !(operation==OP_WCC_ENTRY_TRAP && wcc_at_entry && !wcc_trap_running) &&
       !((operation==OP_ARM || operation==OP_STEP) && wcc_handoff && !wcc_at_entry)) return 0;
    memcpy(packet,area+PAYLOAD,length);
    if(operation==OP_EVENT && !length) {
        if(!next_event()) return 0;
        memset(packet,0,sizeof(packet));packet[0]=event.dwDebugEventCode;
        packet[1]=event.dwProcessId;packet[2]=event.dwThreadId;
        packet[3]=image_base;packet[4]=entry_address;packet[5]=teb;
        if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            packet[6]=event.u.Exception.ExceptionRecord.ExceptionCode;
            packet[7]=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            packet[8]=event.u.Exception.dwFirstChance;
        }
        memcpy(area+PAYLOAD,packet,36);*reply_length=36;return 1;
    }
    if(operation==OP_CONTEXT && !length && context(&value)) {
        pack_context(packet,&value);memcpy(area+PAYLOAD,packet,64);*reply_length=64;return 1;
    }
    if(operation==OP_SELECTORS && !length && context(&value)) {
        selectors[0]=value.SegCs;selectors[1]=value.SegSs;
        selectors[2]=value.SegDs;selectors[3]=value.SegEs;
        for(i=0;i<4;i++) {
            if(!GetThreadSelectorEntry(thread,selectors[i],&descriptor)) return 0;
            packet[i*3]=selectors[i];memcpy(packet+i*3+1,&descriptor,8);
        }
        memcpy(area+PAYLOAD,packet,48);*reply_length=48;return 1;
    }
    if(operation==OP_REGION && length==4 && pending &&
       VirtualQueryEx(child,(void *)packet[0],&region,sizeof(region))==sizeof(region)) {
        packet[0]=(DWORD)region.BaseAddress;packet[1]=(DWORD)region.AllocationBase;
        packet[2]=region.AllocationProtect;packet[3]=region.RegionSize;
        packet[4]=region.State;packet[5]=region.Protect;packet[6]=region.Type;
        memcpy(area+PAYLOAD,packet,28);*reply_length=28;return 1;
    }
    if(operation==OP_READ && length==8 && pending && packet[1] && packet[1]<=MAX_READ &&
       memory(packet[0],area+PAYLOAD,packet[1])) {*reply_length=packet[1];return 1;}
    if(operation==OP_TRAP && length==8 && add_trap(packet[0],packet[1],&index)) {
        *(DWORD *)(area+PAYLOAD)=index;*reply_length=4;return 1;
    }
    if(operation==OP_CONSUME && length==4) return consume(packet[0]);
    if(operation==OP_ARM && length==76) return arm(packet);
    if(operation==OP_STEP && !length && armed && pending && context(&value) && same_context(&value,&armed_context)) {
        if(!budget() || !ContinueDebugEvent(child_pid,child_tid,DBG_CONTINUE)) return 0;
        pending=0;++steps;return 1;
    }
    if(operation==OP_LOADER && !length) return continue_loader();
    if(operation==OP_WCC_PREFIX && !length && continue_wcc_prefix(&index)) {
        *(DWORD *)(area+PAYLOAD)=index;*reply_length=4;return 1;
    }
    if(operation==OP_WCC_ENTRY && !length && prepare_wcc_entry(&index)) {
        *(DWORD *)(area+PAYLOAD)=index;*reply_length=4;return 1;
    }
    if(operation==OP_WCC_ENTRY_TRAP && !length) return continue_wcc_entry_trap();
    if(operation==OP_STOP && !length) return teardown(0x6000);
    return 0;
}
int main(int argc,char **argv)
{
    STARTUPINFOA startup;PROCESS_INFORMATION process;SECURITY_ATTRIBUTES security;
    DWORD high=0,operation,length,status=90,response_length;char command[4096];int ok,descriptor;unsigned i;
    HANDLE inherited[3];
    /* map,target,rawargs,cwd,stdin,stdout,stderr,log,entry-rva,entry-byte,global-ms,lease-ms */
    if(argc!=13 || strlen(argv[2])+strlen(argv[3])+5>=sizeof(command)) return 90;
    entry_rva=strtoul(argv[9],NULL,16);entry_byte=strtoul(argv[10],NULL,16);
    global_ms=strtoul(argv[11],NULL,10);lease_ms=strtoul(argv[12],NULL,10);
    if(entry_byte>255 || !global_ms || global_ms>300000 || !lease_ms || lease_ms>10000) return 90;
    descriptor=open(argv[8],O_WRONLY|O_CREAT|O_EXCL|O_BINARY|O_NOINHERIT,S_IREAD|S_IWRITE);
    if(descriptor<0) return 91;
    log_file=fdopen(descriptor,"wb");if(!log_file) {close(descriptor);return 91;}
    map_file=CreateFileA(argv[1],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
    if(map_file==INVALID_HANDLE_VALUE || GetFileSize(map_file,&high)!=MAP_BYTES || high) goto done;
    map_handle=CreateFileMappingA(map_file,NULL,PAGE_READWRITE,0,MAP_BYTES,NULL);
    if(!map_handle) goto done;
    area=MapViewOfFile(map_handle,FILE_MAP_READ|FILE_MAP_WRITE,0,0,MAP_BYTES);
    if(!area || ((DWORD)area&3) || load(0)!=0x50434757UL || load(4)!=1 || load(8)!=MAP_BYTES) goto done;
    lease_value=load(20);if(!lease_value || lease_value>MAX_SEQUENCE || load(12)>1 || load(16)) goto done;
    started=lease_tick=GetTickCount();
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    input=CreateFileA(argv[5],GENERIC_READ,FILE_SHARE_READ,&security,OPEN_EXISTING,0,NULL);
    output=CreateFileA(argv[6],GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_NEW,0,NULL);
    error_output=CreateFileA(argv[7],GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_NEW,0,NULL);
    if(input==INVALID_HANDLE_VALUE || output==INVALID_HANDLE_VALUE || error_output==INVALID_HANDLE_VALUE) goto done;
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdInput=input;startup.hStdOutput=output;startup.hStdError=error_output;
    inherited[0]=GetStdHandle(STD_INPUT_HANDLE);inherited[1]=GetStdHandle(STD_OUTPUT_HANDLE);
    inherited[2]=GetStdHandle(STD_ERROR_HANDLE);
    for(i=0;i<3;i++) if(inherited[i] && inherited[i]!=INVALID_HANDLE_VALUE &&
                         (i<1 || inherited[i]!=inherited[0]) && (i<2 || inherited[i]!=inherited[1]))
        if(!CloseHandle(inherited[i])) goto done;
    SetStdHandle(STD_INPUT_HANDLE,INVALID_HANDLE_VALUE);SetStdHandle(STD_OUTPUT_HANDLE,INVALID_HANDLE_VALUE);
    SetStdHandle(STD_ERROR_HANDLE,INVALID_HANDLE_VALUE);
    sprintf(command,"\"%s\" %s",argv[2],argv[3]);
    if(!CreateProcessA(argv[2],command,NULL,NULL,TRUE,DEBUG_PROCESS,NULL,argv[4],&startup,&process)) goto done;
    child=process.hProcess;thread=process.hThread;child_pid=process.dwProcessId;child_tid=process.dwThreadId;
    CloseHandle(input);input=INVALID_HANDLE_VALUE;CloseHandle(output);output=INVALID_HANDLE_VALUE;
    CloseHandle(error_output);error_output=INVALID_HANDLE_VALUE;
    fprintf(log_file,"owned-child pid=%lu tid=%lu entry-rva=%08lx entry-byte=%02lx\n",child_pid,child_tid,entry_rva,entry_byte);
    status=92;
    while(budget() && !exiting) {
        DWORD command_sequence=load(12);
        if(load(0)!=0x50434757UL || load(4)!=1 || load(8)!=MAP_BYTES) break;
        if(command_sequence==seen) {Sleep(1);continue;}
        if(command_sequence!=seen+1 || command_sequence>MAX_SEQUENCE || load(16)!=seen) break;
        operation=load(24);length=load(28);if(length>MAX_PAYLOAD) break;
        ok=dispatch(operation,length,&response_length);
        if(load(12)!=command_sequence || load(24)!=operation || load(28)!=length) ok=0;
        *(DWORD *)(area+32)=ok?0:93;*(DWORD *)(area+36)=ok?response_length:0;
        publish(16,command_sequence);seen=command_sequence;
        if(!ok) break;
    }
    if(exiting && teardown_ok) status=0;
done:
    if(child && !exiting && !teardown(status)) status=94;
    if(thread) CloseHandle(thread);if(child) CloseHandle(child);
    for(i=0;i<extra_count;i++) if(extra[i].handle) CloseHandle(extra[i].handle);
    if(input!=INVALID_HANDLE_VALUE) CloseHandle(input);
    if(output!=INVALID_HANDLE_VALUE) CloseHandle(output);
    if(error_output!=INVALID_HANDLE_VALUE) CloseHandle(error_output);
    if(area) UnmapViewOfFile(area);if(map_handle) CloseHandle(map_handle);
    if(map_file!=INVALID_HANDLE_VALUE) CloseHandle(map_file);
    fprintf(log_file,"broker-end status=%lu events=%lu steps=%lu exit-event=%d\n",status,events,steps,child_exit_seen);
    fclose(log_file);return (int)status;
}
