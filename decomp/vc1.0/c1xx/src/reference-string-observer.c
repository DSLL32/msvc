/* Original-only strchr entry/return boundary observer. Not linked
   into the reconstructed application or invoked by cached fixtures. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static DWORD copy_return, copy_source, copy_character;
static HANDLE copy_log;

static void observe_copy_memory(HANDLE process, const char *stage, DWORD address)
{
    DWORD got, written, i;
    unsigned char character;
    char line[1100];
    sprintf(line, "%s ", stage);
    for (i = 0; i < 512; ++i) {
        if (!ReadProcessMemory(process, (void *)(address + i), &character, 1, &got) || got != 1) exit(250);
        sprintf(line + strlen(line), "%02x", character);
        if (!character) break;
    }
    if (i == 512) exit(251);
    strcat(line, "\n");
    WriteFile(copy_log, line, strlen(line), &written, 0);
}

static void capture_string_boundary(HANDLE process, HANDLE thread, DWORD base,
    DWORD address, DWORD code)
{
    CONTEXT context;
    DWORD got, written, words[3], start, offset, length;
    unsigned char body[1024], trap = 0xcc;
    char path[1024], line[512], name[128];
    HANDLE original;
    FILE *plan;
    if (base != 0x10000 || code != EXCEPTION_BREAKPOINT) exit(254);
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) exit(255);
    if (copy_return) {
        if (address != copy_return) exit(256);
        sprintf(line, "result_offset %ld\n", context.Eax ? (long)(context.Eax - copy_source) : -1L);
        WriteFile(copy_log, line, strlen(line), &written, 0);
        observe_copy_memory(process, "after", copy_source);
        CloseHandle(copy_log);
        TerminateProcess(process, 99);
        return;
    }
    if (!GetEnvironmentVariableA("C1XX_STRING_PLAN", path, sizeof(path))) exit(257);
    plan = fopen(path, "r");
    if (!plan || fscanf(plan, "%lx %lx %lx %127s", &start, &offset, &length, name) != 4) exit(258);
    fclose(plan);
    if (address != start || strcmp(name, "_strchr") || length > sizeof(body)) exit(259);
    if (!ReadProcessMemory(process, (void *)context.Esp, words, sizeof(words), &got) || got != sizeof(words)) exit(260);
    copy_return = words[0]; copy_source = words[1]; copy_character = words[2];
    if (!GetEnvironmentVariableA("C1XX_STRING_LOG", path, sizeof(path))) exit(261);
    copy_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    sprintf(line, "runtime _strchr\nreturn_va %08lx\ncharacter %lu\n", copy_return, copy_character);
    WriteFile(copy_log, line, strlen(line), &written, 0);
    observe_copy_memory(process, "before", copy_source);
    if (!WriteProcessMemory(process, (void *)copy_return, &trap, 1, &got) || got != 1) exit(262);
    FlushInstructionCache(process, (void *)copy_return, 1);
    if (!GetEnvironmentVariableA("C1XX_STRING_IMAGE", path, sizeof(path))) exit(263);
    original = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (original == INVALID_HANDLE_VALUE || SetFilePointer(original, offset, 0, FILE_BEGIN) != offset) exit(264);
    if (!ReadFile(original, body, length, &got, 0) || got != length) exit(265);
    CloseHandle(original);
    if (!WriteProcessMemory(process, (void *)address, body, length, &got) || got != length) exit(266);
    FlushInstructionCache(process, (void *)address, length);
    context.Eip = address;
    if (!SetThreadContext(thread, &context)) exit(267);
}

#define C1XX_CLOSE_OBSERVER 1
#define capture_close_boundary capture_string_boundary
#include "debug-run.c"
