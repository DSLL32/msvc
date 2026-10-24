/* Original-only public _atoldbl boundary observation. The untouched target
   supplies all CRT bodies; no implementation or fixture depends on this. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static DWORD decimal_return, decimal_output, decimal_text;
static HANDLE decimal_log;
static unsigned skipped_decimal_state;
static DWORD skipped_decimal_return;
static unsigned char skipped_decimal_byte;

static void decimal_memory(HANDLE process, const char *name, DWORD address,
    unsigned count, int string)
{
    unsigned i;
    DWORD got, written;
    unsigned char byte;
    char line[400];
    sprintf(line, "%s ", name);
    for (i = 0; i < count; ++i) {
        if (!ReadProcessMemory(process, (void *)(address+i), &byte, 1, &got) || got != 1) exit(240);
        sprintf(line+strlen(line), "%02x", byte);
        if (string && !byte) break;
    }
    if (string && i == count) exit(241);
    strcat(line, "\n");
    WriteFile(decimal_log, line, strlen(line), &written, 0);
}

static void capture_decimal_boundary(HANDLE process, HANDLE thread, DWORD base,
    DWORD address, DWORD code)
{
    CONTEXT context;
    DWORD words[3], got, written;
    unsigned char first = 0x83, trap = 0xcc;
    char path[1024], line[400];
    if (base != 0x10000 || code != EXCEPTION_BREAKPOINT) exit(242);
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) exit(243);
    /* Diagnostic only: run and rearm the genuine first call, then
       observe the unchanged case's second conversion. No CRT body changes. */
    if (!skipped_decimal_state) {
        unsigned char spelling[4];
        if (address != 0x84cf0 ||
            !ReadProcessMemory(process, (void *)context.Esp, words, sizeof(words), &got) || got != sizeof(words) ||
            words[0] != 0x74b57 || words[2] != 0x8c158 || words[1] != context.Esp + 44 ||
            !ReadProcessMemory(process, (void *)words[2], spelling, 4, &got) || got != 4 || memcmp(spelling, "1.5", 4)) exit(253);
        skipped_decimal_return = words[0];
        if (!ReadProcessMemory(process, (void *)words[0], &skipped_decimal_byte, 1, &got) || got != 1 ||
            !WriteProcessMemory(process, (void *)address, &first, 1, &got) || got != 1 ||
            !WriteProcessMemory(process, (void *)words[0], &trap, 1, &got) || got != 1) exit(254);
        FlushInstructionCache(process, (void *)address, 1);
        FlushInstructionCache(process, (void *)words[0], 1);
        skipped_decimal_state = 1;
        context.Eip = address;
        if (!SetThreadContext(thread, &context)) exit(255);
        return;
    }
    if (skipped_decimal_state == 1) {
        if (address != skipped_decimal_return || context.Eax != 0 ||
            !WriteProcessMemory(process, (void *)address, &skipped_decimal_byte, 1, &got) || got != 1 ||
            !WriteProcessMemory(process, (void *)0x84cf0, &trap, 1, &got) || got != 1) exit(256);
        FlushInstructionCache(process, (void *)address, 1);
        FlushInstructionCache(process, (void *)0x84cf0, 1);
        skipped_decimal_state = 2;
        context.Eip = address;
        if (!SetThreadContext(thread, &context)) exit(257);
        return;
    }
    if (decimal_return) {
        if (address != decimal_return) exit(244);
        sprintf(line, "result %ld\n", (long)context.Eax);
        WriteFile(decimal_log, line, strlen(line), &written, 0);
        decimal_memory(process, "output", decimal_output, 10, 0);
        decimal_memory(process, "after_text", decimal_text, 129, 1);
        CloseHandle(decimal_log);
        TerminateProcess(process, 99);
        return;
    }
    if (address != 0x84cf0) exit(245);
    if (!ReadProcessMemory(process, (void *)context.Esp, words, sizeof(words), &got) || got != sizeof(words)) exit(246);
    decimal_return = words[0]; decimal_output = words[1]; decimal_text = words[2];
    if (decimal_return != 0x74b57 || decimal_text != 0x8c158 || decimal_output != context.Esp + 44) exit(247);
    if (!GetEnvironmentVariableA("C1XX_DECIMAL_LOG", path, sizeof(path))) exit(248);
    decimal_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (decimal_log == INVALID_HANDLE_VALUE) exit(249);
    sprintf(line, "runtime __atoldbl\nreturn_va %08lx\noutput_address %08lx\ntext_address %08lx\nentry_esp %08lx\n", decimal_return, decimal_output, decimal_text, context.Esp);
    WriteFile(decimal_log, line, strlen(line), &written, 0);
    decimal_memory(process, "text", decimal_text, 129, 1);
    /* Output prestate is unwritten and intentionally not read. */
    if (!WriteProcessMemory(process, (void *)decimal_return, &trap, 1, &got) || got != 1) exit(250);
    if (!WriteProcessMemory(process, (void *)address, &first, 1, &got) || got != 1) exit(251);
    FlushInstructionCache(process, (void *)decimal_return, 1);
    FlushInstructionCache(process, (void *)address, 1);
    context.Eip = address;
    if (!SetThreadContext(thread, &context)) exit(252);
}

#define C1XX_CLOSE_OBSERVER 1
#define capture_close_boundary capture_decimal_boundary
#include "debug-run.c"
