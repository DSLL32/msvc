/* Original-only multiplication argument/return observer. The initial call
   is unchanged; a bounded plan then reuses that call's stack for diagnostic
   vectors. Never linked into the reconstruction or cached fixtures. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static DWORD arithmetic_vectors[16][4], arithmetic_arguments[4];
static DWORD arithmetic_stack, arithmetic_return;
static unsigned arithmetic_count, arithmetic_index;
static HANDLE arithmetic_log;

static void arithmetic_read(HANDLE process, DWORD address, void *value, DWORD bytes)
{
    DWORD got;
    if (!ReadProcessMemory(process, (void *)address, value, bytes, &got) || got != bytes)
        exit(240);
}

static void arithmetic_write(HANDLE process, DWORD address, const void *value, DWORD bytes)
{
    DWORD got;
    if (!WriteProcessMemory(process, (void *)address, value, bytes, &got) || got != bytes ||
        !FlushInstructionCache(process, (void *)address, bytes)) exit(241);
}

static void observe_arithmetic_boundary(HANDLE process, HANDLE thread, DWORD base,
    DWORD address, DWORD code)
{
    CONTEXT context;
    DWORD written;
    unsigned char byte;
    char path[1024], line[512];
    FILE *plan;
    if (base != 0x10000 || code != EXCEPTION_BREAKPOINT) exit(242);
    memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) exit(243);
    if (!arithmetic_stack) {
        if (address != 0x74830) exit(244);
        if (!GetEnvironmentVariableA("C1XX_ARITHMETIC_PLAN", path, sizeof(path))) exit(245);
        plan = fopen(path, "r"); if (!plan) exit(246);
        while (arithmetic_count < 16 && fscanf(plan, "%lx %lx %lx %lx",
            &arithmetic_vectors[arithmetic_count][0], &arithmetic_vectors[arithmetic_count][1],
            &arithmetic_vectors[arithmetic_count][2], &arithmetic_vectors[arithmetic_count][3]) == 4)
            ++arithmetic_count;
        if (!feof(plan)) exit(247);
        fclose(plan);
        if (!GetEnvironmentVariableA("C1XX_ARITHMETIC_LOG", path, sizeof(path))) exit(248);
        arithmetic_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (arithmetic_log == INVALID_HANDLE_VALUE) exit(249);
        arithmetic_stack = context.Esp;
        arithmetic_read(process, arithmetic_stack, &arithmetic_return, 4);
        arithmetic_read(process, arithmetic_stack + 4, arithmetic_arguments, sizeof(arithmetic_arguments));
        sprintf(line, "actual caller %08lx raw_width %08lx\n", arithmetic_return, arithmetic_arguments[3]);
        WriteFile(arithmetic_log, line, strlen(line), &written, 0);
        byte = 0xcc; arithmetic_write(process, arithmetic_return, &byte, 1);
        byte = 0x83; arithmetic_write(process, address, &byte, 1);
        context.Eip = address;
    } else {
        if (address != arithmetic_return) exit(250);
        sprintf(line, "result %u %08lx %08lx %u %u %lu\n", arithmetic_index,
            arithmetic_arguments[0], arithmetic_arguments[1],
            (unsigned)(unsigned short)arithmetic_arguments[2],
            (unsigned)(unsigned short)arithmetic_arguments[3], context.Eax);
        WriteFile(arithmetic_log, line, strlen(line), &written, 0);
        if (arithmetic_index == arithmetic_count) {
            CloseHandle(arithmetic_log); TerminateProcess(process, 99); return;
        }
        memcpy(arithmetic_arguments, arithmetic_vectors[arithmetic_index], sizeof(arithmetic_arguments));
        ++arithmetic_index;
        arithmetic_write(process, arithmetic_stack + 4, arithmetic_arguments, sizeof(arithmetic_arguments));
        context.Esp = arithmetic_stack;
        context.Eip = 0x74830;
    }
    if (!SetThreadContext(thread, &context)) exit(251);
}

#define C1XX_CLOSE_OBSERVER 1
#define capture_close_boundary observe_arithmetic_boundary
#include "debug-run.c"
