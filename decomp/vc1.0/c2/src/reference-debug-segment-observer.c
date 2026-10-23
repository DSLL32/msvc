/* Small debugger used only by the differential harness. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <recon.h>

/* Read-only target storage after the original CodeView signature write. */
static DWORD read_word(HANDLE process,DWORD address)
{
    DWORD value,got;
    if(!ReadProcessMemory(process,(void*)address,&value,4,&got)||got!=4)exit(241);
    return value;
}
static void capture_segment(HANDLE process,HANDLE thread,DWORD base,DWORD address)
{
    DWORD segment,bytes,got; HANDLE log;
    char path[1024],line[1024],name[300],class_name[300];
    if(address!=base+0x59c60)exit(242);
    segment=read_word(process,base+0x6914c);
    if(!segment)exit(243);
    if(!ReadProcessMemory(process,(void*)read_word(process,segment+0x10),name,300,&got))exit(244);
    if(!ReadProcessMemory(process,(void*)read_word(process,segment+0x28),class_name,300,&got))exit(245);
    if(!GetEnvironmentVariableA("C2_SEGMENT_CAPTURE",path,sizeof(path)))exit(246);
    log=CreateFileA(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    sprintf(line,"offset %lu\nkind %lu\nidentifier %lu\nalignment %lu\nformat_word_section %lu\nobject_declared %lu\nidentity %lu\nname %s\nclass %s\n",
        read_word(process,segment+4),read_word(process,segment+0xc)&255,
        read_word(process,segment+0x1c),read_word(process,segment+0x30),
        read_word(process,segment+0x34)&0xffffff,read_word(process,segment+0x38)!=0,
        read_word(process,segment+0x40),name,class_name);
    if(!WriteFile(log,line,strlen(line),&bytes,0)||bytes!=strlen(line))exit(247);
    CloseHandle(log);TerminateProcess(process,99);
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
    si.hStdOutput = out;
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
                capture_segment(pi.hProcess,pi.hThread,base,(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress);
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
