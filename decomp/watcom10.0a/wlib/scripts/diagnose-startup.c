/* Original-only diagnostic harness, not reconstructed WLIB code.
   One-shot breakpoint pattern follows vc1.0/cl/src/trace-run.c. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

typedef struct Site { DWORD rva, address; BYTE saved, expected; int hit, cf, guarded; char name[64]; } Site;
typedef struct Thread { DWORD id; HANDLE handle; } Thread;
static Site sites[64];
static Thread threads[64];
static unsigned site_count, thread_count;
static DWORD reads[32], read_count, image_base;
static DWORD ntdll_base, ntdll_size;
static DEBUG_EVENT pending_event;
static int event_pending, pe_entry_seen;
static DWORD cf_base_slot, cf_entry_slot, cf_entry_offset, cf_limit, cf_relocation_count;
static DWORD cf_relocations[10000][2];
static HANDLE child;
static FILE *events;

static int memory(DWORD at, void *bytes, DWORD count)
{
    DWORD got;
    return ReadProcessMemory(child, (void *)at, bytes, count, &got) && got == count;
}

static int patch(DWORD at, BYTE value)
{
    DWORD old, restored, wrote;
    int ok;
    if (!VirtualProtectEx(child, (void *)at, 1, PAGE_EXECUTE_READWRITE, &old)) return 0;
    ok = WriteProcessMemory(child, (void *)at, &value, 1, &wrote) && wrote == 1;
    if (!FlushInstructionCache(child, (void *)at, 1)) ok = 0;
    if (!VirtualProtectEx(child, (void *)at, 1, old, &restored)) ok = 0;
    return ok;
}

static void bytes_at(const char *kind, DWORD address, DWORD size)
{
    BYTE bytes[512];
    DWORD i;
    if (size > sizeof(bytes)) { fprintf(events, "%s span-held=%lu\n", kind, size); return; }
    if (!memory(address, bytes, size)) { fprintf(events, "%s read-failed=%08lx size=%lu\n", kind, address, size); return; }
    fprintf(events, "%s address=%08lx size=%lu bytes=", kind, address, size);
    for (i = 0; i < size; ++i) fprintf(events, "%02x", bytes[i]);
    fprintf(events, "\n");
}

static int install_site(unsigned i, DWORD address)
{
    if (!memory(address, &sites[i].saved, 1)) return 0;
    if (sites[i].guarded && sites[i].saved != sites[i].expected) {
        fprintf(events, "site-byte-held name=%s address=%08lx actual=%02x expected=%02x\n",
                sites[i].name, address, sites[i].saved, sites[i].expected);
        return 0;
    }
    sites[i].address = address;
    return patch(address, 0xcc);
}

static int install_cf(const CONTEXT *context)
{
    DWORD base, entry, value, i;
    if (!cf_relocation_count) return 1;
    if (!memory(image_base + cf_base_slot, &base, 4) ||
        !memory(image_base + cf_entry_slot, &entry, 4) || !base ||
        cf_entry_offset >= cf_limit || base + cf_entry_offset < base ||
        entry != base + cf_entry_offset || context->Esi != entry) {
        fprintf(events, "cf-identity-held\n"); return 0;
    }
    fprintf(events, "cf-observed-base=%08lx entry=%08lx esi=%08lx expected-image-limit=%08lx\n",
            base, entry, context->Esi, cf_limit);
    for (i = 0; i < cf_relocation_count; ++i) {
        if (cf_relocations[i][0] > cf_limit - 4 ||
            !memory(base + cf_relocations[i][0], &value, 4) ||
            value != base + cf_relocations[i][1]) {
            fprintf(events, "cf-relocation-held index=%lu rva=%08lx\n", i, cf_relocations[i][0]);
            return 0;
        }
    }
    fprintf(events, "cf-all-relocations-verified count=%lu\n", cf_relocation_count);
    for (i = 0; i < site_count; ++i) if (sites[i].cf) {
        if (!sites[i].guarded || sites[i].rva >= cf_limit ||
            !install_site(i, base + sites[i].rva)) return 0;
    }
    return 1;
}

static void resource_state(const char *name, const CONTEXT *context)
{
    DWORD pointer, capacity, caller;
    WORD selector;
    if (!strncmp(name, "owner_", 6)) {
        fprintf(events, "owner-call name=%s ebp=%08lx esp=%08lx edx-buffer=%08lx\n",
                name, context->Ebp, context->Esp, context->Edx);
        bytes_at("owner-buffer-raw", !strncmp(name, "owner_stack", 11) ? context->Esp : context->Edx, 128);
    }
    if (strncmp(name, "resource_", 9)) return;
    bytes_at("resource-frame", context->Ebp - 0x1c, 0x40);
    if (!memory(context->Ebp + 0x14, &pointer, 4) ||
        !memory(context->Ebp + 0x18, &selector, 2) ||
        !memory(context->Ebp + 0x1c, &capacity, 4) ||
        !memory(context->Ebp + 0x10, &caller, 4)) return;
    fprintf(events, "resource-far-buffer pointer=%08lx selector=%04x capacity=%lu caller-return=%08lx ds=%04lx gs=%04lx\n",
            pointer, selector, capacity, caller, context->SegDs, context->SegGs);
    bytes_at("resource-buffer-raw", pointer, capacity);
    /* Mapping metadata is descriptive only; it does not prove object ownership. */
    {
        MEMORY_BASIC_INFORMATION region;
        if (VirtualQueryEx(child, (void *)pointer, &region, sizeof(region)))
            fprintf(events, "resource-mapping-descriptive-only base=%08lx allocation=%08lx size=%lu protect=%08lx\n",
                    (DWORD)region.BaseAddress, (DWORD)region.AllocationBase,
                    region.RegionSize, region.Protect);
    }
}

static void registers(HANDLE handle, const char *why, DWORD tid, int suspend)
{
    CONTEXT context;
    DWORD stack[6];
    unsigned i;
    int paused = 0;
    if (suspend) {
        if (SuspendThread(handle) == 0xffffffffUL) {
            fprintf(events, "suspend-failed tid=%lu error=%lu\n", tid, GetLastError());
            return;
        }
        paused = 1;
    }
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (GetThreadContext(handle, &context)) {
        fprintf(events, "%s tid=%lu eip=%08lx esp=%08lx eax=%08lx edx=%08lx ebx=%08lx ecx=%08lx esi=%08lx edi=%08lx ebp=%08lx cs=%04lx ds=%04lx es=%04lx fs=%04lx gs=%04lx\n",
                why, tid, context.Eip, context.Esp, context.Eax, context.Edx,
                context.Ebx, context.Ecx, context.Esi, context.Edi, context.Ebp,
                context.SegCs, context.SegDs, context.SegEs, context.SegFs, context.SegGs);
        if (memory(context.Esp, stack, sizeof(stack))) {
            fprintf(events, "raw-stack");
            for (i = 0; i < 6; ++i) fprintf(events, " %08lx", stack[i]);
            fprintf(events, "\n");
        }
    } else fprintf(events, "context-failed tid=%lu error=%lu\n", tid, GetLastError());
    if (paused && ResumeThread(handle) == 0xffffffffUL)
        fprintf(events, "resume-failed tid=%lu error=%lu\n", tid, GetLastError());
    fflush(events);
}

static void module_name(DWORD base)
{
    DWORD pe, directory, name, i;
    char text[256];
    if (!memory(base + 0x3c, &pe, 4) || pe > 4096 ||
        !memory(base + pe + 24 + 96, &directory, 4) || !directory ||
        !memory(base + directory + 12, &name, 4)) return;
    for (i = 0; i + 1 < sizeof(text); ++i) {
        if (!memory(base + name + i, text + i, 1)) return;
        if (!text[i]) {
            fprintf(events, "module-export-name base=%08lx name=%s\n", base, text);
            if (!_stricmp(text, "ntdll.dll") && memory(base + pe + 24 + 56, &ntdll_size, 4)) {
                ntdll_base = base;
                fprintf(events, "loader-module base=%08lx size=%08lx\n", ntdll_base, ntdll_size);
            }
            return;
        }
    }
}

static int fail_owned(DWORD status)
{
    DWORD actual = STILL_ACTIVE, began;
    DEBUG_EVENT event;
    int exited = 0;
    if (child) {
        fprintf(events, "terminate-owned-child status=%lu result=%d\n", status,
                TerminateProcess(child, status));
        if (event_pending) {
            fprintf(events, "release-pending-event pid=%lu tid=%lu result=%d\n",
                    pending_event.dwProcessId, pending_event.dwThreadId,
                    ContinueDebugEvent(pending_event.dwProcessId, pending_event.dwThreadId, DBG_CONTINUE));
            event_pending = 0;
        }
        began = GetTickCount();
        while (GetTickCount() - began < 5000) {
            if (!WaitForDebugEvent(&event, 100)) continue;
            fprintf(events, "cleanup-event code=%lu pid=%lu tid=%lu\n",
                    event.dwDebugEventCode, event.dwProcessId, event.dwThreadId);
            if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
                actual = event.u.ExitProcess.dwExitCode; exited = 1;
                fprintf(events, "owned-exit-confirmed status=%lu\n", actual);
            }
            if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT && event.u.LoadDll.hFile)
                CloseHandle(event.u.LoadDll.hFile);
            ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE);
            if (exited) break;
        }
        GetExitCodeProcess(child, &actual);
        fprintf(events, "failure-owned-child exited-event=%d signaled=%d status=%lu\n",
                exited, WaitForSingleObject(child, 0) == WAIT_OBJECT_0, actual);
    }
    fflush(events);
    return (int)status;
}

int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;
    CONTEXT context;
    DWORD base = 0, start, status, code, address, wrote, target_status = 0, value;
    unsigned i, t;
    int loader_break = 0, timeout = 0;
    HANDLE input_read, input_write, out, err;
    FILE *site_file;
    char command[4096], line[256], coordinate[64];
    unsigned expected, parsed;
    if (argc != 12) return 120;
    /* target, raw arguments, child cwd, stdout, stderr, events, RVA list,
       timeout-ms, stdin-newline-count (or @regular-file), read-only DWORD RVA list (or -),
       CF proof config or -. CF config header: base-slot entry-slot entry-offset limit count;
       followed by exactly count rows of hex RVA and unrelocated DWORD. */
    if (argv[9][0] != '@' && strtoul(argv[9], NULL, 10) > 64) return 120;
    site_file = fopen(argv[7], "rt");
    if (!site_file) return 121;
    while (site_count < 64 && fgets(line, sizeof(line), site_file)) {
        expected = 0;
        parsed = sscanf(line, "%63s %63s %x", coordinate, sites[site_count].name, &expected);
        if (parsed < 2) { fclose(site_file); return 121; }
        sites[site_count].cf = !strncmp(coordinate, "cf:", 3);
        sites[site_count].rva = strtoul(coordinate + (sites[site_count].cf ? 3 : 0), NULL, 16);
        sites[site_count].guarded = parsed == 3;
        if (parsed == 3 && expected > 255) { fclose(site_file); return 121; }
        sites[site_count].expected = (BYTE)expected;
        ++site_count;
    }
    if (!feof(site_file)) { fclose(site_file); return 121; }
    fclose(site_file);
    if (strcmp(argv[10], "-")) {
        site_file = fopen(argv[10], "rt");
        if (!site_file) return 121;
        while (read_count < 32 && fscanf(site_file, "%lx", &reads[read_count]) == 1) ++read_count;
        if (!feof(site_file)) { fclose(site_file); return 121; }
        fclose(site_file);
    }
    if (strcmp(argv[11], "-")) {
        site_file = fopen(argv[11], "rt");
        if (!site_file || fscanf(site_file, "%lx %lx %lx %lx %lu", &cf_base_slot,
                &cf_entry_slot, &cf_entry_offset, &cf_limit, &cf_relocation_count) != 5 ||
                !cf_relocation_count || cf_relocation_count > 10000 || cf_limit < 4) return 121;
        for (i = 0; i < cf_relocation_count; ++i)
            if (fscanf(site_file, "%lx %lx", &cf_relocations[i][0], &cf_relocations[i][1]) != 2) return 121;
        if (fscanf(site_file, "%lx", &value) != EOF) return 121;
        fclose(site_file);
    }
    events = fopen(argv[6], "wb");
    if (!events) return 122;
    fprintf(events, "helper-start tick=%lu target=%s arguments=%s cwd=%s\n",
            GetTickCount(), argv[1], argv[2], argv[3]);
    fflush(events);
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security); security.bInheritHandle = TRUE;
    if (argv[9][0] == '@') {
        input_read = CreateFileA(argv[9] + 1, GENERIC_READ, FILE_SHARE_READ,
                                 &security, OPEN_EXISTING, 0, NULL);
        if (input_read == INVALID_HANDLE_VALUE) return 123;
    } else {
        if (!CreatePipe(&input_read, &input_write, &security, 0)) return 123;
        if (!SetHandleInformation(input_write, HANDLE_FLAG_INHERIT, 0)) return 123;
        memset(line, '\n', sizeof(line));
        value = strtoul(argv[9], NULL, 10);
        if (value && (!WriteFile(input_write, line, value, &wrote, NULL) || wrote != value)) return 123;
        CloseHandle(input_write);
    }
    fprintf(events, "actual-stdio-types stdin=%lu stdout-pending stderr-pending\n", GetFileType(input_read));
    out = CreateFileA(argv[4], GENERIC_WRITE, FILE_SHARE_READ, &security, CREATE_NEW, 0, NULL);
    err = CreateFileA(argv[5], GENERIC_WRITE, FILE_SHARE_READ, &security, CREATE_NEW, 0, NULL);
    if (out == INVALID_HANDLE_VALUE || err == INVALID_HANDLE_VALUE) return 124;
    fprintf(events, "actual-stdio-types stdout=%lu stderr=%lu\n", GetFileType(out), GetFileType(err));
    memset(&startup, 0, sizeof(startup)); startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input_read; startup.hStdOutput = out; startup.hStdError = err;
    sprintf(command, "\"%s\" %s", argv[1], argv[2]);
    if (!CreateProcessA(argv[1], command, NULL, NULL, TRUE, DEBUG_ONLY_THIS_PROCESS,
                        NULL, argv[3], &startup, &process)) {
        fprintf(events, "create-failed error=%lu\n", GetLastError()); fclose(events); return 125;
    }
    child = process.hProcess;
    fprintf(events, "owned-child pid=%lu tid=%lu command=%s\n", process.dwProcessId,
            process.dwThreadId, command); fflush(events);
    CloseHandle(input_read); CloseHandle(out); CloseHandle(err);
    start = GetTickCount();
    for (;;) {
        /* Deadline applies even when exception/debug events arrive continuously. */
        if (GetTickCount() - start >= strtoul(argv[8], NULL, 10)) {
            fprintf(events, "timeout elapsed=%lu\n", GetTickCount() - start);
            for (t = 0; t < thread_count; ++t) if (threads[t].id)
                registers(threads[t].handle, "timeout-context", threads[t].id, 1);
            return fail_owned(126);
        }
        if (!WaitForDebugEvent(&event, 100)) continue;
        pending_event = event; event_pending = 1;
        fprintf(events, "event code=%lu pid=%lu tid=%lu elapsed=%lu\n",
                event.dwDebugEventCode, event.dwProcessId, event.dwThreadId, GetTickCount() - start);
        status = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            image_base = base;
            threads[thread_count].id = event.dwThreadId;
            threads[thread_count++].handle = event.u.CreateProcessInfo.hThread;
            fprintf(events, "create base=%08lx entry=%08lx teb=%08lx\n", base,
                    (DWORD)event.u.CreateProcessInfo.lpStartAddress,
                    (DWORD)event.u.CreateProcessInfo.lpThreadLocalBase);
            for (i = 0; i < site_count; ++i) if (!sites[i].cf) {
                address = base + sites[i].rva;
                if (!install_site(i, address)) return fail_owned(127);
            }
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            if (thread_count == 64) return fail_owned(128);
            threads[thread_count].id = event.dwThreadId;
            threads[thread_count++].handle = event.u.CreateThread.hThread;
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            fprintf(events, "load-module base=%08lx file=%08lx image-name-pointer=%08lx unicode=%u\n",
                    (DWORD)event.u.LoadDll.lpBaseOfDll, (DWORD)event.u.LoadDll.hFile,
                    (DWORD)event.u.LoadDll.lpImageName, event.u.LoadDll.fUnicode);
            module_name((DWORD)event.u.LoadDll.lpBaseOfDll);
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT) {
            for (t = 0; t < thread_count; ++t) if (threads[t].id == event.dwThreadId) {
                threads[t].id = 0; CloseHandle(threads[t].handle); break;
            }
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = event.u.Exception.ExceptionRecord.ExceptionCode;
            address = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            fprintf(events, "exception code=%08lx address=%08lx first=%lu\n",
                    code, address, event.u.Exception.dwFirstChance);
            for (t = 0; t < thread_count; ++t) if (threads[t].id == event.dwThreadId) break;
            if (t == thread_count) return fail_owned(129);
            registers(threads[t].handle, "exception-context", event.dwThreadId, 0);
            for (i = 0; i < site_count; ++i)
                if (sites[i].address && address == sites[i].address && !sites[i].hit) break;
            if (code == EXCEPTION_BREAKPOINT && i < site_count) {
                fprintf(events, "one-shot-site name=%s rva=%08lx\n", sites[i].name, sites[i].rva);
                if (!strcmp(sites[i].name, "pe_entry")) pe_entry_seen = 1;
                memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL;
                if (!GetThreadContext(threads[t].handle, &context)) return fail_owned(130);
                resource_state(sites[i].name, &context);
                if (!strcmp(sites[i].name, "cf_launch") && !install_cf(&context)) return fail_owned(134);
                for (value = 0; value < read_count; ++value) {
                    DWORD observed;
                    if (memory(image_base + reads[value], &observed, 4))
                        fprintf(events, "read-only-dword rva=%08lx value=%08lx\n", reads[value], observed);
                    else fprintf(events, "read-only-dword-failed rva=%08lx\n", reads[value]);
                }
                if (!patch(address, sites[i].saved)) return fail_owned(130);
                sites[i].hit = 1;
                memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL;
                if (!GetThreadContext(threads[t].handle, &context)) return fail_owned(130);
                context.Eip = address;
                if (!SetThreadContext(threads[t].handle, &context)) return fail_owned(130);
            } else if (code == EXCEPTION_BREAKPOINT && !loader_break && !pe_entry_seen &&
                       event.u.Exception.dwFirstChance && ntdll_base && address >= ntdll_base &&
                       address - ntdll_base < ntdll_size) {
                BYTE opcode;
                if (!memory(address, &opcode, 1) || opcode != 0xcc) return fail_owned(133);
                fprintf(events, "initial-loader-break classified-ntdll-rva=%08lx\n", address - ntdll_base);
                loader_break = 1;
            } else if (code == EXCEPTION_BREAKPOINT) {
                fprintf(events, "unclassified-breakpoint-held address=%08lx\n", address);
                return fail_owned(133);
            }
            else status = DBG_EXCEPTION_NOT_HANDLED;
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            target_status = event.u.ExitProcess.dwExitCode;
            fprintf(events, "exit status=%lu\n", target_status);
            ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status);
            event_pending = 0;
            break;
        }
        fflush(events);
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status)) return fail_owned(131);
        event_pending = 0;
    }
    fprintf(events, "diagnostic-end timeout=%d child-status=%lu elapsed=%lu\n",
            timeout, target_status, GetTickCount() - start);
    for (t = 0; t < thread_count; ++t) if (threads[t].id) CloseHandle(threads[t].handle);
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    fclose(events);
    return timeout ? 126 : 0;
}
