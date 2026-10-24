/* Original-only _getcwd boundary observer. Never linked into the target
   application or invoked by native fixtures. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static DWORD cwd_return, cwd_buffer;
static HANDLE cwd_log;

static void observe_cwd(HANDLE process, HANDLE thread, const char *stage,
    CONTEXT *context)
{
    unsigned char bytes[256];
    DWORD got, written, errors[2], pool[2], teb, self, last_error, i;
    LDT_ENTRY selector;
    char line[1800];
    if (!ReadProcessMemory(process, (void *)cwd_buffer, bytes, 256, &got) || got != 256) exit(240);
    if (!ReadProcessMemory(process, (void *)0xaaa4c, errors, 8, &got) || got != 8) exit(241);
    if (!ReadProcessMemory(process, (void *)0xa2778, pool, 8, &got) || got != 8) exit(242);
    if (!GetThreadSelectorEntry(thread, context->SegFs, &selector)) exit(243);
    teb = selector.BaseLow | ((DWORD)selector.HighWord.Bytes.BaseMid << 16) |
        ((DWORD)selector.HighWord.Bytes.BaseHi << 24);
    /* The installed Wine kernelbase GetLastError export reads FS:[18h],
       then that TEB's DWORD+34h. Verify the self link before this read. */
    if (!ReadProcessMemory(process, (void *)(teb + 0x18), &self, 4, &got) || got != 4 || self != teb) exit(244);
    if (!ReadProcessMemory(process, (void *)(teb + 0x34), &last_error, 4, &got) || got != 4) exit(245);
    sprintf(line, "%s errno %lu doserrno %lu last_error %lu control %04lx status %04lx tag %04lx next %08lx remaining %ld bytes ",
        stage, errors[0], errors[1], last_error,
        context->FloatSave.ControlWord & 0xffff, context->FloatSave.StatusWord & 0xffff,
        context->FloatSave.TagWord & 0xffff, pool[0], (long)pool[1]);
    for (i = 0; i < 256; ++i) sprintf(line + strlen(line), "%02x", bytes[i]);
    strcat(line, "\n");
    if (!WriteFile(cwd_log, line, strlen(line), &written, 0) || written != strlen(line)) exit(246);
}

static void capture_cwd_boundary(HANDLE process, HANDLE thread, DWORD base,
    DWORD address, DWORD code)
{
    CONTEXT context;
    DWORD got, written, words[3], start, offset, length;
    unsigned char body[32], trap = 0xcc;
    char path[1024], line[256], name[128];
    HANDLE original;
    FILE *plan;
    if (base != 0x10000 || code != EXCEPTION_BREAKPOINT) exit(250);
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
    if (!GetThreadContext(thread, &context)) exit(251);
    if (cwd_return) {
        if (address != cwd_return) exit(252);
        sprintf(line, "returned_buffer %lu\n", (DWORD)(context.Eax == cwd_buffer));
        WriteFile(cwd_log, line, strlen(line), &written, 0);
        observe_cwd(process, thread, "after", &context);
        CloseHandle(cwd_log);
        TerminateProcess(process, 99);
        return;
    }
    if (!GetEnvironmentVariableA("C1XX_CWD_PLAN", path, sizeof(path))) exit(253);
    plan = fopen(path, "r");
    if (!plan || fscanf(plan, "%lx %lx %lx %127s", &start, &offset, &length, name) != 4) exit(254);
    fclose(plan);
    if (address != start || start != 0x87a40 || strcmp(name, "__getcwd") || length != sizeof(body)) exit(255);
    if (!ReadProcessMemory(process, (void *)context.Esp, words, sizeof(words), &got) || got != sizeof(words)) exit(256);
    cwd_return = words[0]; cwd_buffer = words[1];
    if (cwd_return != 0x69ae8 || words[2] != 256 || !cwd_buffer) exit(257);
    if (!GetEnvironmentVariableA("C1XX_CWD_LOG", path, sizeof(path))) exit(258);
    cwd_log = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (cwd_log == INVALID_HANDLE_VALUE) exit(259);
    sprintf(line, "runtime __getcwd\nreturn_va %08lx\nbuffer %08lx\ncapacity %lu\n", cwd_return, cwd_buffer, words[2]);
    WriteFile(cwd_log, line, strlen(line), &written, 0);
    observe_cwd(process, thread, "before", &context);
    if (!WriteProcessMemory(process, (void *)cwd_return, &trap, 1, &got) || got != 1) exit(260);
    FlushInstructionCache(process, (void *)cwd_return, 1);
    if (!GetEnvironmentVariableA("C1XX_CWD_IMAGE", path, sizeof(path))) exit(261);
    original = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (original == INVALID_HANDLE_VALUE || SetFilePointer(original, offset, 0, FILE_BEGIN) != offset) exit(262);
    if (!ReadFile(original, body, length, &got, 0) || got != length) exit(263);
    CloseHandle(original);
    if (!WriteProcessMemory(process, (void *)address, body, length, &got) || got != length) exit(264);
    FlushInstructionCache(process, (void *)address, length);
    context.Eip = address;
    if (!SetThreadContext(thread, &context)) exit(265);
}

#define C1XX_CLOSE_OBSERVER 1
#define capture_close_boundary capture_cwd_boundary
#include "debug-run.c"
