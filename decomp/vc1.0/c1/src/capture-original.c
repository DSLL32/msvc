/* Explicit original-only reference capture; never linked into C1 or fixtures. */
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

/* Keep caller/register evidence for diagnosing a logical-trap divergence. */
static void dump_context(HANDLE process, HANDLE thread, HANDLE log)
{
    CONTEXT context;
    DWORD words[32], got, written;
    char line[256];
    unsigned i;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) return;
    sprintf(line, "registers eax %08lx ecx %08lx edx %08lx ebx %08lx esi %08lx edi %08lx ebp %08lx esp %08lx\n",
            context.Eax, context.Ecx, context.Edx, context.Ebx, context.Esi,
            context.Edi, context.Ebp, context.Esp);
    WriteFile(log, line, strlen(line), &written, 0);
    if (!ReadProcessMemory(process, (void *)context.Esp, words, sizeof(words), &got)) return;
    for (i = 0; i < got / 4; ++i) {
        sprintf(line, "stack %u %08lx\n", i, words[i]);
        WriteFile(log, line, strlen(line), &written, 0);
    }
    if (!ReadProcessMemory(process, (void *)(context.Ebp - sizeof(words)), words, sizeof(words), &got)) return;
    for (i = 0; i < got / 4; ++i) {
        sprintf(line, "frame -%lu %08lx\n", sizeof(words) - i * 4, words[i]);
        WriteFile(log, line, strlen(line), &written, 0);
    }
}

typedef struct EntryPlan {
    unsigned long rva;
    unsigned byte;
    char name[128];
} EntryPlan;
static EntryPlan entries[2048];

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
    int loader_break = 0, failed = 0;
    unsigned entry_count = 0, next_entry = 0;
    FILE *plan;
    CONTEXT context;
    DWORD old_protection, restored_protection, site;
    unsigned char instruction;

    if (argc == 11) {
        plan = fopen(argv[10],"r");
        if (!plan) return 2;
        while (entry_count < 2048 && fscanf(plan,"%lx %x %127s",
                &entries[entry_count].rva,&entries[entry_count].byte,
                entries[entry_count].name) == 3) ++entry_count;
        if (!feof(plan)) { fclose(plan); return 2; }
        fclose(plan);
    }

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
    si.hStdError = argc >= 10 && !strcmp(argv[9], "stderr-to-stdout") ? out : err;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    sprintf(command, "\"%s\" %s", argv[1], argv[2]);
    if (!CreateProcessA(argv[1], command, 0, 0, TRUE,
                        DEBUG_ONLY_THIS_PROCESS, environment, argc >= 7 ? argv[6] : 0, &si, &pi)) {
        sprintf(line, "create-error %lu\n", GetLastError());
        WriteFile(log, line, strlen(line), &written, 0);
        return 3;
    }
    for (;;) {
        if (!WaitForDebugEvent(&ev, 30000)) { TerminateProcess(pi.hProcess,99); return 4; }
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
            } else if (argc == 11 && code == EXCEPTION_BREAKPOINT &&
                       next_entry < entry_count &&
                       (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress - base == entries[next_entry].rva) {
                site = base + entries[next_entry].rva;
                instruction = (unsigned char)entries[next_entry].byte;
                memset(&context,0,sizeof(context));
                context.ContextFlags = CONTEXT_FULL;
                if (!GetThreadContext(pi.hThread,&context) ||
                    !VirtualProtectEx(pi.hProcess,(void *)site,1,PAGE_EXECUTE_READWRITE,&old_protection) ||
                    !WriteProcessMemory(pi.hProcess,(void *)site,&instruction,1,&written) || written != 1 ||
                    !FlushInstructionCache(pi.hProcess,(void *)site,1) ||
                    !VirtualProtectEx(pi.hProcess,(void *)site,1,old_protection,&restored_protection)) {
                    failed = 6;
                    TerminateProcess(pi.hProcess,99);
                } else {
                    context.Eip = site;
                    if (!SetThreadContext(pi.hThread,&context)) {
                        failed = 6;
                        TerminateProcess(pi.hProcess,99);
                    } else {
                        sprintf(line,"entry %s rva %08lx byte %02x\n",
                                entries[next_entry].name,entries[next_entry].rva,
                                entries[next_entry].byte);
                        WriteFile(log,line,strlen(line),&written,0);
                        ++next_entry;
                    }
                }
            } else {
                if (argc == 11 && next_entry < entry_count) {
                    sprintf(line,"entry-divergence expected %s rva %08lx\n",
                            entries[next_entry].name,entries[next_entry].rva);
                    WriteFile(log,line,strlen(line),&written,0);
                    failed = 5;
                }
                sprintf(line, "exception %08lx rva %08lx base %08lx first %lu\n",
                        code, (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress - base,
                        base, ev.u.Exception.dwFirstChance);
                WriteFile(log, line, strlen(line), &written, 0);
                dump_context(pi.hProcess, pi.hThread, log);
                if (argc >= 9) dump_stream_buffers(pi.hProcess, log, base, argv[8]);
                TerminateProcess(pi.hProcess, 99);
            }
        } else if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            if (argc == 11 && next_entry != entry_count) failed = 5;
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
    return failed;
}
