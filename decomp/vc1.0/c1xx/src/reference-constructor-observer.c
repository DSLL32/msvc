/* Original-only constructor marker argument/return observer. Analysis
   infrastructure, never linked into the reconstruction or cached fixtures. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static HANDLE constructor_log;
static DWORD constructor_entry, constructor_return;
static unsigned char constructor_entry_byte, constructor_return_byte;
static unsigned char constructor_low, constructor_member;
static const char *constructor_kind;

static void constructor_read(HANDLE process, DWORD address, void *buffer, DWORD size)
{
    DWORD got;
    if (!ReadProcessMemory(process, (void *)address, buffer, size, &got) || got != size)
        exit(240);
}

static void constructor_write(HANDLE process, DWORD address, unsigned char byte)
{
    DWORD got;
    if (!WriteProcessMemory(process, (void *)address, &byte, 1, &got) || got != 1 ||
        !FlushInstructionCache(process, (void *)address, 1)) exit(241);
}

static void observe_constructor_boundary(HANDLE process, HANDLE thread, DWORD base,
    DWORD address, DWORD code)
{
    CONTEXT context;
    DWORD argument, written;
    unsigned char flags[4];
    char path[1024], line[512];
    if (base != 0x10000 || code != EXCEPTION_BREAKPOINT) exit(242);
    if (!constructor_log) {
        if (!GetEnvironmentVariableA("C1XX_CONSTRUCTOR_LOG", path, sizeof(path))) exit(243);
        constructor_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, 0);
        if (constructor_log == INVALID_HANDLE_VALUE) exit(244);
    }
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) exit(245);
    if (constructor_return) {
        if (address != constructor_return) exit(246);
        sprintf(line, "result %s low %02x member %02x value %lu caller %08lx\n",
            constructor_kind, (unsigned)constructor_low, (unsigned)constructor_member,
            context.Eax, constructor_return);
        WriteFile(constructor_log, line, strlen(line), &written, 0);
        constructor_write(process, constructor_return, constructor_return_byte);
        constructor_write(process, constructor_entry, 0xcc);
        constructor_return = 0;
    } else if (address == 0x37f20) {
        sprintf(line, "terminal make_constructor_access_probe\n");
        WriteFile(constructor_log, line, strlen(line), &written, 0);
        CloseHandle(constructor_log);
        TerminateProcess(process, 99);
        return;
    } else {
        if (address != 0x37f50 && address != 0x37f90) exit(247);
        constructor_entry = address;
        constructor_entry_byte = 0x8b;
        constructor_kind = address == 0x37f50 ? "callback" : "checker";
        constructor_read(process, context.Esp, &constructor_return, 4);
        constructor_read(process, context.Esp + 4, &argument, 4);
        constructor_read(process, argument + 48, flags, sizeof(flags));
        constructor_low = flags[0]; constructor_member = flags[2];
        sprintf(line, "entry %s symbol %08lx flags48 %02x flags50 %02x\n",
            constructor_kind, argument, (unsigned)constructor_low,
            (unsigned)constructor_member);
        WriteFile(constructor_log, line, strlen(line), &written, 0);
        constructor_read(process, constructor_return, &constructor_return_byte, 1);
        constructor_write(process, constructor_return, 0xcc);
        constructor_write(process, address, constructor_entry_byte);
    }
    context.Eip = address;
    if (!SetThreadContext(thread, &context)) exit(248);
}

#define C1XX_CLOSE_OBSERVER 1
#define capture_close_boundary observe_constructor_boundary
#include "debug-run.c"
