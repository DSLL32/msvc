/* Small debugger used only by the differential harness. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <recon.h>

/* Original-only freopen boundary observation, excluded from fixtures. */
static int crt_stage;
static DWORD crt_return, crt_stream, crt_written, crt_base, crt_close_return, inherited_stdout;
static unsigned char close_return_byte;
typedef LONG (WINAPI *QueryProcess)(HANDLE,ULONG,void *,ULONG,ULONG *);
typedef struct BasicInfo { LONG status; void *peb; ULONG affinity; LONG priority; ULONG pid,parent; } BasicInfo;
static HANDLE crt_log;
static void file_state(HANDLE process,char *phase)
{
    DWORD got,fields[8],parameters,standard,oshandle;unsigned char osflag;char line[768];
    QueryProcess query;BasicInfo basic;ULONG length;
    if(!ReadProcessMemory(process,(void*)crt_stream,fields,sizeof(fields),&got)||got!=sizeof(fields))exit(225);
    sprintf(line,"%s_flags %lu\n%s_count %ld\n%s_file %ld\n%s_buffers_null %lu\n%s_bufsize %ld\n",phase,fields[3],phase,(long)fields[1],phase,(long)fields[4],phase,!fields[0]&&!fields[2],phase,(long)fields[6]);
    WriteFile(crt_log,line,strlen(line),&crt_written,0);
    /* Layout independently checked by lib/recon/tests/peb-stdio-probe.c;
       bind the initial child slot to its actual inherited startup handle. */
    query=(QueryProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"),"NtQueryInformationProcess");
    if(!query||query(process,0,&basic,sizeof(basic),&length)||length!=24)exit(226);
    if(!ReadProcessMemory(process,(char*)basic.peb+0x10,&parameters,4,&got)||got!=4)exit(227);
    if(!ReadProcessMemory(process,(void*)(parameters+0x1c),&standard,4,&got)||got!=4)exit(228);
    if(!strcmp(phase,"before")&&standard!=inherited_stdout)exit(229);
    if(!ReadProcessMemory(process,(void*)(crt_base+0x767b8+4),&oshandle,4,&got)||got!=4)exit(230);
    if(!ReadProcessMemory(process,(void*)(crt_base+0x76778+1),&osflag,1,&got)||got!=1)exit(231);
    sprintf(line,"%s_stdhandle %08lx\n%s_oshandle %08lx\n%s_osflags %u\n",phase,standard,phase,oshandle,phase,osflag);
    WriteFile(crt_log,line,strlen(line),&crt_written,0);
}
static void capture_crt(HANDLE process,HANDLE thread,DWORD base,DWORD address)
{
    CONTEXT c;DWORD got,args[4],i;char path[1024],line[2200],left[1024],right[1024];
    unsigned char body[96],trap=0xcc,byte;HANDLE file;
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(!GetThreadContext(thread,&c))exit(211);
    if(!crt_stage){
        if(address!=base+0x61aa0)exit(212);
        if(!GetEnvironmentVariableA("C2_CRT_CAPTURE",path,sizeof(path)))exit(213);
        crt_log=CreateFileA(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
        if(!ReadProcessMemory(process,(void*)c.Esp,args,sizeof(args),&got)||got!=sizeof(args))exit(214);
        for(i=0;i<511;++i){if(!ReadProcessMemory(process,(void*)(args[1]+i),&byte,1,&got)||got!=1)exit(216);sprintf(left+i*2,"%02x",byte);if(!byte){++i;break;}}left[i*2]=0;
        for(i=0;i<511;++i){if(!ReadProcessMemory(process,(void*)(args[2]+i),&byte,1,&got)||got!=1)exit(217);sprintf(right+i*2,"%02x",byte);if(!byte){++i;break;}}right[i*2]=0;
        sprintf(line,"name %s\nmode %s\nstream_is_stdout %lu\nreturn_va %08lx\n",left,right,args[3]==base+0x761b8,args[0]);WriteFile(crt_log,line,strlen(line),&crt_written,0);
        crt_base=base;crt_stream=args[3];file_state(process,"before");
        if(!GetEnvironmentVariableA("C2_CRT_IMAGE",path,sizeof(path)))exit(218);
        file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
        SetFilePointer(file,0x614a0,0,FILE_BEGIN);
        if(!ReadFile(file,body,sizeof(body),&got,0)||got!=sizeof(body))exit(219);CloseHandle(file);
        if(!WriteProcessMemory(process,(void*)address,body,sizeof(body),&got)||got!=sizeof(body))exit(220);
        FlushInstructionCache(process,(void*)address,sizeof(body));crt_return=args[0];
        crt_close_return=base+0x61ab1;close_return_byte=body[0x11];
        if(!WriteProcessMemory(process,(void*)crt_close_return,&trap,1,&got)||got!=1)exit(232);
        FlushInstructionCache(process,(void*)crt_close_return,1);
        if(!WriteProcessMemory(process,(void*)crt_return,&trap,1,&got)||got!=1)exit(221);
        FlushInstructionCache(process,(void*)crt_return,1);c.Eip=address;
        if(!SetThreadContext(thread,&c))exit(222);crt_stage=1;
    }else if(address==crt_close_return){
        if(crt_stage!=1)exit(233);
        file_state(process,"after_close");
        if(!WriteProcessMemory(process,(void*)address,&close_return_byte,1,&got)||got!=1)exit(234);
        FlushInstructionCache(process,(void*)address,1);c.Eip=address;
        if(!SetThreadContext(thread,&c))exit(235);crt_stage=2;
    }else{
        if(address!=crt_return||crt_stage!=2)exit(223);
        sprintf(line,"return_is_stream %lu\n",c.Eax==crt_stream);WriteFile(crt_log,line,strlen(line),&crt_written,0);
        file_state(process,"after");CloseHandle(crt_log);TerminateProcess(process,99);crt_stage=2;
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
