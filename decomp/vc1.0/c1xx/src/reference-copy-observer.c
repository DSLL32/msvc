/* Original-only overlapping memcpy/memmove boundary observer. Not linked
   into the reconstructed application or invoked by cached fixtures. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static DWORD copy_return, copy_destination, copy_source, copy_bytes;
static HANDLE copy_log;

static void observe_copy_memory(HANDLE process, const char *stage, DWORD address)
{
    unsigned char bytes[256];
    DWORD got, written, i, symbol, name_address;
    char line[1024], name[256];
    if (copy_bytes > sizeof(bytes)) exit(250);
    if (!ReadProcessMemory(process, (void *)address, bytes, copy_bytes, &got) || got != copy_bytes) exit(251);
    sprintf(line, "%s ", stage);
    for (i = 0; i < copy_bytes; ++i) sprintf(line + strlen(line), "%02x", bytes[i]);
    strcat(line, "\n");
    WriteFile(copy_log, line, strlen(line), &written, 0);
    /* This observed inline-vector suffix consists of named class tags. */
    for (i = 0; i < copy_bytes / 4; ++i) {
        memcpy(&symbol, bytes + i * 4, 4);
        if (!ReadProcessMemory(process, (void *)(symbol + 8), &name_address, 4, &got) || got != 4) exit(252);
        {
            DWORD j;
            for (j = 0; j < sizeof(name) - 1; ++j) {
                if (!ReadProcessMemory(process, (void *)(name_address + j), &name[j], 1, &got) || got != 1) exit(253);
                if (!name[j]) break;
            }
        }
        name[sizeof(name)-1] = 0;
        sprintf(line, "%s_name %lu %s\n", stage, i, name);
        WriteFile(copy_log, line, strlen(line), &written, 0);
    }
}

static void capture_copy_boundary(HANDLE process, HANDLE thread, DWORD base,
    DWORD address, DWORD code)
{
    CONTEXT context;
    DWORD got, written, words[4], start, offset, length;
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
        sprintf(line, "returned_destination %u\n", context.Eax == copy_destination);
        WriteFile(copy_log, line, strlen(line), &written, 0);
        observe_copy_memory(process, "after", copy_destination);
        CloseHandle(copy_log);
        TerminateProcess(process, 99);
        return;
    }
    if (!GetEnvironmentVariableA("C1XX_COPY_PLAN", path, sizeof(path))) exit(257);
    plan = fopen(path, "r");
    if (!plan || fscanf(plan, "%lx %lx %lx %127s", &start, &offset, &length, name) != 4) exit(258);
    fclose(plan);
    if (address != start || strcmp(name, "_memcpy") || length > sizeof(body)) exit(259);
    if (!ReadProcessMemory(process, (void *)context.Esp, words, sizeof(words), &got) || got != sizeof(words)) exit(260);
    copy_return = words[0]; copy_destination = words[1]; copy_source = words[2]; copy_bytes = words[3];
    if (!GetEnvironmentVariableA("C1XX_COPY_LOG", path, sizeof(path))) exit(261);
    copy_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    sprintf(line, "runtime _memcpy\nreturn_va %08lx\nsize %lu\ndelta %ld\n", copy_return, copy_bytes, copy_destination - copy_source);
    WriteFile(copy_log, line, strlen(line), &written, 0);
    observe_copy_memory(process, "before", copy_source);
    if (!WriteProcessMemory(process, (void *)copy_return, &trap, 1, &got) || got != 1) exit(262);
    FlushInstructionCache(process, (void *)copy_return, 1);
    if (!GetEnvironmentVariableA("C1XX_COPY_IMAGE", path, sizeof(path))) exit(263);
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
#define capture_close_boundary capture_copy_boundary
#include "debug-run.c"
