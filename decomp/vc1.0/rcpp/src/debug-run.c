/* Explicit original-only capture debugger with optional first-entry plan. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <recon.h>

typedef struct { DWORD rva; unsigned int byte; char name[128]; } ENTRY;
static ENTRY entries[256];

int main(int argc, char **argv)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DEBUG_EVENT ev;
    HANDLE out, err, log;
    DWORD base = 0, written, status, code;
    char command[2048], line[256];
    int loader_break = 0, entry_count = 0, next_entry = 0, i;
    FILE *plan;
    DWORD transferred;
    CONTEXT context;
    BYTE trap = 0xcc, saved;

    if (argc < 6 || argc > 8) return 2;
    if (argc == 8) {
        plan = fopen(argv[7], "r");
        if (!plan) return 5;
        while (entry_count < 256 && fscanf(plan, "%lx %x %127s",
               &entries[entry_count].rva, &entries[entry_count].byte,
               entries[entry_count].name) == 3) ++entry_count;
        fclose(plan);
    }
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
                for (i = 0; i < entry_count; ++i)
                    if (!WriteProcessMemory(pi.hProcess, (void *)(base + entries[i].rva),
                                            &trap, 1, &transferred)) return 6;
                FlushInstructionCache(pi.hProcess, 0, 0);
            } else if (code == EXCEPTION_BREAKPOINT && next_entry < entry_count &&
                       (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress == base + entries[next_entry].rva) {
                saved = (BYTE)entries[next_entry].byte;
                WriteProcessMemory(pi.hProcess, (void *)(base + entries[next_entry].rva),
                                   &saved, 1, &transferred);
                FlushInstructionCache(pi.hProcess, 0, 0);
                memset(&context, 0, sizeof(context));
                context.ContextFlags = CONTEXT_CONTROL;
                if (!GetThreadContext(pi.hThread, &context)) return 7;
                context.Eip = base + entries[next_entry].rva;
                if (!SetThreadContext(pi.hThread, &context)) return 8;
                sprintf(line, "entry %s\n", entries[next_entry].name);
                WriteFile(log, line, strlen(line), &written, 0);
                ++next_entry;
            } else {
                sprintf(line, "exception %08lx rva %08lx base %08lx first %lu\n",
                        code, (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress - base,
                        base, ev.u.Exception.dwFirstChance);
                WriteFile(log, line, strlen(line), &written, 0);
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
    CloseHandle(log);
    return 0;
}
