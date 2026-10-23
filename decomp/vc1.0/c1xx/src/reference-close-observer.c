/* Explicit original-only CRT boundary diagnostic. Never linked into the
   reconstruction or invoked by native fixtures. A reviewed plan identifies
   exact LIBC/import ranges to restore at their matching first-entry traps. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static int close_stage;
static DWORD close_return, close_descriptor;
static HANDLE close_log;

static void capture_close_boundary(HANDLE process, HANDLE thread, DWORD base,
    DWORD address, DWORD code)
{
    CONTEXT context;
    DWORD got, written, words[2], start, offset, length, flag;
    unsigned char body[1024], trap = 0xcc;
    char path[1024], line[512], name[128];
    HANDLE original;
    FILE *plan;
    int found = 0;
    if (base != 0x10000 || code != EXCEPTION_BREAKPOINT) exit(210);
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) exit(211);
    if (close_stage && address == close_return) {
        flag = 0;
        if (!ReadProcessMemory(process, (void *)(0xaab70 + close_descriptor), &flag, 1, &got) || got != 1) exit(212);
        sprintf(line, "result %ld\nclosed_flag %lu\n", (long)context.Eax, flag);
        WriteFile(close_log, line, strlen(line), &written, 0);
        CloseHandle(close_log);
        TerminateProcess(process, 99);
        return;
    }
    if (!GetEnvironmentVariableA("C1XX_CLOSE_PLAN", path, sizeof(path))) exit(213);
    plan = fopen(path, "r");
    if (!plan) exit(214);
    while (fscanf(plan, "%lx %lx %lx %127s", &start, &offset, &length, name) == 4)
        if (address == start) { found = 1; break; }
    fclose(plan);
    if (!found || length > sizeof(body)) exit(215);
    if (!ReadProcessMemory(process, (void *)context.Esp, words, sizeof(words), &got) || got != sizeof(words)) exit(216);
    if (!close_stage) {
        if (strcmp(name, "__close")) exit(217);
        if (!GetEnvironmentVariableA("C1XX_CLOSE_LOG", path, sizeof(path))) exit(218);
        close_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        close_descriptor = words[1];
        close_return = words[0];
        flag = 0;
        if (!ReadProcessMemory(process, (void *)(0xaab70 + close_descriptor), &flag, 1, &got) || got != 1) exit(219);
        sprintf(line, "descriptor %lu\nopen_flag %lu\nreturn_va %08lx\n", close_descriptor, flag, close_return);
        WriteFile(close_log, line, strlen(line), &written, 0);
        if (!WriteProcessMemory(process, (void *)close_return, &trap, 1, &got) || got != 1) exit(220);
        FlushInstructionCache(process, (void *)close_return, 1);
        close_stage = 1;
    }
    sprintf(line, "runtime %s argument %08lx\n", name, words[1]);
    WriteFile(close_log, line, strlen(line), &written, 0);
    if (!GetEnvironmentVariableA("C1XX_CLOSE_IMAGE", path, sizeof(path))) exit(221);
    original = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (original == INVALID_HANDLE_VALUE || SetFilePointer(original, offset, 0, FILE_BEGIN) != offset) exit(222);
    if (!ReadFile(original, body, length, &got, 0) || got != length) exit(223);
    CloseHandle(original);
    if (!WriteProcessMemory(process, (void *)address, body, length, &got) || got != length) exit(224);
    FlushInstructionCache(process, (void *)address, length);
    context.Eip = address;
    if (!SetThreadContext(thread, &context)) exit(225);
}

#define C1XX_CLOSE_OBSERVER 1
#include "debug-run.c"
