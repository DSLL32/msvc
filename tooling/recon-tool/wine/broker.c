/* Shared original-only Win32 debugger transport. It is test tooling, not
 * reconstructed application code. No per-function observers or history replay. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#define RECON_ORIGINAL_BROKER
#include "../include/recon-original-direct.h"
typedef LPVOID(WINAPI *AllocateRemote)(HANDLE, LPVOID, DWORD, DWORD, DWORD);
typedef BOOL(WINAPI *HandleFlags)(HANDLE, DWORD, DWORD);
typedef BOOL(WINAPI *FreeRemote)(HANDLE, LPVOID, DWORD, DWORD);
typedef struct Point
{
    DWORD address;
    BYTE saved;
    int active;
} Point;
typedef struct Registers
{
    DWORD eax, ebx, ecx, edx, esi, edi, ebp, esp, eip, eflags;
    BYTE fp[112];
} Registers;
static HANDLE process, thread, input, output, error_output, rpc_input, rpc_output;
static DWORD pid, tid, base, image_size, entry_rva, deadline, exit_status;
static DEBUG_EVENT event;
static int pending, exited, loader_break, target_dll;
static DWORD pending_status = DBG_CONTINUE;
static DWORD helper_base, helper_completion, helper_context;
static BY_HANDLE_FILE_INFORMATION helper_info;
static int typed_helper;
static DWORD helper_state_rva, helper_completion_rva, helper_marker_rva;
static BY_HANDLE_FILE_INFORMATION target_info;
static Point *points;
static DWORD point_count;
static AllocateRemote allocate_remote;
static FreeRemote free_remote;
static HandleFlags handle_flags;
static int transfer(HANDLE handle, void *bytes, DWORD size, int writing)
{
    DWORD done = 0, n;
    while (done < size)
    {
        if (!(writing
                  ? WriteFile(handle, (BYTE *)bytes + done, size - done, &n, NULL)
                  : ReadFile(handle, (BYTE *)bytes + done, size - done, &n, NULL)) ||
            !n)
            return 0;
        done += n;
    }
    return 1;
}
static int read_memory(DWORD address, void *bytes, DWORD size)
{
    DWORD got;
    return !size || (ReadProcessMemory(process, (void *)address, bytes, size, &got) &&
                     got == size);
}
static int write_memory(DWORD address, const void *bytes, DWORD size)
{
    DWORD got;
    return !size || (WriteProcessMemory(process, (void *)address, bytes, size, &got) &&
                     got == size);
}
static int patch(DWORD address, BYTE byte)
{
    DWORD old, ignored;
    int ok;
    if (!VirtualProtectEx(process, (void *)address, 1, PAGE_EXECUTE_READWRITE, &old))
        return 0;
    ok = write_memory(address, &byte, 1) &&
         FlushInstructionCache(process, (void *)address, 1);
    if (!VirtualProtectEx(process, (void *)address, 1, old, &ignored))
        ok = 0;
    return ok;
}
static int context(CONTEXT *c)
{
    memset(c, 0, sizeof(*c));
    c->ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
    return pending && GetThreadContext(thread, c);
}
static void registers(Registers *r, CONTEXT *c)
{
    r->eax = c->Eax;
    r->ebx = c->Ebx;
    r->ecx = c->Ecx;
    r->edx = c->Edx;
    r->esi = c->Esi;
    r->edi = c->Edi;
    r->ebp = c->Ebp;
    r->esp = c->Esp;
    r->eip = c->Eip;
    r->eflags = c->EFlags;
    memcpy(r->fp, &c->FloatSave, 112);
}
static int arm(DWORD address)
{
    Point *p;
    DWORD i;
    BYTE byte;
    for (i = 0; i < point_count; ++i)
        if (points[i].address == address)
        {
            if (points[i].active)
                return 1;
            points[i].active = 1;
            return patch(address, 0xcc);
        }
    if (!read_memory(address, &byte, 1) || byte == 0xcc)
        return 0;
    p = realloc(points, (point_count + 1) * sizeof(*points));
    if (!p)
        return 0;
    points = p;
    p += point_count++;
    p->address = address;
    p->saved = byte;
    p->active = 1;
    return patch(address, 0xcc);
}
/* Returns 0 exit, 1 owned breakpoint, 2 unowned INT3, 3 other exception,
 * -1 transport failure. A reported breakpoint is restored before replying. */
static int image(DWORD address)
{
    DWORD pe;
    if (!read_memory(address + 0x3c, &pe, 4) ||
        !read_memory(address + pe + 24 + 56, &image_size, 4))
        return 0;
    base = address;
    return arm(base + entry_rva);
}
static int file_matches(HANDLE file, const BY_HANDLE_FILE_INFORMATION *wanted)
{
    BY_HANDLE_FILE_INFORMATION actual;
    return GetFileInformationByHandle(file, &actual) &&
           actual.dwVolumeSerialNumber == wanted->dwVolumeSerialNumber &&
           actual.nFileIndexHigh == wanted->nFileIndexHigh &&
           actual.nFileIndexLow == wanted->nFileIndexLow;
}
static int next(DWORD *where, Registers *r)
{
    DWORD i, helper_pe, helper_size;
    CONTEXT c;
    for (;;)
    {
        if ((LONG)(GetTickCount() - deadline) >= 0)
            return -1;
        if (pending)
        {
            if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId,
                                    pending_status))
                return -1;
            pending = 0;
            pending_status = DBG_CONTINUE;
        }
        if (!WaitForDebugEvent(&event, 20))
            continue;
        pending = 1;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            if (event.dwProcessId != pid)
                return -1;
            if (!target_dll && !image((DWORD)event.u.CreateProcessInfo.lpBaseOfImage))
                return -1;
            if (event.u.CreateProcessInfo.hFile)
                CloseHandle(event.u.CreateProcessInfo.hFile);
        }
        else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT)
        {
            if (typed_helper && event.u.LoadDll.hFile &&
                file_matches(event.u.LoadDll.hFile, &helper_info))
            {
                if (helper_base)
                    return -1;
                helper_base = (DWORD)event.u.LoadDll.lpBaseOfDll;
                if (!read_memory(helper_base + 0x3c, &helper_pe, 4) ||
                    !read_memory(helper_base + helper_pe + 24 + 56, &helper_size, 4) ||
                    helper_state_rva >= helper_size ||
                    sizeof(ReconOriginalProbe) > helper_size - helper_state_rva ||
                    helper_completion_rva >= helper_size)
                    return -1;
                helper_context = helper_base + helper_state_rva;
                helper_completion = helper_base + helper_completion_rva;
            }
            if (target_dll && event.u.LoadDll.hFile)
            {
                BY_HANDLE_FILE_INFORMATION info;
                if (GetFileInformationByHandle(event.u.LoadDll.hFile, &info) &&
                    info.dwVolumeSerialNumber == target_info.dwVolumeSerialNumber &&
                    info.nFileIndexHigh == target_info.nFileIndexHigh &&
                    info.nFileIndexLow == target_info.nFileIndexLow)
                {
                    if (base || !image((DWORD)event.u.LoadDll.lpBaseOfDll))
                        return -1;
                }
            }
            if (event.u.LoadDll.hFile)
                CloseHandle(event.u.LoadDll.hFile);
        }
        else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT)
        { /* New threads cannot be controlled by this single-thread direct-call API. */
            return -1;
        }
        else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
        {
            exited = 1;
            exit_status = event.u.ExitProcess.dwExitCode;
            *where = exit_status;
            return 0;
        }
        else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            if (event.dwThreadId != tid || !context(&c))
                return -1;
            *where = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if (event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
            {
                if (typed_helper && *where == base + helper_marker_rva &&
                    event.u.Exception.dwFirstChance)
                {
                    pending_status = DBG_EXCEPTION_NOT_HANDLED;
                    registers(r, &c);
                    return 1;
                }
                if (typed_helper && *where == helper_completion)
                {
                    registers(r, &c);
                    return 2;
                }
                for (i = 0; i < point_count; ++i)
                    if (points[i].active && points[i].address == *where)
                    {
                        if (!patch(*where, points[i].saved))
                            return -1;
                        points[i].active = 0;
                        c.Eip = *where;
                        if (!SetThreadContext(thread, &c))
                            return -1;
                        registers(r, &c);
                        return 1;
                    }
                if (!loader_break &&
                    (!base || *where < base || *where - base >= image_size))
                {
                    loader_break = 1;
                    continue;
                }
                c.Eip = *where;
                registers(r, &c);
                return 2;
            }
            *where = event.u.Exception.ExceptionRecord.ExceptionCode;
            registers(r, &c);
            return 3;
        }
    }
}
static int response(DWORD ok, DWORD a, DWORD b, const void *bytes, DWORD size)
{
    DWORD h[4];
    h[0] = ok;
    h[1] = a;
    h[2] = b;
    h[3] = size;
    return transfer(rpc_output, h, sizeof(h), 1) &&
           transfer(rpc_output, (void *)bytes, size, 1);
}
static int run_until(DWORD rva, Registers *r)
{
    DWORD where;
    int kind;
    CONTEXT c;
    if (!context(&c))
        return 0;
    if (c.Eip == base + rva)
    {
        registers(r, &c);
        return 1;
    }
    if (!arm(base + rva))
        return 0;
    kind = next(&where, r);
    return kind == 1 && where == base + rva;
}
static int typed_completed(void)
{
    ReconOriginalProbe probe;
    BYTE *bytes;
    int result;
    MEMORY_BASIC_INFORMATION allocation;
    if (!helper_context || !read_memory(helper_context, &probe, sizeof(probe)) ||
        probe.abi_version != RECON_ORIGINAL_PROBE_VERSION ||
        probe.struct_size != sizeof(probe) || probe.image_base != base ||
        probe.completed != RECON_ORIGINAL_PROBE_COMPLETE || probe.passed > 1 ||
        probe.marker_rva != helper_marker_rva || !probe.original_esp ||
        !VirtualQueryEx(process, probe.result_buffer, &allocation,
                        sizeof(allocation)) ||
        allocation.AllocationBase != probe.result_buffer ||
        allocation.State != MEM_COMMIT || allocation.Protect != PAGE_READWRITE ||
        allocation.RegionSize < probe.result_capacity ||
        probe.result_capacity != 8 * 1024 * 1024 ||
        probe.result_size > probe.result_capacity || probe.result_count > 100000)
        return response(0, 0, 0, NULL, 0);
    bytes = malloc(probe.result_size + 4);
    if (!bytes)
        return response(0, 0, 0, NULL, 0);
    memcpy(bytes, &probe.result_count, 4);
    result = read_memory((DWORD)probe.result_buffer, bytes + 4, probe.result_size) &&
             response(1, 4, probe.passed, bytes, probe.result_size + 4);
    free(bytes);
    return result;
}

static void stop(void)
{
    DWORD started = GetTickCount();
    if (process && !exited)
        TerminateProcess(process, 125);
    if (pending)
    {
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE);
        pending = 0;
    }
    while (process && !exited && GetTickCount() - started < 3000)
    {
        if (!WaitForDebugEvent(&event, 20))
            continue;
        if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
            exited = 1;
        if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT && event.u.LoadDll.hFile)
            CloseHandle(event.u.LoadDll.hFile);
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE);
    }
}
int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION launch;
    SECURITY_ATTRIBUTES sa;
    DWORD where, h[5], i, address, frame_size, stack, site, trace_count, *trace_words;
    BYTE *data = NULL, *reply, trap = 0xcc;
    Registers r, call_result[2];
    CONTEXT before, c;
    int kind, ok = 0;
    char *command;
    if (argc != 10 && argc != 14)
        return 2;
    typed_helper = argc == 14;
    rpc_input = GetStdHandle(STD_INPUT_HANDLE);
    rpc_output = GetStdHandle(STD_OUTPUT_HANDLE);
    handle_flags = (HandleFlags)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                               "SetHandleInformation");
    if (!handle_flags || !handle_flags(rpc_input, 1, 0) ||
        !handle_flags(rpc_output, 1, 0))
        return 3;
    target_dll = strcmp(argv[9], "-") != 0;
    if (target_dll)
    {
        HANDLE module =
            CreateFileA(argv[9], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, 0, NULL);
        if (module == INVALID_HANDLE_VALUE)
            return 3;
        ok = GetFileInformationByHandle(module, &target_info);
        CloseHandle(module);
        if (!ok)
            return 3;
    }
    if (typed_helper)
    {
        HANDLE module =
            CreateFileA(argv[10], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, 0, NULL);
        if (module == INVALID_HANDLE_VALUE)
            return 3;
        ok = GetFileInformationByHandle(module, &helper_info);
        CloseHandle(module);
        helper_state_rva = strtoul(argv[11], NULL, 0);
        helper_completion_rva = strtoul(argv[12], NULL, 0);
        helper_marker_rva = strtoul(argv[13], NULL, 0);
        if (!ok || target_dll || !helper_state_rva || !helper_completion_rva ||
            !helper_marker_rva)
            return 3;
    }
    entry_rva = strtoul(argv[2], NULL, 0);
    deadline = GetTickCount() + strtoul(argv[8], NULL, 0);
    allocate_remote = (AllocateRemote)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                                     "VirtualAllocEx");
    free_remote =
        (FreeRemote)GetProcAddress(GetModuleHandleA("kernel32.dll"), "VirtualFreeEx");
    if (!allocate_remote || !free_remote)
        return 3;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    input = CreateFileA(argv[4], GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING, 0,
                        NULL);
    output = CreateFileA(argv[5], GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, 0,
                         NULL);
    error_output = CreateFileA(argv[6], GENERIC_WRITE, FILE_SHARE_READ, &sa,
                               CREATE_ALWAYS, 0, NULL);
    if (input == INVALID_HANDLE_VALUE || output == INVALID_HANDLE_VALUE ||
        error_output == INVALID_HANDLE_VALUE)
        return 4;
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input;
    startup.hStdOutput = output;
    startup.hStdError = error_output;
    command = malloc(strlen(argv[1]) + 1);
    if (!command)
        return 5;
    strcpy(command, argv[1]);
    if (!CreateProcessA(argv[7], command, NULL, NULL, TRUE, DEBUG_ONLY_THIS_PROCESS,
                        NULL, argv[3], &startup, &launch))
        return 6;
    process = launch.hProcess;
    thread = launch.hThread;
    pid = launch.dwProcessId;
    tid = launch.dwThreadId;
    kind = next(&where, &r);
    loader_break = 1;
    if (kind != 1 || (typed_helper && !helper_base) || where != base + entry_rva ||
        !response(1, base, entry_rva, &r, sizeof(r)))
        goto done;
    while (transfer(rpc_input, h, sizeof(h), 0))
    {
        free(data);
        data = NULL;
        if (h[4] > 16777216UL)
            break;
        if (h[4])
        {
            data = malloc(h[4]);
            if (!data || !transfer(rpc_input, data, h[4], 0))
                break;
        }
        ok = 0;
        address = 0;
        /* Typed probes execute their own callback locally. Only reads, normal
         * frontier breakpoints, continuation and termination are accepted. */
        if (typed_helper && h[0] != 1 && h[0] != 6 && h[0] != 7 && h[0] != 9)
            break;
        if (h[0] == 1)
        {
            free(data);
            data = malloc(h[2] ? h[2] : 1);
            ok = h[2] <= 16777216UL && data && read_memory(h[1], data, h[2]);
            if (!response(ok, 0, 0, data, ok ? h[2] : 0))
                break;
            continue;
        }
        if (h[0] == 2)
        {
            ok = write_memory(h[1], data, h[4]);
        }
        else if (h[0] == 3)
        {
            address = (DWORD)allocate_remote(process, NULL, h[1],
                                             MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            ok = address != 0;
        }
        else if (h[0] == 4)
        {
            ok = free_remote(process, (void *)h[1], 0, MEM_RELEASE);
        }
        else if (h[0] == 5)
        {
            ok = run_until(h[1], &r);
            if (!response(ok, 0, 0, &r, ok ? sizeof(r) : 0))
                break;
            continue;
        }
        else if (h[0] == 6)
        {
            if (h[4] % 4)
                break;
            ok = 1;
            for (i = 0; i < h[4] / 4; ++i)
                if (!arm(base + ((DWORD *)data)[i]))
                {
                    BYTE byte;
                    if (!read_memory(base + ((DWORD *)data)[i], &byte, 1) ||
                        byte != 0xcc)
                    {
                        ok = 0;
                        break;
                    }
                }
        }
        else if (h[0] == 7)
        {
            kind = next(&where, &r);
            ok = kind >= 0;
            if (kind == 2 && typed_helper && helper_context &&
                where == helper_completion)
            {
                if (!typed_completed())
                    break;
                continue;
            }
            if (!response(ok, (DWORD)kind,
                          kind == 0 || kind == 3 ? where : where - base, &r,
                          kind > 0 ? sizeof(r) : 0))
                break;
            continue;
        }
        else if (h[0] == 8)
        {
            if (h[4] % 4 || h[4] > 4096 || !context(&before))
                break;
            stack = (DWORD)allocate_remote(process, NULL, 65536,
                                           MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            site = (DWORD)allocate_remote(process, NULL, 4096, MEM_COMMIT | MEM_RESERVE,
                                          PAGE_EXECUTE_READWRITE);
            if (!stack || !site || !write_memory(site, &trap, 1))
                break;
            frame_size = h[4] + 4;
            address = stack + 65536 - frame_size;
            if (!write_memory(address, &site, 4) ||
                !write_memory(address + 4, data, h[4]))
                break;
            c = before;
            c.Eip = base + h[1];
            c.Esp = address;
            c.Ecx = h[2];
            c.Edx = h[3];
            trace_words = malloc((point_count + 2) * 4);
            trace_count = 0;
            if (!trace_words)
                break;
            if (!arm(base + h[1]) || !SetThreadContext(thread, &c))
            {
                free(trace_words);
                break;
            }
            kind = next(&where, &call_result[0]);
            if (kind != 1 || where != base + h[1])
            {
                free(trace_words);
                break;
            }
            trace_words[trace_count++] = where - base;
            for (;;)
            {
                kind = next(&where, &call_result[1]);
                if (kind != 1)
                    break;
                trace_words[trace_count++] = where - base;
            }
            ok = kind == 2 && where == site;
            if (!SetThreadContext(thread, &before))
                ok = 0;
            free_remote(process, (void *)stack, 0, MEM_RELEASE);
            free_remote(process, (void *)site, 0, MEM_RELEASE);
            reply = malloc(sizeof(call_result) + 4 + trace_count * 4);
            if (!reply)
            {
                free(trace_words);
                break;
            }
            memcpy(reply, call_result, sizeof(call_result));
            memcpy(reply + sizeof(call_result), &trace_count, 4);
            memcpy(reply + sizeof(call_result) + 4, trace_words, trace_count * 4);
            free(trace_words);
            i = response(ok, 0, 0, reply,
                         ok ? sizeof(call_result) + 4 + trace_count * 4 : 0);
            free(reply);
            if (!i)
                break;
            continue;
        }
        else if (h[0] == 9)
        {
            response(1, 0, 0, NULL, 0);
            ok = 1;
            goto done;
        }
        else
            break;
        if (!response(ok, address, 0, NULL, 0))
            break;
    }
done:
    stop();
    free(data);
    free(points);
    return ok ? 0 : 7;
}
