/* Original-only realloc boundary observer. Never linked into the target or
   invoked by cached fixtures. Restore only the independently identified CRT
   ranges, retaining every first entry and the returned block prefix. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static DWORD realloc_return, realloc_bytes;
static HANDLE realloc_log;

static void observe_realloc_prefix(HANDLE process, const char *stage, DWORD pointer)
{
    unsigned char bytes[1024];
    DWORD got, written, i, symbol, name_address;
    char line[2200], name[256];
    if (realloc_bytes > sizeof(bytes)) exit(250);
    if (!ReadProcessMemory(process, (void *)pointer, bytes, realloc_bytes, &got) || got != realloc_bytes) exit(251);
    sprintf(line, "%s ", stage);
    for (i = 0; i < realloc_bytes; ++i) sprintf(line + strlen(line), "%02x", bytes[i]);
    strcat(line, "\n");
    WriteFile(realloc_log, line, strlen(line), &written, 0);
    memcpy(&symbol, bytes, 4);
    if (!ReadProcessMemory(process, (void *)(symbol + 8), &name_address, 4, &got) || got != 4) exit(252);
    for (i = 0; i < sizeof(name) - 1; ++i) {
        if (!ReadProcessMemory(process, (void *)(name_address + i), &name[i], 1, &got) || got != 1) exit(253);
        if (!name[i]) break;
    }
    name[sizeof(name)-1] = 0;
    sprintf(line, "%s_name %s\n", stage, name);
    WriteFile(realloc_log, line, strlen(line), &written, 0);
}

static void capture_realloc_boundary(HANDLE process, HANDLE thread, DWORD base,
    DWORD address, DWORD code)
{
    CONTEXT context;
    DWORD got, written, words[3], fields[2], start, offset, length;
    unsigned char body[16384], trap = 0xcc;
    char path[1024], line[512], name[128];
    HANDLE original;
    FILE *plan;
    int found = 0;
    if (base != 0x10000 || code != EXCEPTION_BREAKPOINT) exit(254);
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) exit(255);
    if (realloc_return && address == realloc_return) {
        sprintf(line, "result_nonnull %u\n", context.Eax != 0);
        WriteFile(realloc_log, line, strlen(line), &written, 0);
        if (context.Eax) observe_realloc_prefix(process, "after", context.Eax);
        CloseHandle(realloc_log);
        TerminateProcess(process, 99);
        return;
    }
    if (!GetEnvironmentVariableA("C1XX_REALLOC_PLAN", path, sizeof(path))) exit(256);
    plan = fopen(path, "r");
    if (!plan) exit(257);
    while (fscanf(plan, "%lx %lx %lx %127s", &start, &offset, &length, name) == 4)
        if (address == start) { found = 1; break; }
    fclose(plan);
    if (!found || length > sizeof(body)) exit(258);
    if (!ReadProcessMemory(process, (void *)context.Esp, words, sizeof(words), &got) || got != sizeof(words)) exit(259);
    if (!realloc_return) {
        if (strcmp(name, "_realloc")) exit(260);
        if (!GetEnvironmentVariableA("C1XX_REALLOC_LOG", path, sizeof(path))) exit(261);
        realloc_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (!ReadProcessMemory(process, (void *)(context.Esi + 16), fields, sizeof(fields), &got) || got != sizeof(fields)) exit(262);
        realloc_bytes = fields[0] * 16;
        realloc_return = words[0];
        sprintf(line, "return_va %08lx\nsize %lu\ncount %lu\ncapacity %lu\n", realloc_return, words[2], fields[0], fields[1]);
        WriteFile(realloc_log, line, strlen(line), &written, 0);
        observe_realloc_prefix(process, "before", words[1]);
        if (!WriteProcessMemory(process, (void *)realloc_return, &trap, 1, &got) || got != 1) exit(263);
        FlushInstructionCache(process, (void *)realloc_return, 1);
    }
    sprintf(line, "runtime %s\n", name);
    WriteFile(realloc_log, line, strlen(line), &written, 0);
    if (!GetEnvironmentVariableA("C1XX_REALLOC_IMAGE", path, sizeof(path))) exit(264);
    original = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (original == INVALID_HANDLE_VALUE || SetFilePointer(original, offset, 0, FILE_BEGIN) != offset) exit(265);
    if (!ReadFile(original, body, length, &got, 0) || got != length) exit(266);
    CloseHandle(original);
    if (!WriteProcessMemory(process, (void *)address, body, length, &got) || got != length) exit(267);
    FlushInstructionCache(process, (void *)address, length);
    context.Eip = address;
    if (!SetThreadContext(thread, &context)) exit(268);
}

#define C1XX_CLOSE_OBSERVER 1
#define capture_close_boundary capture_realloc_boundary
#include "debug-run.c"
