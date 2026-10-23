/* Small debugger used only by the differential harness. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <recon.h>

/* Observe the actual SetStdHandle import call and its process state. */
typedef LONG (WINAPI *QueryProcess)(HANDLE,ULONG,void *,ULONG,ULONG *);
typedef struct BasicInfo { LONG status; void *peb; ULONG affinity; LONG priority; ULONG pid,parent; } BasicInfo;
static DWORD inherited_stdout,call_return,written;
static HANDLE call_log;
static int call_stage;
static DWORD standard_output(HANDLE process)
{
    QueryProcess query;BasicInfo basic;ULONG length;DWORD parameters,value,got;
    query=(QueryProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"),"NtQueryInformationProcess");
    if(!query||query(process,0,&basic,sizeof(basic),&length)||length!=24)exit(226);
    if(!ReadProcessMemory(process,(char*)basic.peb+0x10,&parameters,4,&got)||got!=4)exit(227);
    if(!ReadProcessMemory(process,(void*)(parameters+0x1c),&value,4,&got)||got!=4)exit(228);
    return value;
}
static void capture_crt(HANDLE process,HANDLE thread,DWORD base,DWORD address)
{
    CONTEXT c;DWORD args[3],got,value,iat;char path[1024],line[512];
    BYTE body[6],trap=0xcc;HANDLE file;
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(!GetThreadContext(thread,&c))exit(211);
    if(!call_stage){
        if(address!=base+0x68816)exit(212);
        if(!GetEnvironmentVariableA("C2_CRT_CAPTURE",path,sizeof(path)))exit(213);
        call_log=CreateFileA(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
        if(!ReadProcessMemory(process,(void*)c.Esp,args,sizeof(args),&got)||got!=sizeof(args))exit(214);
        value=standard_output(process);if(value!=inherited_stdout)exit(229);
        sprintf(line,"std_handle %lu\nhandle %08lx\nbefore_std %08lx\nreturn_va %08lx\n",args[1],args[2],value,args[0]);WriteFile(call_log,line,strlen(line),&written,0);
        if(!GetEnvironmentVariableA("C2_CRT_IMAGE",path,sizeof(path)))exit(218);
        file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
        SetFilePointer(file,0x68216,0,FILE_BEGIN);
        if(!ReadFile(file,body,sizeof(body),&got,0)||got!=sizeof(body))exit(219);CloseHandle(file);
        if(body[0]!=0xff||body[1]!=0x25)exit(230);
        memcpy(&iat,body+2,4);iat+=base-0x10000;memcpy(body+2,&iat,4);
        if(!WriteProcessMemory(process,(void*)address,body,sizeof(body),&got)||got!=sizeof(body))exit(220);
        FlushInstructionCache(process,(void*)address,sizeof(body));call_return=args[0];
        if(!WriteProcessMemory(process,(void*)call_return,&trap,1,&got)||got!=1)exit(221);
        FlushInstructionCache(process,(void*)call_return,1);c.Eip=address;
        if(!SetThreadContext(thread,&c))exit(222);call_stage=1;
    }else{
        if(address!=call_return)exit(223);
        sprintf(line,"result %lu\nafter_std %08lx\n",c.Eax,standard_output(process));WriteFile(call_log,line,strlen(line),&written,0);
        CloseHandle(call_log);TerminateProcess(process,99);call_stage=2;
    }
}
int main(int argc, char **argv)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DEBUG_EVENT ev;
    HANDLE out, err, log;
    DWORD base = 0, written, status, code;
    char command[2048], line[256], environment[4096];
    DWORD environment_length;
    int loader_break = 0;

    if (argc != 6 && argc != 7) return 2;
    memset(&si, 0, sizeof(si));
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    out = CreateFileA(argv[3], GENERIC_WRITE, FILE_SHARE_READ, &sa,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    err = CreateFileA(argv[4], GENERIC_WRITE, FILE_SHARE_READ, &sa,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    log = CreateFileA(argv[5], GENERIC_WRITE, FILE_SHARE_READ, 0,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = out; inherited_stdout=(DWORD)out;
    si.hStdError = err;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    sprintf(command, "\"%s\" %s", argv[1], argv[2]);
    strcpy(environment, "MSC_CMD_FLAGS=");
    environment_length = GetEnvironmentVariableA("MSC_CMD_FLAGS", environment + 14, sizeof(environment) - 15);
    if (!environment_length || environment_length >= sizeof(environment)-15) return 5;
    environment[14 + environment_length + 1] = 0;
    if (!CreateProcessA(argv[1], command, 0, 0, TRUE,
                        DEBUG_ONLY_THIS_PROCESS, environment, argc == 7 ? argv[6] : 0, &si, &pi)) {
        sprintf(line, "create-error %lu\n", GetLastError());
        WriteFile(log, line, strlen(line), &written, 0);
        return 3;
    }
    for (;;) {
        if (!WaitForDebugEvent(&ev, 30000)) return 4;
        status = DBG_CONTINUE;
        if (ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)ev.u.CreateProcessInfo.lpBaseOfImage;
            if (ev.u.CreateProcessInfo.hFile)
                CloseHandle(ev.u.CreateProcessInfo.hFile);
        } else if (ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (ev.u.LoadDll.hFile) CloseHandle(ev.u.LoadDll.hFile);
        } else if (ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = ev.u.Exception.ExceptionRecord.ExceptionCode;
            if (code == EXCEPTION_BREAKPOINT && !loader_break) {
                loader_break = 1;
            } else {
                sprintf(line, "exception %08lx rva %08lx base %08lx first %lu\n",
                        code, (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress - base,
                        base, ev.u.Exception.dwFirstChance);
                WriteFile(log, line, strlen(line), &written, 0);
                if(code!=EXCEPTION_BREAKPOINT) return 224;
                capture_crt(pi.hProcess,pi.hThread,base,(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress);
            }
        } else if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            sprintf(line, "exit %lu\n", ev.u.ExitProcess.dwExitCode);
            WriteFile(log, line, strlen(line), &written, 0);
            ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, status);
            break;
        }
        ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, status);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(out);
    CloseHandle(err);
    CloseHandle(log);
    return 0;
}
