/* Explicit original-only reference capture. Never used by native fixtures. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

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

static char capture_root[1024], work_root[1024];
static DWORD capture_count, spawn_address, return_address, saved_out, saved_err;
static BYTE return_byte;
static FILE *record;
typedef struct AppEntry { DWORD rva; BYTE byte; int active; } AppEntry;
static AppEntry app_entries[128];
static unsigned app_count;

static void copy_tree(char *source, char *destination)
{
    WIN32_FIND_DATAA info;
    HANDLE search;
    char pattern[2048], from[2048], to[2048];
    CreateDirectoryA(destination, NULL);
    sprintf(pattern, "%s\\*", source);
    search = FindFirstFileA(pattern, &info);
    if (search == INVALID_HANDLE_VALUE) return;
    do {
        if (!strcmp(info.cFileName, ".") || !strcmp(info.cFileName, "..")) continue;
        sprintf(from, "%s\\%s", source, info.cFileName);
        sprintf(to, "%s\\%s", destination, info.cFileName);
        if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) copy_tree(from, to);
        else if (!CopyFileA(from, to, FALSE)) ExitProcess(130);
    } while (FindNextFileA(search, &info));
    FindClose(search);
}
static void capture_string(HANDLE process, DWORD address)
{
    char c; DWORD amount;
    if (!address) { fputc(0, record); return; }
    do {
        if (!ReadProcessMemory(process, (void *)address++, &c, 1, &amount)) ExitProcess(131);
        fputc(c, record);
    } while (c);
}
static void capture_vector(HANDLE process, DWORD address)
{
    DWORD item, amount;
    if (address) for (;;) {
        if (!ReadProcessMemory(process, (void *)address, &item, 4, &amount)) ExitProcess(131);
        if (!item) break;
        capture_string(process, item); address += 4;
    }
    fputc(0, record);
}
static void snapshot(char *stage)
{
    char destination[2048];
    sprintf(destination, "%s\\%03lu-%s", capture_root, capture_count, stage);
    copy_tree(work_root, destination);
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
    unsigned app_index;
    int mode = 0, signal_sent = 0;
    HANDLE input_read, input_write;
    BYTE trap = 0xcc;
    char command[4096], ready[1024], release[1024];
    FILE *file;
    if (argc != 10) return 120;
    strcpy(capture_root, argv[9]); strcpy(work_root, argv[6]);
    if (argc >= 9) {
        if (!strcmp(argv[8], "handler-idle")) mode = 1;
        else if (!strcmp(argv[8], "handler-child")) mode = 2;
        else if (!strcmp(argv[8], "clock")) mode = 3;
        else if (!strcmp(argv[8], "omf-state")) mode = 4;
        else if (!strcmp(argv[8], "ide-null-buffer")) mode = 5;
        else if (strcmp(argv[8], "ordinary")) return 120;
    }
    file = fopen(argv[7], "r");
    if (!file) return 121;
    if (fscanf(file, "%lx %lx %lx %lx %lx %lx", &handler_rva, &running_rva, &link_rva, &coff_rva, &ide_rva, &callbacks_rva) != 6) return 121;
    fscanf(file, "%u %u %u %u", &clock_year, &clock_month, &clock_day, &clock_hour);
    if (fscanf(file, "%u", &app_count) != 1 || app_count > 128) return 134;
    for (app_index = 0; app_index < app_count; ++app_index) {
        if (fscanf(file, "%lx", &app_entries[app_index].rva) != 1) return 134;
        app_entries[app_index].active = 1;
    }
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
    SetEnvironmentVariableA("TMP", ".");
    SetEnvironmentVariableA("TEMP", ".");
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
            for (app_index = 0; app_index < app_count; ++app_index) {
                AppEntry *app = &app_entries[app_index];
                address = base + app->rva;
                if (!ReadProcessMemory(child.hProcess, (void *)address, &app->byte, 1, &amount)) return 135;
                if (app->byte == 0xcc) {
                    if (link_hook && address == base + (mode == 4 ? link_rva : ide_rva)) app->byte = link_byte;
                    else return 135;
                }
                if (!WriteProcessMemory(child.hProcess, (void *)address, &trap, 1, &amount)) return 135;
            }
            spawn_address = base + 0xfc20;
            { BYTE first;
              ReadProcessMemory(child.hProcess, (void *)spawn_address, &first, 1, &amount);
              if (first != 0x53) return 132;
              WriteProcessMemory(child.hProcess, (void *)spawn_address, &trap, 1, &amount);
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
            for (app_index = 0; app_index < app_count; ++app_index)
                if (app_entries[app_index].active && address == base + app_entries[app_index].rva) break;
            if (code == EXCEPTION_BREAKPOINT && app_index < app_count) {
                AppEntry *app = &app_entries[app_index];
                if (!GetThreadContext(threads[t].handle, &context)) return 127;
                log_event("application-entry", app->rva);
                if (link_hook && address == base + (mode == 4 ? link_rva : ide_rva)) {
                    if (mode == 4) WriteProcessMemory(child.hProcess, (void *)(base + coff_rva), &zero, sizeof(zero), &amount);
                    else {
                        ReadProcessMemory(child.hProcess, (void *)(base + callbacks_rva), &running, sizeof(running), &amount);
                        WriteProcessMemory(child.hProcess, (void *)(running + 12), &zero, sizeof(zero), &amount);
                    }
                    link_hook = 0;
                    log_event(mode == 4 ? "internal-omf-state" : "internal-ide-null-buffer", 0);
                }
                WriteProcessMemory(child.hProcess, (void *)address, &app->byte, 1, &amount);
                FlushInstructionCache(child.hProcess, (void *)address, 1);
                context.Eip = address; app->active = 0;
                if (!SetThreadContext(threads[t].handle, &context)) return 128;
            } else if (code == EXCEPTION_BREAKPOINT && address == spawn_address) {
                DWORD args[5]; char name[2048];
                if (!GetThreadContext(threads[t].handle, &context)) return 127;
                ReadProcessMemory(child.hProcess, (void *)context.Esp, args, sizeof(args), &amount);
                return_address = args[0];
                ReadProcessMemory(child.hProcess, (void *)return_address, &return_byte, 1, &amount);
                WriteProcessMemory(child.hProcess, (void *)return_address, &trap, 1, &amount);
                ++capture_count; snapshot("before");
                saved_out = GetFileSize(startup.hStdOutput, NULL);
                saved_err = GetFileSize(startup.hStdError, NULL);
                sprintf(name, "%s\\%03lu-request.bin", capture_root, capture_count);
                record = fopen(name, "wb");
                if (!record) return 133;
                fwrite(&args[1], 4, 1, record);
                fwrite(&args[4], 4, 1, record);
                capture_string(child.hProcess, args[2]);
                capture_vector(child.hProcess, args[3]);
                capture_vector(child.hProcess, args[4]);
                fclose(record);
                /* Emulate the original one-byte PUSH EBX; preserve its entry trap. */
                context.Esp -= 4;
                WriteProcessMemory(child.hProcess, (void *)context.Esp, &context.Ebx, 4, &amount);
                context.Eip = spawn_address + 1;
                if (!SetThreadContext(threads[t].handle, &context)) return 128;
                FlushInstructionCache(child.hProcess, NULL, 0);
            } else if (code == EXCEPTION_BREAKPOINT && return_address && address == return_address) {
                char name[2048]; DWORD end_out, end_err, child_errno;
                if (!GetThreadContext(threads[t].handle, &context)) return 127;
                snapshot("after");
                end_out = GetFileSize(startup.hStdOutput, NULL);
                end_err = GetFileSize(startup.hStdError, NULL);
                sprintf(name, "%s\\%03lu-result.txt", capture_root, capture_count);
                record = fopen(name, "w");
                if (!record) return 133;
                ReadProcessMemory(child.hProcess, (void *)(base + 0x1847c), &child_errno, 4, &amount);
                fprintf(record, "%ld %lu %lu %lu %lu %lu\n", (long)context.Eax,
                        saved_out, end_out, saved_err, end_err, child_errno);
                fclose(record);
                WriteProcessMemory(child.hProcess, (void *)return_address, &return_byte, 1, &amount);
                context.Eip = return_address; return_address = 0;
                if (!SetThreadContext(threads[t].handle, &context)) return 128;
                FlushInstructionCache(child.hProcess, NULL, 0);
            } else if (code == EXCEPTION_BREAKPOINT && clock_hook && address == clock_address) {
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
