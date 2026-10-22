/* Debug the complete tool process tree used by the differential harness. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct DebugProcess {
    struct DebugProcess *next;
    DWORD id,base;
    HANDLE handle;
    int loader_break;
} DebugProcess;

int main(int argc,char **argv)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DEBUG_EVENT ev;
    DebugProcess *processes=NULL,*process,**link;
    HANDLE out,err,log;
    DWORD written,status,code,root_id;
    char command[2048],line[256];
    int failed=0,root_seen=0;

    if (argc!=6 && argc!=7) return 2;
    memset(&si,0,sizeof(si));
    memset(&sa,0,sizeof(sa));
    sa.nLength=sizeof(sa);
    sa.bInheritHandle=TRUE;
    out=CreateFileA(argv[3],GENERIC_WRITE,FILE_SHARE_READ,&sa,
                    CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    err=CreateFileA(argv[4],GENERIC_WRITE,FILE_SHARE_READ,&sa,
                    CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    log=CreateFileA(argv[5],GENERIC_WRITE,FILE_SHARE_READ,0,
                    CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    si.cb=sizeof(si);
    si.dwFlags=STARTF_USESTDHANDLES;
    si.hStdOutput=out;
    si.hStdError=err;
    si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    sprintf(command,"\"%s\" %s",argv[1],argv[2]);
    if (!CreateProcessA(argv[1],command,0,0,TRUE,DEBUG_PROCESS,0,
                        argc==7?argv[6]:0,&si,&pi)) {
        sprintf(line,"create-error %lu\n",GetLastError());
        WriteFile(log,line,strlen(line),&written,0);
        return 3;
    }
    root_id=pi.dwProcessId;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    do {
        if (!WaitForDebugEvent(&ev,30000)) return 4;
        status=DBG_CONTINUE;
        for (process=processes;process && process->id!=ev.dwProcessId;process=process->next) {}
        if (ev.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            process=malloc(sizeof(*process));
            if (!process) return 5;
            process->id=ev.dwProcessId;
            process->base=(DWORD)ev.u.CreateProcessInfo.lpBaseOfImage;
            process->handle=ev.u.CreateProcessInfo.hProcess;
            process->loader_break=0;
            process->next=processes;
            processes=process;
            if (process->id==root_id) root_seen=1;
            CloseHandle(ev.u.CreateProcessInfo.hThread);
            if (ev.u.CreateProcessInfo.hFile) CloseHandle(ev.u.CreateProcessInfo.hFile);
            if (failed) TerminateProcess(process->handle,99);
        } else if (ev.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
            CloseHandle(ev.u.CreateThread.hThread);
        } else if (ev.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            if (ev.u.LoadDll.hFile) CloseHandle(ev.u.LoadDll.hFile);
        } else if (ev.dwDebugEventCode==EXCEPTION_DEBUG_EVENT && !failed) {
            if (!process) return 6;
            code=ev.u.Exception.ExceptionRecord.ExceptionCode;
            if (code==EXCEPTION_BREAKPOINT && !process->loader_break) {
                process->loader_break=1;
            } else {
                sprintf(line,"exception %08lx rva %08lx base %08lx first %lu\n",
                        code,(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress-process->base,
                        process->base,ev.u.Exception.dwFirstChance);
                WriteFile(log,line,strlen(line),&written,0);
                failed=1;
                for (process=processes;process;process=process->next)
                    TerminateProcess(process->handle,99);
            }
        } else if (ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            if (ev.dwProcessId==root_id && !failed) {
                sprintf(line,"exit %lu\n",ev.u.ExitProcess.dwExitCode);
                WriteFile(log,line,strlen(line),&written,0);
            }
            for (link=&processes;*link && (*link)->id!=ev.dwProcessId;link=&(*link)->next) {}
            if (*link) {
                process=*link;
                *link=process->next;
                CloseHandle(process->handle);
                free(process);
            }
        }
        ContinueDebugEvent(ev.dwProcessId,ev.dwThreadId,status);
    } while (!root_seen || processes);
    CloseHandle(out);
    CloseHandle(err);
    CloseHandle(log);
    return 0;
}
