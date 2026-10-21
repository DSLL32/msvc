/* Observe original RC's child launch and snapshot temporary files before deletion.
   Capture only MSC_CMD_FLAGS from environment blocks, never unrelated values. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    DWORD sites[2],base,amount,status,stack[11],code;
    BYTE originals[2],trap=0xcc;
    int i,stepping=-1,launches=0,deletes=0; unsigned length;
    char command[4096],name[80],path[1024],*entry;
    if(argc!=7) return 2;
    SetEnvironmentVariableA("TMP",argv[5]); SetEnvironmentVariableA("TEMP",argv[5]);
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);
    sprintf(command,"\"%s\" %s",argv[1],argv[6]);
    if(!CreateProcessA(argv[1],command,0,0,TRUE,DEBUG_ONLY_THIS_PROCESS,0,argv[5],&startup,&child)) return 3;
    for(;;) {
        if(!WaitForDebugEvent(&event,30000)) return 4;
        status=DBG_CONTINUE;
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
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
            for(i=0;i<2;++i) if((DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress==sites[i]) break;
            if(code==EXCEPTION_BREAKPOINT && i<2) {
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                if(!GetThreadContext(child.hThread,&context)) return 7;
                if(!ReadProcessMemory(child.hProcess,(void *)context.Esp,stack,sizeof(stack),&amount)) return 8;
                if(i==0) {
                    ++launches;
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
            printf("exit %lu; captured %d launches and %d deletes\n",event.u.ExitProcess.dwExitCode,launches,deletes);
            ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
            return event.u.ExitProcess.dwExitCode;
        }
        ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
    }
}
