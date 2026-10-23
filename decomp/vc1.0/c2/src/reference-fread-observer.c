/* Small debugger used only by the differential harness. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <recon.h>


/* Explicit original-only fread seam observer; never linked into C2 or fixtures.
   Restore the indexed original library body, then stop at its caller return. */
static int fread_stage;
static DWORD fread_return, fread_destination, fread_written;
static HANDLE fread_log;
static void capture_fread(HANDLE process, HANDLE thread, DWORD base, DWORD address)
{
    CONTEXT c;
    DWORD got, args[5], i;
    char path[1024], line[1024], hex[25];
    unsigned char body[352], bytes[12], trap = 0xcc;
    HANDLE file;
    memset(&c, 0, sizeof(c)); c.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &c)) exit(211);
    if (!fread_stage) {
        if (address != base + 0x62fa0) exit(212);
        if (!GetEnvironmentVariableA("C2_CRT_CAPTURE", path, sizeof(path))) exit(213);
        fread_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (!ReadProcessMemory(process, (void *)c.Esp, args, sizeof(args), &got) || got != sizeof(args)) exit(214);
        if (args[2] != 12 || args[3] != 1) exit(215);
        fread_destination = args[1]; fread_return = args[0];
        sprintf(line, "size %lu\ncount %lu\nreturn_va %08lx\n", args[2], args[3], args[0]);
        WriteFile(fread_log, line, strlen(line), &fread_written, 0);
        if (!GetEnvironmentVariableA("C2_CRT_IMAGE", path, sizeof(path))) exit(218);
        file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        SetFilePointer(file, 0x629a0, 0, FILE_BEGIN);
        if (!ReadFile(file, body, sizeof(body), &got, 0) || got != sizeof(body)) exit(219);
        CloseHandle(file);
        if (!WriteProcessMemory(process, (void *)address, body, sizeof(body), &got) || got != sizeof(body)) exit(220);
        FlushInstructionCache(process, (void *)address, sizeof(body));
        if (!WriteProcessMemory(process, (void *)fread_return, &trap, 1, &got) || got != 1) exit(221);
        FlushInstructionCache(process, (void *)fread_return, 1); c.Eip = address;
        if (!SetThreadContext(thread, &c)) exit(222);
        fread_stage = 1;
    } else {
        if (address != fread_return) exit(223);
        if (!ReadProcessMemory(process, (void *)fread_destination, bytes, sizeof(bytes), &got) || got != sizeof(bytes)) exit(224);
        for (i = 0; i < sizeof(bytes); ++i) sprintf(hex + 2*i, "%02x", bytes[i]);
        sprintf(line, "result %lu\nbytes %s\n", c.Eax, hex);
        WriteFile(fread_log, line, strlen(line), &fread_written, 0);
        CloseHandle(fread_log); TerminateProcess(process, 99); fread_stage = 2;
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
                if (code != EXCEPTION_BREAKPOINT) return 225;
                capture_fread(pi.hProcess, pi.hThread, base, (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress);
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
