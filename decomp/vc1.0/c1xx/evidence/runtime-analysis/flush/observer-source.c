/* Original-only stream boundary diagnostic; never target implementation.
   Restore one reviewed runtime entry and observe its FILE before/after return. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static DWORD stream_return, stream_address;
static HANDLE stream_log;

static void observe_stream(HANDLE process, const char *phase)
{
    DWORD words[8], got, written;
    char line[256];
    if (!ReadProcessMemory(process, (void *)stream_address, words, sizeof(words), &got) || got != sizeof(words)) exit(230);
    sprintf(line, "%s descriptor %lu flags %lu count %ld used %ld size %lu\n", phase,
        words[4], words[3], (long)words[1], (long)(words[0]-words[2]), words[6]);
    WriteFile(stream_log, line, strlen(line), &written, 0);
}

static void capture_stream_boundary(HANDLE process, HANDLE thread, DWORD base,
    DWORD address, DWORD code)
{
    CONTEXT context;
    DWORD got, written, words[2], start, offset, length;
    unsigned char body[1024], trap = 0xcc;
    char path[1024], line[128], name[128];
    HANDLE original;
    FILE *plan;
    if (base != 0x10000 || code != EXCEPTION_BREAKPOINT) exit(231);
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) exit(232);
    if (stream_return) {
        if (address != stream_return) exit(233);
        observe_stream(process, "after");
        sprintf(line, "result %ld\n", (long)context.Eax);
        WriteFile(stream_log, line, strlen(line), &written, 0);
        CloseHandle(stream_log);
        TerminateProcess(process, 99);
        return;
    }
    if (!GetEnvironmentVariableA("C1XX_STREAM_PLAN", path, sizeof(path))) exit(234);
    plan = fopen(path, "r");
    if (!plan || fscanf(plan, "%lx %lx %lx %127s", &start, &offset, &length, name) != 4) exit(235);
    fclose(plan);
    if (address != start || length > sizeof(body) || strcmp(name, "_fflush")) exit(236);
    if (!ReadProcessMemory(process, (void *)context.Esp, words, sizeof(words), &got) || got != sizeof(words)) exit(237);
    stream_return = words[0];
    stream_address = words[1];
    if (!GetEnvironmentVariableA("C1XX_STREAM_LOG", path, sizeof(path))) exit(238);
    stream_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    sprintf(line, "runtime %s\nreturn_va %08lx\n", name, stream_return);
    WriteFile(stream_log, line, strlen(line), &written, 0);
    observe_stream(process, "before");
    if (!WriteProcessMemory(process, (void *)stream_return, &trap, 1, &got) || got != 1) exit(239);
    if (!GetEnvironmentVariableA("C1XX_STREAM_IMAGE", path, sizeof(path))) exit(240);
    original = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (original == INVALID_HANDLE_VALUE || SetFilePointer(original, offset, 0, FILE_BEGIN) != offset) exit(241);
    if (!ReadFile(original, body, length, &got, 0) || got != length) exit(242);
    CloseHandle(original);
    if (!WriteProcessMemory(process, (void *)address, body, length, &got) || got != length) exit(243);
    FlushInstructionCache(process, (void *)address, length);
    FlushInstructionCache(process, (void *)stream_return, 1);
    context.Eip = address;
    if (!SetThreadContext(thread, &context)) exit(244);
}

#define C1XX_CLOSE_OBSERVER 1
#define capture_close_boundary capture_stream_boundary
#include "debug-run.c"
