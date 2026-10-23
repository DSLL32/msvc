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
typedef struct Process { DWORD pid,parent,base; HANDLE handle; int active,target,packer,loader,copy; DWORD copy_output,copy_error; } Process;
typedef struct Thread { DWORD pid,tid,teb; HANDLE handle; int active; } Thread;
typedef struct Entry { DWORD rva; unsigned byte,seen; char name[128]; } Entry;
static Process processes[64];
static Thread threads[256];
static Entry entries[2048];
static unsigned entry_count;
static unsigned char *text;
static DWORD text_rva,text_size;
static FILE *log_file;
typedef struct CopyPlan { char image[4096],command[4096],cwd[4096],source[4096],destination[4096]; } CopyPlan;
static CopyPlan copy_plans[8];
static unsigned copy_count,next_copy;

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

/* Optional narrow external-copy capture. Only explicitly pinned cmd children
 * are allowed; this helper never participates in native fixture execution. */
static int plan_line(FILE *file,char *line)
{
    unsigned n;
    if (!fgets(line,4096,file)) return 0;
    n=strlen(line);
    if (!n || line[n-1]!='\n') return 0;
    line[--n]=0;
    if (n && line[n-1]=='\r') line[--n]=0;
    return n!=0;
}
static int copy_environment(HANDLE process,void *parameters)
{
    WCHAR wide[32768];
    void *environment;
    DWORD got;
    unsigned n,start=0,i;
    char line[4096];
    const char *keys[]={"COMSPEC=","LIB=","LINK_REPRO=","TMP=","TEMP=","COPYCMD=",
        "CL=","_CL_=","LINK=","_LINK_=","MSC_CMD_FLAGS=","LINK32_DB="};
    int written=0,length;
    if (!ReadProcessMemory(process,(char *)parameters+0x48,&environment,4,&got) || got!=4) return 0;
    fprintf(log_file,"[");
    for (n=0;n<32768;++n) {
        if (!ReadProcessMemory(process,(char *)environment+n*2,&wide[n],2,&got) || got!=2) return 0;
        if (!wide[n]) {
            if (n==start) {fprintf(log_file,"]");return 1;}
            length=WideCharToMultiByte(CP_ACP,0,wide+start,n-start,line,4095,NULL,NULL);
            if (!length) return 0;
            line[length]=0;
            for (i=0;i<sizeof(keys)/sizeof(keys[0]);++i)
                if (!_strnicmp(line,keys[i],strlen(keys[i]))) {
                    if (written++) fprintf(log_file,",");quoted(line);break;
                }
            start=n+1;
        }
    }
    return 0;
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

/* Explicit first-call analysis of the resolved OS relocation primitive. */
static DWORD relocation_page,relocation_entries,relocation_pointer,relocation_delta,relocation_stack;
static unsigned short relocation_words[256];
static void relocation_sample(Process *process,CONTEXT *context,DWORD site)
{
    DWORD args[4],got,value,procedure,n;
    if (site==0x50379) {
        if (!ReadProcessMemory(process->handle,(void *)context->Esp,args,16,&got) || got!=16 || args[1]>256) return;
        relocation_page=args[0];relocation_entries=args[1];relocation_pointer=args[2];
        relocation_delta=args[3];relocation_stack=context->Esp;
        if (!ReadProcessMemory(process->handle,(void *)relocation_pointer,relocation_words,relocation_entries*2,&got) || got!=relocation_entries*2) return;
        if (!ReadProcessMemory(process->handle,(void *)0x6d468,&procedure,4,&got) || got!=4) return;
        fprintf(log_file,"{\"event\":\"relocation-primitive-before\",\"procedure\":%lu,\"page\":%lu,\"array\":%lu,\"count\":%lu,\"delta\":%lu,\"esp\":%lu,\"records\":[",
            procedure,relocation_page,relocation_pointer,relocation_entries,relocation_delta,relocation_stack);
    } else if (site==0x5037f) {
        fprintf(log_file,"{\"event\":\"relocation-primitive-after\",\"result\":%lu,\"result_array_offset\":%lu,\"stack_cleanup\":%lu,\"records\":[",
            context->Eax,context->Eax-relocation_pointer,context->Esp-relocation_stack);
    } else return;
    for (n=0;n<relocation_entries;++n) {
        value=0;
        if ((relocation_words[n]>>12)==3 &&
            (!ReadProcessMemory(process->handle,(void *)(relocation_page+(relocation_words[n]&0xfff)),&value,4,&got) || got!=4)) return;
        fprintf(log_file,"%s{\"word\":%u,\"type\":%u,\"offset\":%u,\"value\":%lu}",n?",":"",
            relocation_words[n],relocation_words[n]>>12,relocation_words[n]&0xfff,value);
    }
    fprintf(log_file,"]}\n");fflush(log_file);
}

/* One public base16 conversion in the frozen REBASE argument inventory. */
static DWORD strtoul_input,strtoul_end_location,strtoul_input_length;
static void strtoul_sample(Process *process,CONTEXT *context,DWORD site)
{
    DWORD args[4],got,end,n;
    char number[64];
    if (site==0x541d0) {
        if (!ReadProcessMemory(process->handle,(void *)context->Esp,args,16,&got) || got!=16) return;
        strtoul_input=args[1];strtoul_end_location=args[2];
        for(n=0;n<63;++n) {
            if (!ReadProcessMemory(process->handle,(void *)(strtoul_input+n),number+n,1,&got) || got!=1) return;
            if (!number[n]) break;
        }
        if(n==63)return;
        strtoul_input_length=n;
        fprintf(log_file,"{\"event\":\"strtoul-before\",\"input\":");quoted(number);
        fprintf(log_file,",\"base\":%lu,\"end_pointer_present\":%d}\n",args[3],strtoul_end_location!=0);
    } else if (site==0x4ff96) {
        if (!ReadProcessMemory(process->handle,(void *)strtoul_end_location,&end,4,&got) || got!=4) return;
        fprintf(log_file,"{\"event\":\"strtoul-after\",\"result\":%lu,\"end_offset\":%lu,\"end_points_into_input\":%d}\n",
            context->Eax,end-strtoul_input,end>=strtoul_input && end<=strtoul_input+strtoul_input_length);
    }
    fflush(log_file);
}

/* Explicit editor API observations; no oracle/fixture calls this capture helper. */
static DWORD bind_api_args[4],bind_api_stack;
static void bind_api_sample(Process *process,Thread *thread,CONTEXT *context,DWORD site)
{
    DWORD error=0xffffffffUL,crt_errno=0xffffffffUL,got,i,count=0;
    unsigned short system_time[8];
    DWORD file_time[2];
    const char *name=NULL;
    int before=0;
    switch(site) {
    case 0x4f81f:name="FlushViewOfFile";before=1;count=2;break;
    case 0x4f824:name="FlushViewOfFile";break;
    case 0x4f828:name="GetSystemTime";before=1;count=1;break;
    case 0x4f82d:name="GetSystemTime";break;
    case 0x4f835:name="SystemTimeToFileTime";before=1;count=2;break;
    case 0x4f83a:name="SystemTimeToFileTime";break;
    case 0x4f84d:name="SetFileTime";before=1;count=4;break;
    case 0x4f852:name="SetFileTime";break;
    default:return;
    }
    if(before) {
        memset(bind_api_args,0,sizeof(bind_api_args));
        if(!ReadProcessMemory(process->handle,(void *)context->Esp,bind_api_args,count*4,&got) || got!=count*4)return;
        bind_api_stack=context->Esp;
    }
    if(!ReadProcessMemory(process->handle,(void *)(thread->teb+0x34),&error,4,&got) || got!=4)return;
    if(!ReadProcessMemory(process->handle,(void *)0x852d8,&crt_errno,4,&got) || got!=4)return;
    fprintf(log_file,"{\"event\":\"editor-api\",\"name\":");quoted(name);
    fprintf(log_file,",\"before\":%d,\"result\":%lu,\"errno\":%lu,\"last_error\":%lu,\"cleanup\":%lu,\"args\":[",before,context->Eax,crt_errno,error,context->Esp-bind_api_stack);
    for(i=0;i<4;++i)fprintf(log_file,"%s%lu",i?",":"",bind_api_args[i]);
    fprintf(log_file,"]");
    if((site==0x4f82d || site==0x4f835) && ReadProcessMemory(process->handle,(void *)bind_api_args[0],system_time,16,&got) && got==16) {
        fprintf(log_file,",\"system_time\":[");
        for(i=0;i<8;++i)fprintf(log_file,"%s%u",i?",":"",system_time[i]);
        fprintf(log_file,"]");
    }
    if((site==0x4f83a || site==0x4f84d || site==0x4f852) && ReadProcessMemory(process->handle,(void *)bind_api_args[site==0x4f83a?1:3],file_time,8,&got) && got==8)
        fprintf(log_file,",\"file_time\":[%lu,%lu]",file_time[0],file_time[1]);
    fprintf(log_file,"}\n");fflush(log_file);
}

static void context_log(Process *process,HANDLE thread)
{
    CONTEXT context;
    DWORD stack[24],got,crt_errno=0xffffffffUL;
    unsigned n;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if (!GetThreadContext(thread,&context)) return;
    fprintf(log_file,"{\"event\":\"context\",\"pid\":%lu,\"eip\":%lu,\"esp\":%lu,\"eax\":%lu,\"ecx\":%lu,\"edx\":%lu,\"stack\":[",
        process->pid,context.Eip,context.Esp,context.Eax,context.Ecx,context.Edx);
    if (ReadProcessMemory(process->handle,(void *)context.Esp,stack,sizeof(stack),&got)) {
        for (n=0;n<got/4;++n) fprintf(log_file,"%s%lu",n?",":"",stack[n]);
    }
    ReadProcessMemory(process->handle,(void *)0x852d8,&crt_errno,4,&got);
    fprintf(log_file,"],\"crt_errno\":%lu}\n",crt_errno);fflush(log_file);
    /* Explicit ROM state analysis: only the writer/serializer's live fields.
       Record+4 is unconsumed and is deliberately neither reported nor compared. */
    if (context.Eip==0x1f281) {
        DWORD first,next,record[4],count;
        if (!ReadProcessMemory(process->handle,(void *)0x5c460,&first,4,&got) || got!=4 ||
            !ReadProcessMemory(process->handle,(void *)0x5c3ec,&next,4,&got) || got!=4 ||
            next<first || ((next-first)&15) || (next-first)/16>256) return;
        count=(next-first)/16;
        fprintf(log_file,"{\"event\":\"rom_fixups\",\"stride\":16,\"count\":%lu,\"records\":[",count);
        for (n=0;n<count;++n) {
            if (!ReadProcessMemory(process->handle,(void *)(first+n*16),record,16,&got) || got!=16) return;
            fprintf(log_file,"%s{\"type\":%lu,\"rva\":%lu,\"value\":%lu}",
                n?",":"",record[0]&0xffff,record[2],record[3]);
        }
        fprintf(log_file,"]}\n");fflush(log_file);
    }
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
    char command[8192],image[4096],child_command[4096],child_cwd[4096],dll[4096];
    char line[4096];
    DWORD parent_errno;
    if (argc!=12 && argc!=13) return 2;
    if (argc==13) {
        file=fopen(argv[12],"rt");
        if(!file || !plan_line(file,line))return 4;
        copy_count=atoi(line);if(!copy_count || copy_count>8)return 4;
        for(i=0;i<copy_count;++i)
            if(!plan_line(file,copy_plans[i].image) || !plan_line(file,copy_plans[i].command) ||
               !plan_line(file,copy_plans[i].cwd) || !plan_line(file,copy_plans[i].source) ||
               !plan_line(file,copy_plans[i].destination))return 4;
        if(fgetc(file)!=EOF)return 4;fclose(file);
    }
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
            } else if (next_copy<copy_count && process->parent==root_pid &&
                       !_stricmp(image,copy_plans[next_copy].image) &&
                       !strcmp(child_command,copy_plans[next_copy].command)) {
                if(!remote_string(process->handle,parameters,0x24,child_cwd)){failed=9;break;}
                sprintf(line,"%s\\",copy_plans[next_copy].cwd);
                if((_stricmp(child_cwd,copy_plans[next_copy].cwd) && _stricmp(child_cwd,line)) ||
                   GetFileAttributesA(copy_plans[next_copy].source)==0xffffffffUL ||
                   GetFileAttributesA(copy_plans[next_copy].destination)!=0xffffffffUL){failed=9;break;}
                process->copy=++next_copy;
                process->copy_output=GetFileSize(output,NULL);process->copy_error=GetFileSize(error,NULL);
                parent_errno=0xffffffffUL;
                for(n=0;n<64;++n)if(processes[n].active && processes[n].pid==root_pid)
                    ReadProcessMemory(processes[n].handle,(void *)0x852d8,&parent_errno,4,&got);
                fprintf(log_file,"{\"event\":\"copy-begin\",\"pid\":%lu,\"index\":%d,\"stdout_start\":%lu,\"stderr_start\":%lu,\"parent_errno\":%lu,\"cwd\":",
                    process->pid,process->copy,process->copy_output,process->copy_error,parent_errno);
                quoted(child_cwd);fprintf(log_file,",\"environment\":");
                if(!copy_environment(process->handle,parameters)){failed=9;break;}
                fprintf(log_file,"}\n");fflush(log_file);
            } else {failed=9;break;}
            for(n=0;n<256 && threads[n].active;++n);
            if(n==256){failed=7;break;}
            threads[n].pid=event.dwProcessId;threads[n].tid=event.dwThreadId;
            threads[n].teb=(DWORD)event.u.CreateProcessInfo.lpThreadLocalBase;
            threads[n].handle=event.u.CreateProcessInfo.hThread;threads[n].active=1;
            if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
            for(n=0;n<256 && threads[n].active;++n);
            if(n==256){failed=7;break;}
            threads[n].pid=event.dwProcessId;threads[n].tid=event.dwThreadId;
            threads[n].teb=(DWORD)event.u.CreateThread.lpThreadLocalBase;
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
                quoted(entries[i].name);
                parent_errno=0xffffffffUL;
                ReadProcessMemory(process->handle,(void *)0x852d8,&parent_errno,4,&got);
                fprintf(log_file,",\"crt_errno\":%lu}\n",parent_errno);fflush(log_file);
                relocation_sample(process,&context,site);
                strtoul_sample(process,&context,site);
                bind_api_sample(process,thread,&context,site);
            } else {failed=10;context_log(process,thread->handle);break;}
        } else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            if(!process){failed=7;break;}
            fprintf(log_file,"{\"event\":\"exit\",\"pid\":%lu,\"code\":%lu,\"packer\":%d}\n",
                process->pid,event.u.ExitProcess.dwExitCode,process->packer);fflush(log_file);
            if(process->packer) {
                after_output=GetFileSize(output,NULL);after_error=GetFileSize(error,NULL);
                if(!CopyFileA(dll,argv[10],FALSE)){failed=8;break;}
            }
            if(process->copy) {
                parent_errno=0xffffffffUL;
                for(n=0;n<64;++n)if(processes[n].active && processes[n].pid==root_pid)
                    ReadProcessMemory(processes[n].handle,(void *)0x852d8,&parent_errno,4,&got);
                fprintf(log_file,"{\"event\":\"copy-end\",\"pid\":%lu,\"index\":%d,\"code\":%lu,\"stdout_end\":%lu,\"stderr_end\":%lu,\"parent_errno\":%lu}\n",
                    process->pid,process->copy,event.u.ExitProcess.dwExitCode,
                    GetFileSize(output,NULL),GetFileSize(error,NULL),parent_errno);fflush(log_file);
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
