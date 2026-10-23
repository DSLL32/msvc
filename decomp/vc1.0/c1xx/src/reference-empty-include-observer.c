/* Original-only empty -I diagnostic. Never linked into the reconstruction or
   called by cached fixtures. Observe the historical unwritten pointer read. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static unsigned probe_stage;
static DWORD probe_return, probe_local;
static HANDLE probe_log;

static DWORD read_word(HANDLE process, DWORD address)
{
    DWORD value, got;
    if (!ReadProcessMemory(process, (void *)address, &value, 4, &got) || got != 4) exit(240);
    return value;
}

static void put_byte(HANDLE process, DWORD address, unsigned char value)
{
    DWORD written;
    if (!WriteProcessMemory(process, (void *)address, &value, 1, &written) || written != 1) exit(241);
    if (!FlushInstructionCache(process, (void *)address, 1)) exit(242);
}

static void capture_empty_include(HANDLE process, HANDLE thread, DWORD base,
    DWORD address, DWORD code)
{
    CONTEXT context;
    DWORD got, written, vector, items, first, i;
    unsigned char byte;
    char line[1024], path[1024];
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context) || base != 0x10000) exit(243);
    if (!probe_stage) {
        if (code != EXCEPTION_BREAKPOINT || address != 0x24290) exit(244);
        if (!GetEnvironmentVariableA("C1XX_EMPTY_INCLUDE_LOG", path, sizeof(path))) exit(245);
        probe_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        probe_return = read_word(process, context.Esp);
        probe_local = read_word(process, context.Esp - 16);
        vector = read_word(process, 0x8cfd4);
        items = read_word(process, vector + 8);
        first = read_word(process, items);
        sprintf(line, "entry %08lx caller %08lx entry_esp %08lx unwritten_slot %08lx unwritten_pointer %08lx\nfirst_directory %08lx bytes ",
            address, probe_return, context.Esp, context.Esp - 16, probe_local, first);
        for (i = 0; i < 256; ++i) {
            if (!ReadProcessMemory(process, (void *)(first + i), &byte, 1, &got) || got != 1) exit(246);
            sprintf(line + strlen(line), "%02x", byte);
            if (!byte) break;
        }
        strcat(line, "\n");
        WriteFile(probe_log, line, strlen(line), &written, 0);
        put_byte(process, 0x24290, 0x83);
        put_byte(process, 0x24310, 0xcc);
        probe_stage = 1;
    } else if (probe_stage == 1) {
        if (code != EXCEPTION_BREAKPOINT || address != 0x24310) exit(247);
        sprintf(line, "read %08lx esi %08lx slot %08lx\n", address, context.Esi,
            read_word(process, context.Esp + 16));
        WriteFile(probe_log, line, strlen(line), &written, 0);
        if (ReadProcessMemory(process, (void *)context.Esi, &byte, 1, &got) && got == 1)
            sprintf(line, "pointed_byte %02x\n", byte);
        else sprintf(line, "pointed_byte unreadable\n");
        WriteFile(probe_log, line, strlen(line), &written, 0);
        put_byte(process, 0x24310, 0x8a);
        put_byte(process, probe_return, 0xcc);
        probe_stage = 2;
    } else {
        sprintf(line, "outcome code %08lx address %08lx eip %08lx eax %08lx\n", code, address, context.Eip, context.Eax);
        WriteFile(probe_log, line, strlen(line), &written, 0);
        CloseHandle(probe_log);
        TerminateProcess(process, 99);
        return;
    }
    context.Eip = address;
    if (!SetThreadContext(thread, &context)) exit(248);
}

#define C1XX_CLOSE_OBSERVER 1
#define capture_close_boundary capture_empty_include
#include "debug-run.c"
