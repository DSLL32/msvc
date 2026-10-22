/* One-shot entry breakpoints for case discovery in the unmodified original. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Site { DWORD rva; BYTE original; int hit; } Site;
typedef struct Thread { DWORD id; HANDLE handle; } Thread;
static Site sites[256];
static Thread threads[128];
static HANDLE events;

static void log_event(char *kind, DWORD value)
{
    char line[96]; DWORD length;
    sprintf(line, "%s %08lx\n", kind, value);
    WriteFile(events, line, strlen(line), &length, NULL);
}

static DWORD dispatch_handler(HANDLE process, DWORD base)
{
    DWORD id;
    HANDLE thread = CreateRemoteThread(process, NULL, 0,
        (LPTHREAD_START_ROUTINE)(base + 0xafcc), (void *)CTRL_BREAK_EVENT, 0, &id);
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
    int count = 0, thread_count = 0, i, t, loader_break = 0;
    int mode = 0, signal_sent = 0;
    HANDLE input_read, input_write;
    BYTE trap = 0xcc;
    char command[4096], ready[1024], release[1024];
    FILE *file;
    if (argc != 8 && argc != 9) return 120;
    if (argc == 9) {
        if (!strcmp(argv[8], "handler-idle")) mode = 1;
        else if (!strcmp(argv[8], "handler-child")) mode = 2;
        else return 120;
    }
    file = fopen(argv[7], "r");
    if (!file) return 121;
    while (count < 256 && fscanf(file, "%lx", &sites[count].rva) == 1) ++count;
    fclose(file);
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
            for (i = 0; i < count; ++i) {
                address = base + sites[i].rva;
                if (!ReadProcessMemory(child.hProcess, (void *)address, &sites[i].original, 1, &amount) ||
                    !WriteProcessMemory(child.hProcess, (void *)address, &trap, 1, &amount)) return 124;
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
            for (i = 0; i < count; ++i) if (base + sites[i].rva == address) break;
            if (code == EXCEPTION_BREAKPOINT && i < count && !sites[i].hit) {
                log_event("hit", sites[i].rva);
                if (sites[i].rva == 0xafcc) {
                    ReadProcessMemory(child.hProcess, (void *)(base + 0x140d4), &running, sizeof(running), &amount);
                    log_event("child-running", running);
                }
                sites[i].hit = 1;
                for (t = 0; t < thread_count; ++t) if (threads[t].id == event.dwThreadId) break;
                if (t == thread_count) return 126;
                memset(&context, 0, sizeof(context));
                context.ContextFlags = CONTEXT_FULL;
                if (!GetThreadContext(threads[t].handle, &context)) return 127;
                WriteProcessMemory(child.hProcess, (void *)address, &sites[i].original, 1, &amount);
                FlushInstructionCache(child.hProcess, (void *)address, 1);
                context.Eip = address;
                if (!SetThreadContext(threads[t].handle, &context)) return 128;
            } else if (code == EXCEPTION_BREAKPOINT && !loader_break) loader_break = 1;
            else {
                log_event("exception", code);
                status = DBG_EXCEPTION_NOT_HANDLED;
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            log_event("exit", event.u.ExitProcess.dwExitCode);
            ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status);
            return 0;
        }
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status);
    }
}
