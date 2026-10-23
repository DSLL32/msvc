/* Original-only CL/C2 debugger: freeze actual backend transport and inputs. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <recon.h>

typedef struct ProcessState {
    DWORD id, points[3];
    HANDLE handle;
    BYTE saved[3];
    int pending, backend, loader_break;
} ProcessState;
typedef struct ThreadState { DWORD id, process; HANDLE handle; } ThreadState;
static ProcessState processes[4];
static ThreadState threads[128];
static unsigned process_count, thread_count;
static HANDLE child;
static FILE *events;
static char compiler_environment[8192];

static void read_memory(DWORD address, void *buffer, DWORD size)
{
    DWORD got;
    if (!ReadProcessMemory(child,(void *)address,buffer,size,&got) || got!=size)
        ExitProcess(101);
}
static void patch(DWORD address, BYTE byte)
{
    DWORD written;
    if (!WriteProcessMemory(child,(void *)address,&byte,1,&written) || written!=1)
        ExitProcess(102);
    FlushInstructionCache(child,(void *)address,1);
}
static void string_at(DWORD address, char *text, unsigned capacity)
{
    unsigned i;
    for(i=0;i+1<capacity;++i) {
        read_memory(address+i,text+i,1);
        if(!text[i])return;
    }
    ExitProcess(103);
}
static void save_file(HANDLE input,const char *name)
{
    BYTE buffer[16384];
    DWORD got,written,previous;
    HANDLE output;
    previous=SetFilePointer(input,0,NULL,FILE_CURRENT);
    if(previous==0xffffffffUL || SetFilePointer(input,0,NULL,FILE_BEGIN)==0xffffffffUL)
        ExitProcess(104);
    output=CreateFileA(name,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
    if(output==INVALID_HANDLE_VALUE)ExitProcess(105);
    for(;;) {
        if(!ReadFile(input,buffer,sizeof(buffer),&got,NULL))ExitProcess(106);
        if(!got)break;
        if(!WriteFile(output,buffer,got,&written,NULL)||written!=got)ExitProcess(107);
    }
    CloseHandle(output);SetFilePointer(input,previous,NULL,FILE_BEGIN);
}
static void save_vector(DWORD address,const char *name,int print)
{
    FILE *output=fopen(name,"wb");
    DWORD pointer,rows=0,length;
    char value;
    if(!output)ExitProcess(108);
    for(;;) {
        if(++rows>4096)ExitProcess(109);
        read_memory(address,&pointer,4);address+=4;
        if(!pointer)break;
        if(print)fprintf(events,"vector %s %lu ",name,rows-1);
        length=0;
        do {
            if(++length>65536)ExitProcess(110);
            read_memory(pointer++,&value,1);fputc(value,output);
            if(print&&value)fputc(value,events);
        }while(value);
        if(print)fputc('\n',events);
    }
    fputc(0,output);fclose(output);
}
static void copy_ir(void)
{
    const char *at=strstr(compiler_environment,"-il ");
    char prefix[MAX_PATH],pattern[MAX_PATH],directory[MAX_PATH];
    char input[MAX_PATH],output[MAX_PATH],*slash;
    DWORD length=0,files=0;
    WIN32_FIND_DATAA found;
    HANDLE search;
    if(!at)ExitProcess(111);
    at+=4;
    while(*at&&*at!=' ') {
        if(*at=='"'||length+1>=sizeof(prefix))ExitProcess(112);
        prefix[length++]=*at++;
    }
    prefix[length]=0;sprintf(pattern,"%s*",prefix);strcpy(directory,prefix);
    slash=strrchr(directory,'\\');if(!slash)slash=strrchr(directory,'/');
    if(slash)slash[1]=0;else directory[0]=0;
    if(!CreateDirectoryA("captured-ir",NULL))ExitProcess(113);
    search=FindFirstFileA(pattern,&found);
    if(search==INVALID_HANDLE_VALUE)ExitProcess(114);
    do {
        if(!(found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
            sprintf(input,"%s%s",directory,found.cFileName);
            sprintf(output,"captured-ir\\%s",found.cFileName);
            if(!CopyFileA(input,output,TRUE))ExitProcess(115);
            fprintf(events,"ir %s bytes=%lu:%lu\n",found.cFileName,found.nFileSizeHigh,found.nFileSizeLow);
            ++files;
        }
    }while(FindNextFileA(search,&found));
    FindClose(search);if(!files)ExitProcess(116);
}
static void loaded_module(ProcessState *process,DWORD base,HANDLE file)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_EXPORT_DIRECTORY exports;
    char name[MAX_PATH],output[MAX_PATH];
    read_memory(base,&dos,sizeof(dos));read_memory(base+dos.e_lfanew,&nt,sizeof(nt));
    strcpy(name,"<no-exports>");
    if(nt.OptionalHeader.DataDirectory[0].VirtualAddress) {
        read_memory(base+nt.OptionalHeader.DataDirectory[0].VirtualAddress,&exports,sizeof(exports));
        string_at(base+exports.Name,name,sizeof(name));
    }
    fprintf(events,"module pid=%lu backend=%d base=%08lx preferred=%08lx name=%s\n",process->id,process->backend,base,nt.OptionalHeader.ImageBase,name);
    if(!_stricmp(name,"msvcrt.dll")) {
        sprintf(output,"loaded-%s-msvcrt.bin",process->backend?"c2":"cl");save_file(file,output);
    }
}
int main(int argc,char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION root;
    DEBUG_EVENT event;
    CONTEXT context;
    ProcessState *process;
    ThreadState *thread;
    DWORD status,stack[4],base,address,code;
    unsigned i;
    char command[8192],text[8192];
    int finished=0;
    char environment[]="PATH=R:\\toolchains\\5.10\\bin\0SystemRoot=C:\\windows\0TEMP=.\0TMP=.\0WINEDLLOVERRIDES=msvcrt=n\0WINEPATH=R:\\toolchains\\5.10\\bin\0windir=C:\\windows\0INCLUDE=R:\\toolchains\\5.10\\include\0LIB=R:\\toolchains\\5.10\\lib\0";
    if(argc!=3)return 100;
    events=fopen("events.txt","wb");if(!events)return 100;
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);
    SetEnvironmentVariableA("TMP",".");SetEnvironmentVariableA("TEMP",".");
    sprintf(command,"\"%s\" %s",argv[1],argv[2]);
    {
        FILE *output;
        char directory[MAX_PATH];
        output=fopen("cl-environment-input.bin","wb");if(!output)return 131;
        fwrite(environment,1,sizeof(environment),output);fclose(output);
        output=fopen("cl-command-input.bin","wb");if(!output)return 132;
        fwrite(command,1,strlen(command)+1,output);fclose(output);
        if(!GetCurrentDirectoryA(sizeof(directory),directory))return 133;
        output=fopen("working-directory.bin","wb");if(!output)return 134;
        fwrite(directory,1,strlen(directory)+1,output);fclose(output);
    }
    if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_PROCESS,environment,NULL,&startup,&root))return 117;
    while(!finished&&WaitForDebugEvent(&event,30000)) {
        status=DBG_CONTINUE;process=NULL;
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            IMAGE_DOS_HEADER dos;
            IMAGE_NT_HEADERS nt;
            if(process_count==4)return 118;
            process=&processes[process_count++];process->id=event.dwProcessId;
            process->handle=event.u.CreateProcessInfo.hProcess;process->pending=-1;
            process->backend=event.dwProcessId!=root.dwProcessId;
            child=process->handle;base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            read_memory(base,&dos,sizeof(dos));read_memory(base+dos.e_lfanew,&nt,sizeof(nt));
            fprintf(events,"process pid=%lu backend=%d base=%08lx entry=%08lx\n",process->id,process->backend,base,base+nt.OptionalHeader.AddressOfEntryPoint);
            if(process->backend) {
                if(process_count!=2||base!=0x400000||base+nt.OptionalHeader.AddressOfEntryPoint!=0x452797)return 119;
                save_file(event.u.CreateProcessInfo.hFile,"loaded-c2.exe");process->points[0]=0x4528b0;
            }else {
                save_file(event.u.CreateProcessInfo.hFile,"loaded-cl.exe");
                process->points[0]=0x4056c9;process->points[1]=0x40572e;process->points[2]=0x4055d2;
            }
            for(i=0;i<3;++i)if(process->points[i]) {
                read_memory(process->points[i],process->saved+i,1);patch(process->points[i],0xcc);
            }
            if(thread_count==128)return 120;
            threads[thread_count].id=event.dwThreadId;threads[thread_count].process=process->id;
            threads[thread_count++].handle=event.u.CreateProcessInfo.hThread;
            if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
        }else {
            for(i=0;i<process_count;++i)if(processes[i].id==event.dwProcessId)process=&processes[i];
            if(!process)return 121;
            child=process->handle;
            if(event.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
                if(thread_count==128)return 122;
                threads[thread_count].id=event.dwThreadId;threads[thread_count].process=process->id;
                threads[thread_count++].handle=event.u.CreateThread.hThread;
            }else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
                loaded_module(process,(DWORD)event.u.LoadDll.lpBaseOfDll,event.u.LoadDll.hFile);
                if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);
            }else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
                code=event.u.Exception.ExceptionRecord.ExceptionCode;
                address=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;thread=NULL;
                for(i=0;i<thread_count;++i)if(threads[i].id==event.dwThreadId&&threads[i].process==process->id)thread=&threads[i];
                if(!thread)return 123;
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                if(!GetThreadContext(thread->handle,&context))return 124;
                if(code==EXCEPTION_SINGLE_STEP&&process->pending>=0) {
                    patch(process->points[process->pending],0xcc);context.EFlags&=~0x100;process->pending=-1;
                    if(!SetThreadContext(thread->handle,&context))return 125;
                }else if(code==EXCEPTION_BREAKPOINT) {
                    for(i=0;i<3;++i)if(process->points[i]&&process->points[i]==address)break;
                    if(i<3) {
                        read_memory(context.Esp,stack,sizeof(stack));
                        fprintf(events,"boundary pid=%lu backend=%d address=%08lx esp=%08lx\n",process->id,process->backend,address,context.Esp);
                        if(process->backend) {
                            fprintf(events,"c2-main argc=%lu argv=%08lx environment=%08lx\n",stack[1],stack[2],stack[3]);
                            save_vector(stack[2],"c2-argv.bin",1);save_vector(stack[3],"c2-environment.bin",1);
                        }else if(i==0) {
                            fprintf(events,"frontend argc=%lu boolean=%lu\n",stack[0],stack[2]);
                            save_vector(stack[1],"frontend-argv.bin",1);
                        }else if(i==1) {
                            string_at(stack[1],text,sizeof(text));fprintf(events,"spawn mode=%lu program=%s\n",stack[0],text);
                            save_vector(stack[2],"spawn-argv.bin",1);copy_ir();
                        }else {
                            FILE *output;
                            string_at(stack[0],compiler_environment,sizeof(compiler_environment));
                            if(strncmp(compiler_environment,"MSC_CMD_FLAGS=",14))return 126;
                            output=fopen("msc-cmd-flags.bin","wb");if(!output)return 127;
                            fwrite(compiler_environment,1,strlen(compiler_environment)+1,output);fclose(output);
                        }
                        patch(address,process->saved[i]);context.Eip=address;context.EFlags|=0x100;process->pending=i;
                        if(!SetThreadContext(thread->handle,&context))return 128;
                    }else if(!process->loader_break)process->loader_break=1;
                    else {fprintf(events,"unexpected-break pid=%lu address=%08lx\n",process->id,address);return 129;}
                }else {
                    fprintf(events,"exception pid=%lu code=%08lx address=%08lx first=%lu\n",process->id,code,address,event.u.Exception.dwFirstChance);
                    status=DBG_EXCEPTION_NOT_HANDLED;
                }
            }else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
                fprintf(events,"exit pid=%lu backend=%d status=%lu\n",process->id,process->backend,event.u.ExitProcess.dwExitCode);
                if(!process->backend)finished=1;
            }
        }
        fflush(events);ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
    }
    fclose(events);return finished?0:130;
}
