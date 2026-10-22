/* Analysis-only debugger. Observe original functions, repair observation INT3s,
 * and retain native stdout/stderr/exit and child image paths without rewriting. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
typedef DWORD (WINAPI *FinalPath)(HANDLE, char *, DWORD, DWORD);
struct Point { DWORD rva; BYTE original; int active; };
static struct Point points[] = {{0x8110,0,0},{0xdb80,0,0},{0xed20,0,0},{0xed50,0,0},{0xeef0,0,0}};
static HANDLE proof;
static void note(char *format,...)
{
    char text[4096]; DWORD done; va_list ap;
    va_start(ap,format); vsprintf(text,format,ap); va_end(ap);
    WriteFile(proof,text,strlen(text),&done,0); FlushFileBuffers(proof);
}
static void require(BOOL ok,char *what)
{
    if(!ok) { note("failure=%s error=%lu\n",what,GetLastError()); ExitProcess(20); }
}
int main(int argc,char **argv)
{
    STARTUPINFOA si; PROCESS_INFORMATION pi; SECURITY_ATTRIBUTES sa;
    DEBUG_EVENT ev; CONTEXT cx; FinalPath final_path;
    HANDLE out,err,thread=0; DWORD base=0,done,status,code,address,i,j,words[5];
    DWORD loaders[32],loader_count=0; BYTE trap=0xcc; int initial;
    char command[2048],path[2048],text[2048];
    if(argc!=8) return 2;
    proof=CreateFileA(argv[5],GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);
    require(proof!=INVALID_HANDLE_VALUE,"proof");
    if(*argv[7]) require(SetEnvironmentVariableA("PATH",argv[7]),"controlled target PATH");
    GetEnvironmentVariableA("PATH",path,sizeof(path)); note("target_PATH=%s\n",path);
    final_path=(FinalPath)GetProcAddress(GetModuleHandleA("kernel32.dll"),"GetFinalPathNameByHandleA");
    memset(&sa,0,sizeof(sa));sa.nLength=sizeof(sa);sa.bInheritHandle=TRUE;
    out=CreateFileA(argv[3],GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,0,0);
    err=CreateFileA(argv[4],GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,0,0);
    require(out!=INVALID_HANDLE_VALUE && err!=INVALID_HANDLE_VALUE,"streams");
    memset(&si,0,sizeof(si));si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;
    si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);si.hStdOutput=out;si.hStdError=err;
    sprintf(command,"\"%s\" %s",argv[1],argv[2]);
    /* Debug children observationally, to prove the launched original RCPP path. */
    require(CreateProcessA(argv[1],command,0,0,TRUE,DEBUG_PROCESS,0,argv[6],&si,&pi),"CreateProcess");
    note("target_pid=%lu primary_tid=%lu\n",pi.dwProcessId,pi.dwThreadId);
    for(;;) {
        require(WaitForDebugEvent(&ev,30000),"WaitForDebugEvent");status=DBG_CONTINUE;
        if(ev.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            path[0]=0;
            if(final_path && ev.u.CreateProcessInfo.hFile)
                final_path(ev.u.CreateProcessInfo.hFile,path,sizeof(path),0);
            note("process_created pid=%lu base=%08lx image=%s\n",ev.dwProcessId,(DWORD)ev.u.CreateProcessInfo.lpBaseOfImage,path);
            if(ev.dwProcessId==pi.dwProcessId) {
                base=(DWORD)ev.u.CreateProcessInfo.lpBaseOfImage;
                thread=pi.hThread;
                for(i=0;i<sizeof(points)/sizeof(points[0]);++i) {
                    require(ReadProcessMemory(pi.hProcess,(void *)(base+points[i].rva),&points[i].original,1,&done),"read point");
                    require(points[i].original!=trap,"unmodified original point");
                    require(WriteProcessMemory(pi.hProcess,(void *)(base+points[i].rva),&trap,1,&done),"set point");points[i].active=1;
                    FlushInstructionCache(pi.hProcess,(void *)(base+points[i].rva),1);
                }
            }
            if(ev.u.CreateProcessInfo.hFile)CloseHandle(ev.u.CreateProcessInfo.hFile);
        } else if(ev.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            if(ev.u.LoadDll.hFile)CloseHandle(ev.u.LoadDll.hFile);
        } else if(ev.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            code=ev.u.Exception.ExceptionRecord.ExceptionCode;
            address=(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress;
            initial=1;for(i=0;i<loader_count;++i)if(loaders[i]==ev.dwProcessId)initial=0;
            if(code==EXCEPTION_BREAKPOINT && initial) {
                require(loader_count<32,"loader slots");loaders[loader_count++]=ev.dwProcessId;
            } else {
                for(i=0;i<sizeof(points)/sizeof(points[0]);++i)
                    if(ev.dwProcessId==pi.dwProcessId && points[i].active && address==base+points[i].rva)break;
                if(code==EXCEPTION_BREAKPOINT && i<sizeof(points)/sizeof(points[0])) {
                    require(ev.dwThreadId==pi.dwThreadId,"observed primary function");
                    memset(&cx,0,sizeof(cx));cx.ContextFlags=CONTEXT_CONTROL|CONTEXT_INTEGER;
                    require(GetThreadContext(thread,&cx),"point context");
                    require(ReadProcessMemory(pi.hProcess,(void *)cx.Esp,words,sizeof(words),&done),"point stack");
                    note("function_rva=%08lx caller=%08lx args=%08lx,%08lx,%08lx,%08lx\n",points[i].rva,words[0],words[1],words[2],words[3],words[4]);
                    if(points[i].rva==0x8110 || points[i].rva==0xed20) {
                        DWORD pointer;
                        require(ReadProcessMemory(pi.hProcess,(void *)(base+0x1b024),&pointer,4,&done),"native temporary input pointer");
                        memset(text,0,sizeof(text));
                        for(j=0;j+1<sizeof(text);++j) {
                            require(ReadProcessMemory(pi.hProcess,(void *)(pointer+j),text+j,1,&done),"native temporary input");
                            if(!text[j])break;
                        }
                        note("native_temporary_input=%s\n",text);
                        require(ReadProcessMemory(pi.hProcess,(void *)(base+0x1b028),&pointer,4,&done),"native temporary output pointer");
                        memset(text,0,sizeof(text));
                        for(j=0;j+1<sizeof(text);++j) {
                            require(ReadProcessMemory(pi.hProcess,(void *)(pointer+j),text+j,1,&done),"native temporary output");
                            if(!text[j])break;
                        }
                        note("native_temporary_output=%s\n",text);
                    }
                    if(points[i].rva==0xeef0) {
                        memset(text,0,sizeof(text));
                        for(j=0;j+1<sizeof(text);++j) {
                            require(ReadProcessMemory(pi.hProcess,(void *)(words[1]+j),text+j,1,&done),"PATH parser input");
                            if(!text[j])break;
                        }
                        note("PATH_parser_input=%s\n",text);
                    }
                    require(WriteProcessMemory(pi.hProcess,(void *)address,&points[i].original,1,&done),"restore point");
                    FlushInstructionCache(pi.hProcess,(void *)address,1);points[i].active=0;
                    cx.Eip=address;require(SetThreadContext(thread,&cx),"repair point EIP");
                } else {
                    note("exception pid=%lu code=%08lx address=%08lx first=%lu disposition=not_handled\n",ev.dwProcessId,code,address,ev.u.Exception.dwFirstChance);
                    status=DBG_EXCEPTION_NOT_HANDLED;
                }
            }
        } else if(ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            note("exit pid=%lu status=%lu\n",ev.dwProcessId,ev.u.ExitProcess.dwExitCode);
            ContinueDebugEvent(ev.dwProcessId,ev.dwThreadId,status);
            if(ev.dwProcessId==pi.dwProcessId)break;
            continue;
        }
        ContinueDebugEvent(ev.dwProcessId,ev.dwThreadId,status);
    }
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);CloseHandle(out);CloseHandle(err);CloseHandle(proof);
    return 0;
}
