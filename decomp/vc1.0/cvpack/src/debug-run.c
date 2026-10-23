/* Original-reference capture only. Never linked into the reconstruction. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <recon.h>

typedef struct RuntimeRange {
    DWORD start, end, offset;
} RuntimeRange;

static RuntimeRange ranges[512];
static unsigned range_count;
static unsigned char original_bytes[65536];
static DWORD watched_spawn, spawn_return;
static unsigned char spawn_return_byte;

static int watch_spawn(HANDLE child, HANDLE thread, HANDLE log)
{
    CONTEXT context;
    DWORD arguments[24], bytes, written, i, j;
    unsigned char byte, trap = 0xcc;
    char line[2048];
    int length;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context) ||
        !ReadProcessMemory(child, (void *)context.Esp, arguments, sizeof(arguments), &bytes) ||
        bytes != sizeof(arguments)) return 0;
    sprintf(line, "spawn-mode %lu\n", arguments[1]);
    WriteFile(log, line, strlen(line), &written, 0);
    for (i = 2; i < 24 && arguments[i]; ++i) {
        length = sprintf(line, "spawn-string %lu ", i-2);
        for (j = 0; j < 512; ++j) {
            if (!ReadProcessMemory(child, (void *)(arguments[i]+j), &byte, 1, &bytes) || bytes != 1)
                return 0;
            if (!byte) break;
            length += sprintf(line+length, "%02x", byte);
        }
        if (j == 512) return 0;
        line[length++] = '\n';
        WriteFile(log, line, length, &written, 0);
    }
    if (i == 24) return 0;
    spawn_return = arguments[0];
    if (!ReadProcessMemory(child, (void *)spawn_return, &spawn_return_byte, 1, &bytes) || bytes != 1 ||
        !WriteProcessMemory(child, (void *)spawn_return, &trap, 1, &bytes) || bytes != 1)
        return 0;
    return 1;
}

static int finish_spawn(HANDLE child, HANDLE thread, HANDLE log)
{
    CONTEXT context;
    DWORD old_errno, bytes, written;
    char line[128];
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context) ||
        !ReadProcessMemory(child, (void *)0x39614, &old_errno, 4, &bytes) || bytes != 4)
        return 0;
    sprintf(line, "spawn-return %ld errno %lu\n", (long)context.Eax, old_errno);
    WriteFile(log, line, strlen(line), &written, 0);
    if (!WriteProcessMemory(child, (void *)spawn_return, &spawn_return_byte, 1, &bytes) || bytes != 1)
        return 0;
    FlushInstructionCache(child, (void *)spawn_return, 1);
    context.Eip = spawn_return;
    spawn_return = 0;
    return SetThreadContext(thread, &context);
}

static int load_plan(char *path)
{
    FILE *plan;
    RuntimeRange *range;
    plan = fopen(path, "r");
    if (!plan) return 0;
    while (range_count < sizeof(ranges)/sizeof(ranges[0])) {
        range = ranges + range_count;
        if (fscanf(plan, "%lx %lx %lx", &range->start, &range->end, &range->offset) != 3)
            break;
        if (range->start >= range->end || range->end-range->start > sizeof(original_bytes)) {
            fclose(plan);
            return 0;
        }
        ++range_count;
    }
    if (!feof(plan)) { fclose(plan); return 0; }
    fclose(plan);
    return 1;
}

static int restore_runtime(HANDLE child, HANDLE original, HANDLE log,
                           HANDLE thread, DWORD address)
{
    unsigned i;
    DWORD bytes, length, written;
    CONTEXT context;
    char line[128];
    for (i = 0; i < range_count; ++i) {
        if (ranges[i].start != address) continue;
        length = ranges[i].end-ranges[i].start;
        if (SetFilePointer(original, ranges[i].offset, 0, FILE_BEGIN) == 0xffffffff ||
            !ReadFile(original, original_bytes, length, &bytes, 0) || bytes != length ||
            !WriteProcessMemory(child, (void *)address, original_bytes, length, &bytes) ||
            bytes != length) return -1;
        FlushInstructionCache(child, (void *)address, length);
        memset(&context, 0, sizeof(context));
        context.ContextFlags = CONTEXT_CONTROL;
        if (!GetThreadContext(thread, &context)) return -1;
        context.Eip = address;
        if (!SetThreadContext(thread, &context)) return -1;
        sprintf(line, "runtime %08lx %08lx\n", ranges[i].start, ranges[i].end);
        WriteFile(log, line, strlen(line), &written, 0);
        return 1;
    }
    return 0;
}

static void dump_arguments(HANDLE child, HANDLE log, DWORD *words, DWORD word_count)
{
    DWORD i, j, pointer, argument, bytes, written;
    unsigned char value;
    char line[2048];
    int length;
    if (word_count < 3 || words[1] > 64) return;
    for (i = 0; i < words[1]; ++i) {
        pointer = words[2] + 4*i;
        if (!ReadProcessMemory(child, (void *)pointer, &argument, 4, &bytes) || bytes != 4)
            return;
        length = sprintf(line, "argv %lu ", i);
        for (j = 0; j < 512; ++j) {
            if (!ReadProcessMemory(child, (void *)(argument+j), &value, 1, &bytes) || bytes != 1)
                return;
            if (!value) break;
            length += sprintf(line+length, "%02x", value);
        }
        line[length++] = '\n';
        WriteFile(log, line, length, &written, 0);
    }
}

static void dump_inspections(HANDLE child, HANDLE log)
{
    char specifications[4096], *cursor, *next;
    char line[160];
    DWORD address, length, count, i, j, written;
    int used;
    if (!GetEnvironmentVariableA("CVPACK_CAPTURE_INSPECT", specifications, sizeof(specifications)))
        return;
    cursor = specifications;
    while (*cursor) {
        address = strtoul(cursor, &next, 16);
        if (next == cursor || *next != ':') return;
        cursor = next+1;
        length = strtoul(cursor, &next, 10);
        if (next == cursor || length > sizeof(original_bytes)) return;
        if (!ReadProcessMemory(child, (void *)address, original_bytes, length, &count) ||
            count != length) return;
        for (i = 0; i < length; i += 32) {
            used = sprintf(line, "memory %08lx ", address+i);
            for (j = i; j < length && j < i+32; ++j)
                used += sprintf(line+used, "%02x", original_bytes[j]);
            line[used++] = '\n';
            WriteFile(log, line, used, &written, 0);
        }
        if (*next && *next != ',') return;
        cursor = *next ? next+1 : next;
    }
}

static void dump_process_arguments(HANDLE child, HANDLE log)
{
    DWORD arguments[3], bytes;
    arguments[0] = 0;
    if (!ReadProcessMemory(child, (void *)0x39630, arguments+1, 4, &bytes) || bytes != 4 ||
        !ReadProcessMemory(child, (void *)0x39634, arguments+2, 4, &bytes) || bytes != 4)
        return;
    dump_arguments(child, log, arguments, 3);
}

int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION child;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;
    CONTEXT context;
    HANDLE output, error, log, thread, original;
    DWORD base, written, disposition, i, words[24], count;
    char command[2048], line[1024];
    int loader_break, restored;
    char program[1024], child_environment[2048];
    DWORD environment_length;
    if (argc != 7 && argc != 9) return 2;
    original = INVALID_HANDLE_VALUE;
    if (argc == 9) {
        if (!load_plan(argv[7])) return 6;
        original = CreateFileA(argv[8], GENERIC_READ, FILE_SHARE_READ, 0,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (original == INVALID_HANDLE_VALUE) return 7;
    }
    memset(&startup, 0, sizeof(startup));
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    output = CreateFileA(argv[3], GENERIC_WRITE, FILE_SHARE_READ, &security,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    error = CreateFileA(argv[4], GENERIC_WRITE, FILE_SHARE_READ, &security,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    log = CreateFileA(argv[5], GENERIC_WRITE, FILE_SHARE_READ, 0,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (output == INVALID_HANDLE_VALUE || error == INVALID_HANDLE_VALUE ||
        log == INVALID_HANDLE_VALUE) return 3;
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = output;
    startup.hStdError = error;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetEnvironmentVariableA("CVPACK_CAPTURE_ARGV0", program, sizeof(program)))
        strcpy(program, argv[1]);
    sprintf(command, "\"%s\" %s", program, argv[2]);
    if (GetEnvironmentVariableA("CVPACK_CAPTURE_WATCH_SPAWN", program, sizeof(program)))
        watched_spawn = strtoul(program, 0, 16);
    strcpy(child_environment, "LIB=");
    environment_length = GetEnvironmentVariableA("LIB", child_environment+4, sizeof(child_environment)-16);
    if (!environment_length || environment_length >= sizeof(child_environment)-16) return 10;
    strcpy(child_environment+4+environment_length+1, "PATH=");
    child_environment[4+environment_length+1+6] = 0;
    if (!CreateProcessA(argv[1], command, 0, 0, TRUE, DEBUG_ONLY_THIS_PROCESS,
                         child_environment, argv[6], &startup, &child)) return 4;
    base = 0;
    loader_break = 0;
    for (;;) {
        if (!WaitForDebugEvent(&event, 60000)) {
            TerminateProcess(child.hProcess, 98);
            return 5;
        }
        disposition = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            if (base != 0x10000) {
                TerminateProcess(child.hProcess, 97);
                return 8;
            }
            if (event.u.CreateProcessInfo.hFile)
                CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            if (!loader_break && event.u.Exception.ExceptionRecord.ExceptionCode ==
                                  EXCEPTION_BREAKPOINT) {
                loader_break = 1;
            } else {
                thread = event.dwThreadId == child.dwThreadId ? child.hThread : 0;
                if (thread && spawn_return &&
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress == spawn_return) {
                    if (!finish_spawn(child.hProcess, thread, log)) return 11;
                    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
                    continue;
                }
                if (thread && watched_spawn &&
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress == watched_spawn)
                    if (!watch_spawn(child.hProcess, thread, log)) return 12;
                restored = 0;
                if (thread && original != INVALID_HANDLE_VALUE &&
                    event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                    restored = restore_runtime(child.hProcess, original, log, thread,
                        (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress);
                if (restored < 0) return 9;
                if (restored > 0) {
                    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
                    continue;
                }
                sprintf(line, "exception %08lx va %08lx base %08lx first %lu\n",
                    event.u.Exception.ExceptionRecord.ExceptionCode,
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress,
                    base, event.u.Exception.dwFirstChance);
                WriteFile(log, line, strlen(line), &written, 0);
                memset(&context, 0, sizeof(context));
                context.ContextFlags = CONTEXT_FULL;
                if (thread && GetThreadContext(thread, &context)) {
                    sprintf(line, "registers eax %08lx ebx %08lx ecx %08lx edx %08lx esi %08lx edi %08lx ebp %08lx esp %08lx eip %08lx\n",
                        context.Eax, context.Ebx, context.Ecx, context.Edx,
                        context.Esi, context.Edi, context.Ebp, context.Esp, context.Eip);
                    WriteFile(log, line, strlen(line), &written, 0);
                    if (ReadProcessMemory(child.hProcess, (void *)context.Esp,
                                          words, sizeof(words), &count)) {
                        for (i = 0; i < count / 4; ++i) {
                            sprintf(line, "stack %lu %08lx\n", i, words[i]);
                            WriteFile(log, line, strlen(line), &written, 0);
                        }
                    }
                }
                dump_process_arguments(child.hProcess, log);
                dump_inspections(child.hProcess, log);
                TerminateProcess(child.hProcess, 99);
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            sprintf(line, "exit %lu\n", event.u.ExitProcess.dwExitCode);
            WriteFile(log, line, strlen(line), &written, 0);
            ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
            break;
        }
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
    }
    CloseHandle(child.hThread);
    CloseHandle(child.hProcess);
    CloseHandle(output);
    CloseHandle(error);
    CloseHandle(log);
    if (original != INVALID_HANDLE_VALUE) CloseHandle(original);
    return 0;
}
