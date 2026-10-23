/* Original-only parser repair/stack-origin observer. This source is never
   linked into the reconstruction or run by native fixture checks. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

typedef struct ScopeObservationSite {
    DWORD address;
    unsigned byte;
    char name[64];
} ScopeObservationSite;
static ScopeObservationSite scope_sites[16];
static unsigned scope_count;
static HANDLE scope_log;
static DWORD scope_writer_slot;

static DWORD scope_read_word(HANDLE process, DWORD address)
{
    DWORD value, got;
    if (!ReadProcessMemory(process, (void *)address, &value, 4, &got) || got != 4) exit(240);
    return value;
}

static void capture_scope_boundary(HANDLE process, HANDLE thread, DWORD base,
    DWORD address, DWORD code)
{
    CONTEXT context;
    DWORD got, written, old, ignored, lookahead;
    unsigned i, j;
    unsigned char bytes[32], byte;
    char path[1024], line[2048];
    FILE *plan;
    ScopeObservationSite *site = 0;
    if (base != 0x10000 || code != EXCEPTION_BREAKPOINT) exit(241);
    if (!scope_log) {
        if (!GetEnvironmentVariableA("C1XX_SCOPE_PLAN", path, sizeof(path))) exit(242);
        plan = fopen(path, "r");
        if (!plan) exit(243);
        while (scope_count < 16 && fscanf(plan, "%lx %x %63s", &scope_sites[scope_count].address,
                &scope_sites[scope_count].byte, scope_sites[scope_count].name) == 3) ++scope_count;
        if (!feof(plan)) exit(244);
        fclose(plan);
        if (!GetEnvironmentVariableA("C1XX_SCOPE_LOG", path, sizeof(path))) exit(245);
        scope_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (scope_log == INVALID_HANDLE_VALUE) exit(246);
    }
    for (i = 0; i < scope_count; ++i)
        if (scope_sites[i].address == address) site = &scope_sites[i];
    if (!site) exit(247);
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) exit(248);
    lookahead = scope_read_word(process, 0x9eb9c);
    if (!ReadProcessMemory(process, (void *)(context.Esp - 16), bytes, sizeof(bytes), &got) || got != sizeof(bytes)) exit(249);
    sprintf(line, "site %s va %08lx esp %08lx ebp %08lx eax %08lx ebx %08lx lookahead %08lx\nstack_from_esp_minus_16 ",
        site->name, address, context.Esp, context.Ebp, context.Eax, context.Ebx, lookahead);
    for (j = 0; j < sizeof(bytes); ++j) sprintf(line + strlen(line), "%02x", bytes[j]);
    strcat(line, "\n");
    WriteFile(scope_log, line, strlen(line), &written, 0);
    if (!strcmp(site->name, "after_writer")) {
        scope_writer_slot = context.Ebp - 4;
        sprintf(line, "writer_slot %08lx word %08lx\n", scope_writer_slot, scope_read_word(process, scope_writer_slot));
        WriteFile(scope_log, line, strlen(line), &written, 0);
    } else if (!strcmp(site->name, "scope_entry")) {
        if (!ReadProcessMemory(process, (void *)(context.Esp - 1), &byte, 1, &got) || got != 1) exit(250);
        sprintf(line, "entry_stale_byte %02x writer_high_byte_same_address %u caller %08lx argument %08lx\n",
            (unsigned)byte, scope_writer_slot && scope_writer_slot + 3 == context.Esp - 1,
            scope_read_word(process, context.Esp), scope_read_word(process, context.Esp + 4));
        WriteFile(scope_log, line, strlen(line), &written, 0);
    } else if (!strcmp(site->name, "token_comparison")) {
        sprintf(line, "comparison_bl %02x equals_open_brace %u\n", (unsigned)(context.Ebx & 255), (context.Ebx & 255) == 127);
        WriteFile(scope_log, line, strlen(line), &written, 0);
    } else if (!strcmp(site->name, "scope_return")) {
        sprintf(line, "scope_result %lu\n", context.Eax);
        WriteFile(scope_log, line, strlen(line), &written, 0);
    }
    byte = (unsigned char)site->byte;
    if (!VirtualProtectEx(process, (void *)address, 1, PAGE_EXECUTE_READWRITE, &old) ||
        !WriteProcessMemory(process, (void *)address, &byte, 1, &got) || got != 1 ||
        !FlushInstructionCache(process, (void *)address, 1) ||
        !VirtualProtectEx(process, (void *)address, 1, old, &ignored)) exit(251);
    context.Eip = address;
    if (!SetThreadContext(thread, &context)) exit(252);
}

#define C1XX_CLOSE_OBSERVER 1
#define capture_close_boundary capture_scope_boundary
#include "debug-run.c"
