/* Observe original RC's child launch and snapshot temporary files before deletion.
   Capture only MSC_CMD_FLAGS from environment blocks, never unrelated values. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>
static unsigned char buffer[65536];
static void save(char *root,char *name,void *data,unsigned length)
{
    char path[1024]; FILE *file;
    sprintf(path,"%s\\%s",root,name);
    file=fopen(path,"wb"); if(!file) exit(10);
    fwrite(data,1,length,file); fclose(file);
}
static unsigned read_string(HANDLE process,DWORD address)
{
    unsigned i; DWORD got;
    if(!address) { buffer[0]=0; return 0; }
    for(i=0;i<sizeof(buffer)-1;++i) {
        if(!ReadProcessMemory(process,(void *)(address+i),buffer+i,1,&got)) exit(11);
        if(!buffer[i]) return i;
    }
    exit(12); return 0;
}
int main(int argc,char **argv)
{
    STARTUPINFOA startup; PROCESS_INFORMATION child; DEBUG_EVENT event; CONTEXT context;
    DWORD sites[2],base=0,amount,status,stack[11],code;
    HANDLE out,err,cpp_process=0,cpp_thread=0; SECURITY_ATTRIBUTES security;
    DWORD cpp_pid=0,cpp_site=0;BYTE cpp_original=0;int cpp_stepping=0,reads=0;
    BYTE originals[2],trap=0xcc;
    int i,stepping=-1,launches=0,deletes=0; unsigned length;
    char command[4096],name[80],path[1024],*entry;
    if(argc!=7 && argc!=8) return 2;
    if(getenv("RC_REFERENCE_PATH"))SetEnvironmentVariableA("PATH",getenv("RC_REFERENCE_PATH"));
    SetEnvironmentVariableA("TMP",argv[5]); SetEnvironmentVariableA("TEMP",argv[5]);
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    sprintf(path,"%s\\stdout.bin",argv[4]);out=CreateFileA(path,GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_ALWAYS,0,0);
    sprintf(path,"%s\\stderr.bin",argv[4]);err=CreateFileA(path,GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_ALWAYS,0,0);
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);
    startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdOutput=out;startup.hStdError=err;
    startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    if(argc==8 && !strcmp(argv[7],"replay")) {
        HANDLE input;DWORD size;
        input=CreateFileA(argv[6],GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
        if(input==INVALID_HANDLE_VALUE)return 30;
        if(!ReadFile(input,command,sizeof(command)-1,&size,0))return 31;
        command[size]=0;CloseHandle(input);
        if(!CreateProcessA(argv[1],command,0,0,TRUE,0,0,argv[5],&startup,&child))return 32;
        WaitForSingleObject(child.hProcess,INFINITE);GetExitCodeProcess(child.hProcess,&status);
        CloseHandle(child.hProcess);CloseHandle(child.hThread);CloseHandle(out);CloseHandle(err);
        return status;
    }
    sprintf(command,"\"%s\" %s",argv[1],argv[6]);
    if(!CreateProcessA(argv[1],command,0,0,TRUE,DEBUG_PROCESS,0,argv[5],&startup,&child)) return 3;
    for(;;) {
        if(!WaitForDebugEvent(&event,30000)) return 4;
        status=DBG_CONTINUE;
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            if(event.dwProcessId!=child.dwProcessId) {
                printf("child pid %lu\n",event.dwProcessId);
                cpp_pid=event.dwProcessId;cpp_process=event.u.CreateProcessInfo.hProcess;cpp_thread=event.u.CreateProcessInfo.hThread;
                cpp_site=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage+0xf2f0;
                if(!ReadProcessMemory(cpp_process,(void *)cpp_site,&cpp_original,1,&amount))return 40;
                if(!WriteProcessMemory(cpp_process,(void *)cpp_site,&trap,1,&amount))return 41;
                FlushInstructionCache(cpp_process,(void *)cpp_site,1);
                if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
                ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);continue;
            }
            base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            for(i=0;i<2;++i) {
                sites[i]=base+strtoul(argv[2+i],NULL,16);
                if(!ReadProcessMemory(child.hProcess,(void *)sites[i],originals+i,1,&amount)) return 5;
                if(!WriteProcessMemory(child.hProcess,(void *)sites[i],&trap,1,&amount)) return 6;
            }
            FlushInstructionCache(child.hProcess,NULL,0);
            if(event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            if(event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            code=event.u.Exception.ExceptionRecord.ExceptionCode;
            if(event.dwProcessId!=child.dwProcessId) {
                if(event.dwProcessId!=cpp_pid)return 42;
                if(code==EXCEPTION_BREAKPOINT && (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress==cpp_site) {
                    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                    if(!GetThreadContext(cpp_thread,&context))return 43;
                    if(!ReadProcessMemory(cpp_process,(void *)context.Esp,stack,sizeof(stack),&amount))return 44;
                    if(stack[2]&GENERIC_READ) {
                        ++reads;length=read_string(cpp_process,stack[1]);
                        sprintf(name,"read-%04d.name",reads);save(argv[4],name,buffer,length);
                        sprintf(path,"%s\\read-%04d.bin",argv[4],reads);
                        if(!CopyFileA((char *)buffer,path,FALSE))printf("read missing %d error %lu\n",reads,GetLastError());
                    }
                    WriteProcessMemory(cpp_process,(void *)cpp_site,&cpp_original,1,&amount);FlushInstructionCache(cpp_process,(void *)cpp_site,1);
                    context.Eip=cpp_site;context.EFlags|=0x100;SetThreadContext(cpp_thread,&context);cpp_stepping=1;
                } else if(code==EXCEPTION_SINGLE_STEP && cpp_stepping) {
                    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                    GetThreadContext(cpp_thread,&context);context.EFlags&=~0x100;SetThreadContext(cpp_thread,&context);
                    WriteProcessMemory(cpp_process,(void *)cpp_site,&trap,1,&amount);FlushInstructionCache(cpp_process,(void *)cpp_site,1);cpp_stepping=0;
                } else if(code!=EXCEPTION_BREAKPOINT) {fprintf(stderr,"child exception %08lx\n",code);return 45;}
                ContinueDebugEvent(event.dwProcessId,event.dwThreadId,DBG_CONTINUE);continue;
            }
            for(i=0;i<2;++i) if((DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress==sites[i]) break;
            if(code==EXCEPTION_BREAKPOINT && i<2) {
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                if(!GetThreadContext(child.hThread,&context)) return 7;
                if(!ReadProcessMemory(child.hProcess,(void *)context.Esp,stack,sizeof(stack),&amount)) return 8;
                if(i==0) {
                    ++launches;
                    sprintf(name,"launch-%d.command",launches);length=read_string(child.hProcess,stack[2]);save(argv[4],name,buffer,length);
                    sprintf(name,"launch-%d.application",launches);length=read_string(child.hProcess,stack[1]);save(argv[4],name,buffer,length);
                    length=read_string(child.hProcess,stack[1]);save(argv[4],"application.txt",buffer,length);
                    length=read_string(child.hProcess,stack[2]);save(argv[4],"command.txt",buffer,length);
                    length=read_string(child.hProcess,stack[8]);save(argv[4],"cwd.txt",buffer,length);
                    sprintf(name,"%08lx\n",stack[6]);save(argv[4],"flags.txt",name,strlen(name));
                    if(stack[7]) {
                        if(stack[6]&0x400) return 13;
                        for(length=0;length<sizeof(buffer)-1;++length) {
                            if(!ReadProcessMemory(child.hProcess,(void *)(stack[7]+length),buffer+length,1,&amount)) return 14;
                            if(length && !buffer[length] && !buffer[length-1]) break;
                        }
                        entry=(char *)buffer;
                        while(*entry && strncmp(entry,"MSC_CMD_FLAGS=",14)) entry+=strlen(entry)+1;
                        save(argv[4],"MSC_CMD_FLAGS.txt",*entry?entry+14:entry,*entry?strlen(entry+14):0);
                        save(argv[4],"environment-kind.txt","explicit",8);
                    } else save(argv[4],"environment-kind.txt","inherited",9);
                } else {
                    ++deletes;
                    length=read_string(child.hProcess,stack[1]);
                    sprintf(name,"deleted-%d.name",deletes);save(argv[4],name,buffer,length);
                    sprintf(path,"%s\\deleted-%d.bin",argv[4],deletes);
                    if(!CopyFileA((char *)buffer,path,FALSE)) printf("snapshot failed %lu\n",GetLastError());
                }
                WriteProcessMemory(child.hProcess,(void *)sites[i],originals+i,1,&amount);
                FlushInstructionCache(child.hProcess,(void *)sites[i],1);
                context.Eip=sites[i];context.EFlags|=0x100;
                if(!SetThreadContext(child.hThread,&context)) return 9;
                stepping=i;
            } else if(code==EXCEPTION_SINGLE_STEP && stepping>=0) {
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                GetThreadContext(child.hThread,&context);context.EFlags&=~0x100;SetThreadContext(child.hThread,&context);
                WriteProcessMemory(child.hProcess,(void *)sites[stepping],&trap,1,&amount);
                FlushInstructionCache(child.hProcess,(void *)sites[stepping],1);stepping=-1;
            } else if(code!=EXCEPTION_BREAKPOINT) { fprintf(stderr,"exception %08lx\n",code);return 18; }
        } else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            if(event.dwProcessId!=child.dwProcessId) {
                printf("child exit %lu pid %lu\n",event.u.ExitProcess.dwExitCode,event.dwProcessId);
                ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);continue;
            }
            printf("exit %lu; captured %d launches and %d deletes\n",event.u.ExitProcess.dwExitCode,launches,deletes);
            ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
            return event.u.ExitProcess.dwExitCode;
        }
        ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
    }
}
