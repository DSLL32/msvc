/* Small debugger used only by the differential harness. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <recon.h>

/* Inspect pending bytes without flushing or changing the debugged process.
   The addresses are RVAs of FILE* objects in the old 32-bit CRT ABI. */
static void dump_stream_buffers(HANDLE process, HANDLE log, DWORD base, char *rvas)
{
    static unsigned char bytes[65536];
    static char line[131136];
    static char digits[] = "0123456789abcdef";
    DWORD address, stream, fields[3], got, written, count;
    char *next;
    unsigned index = 0, i, length;
    while (*rvas) {
        address = base + strtoul(rvas, &next, 16);
        if (next == rvas) return;
        length = sprintf(line, "buffer %u ", index++);
        if (!ReadProcessMemory(process, (void *)address, &stream, 4, &got) || got != 4)
            length += sprintf(line + length, "unreadable\n");
        else if (!stream)
            length += sprintf(line + length, "unopened\n");
        else if (!ReadProcessMemory(process, (void *)stream, fields, 12, &got) || got != 12)
            length += sprintf(line + length, "unreadable-file\n");
        else {
            count = fields[0] - fields[2]; /* FILE._ptr - FILE._base */
            if (count > sizeof(bytes))
                length += sprintf(line + length, "invalid\n");
            else if (count && (!ReadProcessMemory(process, (void *)fields[2], bytes, count, &got) || got != count))
                length += sprintf(line + length, "unreadable-bytes\n");
            else {
                length += sprintf(line + length, "%lu ", count);
                for (i = 0; i < count; ++i) {
                    line[length++] = digits[bytes[i] >> 4];
                    line[length++] = digits[bytes[i] & 15];
                }
                line[length++] = '\n';
            }
        }
        WriteFile(log, line, length, &written, 0);
        rvas = *next == ',' ? next + 1 : next;
    }
}

/* Compare selected semantic state while the child is stopped. Pointer
   fields report nullness, so addresses need not match between linked images. */
static void dump_checkpoint_state(HANDLE process, HANDLE thread, HANDLE log,
    DWORD base, char *specifications)
{
    DWORD address, value, got, written, index = 0;
    CONTEXT context;
    char *next, mode, line[128];
    while (*specifications) {
        address = base + strtoul(specifications, &next, 16);
        if (next == specifications || *next != ':') return;
        mode = next[1];
        next += 2;
        value = got = 0;
        if (!ReadProcessMemory(process, (void *)address, &value, 4, &got) || got != 4)
            sprintf(line, "state %lu unreadable\n", index++);
        else {
            if (mode == 'p') value = value != 0;
            sprintf(line, "state %lu %08lx\n", index++, value);
        }
        WriteFile(log, line, strlen(line), &written, 0);
        specifications = *next == ',' ? next + 1 : next;
    }
    context.ContextFlags = CONTEXT_CONTROL;
    value = got = 0;
    if (!GetThreadContext(thread, &context) ||
        !ReadProcessMemory(process, (void *)context.Esp, &value, 4, &got) || got != 4)
        sprintf(line, "caller unreadable\n");
    else sprintf(line, "caller %08lx\n", value - base);
    WriteFile(log, line, strlen(line), &written, 0);
}

int main(int argc, char **argv)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DEBUG_EVENT ev;
    HANDLE out, err, log;
    DWORD base = 0, written, status, code;
    char command[2048], line[256], child_environment[8192];
    void *environment = 0;
    int loader_break = 0;

    if (argc < 6 || argc > 11) return 2;
    if (argc >= 8) {
        if (strlen(argv[7]) + 2 > sizeof(child_environment)) return 2;
        strcpy(child_environment, argv[7]);
        child_environment[strlen(argv[7]) + 1] = 0;
        environment = child_environment;
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
    if (argc >= 9) {
        if (strlen(argv[8]) + strlen(argv[2]) + 2 > sizeof(command)) return 2;
        strcpy(command, argv[8]);
        if (*argv[2]) { strcat(command, " "); strcat(command, argv[2]); }
    } else
        sprintf(command, "\"%s\" %s", argv[1], argv[2]);
    if (!CreateProcessA(argv[1], command, 0, 0, TRUE,
                        DEBUG_ONLY_THIS_PROCESS, environment, argc >= 7 ? argv[6] : 0, &si, &pi)) {
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
#ifdef C1XX_CLOSE_OBSERVER
                capture_close_boundary(pi.hProcess, pi.hThread, base,
                    (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress, code);
#else
                if (argc >= 10)
                    dump_stream_buffers(pi.hProcess, log, base, argv[9]);
                if (argc == 11)
                    dump_checkpoint_state(pi.hProcess, pi.hThread, log, base, argv[10]);
                TerminateProcess(pi.hProcess, 99);
#endif
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
