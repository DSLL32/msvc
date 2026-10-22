/* Paired frontier debugger; optional deterministic system clock, callback
   dispatch, or an explicitly documented legacy-state injection. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Thread { DWORD id; HANDLE handle; } Thread;
static Thread threads[128];
static HANDLE events;

static void log_event(char *kind, DWORD value)
{
    char line[96]; DWORD length;
    sprintf(line, "%s %08lx\n", kind, value);
    WriteFile(events, line, strlen(line), &length, NULL);
}

static DWORD handler_rva, running_rva, link_rva, coff_rva, ide_rva, callbacks_rva;

static DWORD dispatch_handler(HANDLE process, DWORD base)
{
    DWORD id;
    HANDLE thread = CreateRemoteThread(process, NULL, 0,
        (LPTHREAD_START_ROUTINE)(base + handler_rva), (void *)CTRL_BREAK_EVENT, 0, &id);
    if (!thread) { log_event("dispatch-error", GetLastError()); return 0; }
    CloseHandle(thread);
    return id;
}

static int help_is_waiting(char *name)
{
    char text[8192]; DWORD length;
    HANDLE file = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    if (!ReadFile(file, text, sizeof(text) - 1, &length, NULL)) length = 0;
    text[length] = 0;
    CloseHandle(file);
    return strstr(text, "(press <return> to continue)") != NULL;
}

int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION child;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;
    CONTEXT context;
    DWORD base = 0, amount, status, address, code, start, handler_thread = 0, running;
    DWORD clock_address = 0, clock_count = 0, stack[2], zero = 0;
    BYTE link_byte = 0, clock_byte = 0;
    int link_hook = 0, clock_hook = 0;
    SYSTEMTIME fixed_time;
    unsigned clock_year = 2026, clock_month = 1, clock_day = 15, clock_hour = 0;
    int thread_count = 0, t, loader_break = 0;
    int mode = 0, signal_sent = 0;
    HANDLE input_read, input_write;
    BYTE trap = 0xcc;
    char command[4096], ready[1024], release[1024];
    FILE *file;
    if (argc != 8 && argc != 9) return 120;
    if (argc == 9) {
        if (!strcmp(argv[8], "handler-idle")) mode = 1;
        else if (!strcmp(argv[8], "handler-child")) mode = 2;
        else if (!strcmp(argv[8], "clock")) mode = 3;
        else if (!strcmp(argv[8], "omf-state")) mode = 4;
        else if (!strcmp(argv[8], "ide-null-buffer")) mode = 5;
        else return 120;
    }
    file = fopen(argv[7], "r");
    if (!file) return 121;
    if (fscanf(file, "%lx %lx %lx %lx %lx %lx", &handler_rva, &running_rva, &link_rva, &coff_rva, &ide_rva, &callbacks_rva) != 6) return 121;
    fscanf(file, "%u %u %u %u", &clock_year, &clock_month, &clock_day, &clock_hour);
    fclose(file);
    if (mode == 3) clock_address = (DWORD)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetLocalTime");
    events = CreateFileA(argv[5], GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    memset(&startup, 0, sizeof(startup));
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = CreateFileA(argv[3], GENERIC_WRITE, FILE_SHARE_READ, &security, CREATE_ALWAYS, 0, NULL);
    startup.hStdError = CreateFileA(argv[4], GENERIC_WRITE, FILE_SHARE_READ, &security, CREATE_ALWAYS, 0, NULL);
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    if (mode == 1) {
        if (!CreatePipe(&input_read, &input_write, &security, 0)) return 129;
        startup.hStdInput = input_read;
    }
    sprintf(command, "\"%s\" %s", argv[1], argv[2]);
    if (!CreateProcessA(argv[1], command, NULL, NULL, TRUE, DEBUG_ONLY_THIS_PROCESS, NULL, argv[6], &startup, &child)) return 122;
    sprintf(ready, "%s\\child-ready", argv[6]);
    sprintf(release, "%s\\release-pass", argv[6]);
    start = GetTickCount();
    for (;;) {
        if (!WaitForDebugEvent(&event, 250)) {
            if (!signal_sent && ((mode == 1 && help_is_waiting(argv[3])) ||
                (mode == 2 && GetFileAttributesA(ready) != 0xffffffffUL))) {
                ReadProcessMemory(child.hProcess, (void *)(base + running_rva), &running, sizeof(running), &amount);
                log_event("child-running", running);
                handler_thread = dispatch_handler(child.hProcess, base);
                signal_sent = 1;
            }
            if (GetTickCount() - start > 60000) {
                TerminateProcess(child.hProcess, 123); log_event("timeout", 123); return 123;
            }
            continue;
        }
        status = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            threads[thread_count].id = event.dwThreadId;
            threads[thread_count++].handle = event.u.CreateProcessInfo.hThread;
            if (mode == 4 || mode == 5) {
                address = base + (mode == 4 ? link_rva : ide_rva);
                if (!ReadProcessMemory(child.hProcess, (void *)address, &link_byte, 1, &amount)) return 124;
                /* An unaccepted link_pass must remain an ordinary frontier. */
                if (link_byte != 0xcc) {
                    if (!WriteProcessMemory(child.hProcess, (void *)address, &trap, 1, &amount)) return 124;
                    link_hook = 1;
                }
            }
            FlushInstructionCache(child.hProcess, NULL, 0);
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            if (thread_count == 128) return 125;
            threads[thread_count].id = event.dwThreadId;
            threads[thread_count++].handle = event.u.CreateThread.hThread;
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT) {
            if (handler_thread && event.dwThreadId == handler_thread) {
                HANDLE released = CreateFileA(release, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
                if (released != INVALID_HANDLE_VALUE) CloseHandle(released);
            }
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = event.u.Exception.ExceptionRecord.ExceptionCode;
            address = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            for (t = 0; t < thread_count; ++t) if (threads[t].id == event.dwThreadId) break;
            if (t == thread_count) return 126;
            memset(&context, 0, sizeof(context));
            context.ContextFlags = CONTEXT_FULL;
            if (code == EXCEPTION_BREAKPOINT && clock_hook && address == clock_address) {
                if (!GetThreadContext(threads[t].handle, &context)) return 127;
                if (!ReadProcessMemory(child.hProcess, (void *)context.Esp, stack, sizeof(stack), &amount)) return 127;
                memset(&fixed_time, 0, sizeof(fixed_time));
                fixed_time.wYear = (WORD)clock_year; fixed_time.wMonth = (WORD)clock_month;
                fixed_time.wDay = (WORD)clock_day; fixed_time.wHour = (WORD)clock_hour;
                fixed_time.wSecond = (WORD)((900 + 1225 * clock_count) / 1000);
                fixed_time.wMilliseconds = (WORD)((900 + 1225 * clock_count) % 1000);
                ++clock_count;
                WriteProcessMemory(child.hProcess, (void *)stack[1], &fixed_time, sizeof(fixed_time), &amount);
                context.Eip = stack[0]; context.Esp += 8;
                if (!SetThreadContext(threads[t].handle, &context)) return 128;
                log_event("clock-call", clock_count);
            } else if (code == EXCEPTION_BREAKPOINT && link_hook && address == base + (mode == 4 ? link_rva : ide_rva)) {
                if (!GetThreadContext(threads[t].handle, &context)) return 127;
                if (mode == 4) WriteProcessMemory(child.hProcess, (void *)(base + coff_rva), &zero, sizeof(zero), &amount);
                else {
                    ReadProcessMemory(child.hProcess, (void *)(base + callbacks_rva), &running, sizeof(running), &amount);
                    WriteProcessMemory(child.hProcess, (void *)(running + 12), &zero, sizeof(zero), &amount);
                }
                WriteProcessMemory(child.hProcess, (void *)address, &link_byte, 1, &amount);
                FlushInstructionCache(child.hProcess, (void *)address, 1);
                context.Eip = address;
                if (!SetThreadContext(threads[t].handle, &context)) return 128;
                link_hook = 0;
                log_event(mode == 4 ? "internal-omf-state" : "internal-ide-null-buffer", 0);
            } else if (code == EXCEPTION_BREAKPOINT && !loader_break) loader_break = 1;
            else {
                char line[256];
                sprintf(line, "exception %08lx rva %08lx base %08lx first %lu\n", code,
                        address - base, base, event.u.Exception.dwFirstChance);
                WriteFile(events, line, strlen(line), &amount, NULL);
                TerminateProcess(child.hProcess, 99);
                if (mode == 2) {
                    HANDLE released = CreateFileA(release, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
                    if (released != INVALID_HANDLE_VALUE) CloseHandle(released);
                    Sleep(200);
                }
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            log_event("exit", event.u.ExitProcess.dwExitCode);
            ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status);
            return 0;
        }
        if (clock_address && !clock_hook &&
            ReadProcessMemory(child.hProcess, (void *)clock_address, &clock_byte, 1, &amount)) {
            if (!WriteProcessMemory(child.hProcess, (void *)clock_address, &trap, 1, &amount)) return 124;
            FlushInstructionCache(child.hProcess, (void *)clock_address, 1);
            clock_hook = 1;
        }
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status);
    }
}
