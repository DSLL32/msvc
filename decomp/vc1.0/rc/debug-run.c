/* Small debugger used only by the differential harness. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Analysis-only snapshot of RC's current in-memory resource buffer. */
static void capture_payload(HANDLE process, DWORD base, char **arguments)
{
    DWORD address, size, received, written;
    HANDLE output;
    static BYTE bytes[65500];
    if (!ReadProcessMemory(process, (void *)(base + strtoul(arguments[1], 0, 16)),
                           &address, sizeof(address), &received) || received != sizeof(address))
        return;
    if (!ReadProcessMemory(process, (void *)(base + strtoul(arguments[2], 0, 16)),
                           &size, sizeof(size), &received) || received != sizeof(size))
        return;
    if (!address || size > sizeof(bytes))
        return;
    if (!ReadProcessMemory(process, (void *)address, bytes, size, &received) || received != size)
        return;
    output = CreateFileA(arguments[0], GENERIC_WRITE, FILE_SHARE_READ, 0,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (output == INVALID_HANDLE_VALUE)
        return;
    WriteFile(output, bytes, size, &written, 0);
    CloseHandle(output);
}

/* Optional analysis proof for precisely bounded verbose PID fields. */
static void capture_temporary_names(HANDLE process, DWORD base, char **argv, HANDLE metadata)
{
    DWORD pointer, done, written, index, which;
    char text[260], line[320];
    for (which = 0; which != 2; ++which) {
        if (!ReadProcessMemory(process, (void *)(base + strtoul(argv[9+which],0,16)),
                               &pointer, 4, &done) || done != 4) ExitProcess(20);
        memset(text,0,sizeof(text));
        for (index=0; index+1<sizeof(text); ++index) {
            if (!ReadProcessMemory(process,(void *)(pointer+index),text+index,1,&done) || done!=1) ExitProcess(20);
            if (!text[index]) break;
        }
        sprintf(line,"native_temporary_%s %s\n",which ? "output" : "input",text);
        WriteFile(metadata,line,strlen(line),&written,0);
    }
}

int main(int argc, char **argv)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DEBUG_EVENT ev;
    HANDLE out, err, log, metadata=0;
    CONTEXT context;
    DWORD registration=0; BYTE saved_registration=0, trap=0xcc; int registration_active=0;
    DWORD base = 0, written, status, code;
    char command[2048], line[1024];
    int loader_break = 0;

    if (argc != 6 && argc != 7 && argc != 10 && argc != 11) return 2;
    if (argc==11 && *argv[7] && !SetEnvironmentVariableA("PATH",argv[7])) return 20;
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
    si.hStdOutput = out;
    si.hStdError = err;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    sprintf(command, "\"%s\" %s", argv[1], argv[2]);
    if (!CreateProcessA(argv[1], command, 0, 0, TRUE,
                        DEBUG_ONLY_THIS_PROCESS, 0, argc >= 7 ? argv[6] : 0, &si, &pi)) {
        sprintf(line, "create-error %lu\n", GetLastError());
        WriteFile(log, line, strlen(line), &written, 0);
        return 3;
    }
    if(argc==11) {
        sprintf(line,"%s.process",argv[5]);
        metadata=CreateFileA(line,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);
        if(metadata==INVALID_HANDLE_VALUE) return 20;
        sprintf(line,"pid %lu tid %lu\n",pi.dwProcessId,pi.dwThreadId);
        WriteFile(metadata,line,strlen(line),&written,0);
    }
    for (;;) {
        if (!WaitForDebugEvent(&ev, 30000)) return 4;
        status = DBG_CONTINUE;
        if (ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)ev.u.CreateProcessInfo.lpBaseOfImage;
            if(argc==11) {
                registration=base+strtoul(argv[8],0,16);
                if(!ReadProcessMemory(pi.hProcess,(void *)registration,&saved_registration,1,&written)) return 20;
                registration_active=saved_registration!=trap;
                if(registration_active) {
                    if(!WriteProcessMemory(pi.hProcess,(void *)registration,&trap,1,&written)) return 20;
                    FlushInstructionCache(pi.hProcess,(void *)registration,1);
                }
            }
            if (ev.u.CreateProcessInfo.hFile)
                CloseHandle(ev.u.CreateProcessInfo.hFile);
        } else if (ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (ev.u.LoadDll.hFile) CloseHandle(ev.u.LoadDll.hFile);
        } else if (ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = ev.u.Exception.ExceptionRecord.ExceptionCode;
            if (code == EXCEPTION_BREAKPOINT && !loader_break) {
                loader_break = 1;
            } else if(code==EXCEPTION_BREAKPOINT && registration_active &&
                      (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress==registration) {
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_CONTROL|CONTEXT_INTEGER;
                if(ev.dwThreadId!=pi.dwThreadId || !GetThreadContext(pi.hThread,&context)) return 20;
                capture_temporary_names(pi.hProcess,base,argv,metadata);
                if(!WriteProcessMemory(pi.hProcess,(void *)registration,&saved_registration,1,&written)) return 20;
                FlushInstructionCache(pi.hProcess,(void *)registration,1);registration_active=0;
                context.Eip=registration;
                if(!SetThreadContext(pi.hThread,&context)) return 20;
            } else {
                sprintf(line, "exception %08lx rva %08lx base %08lx first %lu\n",
                        code, (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress - base,
                        base, ev.u.Exception.dwFirstChance);
                WriteFile(log, line, strlen(line), &written, 0);
                if (argc == 10)
                    capture_payload(pi.hProcess, base, argv + 7);
                TerminateProcess(pi.hProcess, 99);
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
    if(metadata)CloseHandle(metadata);
    CloseHandle(log);
    return 0;
}
