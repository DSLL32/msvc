/* Explicit original-only reference capture. Never linked into the reconstruction
   or launched by a fixture. All breakpoint/mask bytes are analysis inputs. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

typedef LONG (WINAPI *QueryProcess)(HANDLE,ULONG,void *,ULONG,ULONG *);
typedef struct BasicInfo { LONG status; void *peb; ULONG affinity; LONG priority; ULONG pid,parent; } BasicInfo;
typedef struct String32 { unsigned short length,maximum; void *buffer; } String32;
typedef struct Process { DWORD pid,parent,base; HANDLE handle; int active,target,packer,loader; } Process;
typedef struct Thread { DWORD pid,tid; HANDLE handle; int active; } Thread;
typedef struct Entry { DWORD rva; unsigned byte,seen; char name[128]; } Entry;
static Process processes[64];
static Thread threads[256];
static Entry entries[2048];
static unsigned entry_count;
static unsigned char *text;
static DWORD text_rva,text_size;
static FILE *log_file;

static void quoted(const char *s)
{
    unsigned char c;
    fputc('"',log_file);
    while (*s) {
        c=(unsigned char)*s++;
        if (c=='"' || c=='\\') fputc('\\',log_file);
        if (c<32) fprintf(log_file,"\\u%04x",c);
        else fputc(c,log_file);
    }
    fputc('"',log_file);
}

static int remote_string(HANDLE process,void *parameters,int offset,char *out)
{
    String32 value;
    WCHAR wide[2048];
    DWORD got;
    int n;
    if (!ReadProcessMemory(process,(char *)parameters+offset,&value,sizeof(value),&got) ||
        got!=sizeof(value) || value.length>=sizeof(wide)) return 0;
    if (!ReadProcessMemory(process,value.buffer,wide,value.length,&got) || got!=value.length)
        return 0;
    n=WideCharToMultiByte(CP_ACP,0,wide,value.length/2,out,4095,NULL,NULL);
    out[n]=0;
    return n!=0;
}

static int write_code(Process *process,DWORD rva,const void *data,DWORD size)
{
    DWORD old,restored,written;
    void *where=(void *)(process->base+rva);
    return VirtualProtectEx(process->handle,where,size,PAGE_EXECUTE_READWRITE,&old) &&
        WriteProcessMemory(process->handle,where,(void *)data,size,&written) && written==size &&
        FlushInstructionCache(process->handle,where,size) &&
        VirtualProtectEx(process->handle,where,size,old,&restored);
}

static void context_log(Process *process,HANDLE thread)
{
    CONTEXT context;
    DWORD stack[24],got;
    unsigned n;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if (!GetThreadContext(thread,&context)) return;
    fprintf(log_file,"{\"event\":\"context\",\"pid\":%lu,\"eip\":%lu,\"esp\":%lu,\"eax\":%lu,\"ecx\":%lu,\"edx\":%lu,\"stack\":[",
        process->pid,context.Eip,context.Esp,context.Eax,context.Ecx,context.Edx);
    if (ReadProcessMemory(process->handle,(void *)context.Esp,stack,sizeof(stack),&got)) {
        for (n=0;n<got/4;++n) fprintf(log_file,"%s%lu",n?",":"",stack[n]);
    }
    fprintf(log_file,"]}\n");fflush(log_file);
}

int main(int argc,char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION launched;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;
    CONTEXT context;
    BasicInfo basic;
    QueryProcess query;
    HANDLE output,error;
    FILE *file;
    Process *process;
    Thread *thread;
    DWORD got,root_pid,root_exit=999,status,site,code,live=0,packers=0;
    DWORD before_output=0,after_output=0,before_error=0,after_error=0;
    unsigned n,i;
    int failed=0;
    unsigned char instruction;
    void *parameters;
    char command[8192],image[4096],child_command[4096],dll[4096];
    if (argc!=12) return 2;
    query=(QueryProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"),"NtQueryInformationProcess");
    if (!query) return 3;
    file=fopen(argv[7],"rb");
    if (!file) return 4;
    if (fread(&text_rva,4,1,file)!=1 || fread(&text_size,4,1,file)!=1 || text_size>0x1000000)
        return 4;
    text=malloc(text_size);
    if (!text || fread(text,1,text_size,file)!=text_size || fgetc(file)!=EOF) return 4;
    fclose(file);
    file=fopen(argv[8],"rt");
    if (!file) return 4;
    while (entry_count<2048 && fscanf(file,"%lx %x %u %127s",&entries[entry_count].rva,
        &entries[entry_count].byte,&entries[entry_count].seen,entries[entry_count].name)==4) {
        if (entries[entry_count].rva<text_rva || entries[entry_count].rva>=text_rva+text_size ||
            entries[entry_count].byte>255 || entries[entry_count].seen>1) return 4;
        ++entry_count;
    }
    if (!feof(file)) return 4;
    fclose(file);
    log_file=fopen(argv[5],"wt");if(!log_file)return 4;
    memset(&startup,0,sizeof(startup));memset(&security,0,sizeof(security));
    security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    output=CreateFileA(argv[3],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
        &security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    error=CreateFileA(argv[4],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
        &security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if (output==INVALID_HANDLE_VALUE || error==INVALID_HANDLE_VALUE) return 4;
    startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    if (strlen(argv[1])+strlen(argv[2])+5>=sizeof(command))return 4;
    sprintf(command,"\"%s\" %s",argv[1],argv[2]);
    sprintf(dll,"%s\\hello-debug.dll",argv[6]);
    if (!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_PROCESS,NULL,argv[6],&startup,&launched))
        return 5;
    root_pid=launched.dwProcessId;CloseHandle(launched.hProcess);CloseHandle(launched.hThread);
    do {
        if (!WaitForDebugEvent(&event,30000)) {failed=6;break;}
        status=DBG_CONTINUE;process=NULL;thread=NULL;
        for(n=0;n<64;++n)if(processes[n].active && processes[n].pid==event.dwProcessId)process=&processes[n];
        for(n=0;n<256;++n)if(threads[n].active && threads[n].tid==event.dwThreadId)thread=&threads[n];
        if (event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            for(n=0;n<64 && processes[n].active;++n);
            if(n==64){failed=7;break;}
            process=&processes[n];memset(process,0,sizeof(*process));
            process->pid=event.dwProcessId;process->handle=event.u.CreateProcessInfo.hProcess;
            process->base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;process->active=1;++live;
            if(query(process->handle,0,&basic,sizeof(basic),NULL) ||
                !ReadProcessMemory(process->handle,(char *)basic.peb+0x10,&parameters,4,&got) ||
                !remote_string(process->handle,parameters,0x38,image) ||
                !remote_string(process->handle,parameters,0x40,child_command)){failed=7;break;}
            process->parent=basic.parent;process->target=!_stricmp(image,argv[1]);
            process->packer=strcmp(argv[11],"-") && !_stricmp(image,argv[11]);
            fprintf(log_file,"{\"event\":\"create\",\"pid\":%lu,\"parent\":%lu,\"target\":%d,\"packer\":%d,\"image\":",
                process->pid,process->parent,process->target,process->packer);quoted(image);
            fprintf(log_file,",\"command_line\":");quoted(child_command);fprintf(log_file,"}\n");fflush(log_file);
            if (process->target) {
                if(process->base!=0x10000 || !write_code(process,text_rva,text,text_size)){failed=8;break;}
            } else if (process->packer) {
                ++packers;before_output=GetFileSize(output,NULL);before_error=GetFileSize(error,NULL);
                if(!CopyFileA(dll,argv[9],FALSE)){failed=8;break;}
            } else {failed=9;break;}
            for(n=0;n<256 && threads[n].active;++n);
            if(n==256){failed=7;break;}
            threads[n].pid=event.dwProcessId;threads[n].tid=event.dwThreadId;
            threads[n].handle=event.u.CreateProcessInfo.hThread;threads[n].active=1;
            if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
            for(n=0;n<256 && threads[n].active;++n);
            if(n==256){failed=7;break;}
            threads[n].pid=event.dwProcessId;threads[n].tid=event.dwThreadId;
            threads[n].handle=event.u.CreateThread.hThread;threads[n].active=1;
        } else if (event.dwDebugEventCode==EXIT_THREAD_DEBUG_EVENT) {
            if(thread){CloseHandle(thread->handle);thread->active=0;}
        } else if (event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            code=event.u.Exception.ExceptionRecord.ExceptionCode;
            site=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if(!process || !thread){failed=7;break;}
            if(code==EXCEPTION_BREAKPOINT && !process->loader &&
                (!process->target || site<process->base+text_rva || site>=process->base+text_rva+text_size))
                process->loader=1;
            else if(code==EXCEPTION_BREAKPOINT && process->target) {
                for(i=0;i<entry_count && entries[i].rva!=site-process->base;++i);
                if(i==entry_count || entries[i].seen){failed=10;context_log(process,thread->handle);break;}
                instruction=(unsigned char)entries[i].byte;
                text[entries[i].rva-text_rva]=instruction;
                for(n=0;n<64;++n)if(processes[n].active && processes[n].target &&
                    !write_code(&processes[n],entries[i].rva,&instruction,1)){failed=8;break;}
                if(failed)break;
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                if(!GetThreadContext(thread->handle,&context)){failed=8;break;}
                context.Eip=site;
                if(!SetThreadContext(thread->handle,&context)){failed=8;break;}
                entries[i].seen=1;
                fprintf(log_file,"{\"event\":\"entry\",\"pid\":%lu,\"rva\":%lu,\"name\":",process->pid,entries[i].rva);
                quoted(entries[i].name);fprintf(log_file,"}\n");fflush(log_file);
            } else {failed=10;context_log(process,thread->handle);break;}
        } else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            if(!process){failed=7;break;}
            fprintf(log_file,"{\"event\":\"exit\",\"pid\":%lu,\"code\":%lu,\"packer\":%d}\n",
                process->pid,event.u.ExitProcess.dwExitCode,process->packer);fflush(log_file);
            if(process->packer) {
                after_output=GetFileSize(output,NULL);after_error=GetFileSize(error,NULL);
                if(!CopyFileA(dll,argv[10],FALSE)){failed=8;break;}
            }
            if(process->pid==root_pid)root_exit=event.u.ExitProcess.dwExitCode;
            CloseHandle(process->handle);process->active=0;--live;
            for(n=0;n<256;++n)if(threads[n].active && threads[n].pid==event.dwProcessId){
                CloseHandle(threads[n].handle);threads[n].active=0;
            }
        }
        ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
    } while(live);
    if(failed)for(n=0;n<64;++n)if(processes[n].active)TerminateProcess(processes[n].handle,99);
    fprintf(log_file,"{\"event\":\"summary\",\"root_exit\":%lu,\"failed\":%d,\"packers\":%lu,\"packer_stdout_start\":%lu,\"packer_stdout_end\":%lu,\"packer_stderr_start\":%lu,\"packer_stderr_end\":%lu}\n",
        root_exit,failed,packers,before_output,after_output,before_error,after_error);fclose(log_file);
    CloseHandle(output);CloseHandle(error);free(text);
    return failed;
}
