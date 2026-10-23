/* Original-only debugger. Native tests never build or launch this program. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HANDLE child;
static FILE *events;
static char directory[MAX_PATH];
static DWORD target_base,target_size;
static DWORD event_number;
static DWORD crt_base;
static int observe_resources;
static FILE *resource_events;
static DWORD resource_arguments[4], resource_call_count;
typedef struct EntryGuard {
    DWORD rva;
    unsigned byte;
    char name[128];
    int seen;
} EntryGuard;
static EntryGuard entry_guards[1024];
static unsigned entry_count;
static int root_inputs_captured;
static FILE *trace;
static int observe_mktemp;
static FILE *mktemp_events;
static DWORD mktemp_argument,mktemp_calls;
static int observe_mre;
static DWORD mre_calls;
static DWORD mre_sites[] = { 0x3597,0x35c1,0x35ca,0x3686,0x36b9,0x36e2 };
static DWORD mre_slots[] = { 0x40a300,0x40a314,0x40a304,0x40a308,0x40a30c,0x40a310 };
static int observe_access;
static FILE *access_events;
static DWORD access_calls,access_argument,access_mode;
static DWORD access_sites[] = { 0x3023,0x3066,0x309c,0x3173 };
static unsigned access_site_count;
static int access_pending = -1;
static int observe_mbspan,mbspan_pending=-1;
static FILE *mbspan_events;
static DWORD mbspan_calls;
static DWORD mbspan_sites[] = { 0x40db,0x4130 };



static void patch_byte(DWORD address, BYTE value)
{
    DWORD wrote;
    if (!WriteProcessMemory(child,(void *)address,&value,1,&wrote) || wrote!=1)
        ExitProcess(35);
    FlushInstructionCache(child,(void *)address,1);
}

static void output_path(char *out,const char *name)
{
    sprintf(out,"%s\\%s",directory,name);
}
static void memory(DWORD address,void *out,DWORD bytes)
{
    DWORD got;
    if (!ReadProcessMemory(child,(void *)address,out,bytes,&got) || got!=bytes)
        ExitProcess(20);
}
static void remote_string(DWORD address,char *out,DWORD capacity)
{
    DWORD i;
    for (i=0;i+1<capacity;++i) {
        memory(address+i,out+i,1);
        if (!out[i]) return;
    }
    out[capacity-1]=0;
}
static void save_file(HANDLE source,const char *name)
{
    char path[MAX_PATH];
    HANDLE destination;
    BYTE buffer[16384];
    DWORD got,wrote,total=0;
    output_path(path,name);
    if (!source || source==INVALID_HANDLE_VALUE) ExitProcess(21);
    if (SetFilePointer(source,0,NULL,FILE_BEGIN)==0xffffffffUL) ExitProcess(22);
    destination=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
    if (destination==INVALID_HANDLE_VALUE) ExitProcess(23);
    for (;;) {
        if (!ReadFile(source,buffer,sizeof(buffer),&got,NULL)) ExitProcess(24);
        if (!got) break;
        if (!WriteFile(destination,buffer,got,&wrote,NULL) || wrote!=got) ExitProcess(25);
        total+=got;
    }
    CloseHandle(destination);
    fprintf(events,"loaded-file %s bytes=%lu\n",name,total);
}
static DWORD exported(DWORD base,const char *wanted)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_EXPORT_DIRECTORY exports;
    DWORD i,name_rva,function_rva;
    WORD ordinal;
    char name[256];
    memory(base,&dos,sizeof(dos));memory(base+dos.e_lfanew,&nt,sizeof(nt));
    memory(base+nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,&exports,sizeof(exports));
    for (i=0;i<exports.NumberOfNames;++i) {
        memory(base+(DWORD)exports.AddressOfNames+4*i,&name_rva,4);
        remote_string(base+name_rva,name,sizeof(name));
        if (!strcmp(name,wanted)) {
            memory(base+(DWORD)exports.AddressOfNameOrdinals+2*i,&ordinal,2);
            memory(base+(DWORD)exports.AddressOfFunctions+4*ordinal,&function_rva,4);
            return base+function_rva;
        }
    }
    fprintf(events,"missing-export %s\n",wanted);fflush(events);ExitProcess(26);return 0;
}
/* Original CRT state inspection only: no code executes in the stopped child.
 * The actual loaded canonical provider proves _errno -> getptd + 8 and its
 * TlsGetValue index at RVA3d004. Capture the complete addressed TEB/PTD bytes. */
static void error_state_to(DWORD teb,DWORD tid,const char *phase,FILE *sink)
{
    BYTE raw_teb[4096],ptd[116],body[9];
    DWORD index,pointer,self,actual_tid,slot_address;
    char path[MAX_PATH];
    FILE *file;
    if (!teb || !crt_base || exported(crt_base,"_errno")!=crt_base+0x6436) ExitProcess(49);
    memory(crt_base+0x6436,body,sizeof(body));
    if (memcmp(body,"\xe8\x88\xb1\xff\xff\x83\xc0\x08\xc3",9)) ExitProcess(50);
    memory(crt_base+0x15ce,&slot_address,4);
    if (slot_address!=crt_base+0x3d004) ExitProcess(51);
    memory(slot_address,&index,4);
    if (index>=64) ExitProcess(52);
    memory(teb,raw_teb,sizeof(raw_teb));
    memcpy(&self,raw_teb+0x18,4);memcpy(&actual_tid,raw_teb+0x24,4);
    if (self!=teb || actual_tid!=tid) ExitProcess(53);
    memcpy(&pointer,raw_teb+0xe10+4*index,4);
    if (!pointer) ExitProcess(54);
    memory(pointer,ptd,sizeof(ptd));
    sprintf(path,"%s\\mktemp-%s-teb.bin",directory,phase);
    file=fopen(path,"wb");fwrite(raw_teb,1,sizeof(raw_teb),file);fclose(file);
    sprintf(path,"%s\\mktemp-%s-tls-index.bin",directory,phase);
    file=fopen(path,"wb");fwrite(&index,4,1,file);fclose(file);
    sprintf(path,"%s\\mktemp-%s-ptd.bin",directory,phase);
    file=fopen(path,"wb");fwrite(ptd,1,sizeof(ptd),file);fclose(file);
    fprintf(sink,"state phase=%s teb=%08lx tid=%lu tls_index=%lu ptd=%08lx\n",phase,teb,tid,index,pointer);
}
static void error_state(DWORD teb,DWORD tid,const char *phase)
{
    error_state_to(teb,tid,phase,mktemp_events);
}
/* Observe the stopped child's slots. Queries execute only in this debugger;
 * duplicates are closed here without changing the child's file positions. */
static void save_bytes(const char *phase,const char *kind,const void *bytes,DWORD count)
{
    char path[MAX_PATH];FILE *file;
    sprintf(path,"%s\\streams-%s-%s.bin",directory,phase,kind);
    file=fopen(path,"wb");if (!file) ExitProcess(60);
    if (fwrite(bytes,1,count,file)!=count) ExitProcess(61);
    fclose(file);
}
static void handle_query(FILE *file,HANDLE handle,int remote)
{
    HANDLE local=handle;
    BY_HANDLE_FILE_INFORMATION info;
    DWORD type,position=0,high=0,ok=0;
    if (remote && !DuplicateHandle(child,handle,GetCurrentProcess(),&local,0,FALSE,DUPLICATE_SAME_ACCESS))
        ExitProcess(62);
    type=GetFileType(local);memset(&info,0,sizeof(info));
    if (type==FILE_TYPE_DISK) {
        ok=GetFileInformationByHandle(local,&info);
        if (!ok) ExitProcess(63);
        SetLastError(0);position=SetFilePointer(local,0,&high,FILE_CURRENT);
        if (position==0xffffffffUL && GetLastError()) ExitProcess(64);
    }
    fprintf(file,"{\"handle\":%lu,\"type\":%lu,\"information_valid\":%lu,\"volume\":%lu,\"index_high\":%lu,\"index_low\":%lu,\"position_high\":%lu,\"position_low\":%lu}",
        (DWORD)handle,type,ok,info.dwVolumeSerialNumber,info.nFileIndexHigh,info.nFileIndexLow,high,position);
    if (remote) CloseHandle(local);
}
static void stream_state(DWORD teb,const char *phase,HANDLE input,HANDLE out,HANDLE err)
{
    BYTE raw_teb[4096],peb[32],parameters[36],lowio[108],iob[96];
    DWORD peb_address,parameters_address,lowio_address,iob_address,handle;
    char path[MAX_PATH];FILE *file;int i;
    memory(teb,raw_teb,sizeof(raw_teb));memcpy(&peb_address,raw_teb+0x30,4);
    memory(peb_address,peb,sizeof(peb));memcpy(&parameters_address,peb+0x10,4);
    memory(parameters_address,parameters,sizeof(parameters));
    memory(exported(crt_base,"__pioinfo"),&lowio_address,4);
    iob_address=exported(crt_base,"_iob");
    memory(lowio_address,lowio,sizeof(lowio));memory(iob_address,iob,sizeof(iob));
    save_bytes(phase,"teb",raw_teb,sizeof(raw_teb));save_bytes(phase,"peb",peb,sizeof(peb));
    save_bytes(phase,"parameters",parameters,sizeof(parameters));
    save_bytes(phase,"pioinfo-slot",&lowio_address,4);
    save_bytes(phase,"lowio",lowio,sizeof(lowio));save_bytes(phase,"iob",iob,sizeof(iob));
    sprintf(path,"%s\\streams-%s.json",directory,phase);file=fopen(path,"wb");if (!file) ExitProcess(65);
    fprintf(file,"{\"teb\":%lu,\"peb\":%lu,\"parameters\":%lu,\"lowio\":%lu,\"iob\":%lu,\"inherited\":[",teb,peb_address,parameters_address,lowio_address,iob_address);
    handle_query(file,input,0);fprintf(file,",");handle_query(file,out,0);fprintf(file,",");handle_query(file,err,0);
    fprintf(file,"],\"crt\":[");
    for (i=0;i<3;++i) { if (i) fprintf(file,",");memcpy(&handle,lowio+36*i,4);handle_query(file,(HANDLE)handle,1); }
    fprintf(file,"],\"win32\":[");
    for (i=0;i<3;++i) { if (i) fprintf(file,",");memcpy(&handle,parameters+0x18+4*i,4);handle_query(file,(HANDLE)handle,1); }
    fprintf(file,"]}\n");fclose(file);
}
static void raw_vector(const char *name,DWORD vector,int expected_count)
{
    DWORD count=0,pointer,length;
    char path[MAX_PATH],text[32768];
    FILE *file;
    output_path(path,name);file=fopen(path,"wb");
    if (!file) ExitProcess(27);
    fwrite(&vector,4,1,file);fwrite(&count,4,1,file);
    for (;;) {
        memory(vector+count*4,&pointer,4);
        if (!pointer) break;
        if (count==4096) ExitProcess(28);
        for (length=0;length<sizeof(text);++length) {
            memory(pointer+length,text+length,1);
            if (!text[length]) break;
        }
        if (length==sizeof(text)) ExitProcess(29);
        ++length;
        fwrite(&pointer,4,1,file);fwrite(&length,4,1,file);fwrite(text,1,length,file);
        ++count;
    }
    if (expected_count>=0 && count!=(DWORD)expected_count) ExitProcess(30);
    fseek(file,4,SEEK_SET);fwrite(&count,4,1,file);fclose(file);
}
static void crt_state(DWORD *stack)
{
    DWORD values[6],pointer,length;
    BYTE info[108],mb_state[261];
    char path[MAX_PATH],program[32768];
    FILE *file;
    if (!crt_base) ExitProcess(31);
    values[0]=crt_base;
    memory(exported(crt_base,"_fmode"),values+1,4);
    memory(exported(crt_base,"_commode"),values+2,4);
    memory(exported(crt_base,"__mb_cur_max"),values+3,4);
    memory(exported(crt_base,"_pgmptr"),values+4,4);
    memory(exported(crt_base,"__pioinfo"),values+5,4);
    memory(values[5],info,sizeof(info));
    output_path(path,"crt-state.bin");file=fopen(path,"wb");
    fwrite(values,4,6,file);fwrite(info,1,sizeof(info),file);fclose(file);
    pointer=values[4];
    for (length=0;length<sizeof(program);++length) {
        memory(pointer+length,program+length,1);if (!program[length]) break;
    }
    if (length==sizeof(program)) ExitProcess(32);
    output_path(path,"pgmptr.bin");file=fopen(path,"wb");fwrite(program,1,length+1,file);fclose(file);
    /* Canonical provider: actual mb codepage DWORD plus _mbctype[257]. */
    memory(crt_base+0x3f1c0,mb_state,4);
    memory(crt_base+0x3f1c8,mb_state+4,257);
    output_path(path,"mb-state.bin");file=fopen(path,"wb");fwrite(mb_state,1,sizeof(mb_state),file);fclose(file);
    raw_vector("argv.bin",stack[2],(int)stack[1]);
    raw_vector("environment.bin",stack[3],-1);
}
static void frontier_requests(DWORD address, DWORD *stack)
{
    char line[128];
    DWORD requested, length, got, i;
    BYTE buffer[65536];
    printf("TRAP %08lx\n", address);fflush(stdout);
    while (fgets(line,sizeof(line),stdin)) {
        if (!strcmp(line,"DONE\n")) break;
        if (sscanf(line,"%lx %lu",&requested,&length)!=2 || !length || length>sizeof(buffer))
            ExitProcess(33);
        if (!ReadProcessMemory(child,(void *)requested,buffer,length,&got) || got!=length)
            { printf("ERROR\n");fflush(stdout);ExitProcess(34); }
        printf("DATA %08lx %lu ",requested,length);
        for (i=0;i<length;++i) printf("%02x",(unsigned)buffer[i]);
        printf("\n");fflush(stdout);
    }
}
#include "dependency-observer.inc"

static void module(DWORD base,HANDLE file)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_EXPORT_DIRECTORY exports;
    char name[MAX_PATH];
    memory(base,&dos,sizeof(dos));
    memory(base+dos.e_lfanew,&nt,sizeof(nt));
    strcpy(name,"<no-exports>");
    if (nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress) {
        memory(base+nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
            &exports,sizeof(exports));
        remote_string(base+exports.Name,name,sizeof(name));
    }
    fprintf(events,"module event=%lu name=%s base=%08lx preferred=%08lx timestamp=%08lx size=%08lx\n",
        event_number,name,base,nt.OptionalHeader.ImageBase,nt.FileHeader.TimeDateStamp,
        nt.OptionalHeader.SizeOfImage);
    if (!_stricmp(name,"msvcrt.dll")) { crt_base=base;save_file(file,"loaded-msvcrt.bin"); }
    if (!_stricmp(name,"mspdb50.dll")) save_file(file,"loaded-mspdb50.bin");
    dependency_loaded(base,file,&nt);
    fflush(events);
}
int main(int argc,char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;
    CONTEXT context;
    DWORD status,threads_count=0,i,stack[16];
    DWORD thread_ids[64],thread_tebs[64],teb;
    HANDLE threads[64],thread;
    HANDLE out,err,input;
    char command[4096],path[MAX_PATH];
    FILE *snapshot;
    BYTE globals[2000];
    int stop_at_trap;
    if (argc!=6) return 2;
    observe_resources=atoi(argv[5]);
    strcpy(directory,argv[3]);
    stop_at_trap=atoi(argv[4]);
    output_path(path,"entries.txt");snapshot=fopen(path,"r");
    if (snapshot) {
        while (entry_count<1024 && fscanf(snapshot,"%lx %u %127s",&entry_guards[entry_count].rva,
                &entry_guards[entry_count].byte,entry_guards[entry_count].name)==3)
            ++entry_count;
        if (!feof(snapshot)) return 42;
        fclose(snapshot);
    }
    for (i=0;i<entry_count;++i)
        if (!strcmp(entry_guards[i].name,"prepare_environment")) observe_mktemp=1;
        else if (!strcmp(entry_guards[i].name,"prepare_jobs")) observe_mre=1;
        else if (!strcmp(entry_guards[i].name,"find_file")) { observe_access=1;access_site_count=3; }
        else if (!strcmp(entry_guards[i].name,"final_pass")) observe_mbspan=1;
    for (i=0;i<entry_count;++i)
        if (!strcmp(entry_guards[i].name,"search_path") && observe_access) access_site_count=4;
    output_path(path,"trace.txt");trace=fopen(path,"wb");if (!trace) return 43;
    output_path(path,"events.txt");events=fopen(path,"wb");
    if (!events) return 3;
    if (observe_resources) {
        output_path(path,"resource-events.txt");resource_events=fopen(path,"wb");
        if (!resource_events) return 36;
    }
    if (observe_mktemp) {
        output_path(path,"mktemp-events.txt");mktemp_events=fopen(path,"wb");
        if (!mktemp_events) return 48;
    }
    if (observe_access) {
        output_path(path,"access-events.txt");access_events=fopen(path,"wb");if (!access_events) return 68;
    }
    if (observe_mbspan) {
        output_path(path,"mbspan-events.txt");mbspan_events=fopen(path,"wb");if (!mbspan_events) return 75;
    }
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);
    security.bInheritHandle=TRUE;
    output_path(path,"stdout.bin");
    out=CreateFileA(path,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,0,NULL);
    output_path(path,"stderr.bin");
    err=CreateFileA(path,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,0,NULL);
    input=CreateFileA("NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,NULL);
    if (out==INVALID_HANDLE_VALUE || err==INVALID_HANDLE_VALUE || input==INVALID_HANDLE_VALUE) return 4;
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);
    startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdOutput=out;startup.hStdError=err;startup.hStdInput=input;
    /* Wine initializes its own TMP before this helper starts. Set the actual
     * child input explicitly, rather than trusting the Unix launcher value. */
    if (!SetEnvironmentVariableA("TMP",".") || !SetEnvironmentVariableA("TEMP","."))
        return 59;
    fprintf(events,"child-environment TMP=. TEMP=.\n");fflush(events);
    sprintf(command,"\"%s\" %s",argv[1],argv[2]);
    if (!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,
        NULL,NULL,&startup,&process)) return 5;
    for (;;) {
        if (!WaitForDebugEvent(&event,30000)) return 6;
        ++event_number;status=DBG_CONTINUE;
        if (event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            child=event.u.CreateProcessInfo.hProcess;
            target_base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            target_size=0x4c00;
            threads[threads_count]=event.u.CreateProcessInfo.hThread;
            thread_tebs[threads_count]=(DWORD)event.u.CreateProcessInfo.lpThreadLocalBase;
            thread_ids[threads_count++]=event.dwThreadId;
            dependency_install();
            fprintf(events,"create event=%lu base=%08lx entry=%08lx\n",event_number,
                target_base,(DWORD)event.u.CreateProcessInfo.lpStartAddress);
            save_file(event.u.CreateProcessInfo.hFile,"loaded-target.bin");
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
            if (observe_mre) {
                BYTE call[6];DWORD slot;
                for (i=0;i<6;++i) {
                    memory(target_base+mre_sites[i],call,sizeof(call));memcpy(&slot,call+2,4);
                    if (call[0]!=0xff || call[1]!=0x15 || slot!=mre_slots[i]) return 67;
                    fprintf(events,"mre-guard site=%08lx iat=%08lx\n",target_base+mre_sites[i],slot);
                    patch_byte(target_base+mre_sites[i],0xcc);
                }
            }
            if (observe_resources) {
                BYTE before,after;
                memory(target_base+0x534d,&before,1);memory(target_base+0x5353,&after,1);
                if (before!=0xff || after!=0x85) return 37;
                patch_byte(target_base+0x534d,0xcc);
            }
            if (observe_mktemp) {
                BYTE before,after;
                memory(target_base+0x23a5,&before,1);memory(target_base+0x23ab,&after,1);
                if (before!=0xff || after!=0x59) return 55;
                patch_byte(target_base+0x23a5,0xcc);
            }
            if (observe_access) {
                BYTE call[6],after;DWORD slot;
                for (i=0;i<access_site_count;++i) {
                    memory(target_base+access_sites[i],call,6);memcpy(&slot,call+2,4);
                    memory(target_base+access_sites[i]+(i==1 || i==2 ? 2 : 6),&after,1);
                    if (call[0]!=0xff || after!=0x59) return 69;
                    if (i==1 || i==2) { if (call[1]!=0xd6) return 69; }
                    else if (call[1]!=0x15 || slot!=target_base+0xa25c) return 69;
                    patch_byte(target_base+access_sites[i],0xcc);
                }
            }
            if (observe_mbspan) {
                BYTE call[6],after;DWORD slot;
                for (i=0;i<2;++i) {
                    memory(target_base+mbspan_sites[i],call,6);memcpy(&slot,call+2,4);
                    memory(target_base+mbspan_sites[i]+6,&after,1);
                    if (call[0]!=0xff || call[1]!=0x15 || slot!=target_base+0xa274 || after!=0x59) return 76;
                    patch_byte(target_base+mbspan_sites[i],0xcc);
                }
            }
            for (i=0;i<entry_count;++i) {
                BYTE byte;
                memory(target_base+entry_guards[i].rva,&byte,1);
                if (byte!=entry_guards[i].byte || byte==0xcc) return 44;
                patch_byte(target_base+entry_guards[i].rva,0xcc);
            }
        } else if (event.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
            if (threads_count==64) return 7;
            threads[threads_count]=event.u.CreateThread.hThread;
            thread_tebs[threads_count]=(DWORD)event.u.CreateThread.lpThreadLocalBase;
            thread_ids[threads_count++]=event.dwThreadId;
        } else if (event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            module((DWORD)event.u.LoadDll.lpBaseOfDll,event.u.LoadDll.hFile);
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            DWORD code=event.u.Exception.ExceptionRecord.ExceptionCode;
            DWORD address=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            EntryGuard *guard=NULL;
            int mre_site=0,access_site=-1,access_return=0;
            int mbspan_site=-1,mbspan_return=0;
            if (observe_mbspan) for (i=0;i<2;++i) {
                if (address==target_base+mbspan_sites[i]) mbspan_site=i;
                if (mbspan_pending==(int)i && address==target_base+mbspan_sites[i]+6) { mbspan_site=i;mbspan_return=1; }
            }
            if (observe_access) for (i=0;i<access_site_count;++i) {
                if (address==target_base+access_sites[i]) access_site=i;
                if (access_pending==(int)i && address==target_base+access_sites[i]+(i==1 || i==2 ? 2 : 6)) { access_site=i;access_return=1; }
            }
            if (observe_mre) for (i=0;i<6;++i)
                if (address==target_base+mre_sites[i]) mre_site=1;
            for (i=0;i<entry_count;++i)
                if (!entry_guards[i].seen && target_base+entry_guards[i].rva==address) guard=entry_guards+i;
            if (code==EXCEPTION_BREAKPOINT && guard) {
                thread=NULL;teb=0;
                for (i=0;i<threads_count;++i)
                    if (thread_ids[i]==event.dwThreadId) {
                        thread=threads[i];teb=thread_tebs[i];
                    }
                if (!thread) return 45;
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
                if (!GetThreadContext(thread,&context)) return 46;
                if (guard->rva==0x1743) {
                    memory(context.Esp,stack,sizeof(stack));crt_state(stack);root_inputs_captured=1;
                    if (observe_mktemp) error_state(teb,event.dwThreadId,"entry");
                    if (observe_mktemp) stream_state(teb,"entry",input,out,err);
                    output_path(path,"main-entry-context.bin");snapshot=fopen(path,"wb");
                    fwrite(&context,1,sizeof(context),snapshot);fwrite(stack,1,sizeof(stack),snapshot);fclose(snapshot);
                }
                fprintf(events,"entry event=%lu address=%08lx name=%s\n",event_number,address,guard->name);fflush(events);
                fprintf(trace,"%s\n",guard->name);fflush(trace);guard->seen=1;
                patch_byte(address,(BYTE)guard->byte);context.Eip=address;
                if (!SetThreadContext(thread,&context)) return 47;
            } else if (code==EXCEPTION_BREAKPOINT && dependency_point(address)>=0) {
                int point=dependency_point(address);
                thread=NULL;teb=0;
                for (i=0;i<threads_count;++i)
                    if (thread_ids[i]==event.dwThreadId) { thread=threads[i];teb=thread_tebs[i]; }
                if (!thread) return 98;
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
                if (!GetThreadContext(thread,&context)) return 99;
                dependency_trap(point,address,teb,event.dwThreadId,&context,input,out,err);
                if (!SetThreadContext(thread,&context) && point!=5) return 100;
            } else if (code==EXCEPTION_BREAKPOINT && mre_site) {
                ++mre_calls;
                fprintf(events,"mre-unexpected-call address=%08lx\n",address);fflush(events);
                TerminateProcess(child,0x7002);
            } else if (code==EXCEPTION_BREAKPOINT && access_site>=0) {
                char text[1024],phase[64],name[80];DWORD actual;
                thread=NULL;teb=0;
                for (i=0;i<threads_count;++i)
                    if (thread_ids[i]==event.dwThreadId) { thread=threads[i];teb=thread_tebs[i]; }
                if (!thread) return 70;
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                if (!GetThreadContext(thread,&context)) return 71;
                if (!access_return) {
                    if (access_pending!=-1 || ++access_calls>64) return 72;
                    memory(context.Esp,&access_argument,4);memory(context.Esp+4,&access_mode,4);
                    remote_string(access_argument,text,sizeof(text));
                    actual=access_site==1 || access_site==2 ? context.Esi : 0;
                    if (!actual) memory(target_base+0xa25c,&actual,4);
                    if (actual!=crt_base+0x63f8) return 73;
                    sprintf(name,"access-%lu-path.bin",access_calls);output_path(path,name);
                    snapshot=fopen(path,"wb");fwrite(text,1,strlen(text)+1,snapshot);fclose(snapshot);
                    fprintf(access_events,"call sequence=%lu site=%08lx function=%08lx argument=%08lx mode=%lu\n",access_calls,address,actual,access_argument,access_mode);
                    sprintf(phase,"access-%lu-before",access_calls);error_state_to(teb,event.dwThreadId,phase,access_events);
                    access_pending=access_site;patch_byte(address,0xff);
                    patch_byte(target_base+access_sites[access_site]+(access_site==1 || access_site==2 ? 2 : 6),0xcc);
                } else {
                    fprintf(access_events,"return sequence=%lu result=%08lx\n",access_calls,context.Eax);
                    sprintf(phase,"access-%lu-after",access_calls);error_state_to(teb,event.dwThreadId,phase,access_events);
                    patch_byte(address,0x59);patch_byte(target_base+access_sites[access_site],0xcc);access_pending=-1;
                }
                fflush(access_events);context.Eip=address;if (!SetThreadContext(thread,&context)) return 74;
            } else if (code==EXCEPTION_BREAKPOINT && mbspan_site>=0) {
                char text[1024],phase[64],name[80];DWORD args[2],actual;
                thread=NULL;teb=0;
                for (i=0;i<threads_count;++i)
                    if (thread_ids[i]==event.dwThreadId) { thread=threads[i];teb=thread_tebs[i]; }
                if (!thread) return 77;
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                if (!GetThreadContext(thread,&context)) return 78;
                if (!mbspan_return) {
                    if (mbspan_pending!=-1 || ++mbspan_calls>64) return 79;
                    memory(context.Esp,args,8);memory(target_base+0xa274,&actual,4);
                    if (actual!=crt_base+0x620c) return 80;
                    for (i=0;i<2;++i) {
                        remote_string(args[i],text,sizeof(text));
                        sprintf(name,"mbspan-%lu-%s.bin",mbspan_calls,i ? "set" : "text");output_path(path,name);
                        snapshot=fopen(path,"wb");fwrite(text,1,strlen(text)+1,snapshot);fclose(snapshot);
                    }
                    fprintf(mbspan_events,"call sequence=%lu site=%08lx function=%08lx text=%08lx set=%08lx\n",mbspan_calls,address,actual,args[0],args[1]);
                    sprintf(phase,"mbspan-%lu-before",mbspan_calls);error_state_to(teb,event.dwThreadId,phase,mbspan_events);
                    mbspan_pending=mbspan_site;patch_byte(address,0xff);patch_byte(address+6,0xcc);
                } else {
                    fprintf(mbspan_events,"return sequence=%lu result=%08lx\n",mbspan_calls,context.Eax);
                    sprintf(phase,"mbspan-%lu-after",mbspan_calls);error_state_to(teb,event.dwThreadId,phase,mbspan_events);
                    patch_byte(address,0x59);patch_byte(target_base+mbspan_sites[mbspan_site],0xcc);mbspan_pending=-1;
                }
                sprintf(name,"%s-context.bin",phase);output_path(path,name);snapshot=fopen(path,"wb");
                fwrite(&context,1,sizeof(context),snapshot);fclose(snapshot);
                fflush(mbspan_events);context.Eip=address;if (!SetThreadContext(thread,&context)) return 81;
            } else if (code==EXCEPTION_BREAKPOINT && observe_resources &&
                (address==target_base+0x534d || address==target_base+0x5353)) {
                BYTE output[256];
                thread=NULL;teb=0;
                for (i=0;i<threads_count;++i)
                    if (thread_ids[i]==event.dwThreadId) { thread=threads[i];teb=thread_tebs[i]; }
                if (!thread) return 38;
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                if (!GetThreadContext(thread,&context)) return 39;
                if (address==target_base+0x534d) {
                    memory(context.Esp,resource_arguments,sizeof(resource_arguments));
                    if (resource_arguments[0]!=target_base || resource_arguments[2]!=target_base+0x9698 || resource_arguments[3]!=256) return 40;
                    ++resource_call_count;
                    memory(resource_arguments[2],output,sizeof(output));
                    sprintf(path,"%s\\resource-%04lu-before.bin",directory,resource_call_count);
                    snapshot=fopen(path,"wb");fwrite(output,1,sizeof(output),snapshot);fclose(snapshot);
                    fprintf(resource_events,"call sequence=%lu id=%lu module=%08lx output=%08lx capacity=%lu\n",
                        resource_call_count,resource_arguments[1],resource_arguments[0],resource_arguments[2],resource_arguments[3]);
                    patch_byte(address,0xff);patch_byte(target_base+0x5353,0xcc);
                } else {
                    memory(resource_arguments[2],output,sizeof(output));
                    sprintf(path,"%s\\resource-%04lu-after.bin",directory,resource_call_count);
                    snapshot=fopen(path,"wb");fwrite(output,1,sizeof(output),snapshot);fclose(snapshot);
                    fprintf(resource_events,"return sequence=%lu id=%lu value=%lu\n",resource_call_count,resource_arguments[1],context.Eax);
                    patch_byte(address,0x85);patch_byte(target_base+0x534d,0xcc);
                }
                fflush(resource_events);context.Eip=address;
                if (!SetThreadContext(thread,&context)) return 41;
            } else if (code==EXCEPTION_BREAKPOINT && observe_mktemp &&
                (address==target_base+0x23a5 || address==target_base+0x23ab)) {
                char text[1024];
                thread=NULL;teb=0;
                for (i=0;i<threads_count;++i)
                    if (thread_ids[i]==event.dwThreadId) { thread=threads[i];teb=thread_tebs[i]; }
                if (!thread) return 56;
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                if (!GetThreadContext(thread,&context)) return 57;
                if (address==target_base+0x23a5) {
                    if (++mktemp_calls!=1) return 58;
                    memory(context.Esp,&mktemp_argument,4);
                    remote_string(mktemp_argument,text,sizeof(text));
                    output_path(path,"mktemp-before.bin");snapshot=fopen(path,"wb");
                    fwrite(text,1,strlen(text)+1,snapshot);fclose(snapshot);
                    fprintf(mktemp_events,"call sequence=%lu argument=%08lx\n",mktemp_calls,mktemp_argument);
                    error_state(teb,event.dwThreadId,"before");
                    patch_byte(address,0xff);patch_byte(target_base+0x23ab,0xcc);
                } else {
                    remote_string(mktemp_argument,text,sizeof(text));
                    output_path(path,"mktemp-after.bin");snapshot=fopen(path,"wb");
                    fwrite(text,1,strlen(text)+1,snapshot);fclose(snapshot);
                    fprintf(mktemp_events,"return sequence=%lu result=%08lx argument=%08lx\n",mktemp_calls,context.Eax,mktemp_argument);
                    error_state(teb,event.dwThreadId,"after");
                    patch_byte(address,0x59);patch_byte(target_base+0x23a5,0xcc);
                }
                fflush(mktemp_events);context.Eip=address;
                if (!SetThreadContext(thread,&context)) return 59;
            } else if (code==EXCEPTION_BREAKPOINT && address>=target_base+0x1000 &&
                address<target_base+0x1000+target_size && stop_at_trap) {
                thread=NULL;teb=0;
                for (i=0;i<threads_count;++i)
                    if (thread_ids[i]==event.dwThreadId) { thread=threads[i];teb=thread_tebs[i]; }
                if (!thread) return 8;
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
                if (!GetThreadContext(thread,&context)) return 9;
                memory(context.Esp,stack,sizeof(stack));
                memory(target_base+0x9000,globals,sizeof(globals));
                output_path(path,"context.bin");snapshot=fopen(path,"wb");
                fwrite(&context,1,sizeof(context),snapshot);fwrite(stack,1,sizeof(stack),snapshot);fclose(snapshot);
                output_path(path,"globals.bin");snapshot=fopen(path,"wb");
                fwrite(globals,1,sizeof(globals),snapshot);fclose(snapshot);
                if (address==target_base+0x1743) crt_state(stack);
                else if (!root_inputs_captured && address!=target_base+0x58e0) {
                    DWORD root[4];
                    root[0]=0;
                    memory(exported(crt_base,"__argc"),root+1,4);
                    memory(exported(crt_base,"__argv"),root+2,4);
                    memory(exported(crt_base,"_environ"),root+3,4);
                    crt_state(root);
                }
                fprintf(events,"trap event=%lu address=%08lx rva=%08lx eax=%08lx ecx=%08lx edx=%08lx esp=%08lx\n",
                    event_number,address,address-target_base,context.Eax,context.Ecx,context.Edx,context.Esp);
                fflush(events);
                if (observe_mktemp && address!=target_base+0x58e0) {
                    error_state(teb,event.dwThreadId,"frontier");fflush(mktemp_events);
                    stream_state(teb,"frontier",input,out,err);
                }
                if (address!=target_base+0x1743 && address!=target_base+0x58e0)
                    frontier_requests(address,stack);
                TerminateProcess(child,0x7001);
            } else if (code!=EXCEPTION_BREAKPOINT) {
                fprintf(events,"exception event=%lu code=%08lx address=%08lx first=%lu\n",
                    event_number,code,address,event.u.Exception.dwFirstChance);
                fflush(events);status=DBG_EXCEPTION_NOT_HANDLED;
            }
        } else if (event.dwDebugEventCode==UNLOAD_DLL_DEBUG_EVENT) {
            if (dependency_analysis) {
                if ((DWORD)event.u.UnloadDll.lpBaseOfDll==dependency_c1) ++dependency_unloads;
                fprintf(dependency_events,"module-unload event=%lu base=%08lx\n",event_number,(DWORD)event.u.UnloadDll.lpBaseOfDll);fflush(dependency_events);
            }
        } else if (event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            fprintf(events,"exit event=%lu status=%lu\n",event_number,event.u.ExitProcess.dwExitCode);
            if (observe_mre) {
                output_path(path,"mre-calls.txt");snapshot=fopen(path,"wb");
                if (!snapshot) return 68;
                fprintf(snapshot,"%lu\n",mre_calls);fclose(snapshot);
            }
            fclose(events);fclose(trace);if (mktemp_events) fclose(mktemp_events);if (resource_events) fclose(resource_events);CloseHandle(out);CloseHandle(err);CloseHandle(input);
            ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
            return event.u.ExitProcess.dwExitCode==0x7001 || (dependency_analysis && event.u.ExitProcess.dwExitCode==0x7003) ? 0:event.u.ExitProcess.dwExitCode;
        }
        ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
    }
}
