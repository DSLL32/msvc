/* Shared original-only Win32 debugger transport. It is test tooling, not
 * reconstructed application code. No per-function observers or history replay. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "environment.h"
#include "loader-break.h"
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
static DWORD helper_callback_rva, helper_callback, callback_entered, marker_seen;
static BY_HANDLE_FILE_INFORMATION target_info, provider_info;
static int provider_required, provider_seen;
static Point *points;
static DWORD point_count;
/* Explicit PE SELF mode owns the root, one same-image child, and at most one
 * rejected create held only for teardown. Default transport keeps its state. */
typedef struct ProcessState
{
    HANDLE process, thread;
    DWORD pid, tid, base, image_size, exit_status;
    int loader_break, exited, provider_seen, entry_seen, qualified, forced;
    Point *points;
    DWORD point_count;
} ProcessState;
static ProcessState family[3];
static unsigned family_count, owner_index;
static DWORD root_pid, *trace_rvas, self_trace_count;
static int self_mode, entry_seen, trace_configured;
static void save_owner(void)
{
    ProcessState *p;
    if (!self_mode || !family_count) return;
    p = &family[owner_index];
    p->process = process; p->thread = thread;
    p->pid = pid; p->tid = tid; p->base = base; p->image_size = image_size;
    p->exit_status = exit_status; p->loader_break = loader_break;
    p->exited = exited; p->provider_seen = provider_seen; p->entry_seen = entry_seen;
    p->points = points; p->point_count = point_count;
}
static int select_owner(DWORD wanted)
{
    unsigned i;
    save_owner();
    for (i = 0; i < family_count; ++i) if (family[i].pid == wanted)
    {
        ProcessState *p = &family[i];
        owner_index = i;
        process = p->process; thread = p->thread;
        pid = p->pid; tid = p->tid; base = p->base; image_size = p->image_size;
        exit_status = p->exit_status; loader_break = p->loader_break;
        exited = p->exited; provider_seen = p->provider_seen; entry_seen = p->entry_seen;
        points = p->points; point_count = p->point_count;
        return 1;
    }
    return 0;
}

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
static int arm_trace(void);
static int image(DWORD address)
{
    DWORD pe;
    if (!read_memory(address + 0x3c, &pe, 4) ||
        !read_memory(address + pe + 24 + 56, &image_size, 4))
        return 0;
    base = address;
    if (!arm(base + entry_rva)) return 0;
    return !self_mode || pid == root_pid || arm_trace();
}
static int file_matches(HANDLE file, const BY_HANDLE_FILE_INFORMATION *wanted)
{
    BY_HANDLE_FILE_INFORMATION actual;
    return GetFileInformationByHandle(file, &actual) &&
           actual.dwVolumeSerialNumber == wanted->dwVolumeSerialNumber &&
           actual.nFileIndexHigh == wanted->nFileIndexHigh &&
           actual.nFileIndexLow == wanted->nFileIndexLow;
}
static int arm_trace(void)
{
    DWORD i;
    BYTE byte;
    if (!trace_configured) return 0;
    for (i = 0; i < self_trace_count; ++i)
    {
        if (trace_rvas[i] >= image_size || base > 0xffffffffUL - trace_rvas[i]) return 0;
        if (!arm(base + trace_rvas[i]) &&
            (!read_memory(base + trace_rvas[i], &byte, 1) || byte != 0xcc)) return 0;
    }
    return 1;
}
static int staged_file_matches(HANDLE file)
{
    BY_HANDLE_FILE_INFORMATION info;
    return file_matches(file, &target_info) && GetFileInformationByHandle(file, &info) &&
           info.nFileSizeHigh == target_info.nFileSizeHigh && info.nFileSizeLow == target_info.nFileSizeLow &&
           info.ftLastWriteTime.dwHighDateTime == target_info.ftLastWriteTime.dwHighDateTime &&
           info.ftLastWriteTime.dwLowDateTime == target_info.ftLastWriteTime.dwLowDateTime;
}
static int process_created(void)
{
    CREATE_PROCESS_DEBUG_INFO *info = &event.u.CreateProcessInfo;
    int known = select_owner(event.dwProcessId);
    if (!known)
    {
        ProcessState *p;
        save_owner();
        if (family_count == 3) return 0;
        p = &family[family_count++];
        memset(p, 0, sizeof(*p));
        p->process = info->hProcess; p->thread = info->hThread;
        p->pid = event.dwProcessId; p->tid = event.dwThreadId;
        p->qualified = family_count == 2 && trace_configured && info->hFile &&
                       staged_file_matches(info->hFile);
        if (!select_owner(p->pid)) return 0;
    }
    else
    {
        if (info->hProcess != process) CloseHandle(info->hProcess);
        if (info->hThread != thread) CloseHandle(info->hThread);
    }
    if (!info->hFile || !staged_file_matches(info->hFile) ||
        !family[owner_index].qualified || event.dwThreadId != tid)
    {
        if (info->hFile) { CloseHandle(info->hFile); info->hFile = NULL; }
        fprintf(stderr, "pe-self rejected process pid=%lu count=%u\n", pid, family_count);
        return 0;
    }
    return 1;
}

/* Wine's initial process breakpoint follows static DLL attachment. Match its
 * qualified ntdll process_breakpoint SEH wrapper, rather than any external INT3. */
static int breakpoint_module(DWORD address, DWORD *allocation, DWORD *size,
                             char module[256])
{
    MEMORY_BASIC_INFORMATION memory;
    DWORD pe, signature, directory, name, i;
    WORD magic;
    module[0]=0; *allocation=0; *size=0;
    if (!VirtualQueryEx(process,(void *)address,&memory,sizeof(memory))) return 0;
    *allocation=(DWORD)memory.AllocationBase;
    if (!*allocation || *allocation>0xffefffffUL ||
        !read_memory(*allocation,&magic,2) || magic!=0x5a4d ||
        !read_memory(*allocation+0x3c,&pe,4) || pe<64 || pe>0x100000 ||
        *allocation>0xffffffffUL-pe-128 ||
        !read_memory(*allocation+pe,&signature,4) || signature!=0x4550 ||
        !read_memory(*allocation+pe+24,&magic,2) || magic!=0x10b ||
        !read_memory(*allocation+pe+24+56,size,4) ||
        *size>0xffffffffUL-*allocation || *size<pe+128 ||
        address<*allocation || address-*allocation>=*size ||
        !read_memory(*allocation+pe+24+96,&directory,4) ||
        !directory || directory>*size-40 ||
        !read_memory(*allocation+directory+12,&name,4) || !name || name>=*size)
        return 0;
    for(i=0;i<255 && i<*size-name;++i) {
        if(!read_memory(*allocation+name+i,module+i,1)) return 0;
        if(!module[i]) return 1;
    }
    module[0]=0;
    return 0;
}
static int initial_loader_break(DWORD address, CONTEXT *c)
{
    DWORD allocation,size;
    char module[256];
    BYTE bytes[18];
    if (loader_break || !event.u.Exception.dwFirstChance || c->Eip!=address+1 ||
        !breakpoint_module(address,&allocation,&size,module) ||
        address-allocation<6 || address-allocation>size-12 ||
        !read_memory(address-6,bytes,sizeof(bytes))) return 0;
    return original_loader_break(module,bytes,sizeof(bytes));
}
static void external_break(DWORD address, CONTEXT *c)
{
    DWORD allocation,size,words[5],i;
    char module[256];
    if(!breakpoint_module(address,&allocation,&size,module)) strcpy(module,"<unknown>");
    fprintf(stderr,"original external breakpoint: pc=%08lx first_chance=%lu loader_seen=%d target_base=%08lx module_base=%08lx module=%s\n",address,event.u.Exception.dwFirstChance,loader_break,base,allocation,module);
    fprintf(stderr,"registers eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx eip=%08lx flags=%08lx\n",c->Eax,c->Ebx,c->Ecx,c->Edx,c->Esi,c->Edi,c->Ebp,c->Esp,c->Eip,c->EFlags);
    if(read_memory(c->Esp,words,sizeof(words))) {
        fputs("stack20:",stderr);for(i=0;i<5;++i)fprintf(stderr," %08lx",words[i]);fputs("\n",stderr);
    } else fputs("stack20: unreadable\n",stderr);
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
        if (self_mode && (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT
                ? !process_created() : !select_owner(event.dwProcessId))) return -1;
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
            if (provider_required && event.u.LoadDll.hFile &&
                file_matches(event.u.LoadDll.hFile, &provider_info))
                provider_seen = 1;
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
                    helper_completion_rva >= helper_size ||
                    (helper_callback_rva && helper_callback_rva >= helper_size))
                    return -1;
                helper_context = helper_base + helper_state_rva;
                helper_completion = helper_base + helper_completion_rva;
                if (helper_callback_rva) {
                    helper_callback = helper_base + helper_callback_rva;
                    if (!arm(helper_callback)) return -1;
                }
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
        { /* Each permitted process retains its one original primary thread. */
            if (self_mode) CloseHandle(event.u.CreateThread.hThread);
            return -1;
        }
        else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
        {
            exited = 1;
            exit_status = event.u.ExitProcess.dwExitCode;
            *where = exit_status;
            if (self_mode)
            {
                save_owner();
                if (pid != root_pid) return entry_seen ? 5 : -1;
                for (i = 0; i < family_count; ++i)
                    if (!family[i].exited) return -1;
            }
            return 0;
        }
        else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            if (self_mode && pid == root_pid)
                for (i = 1; i < family_count; ++i)
                    if (!family[i].exited) return -1; /* Only synchronous SELF is supported. */
            if (event.dwThreadId != tid || !context(&c))
                return -1;
            *where = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if (event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
            {
                if (typed_helper && *where == base + helper_marker_rva &&
                    event.u.Exception.dwFirstChance)
                {
                    marker_seen = 1;
                    pending_status = DBG_EXCEPTION_NOT_HANDLED;
                    registers(r, &c);
                    return 1;
                }
                if (typed_helper && *where == helper_completion)
                {
                    registers(r, &c);
                    return 2;
                }
                if (helper_callback && *where == helper_callback &&
                    (!marker_seen || callback_entered || !event.u.Exception.dwFirstChance ||
                     c.Eip != *where + 1)) return -1;
                for (i = 0; i < point_count; ++i)
                    if (points[i].active && points[i].address == *where)
                    {
                        if (!patch(*where, points[i].saved))
                            return -1;
                        points[i].active = 0;
                        if (self_mode && *where == base + entry_rva)
                        {
                            if (provider_required && !provider_seen) return -1;
                            entry_seen = 1;
                        }
                        c.Eip = *where;
                        if (!SetThreadContext(thread, &c))
                            return -1;
                        if (helper_callback && *where == helper_callback) {
                            if (callback_entered || !marker_seen ||
                                !event.u.Exception.dwFirstChance) return -1;
                            callback_entered = 1;
                            break;
                        }
                        registers(r, &c);
                        return 1;
                    }
                if (i < point_count && helper_callback && *where == helper_callback)
                    continue;
                if ((!base || *where < base || *where - base >= image_size) &&
                    initial_loader_break(*where,&c))
                {
                    loader_break = 1;
                    continue;
                }
                if (!base || *where<base || *where-base>=image_size) external_break(*where,&c);
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
static int event_response(DWORD ok, DWORD a, DWORD b, Registers *r, DWORD size)
{
    DWORD header[6];
    BYTE bytes[24 + sizeof(Registers)];
    if (!self_mode) return response(ok, a, b, r, size);
    header[0] = 1; header[1] = pid; header[2] = tid; header[3] = base;
    header[4] = pid == root_pid ? 0 : 1; header[5] = entry_seen;
    memcpy(bytes, header, sizeof(header));
    if (size) memcpy(bytes + sizeof(header), r, size);
    return response(ok, a, b, bytes, sizeof(header) + size);
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

static int stop_family(void)
{
    DWORD started = GetTickCount(), code;
    unsigned i, live;
    int clean = 1;
    save_owner();
    for (i = 0; i < family_count; ++i) if (!family[i].exited)
    {
        family[i].forced = 1;
        if (!TerminateProcess(family[i].process, 125)) clean = 0;
    }
    if (pending)
    {
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE)) clean = 0;
        pending = 0;
    }
    do
    {
        live = 0;
        for (i = 0; i < family_count; ++i) if (!family[i].exited) ++live;
        if (!live || GetTickCount() - started >= 3000) break;
        if (!WaitForDebugEvent(&event, 20)) continue;
        if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
            for (i = 0; i < family_count; ++i) if (family[i].pid == event.dwProcessId)
            {
                family[i].exited = 1;
                family[i].exit_status = event.u.ExitProcess.dwExitCode;
            }
        if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT && event.u.LoadDll.hFile)
            CloseHandle(event.u.LoadDll.hFile);
        if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT)
            CloseHandle(event.u.CreateThread.hThread);
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            TerminateProcess(event.u.CreateProcessInfo.hProcess, 125);
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
            CloseHandle(event.u.CreateProcessInfo.hThread);
            CloseHandle(event.u.CreateProcessInfo.hProcess);
            clean = 0; /* No unaccounted create can produce a successful receipt. */
        }
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE)) clean = 0;
    } while (live);
    for (i = 0; i < family_count; ++i)
    {
        DWORD elapsed = GetTickCount() - started;
        int signaled = WaitForSingleObject(family[i].process, elapsed < 3000 ? 3000 - elapsed : 0) == WAIT_OBJECT_0;
        code = 0;
        if (!family[i].exited || !signaled || !GetExitCodeProcess(family[i].process, &code) ||
            code != family[i].exit_status) clean = 0;
        fprintf(stderr, "pe-self-process %lu %lu %lu %u %d %d %lu %d %d\n",
                family[i].pid, family[i].tid, family[i].base, i == 0 ? 0 : 1,
                family[i].entry_seen, family[i].exited, family[i].exit_status,
                family[i].forced, signaled);
        CloseHandle(family[i].thread); CloseHandle(family[i].process);
        free(family[i].points);
    }
    process = thread = NULL; points = NULL; point_count = 0;
    return clean;
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
/* GetEnvironmentStrings in the original SDK is the ANSI baseline. Copy it;
 * transport configuration belongs to the broker, never to the target overlay. */
static char *target_environment(const char *path)
{
    HANDLE file;
    DWORD size,got;
    size_t base_size;
    const char *base;
    char *overlay,*result;
    file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if(file==INVALID_HANDLE_VALUE)return NULL;
    size=GetFileSize(file,NULL);
    if(size<2 || size>ORIGINAL_ENVIRONMENT_LIMIT){CloseHandle(file);return NULL;}
    overlay=malloc(size);
    if(!overlay){CloseHandle(file);return NULL;}
    got=0;
    if(!ReadFile(file,overlay,size,&got,NULL) || got!=size) {
        CloseHandle(file);free(overlay);return NULL;
    }
    CloseHandle(file);
    base=(const char *)GetEnvironmentStrings();
    if(!base){free(overlay);return NULL;}
    for(base_size=0;base_size+1<ORIGINAL_ENVIRONMENT_LIMIT;++base_size)
        if(!base[base_size] && !base[base_size+1])break;
    if(base_size+1==ORIGINAL_ENVIRONMENT_LIMIT){free(overlay);return NULL;}
    result=original_environment_merge(base,base_size+2,overlay,size);
    free(overlay);
    return result;
}
/* Preserve the exact already-built launch input, not an inferred child/CRT
 * environment. A declared saved input must compare before any target launch. */
static void environment_record_io(const char *operation, DWORD error,
                                  DWORD expected, DWORD actual)
{
    fprintf(stderr, "original OS environment %s failed: win32=%lu expected_bytes=%lu actual_bytes=%lu\n",
            operation, error, expected, actual);
}
static void environment_record_entry(const char *label, const char *bytes,
                                     DWORD size, DWORD offset)
{
    DWORD start = 0, end, i, name;
    char text[129];
    for (i = 0; i < offset && i < size; ++i)
        if (!bytes[i]) start = i + 1;
    end = start;
    while (end < size && bytes[end]) ++end;
    name = start < size && bytes[start]
        ? (DWORD)original_environment_name_size(bytes + start) : 0;
    for (i = 0; i < name && i < sizeof(text) - 1; ++i)
        text[i] = bytes[start + i] >= 32 && bytes[start + i] <= 126
            ? bytes[start + i] : '?';
    text[i] = 0;
    fprintf(stderr, " %s_name=%s %s_name_bytes=%lu %s_entry_bytes=%lu",
            label, name ? text : "<end>", label, name, label, end - start);
}
static void environment_record_difference(const char *expected, DWORD expected_size,
                                          const char *actual, DWORD actual_size)
{
    DWORD offset = 0;
    while (offset < expected_size && offset < actual_size &&
           expected[offset] == actual[offset]) ++offset;
    fprintf(stderr, "original OS environment mismatch: expected_bytes=%lu actual_bytes=%lu first_offset=%lu",
            expected_size, actual_size, offset);
    if (original_environment_valid(expected, expected_size, 0) &&
        original_environment_valid(actual, actual_size, 0)) {
        environment_record_entry("expected", expected, expected_size, offset);
        environment_record_entry("actual", actual, actual_size, offset);
    }
    fputc('\n', stderr);
}
static int record_process_environment(const char *path, const char *environment)
{
    DWORD size = 0, expected_size, got = 0, open_error, saved_error;
    HANDLE file;
    char *expected_path, *bytes;
    int ok;
    while (size + 1 < ORIGINAL_ENVIRONMENT_LIMIT &&
           (environment[size] || environment[size + 1])) ++size;
    size += 2;
    if (size > ORIGINAL_ENVIRONMENT_LIMIT ||
        !original_environment_valid(environment, size, 0)) {
        environment_record_io("actual-block-validation", 0, 0, size); return 0;
    }
    expected_path = malloc(strlen(path) + 10);
    if (!expected_path) {
        environment_record_io("expected-path-allocation", 0, 0, 0); return 0;
    }
    strcpy(expected_path, path); strcat(expected_path, ".expected");
    file = CreateFileA(expected_path, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, 0, NULL);
    open_error = file == INVALID_HANDLE_VALUE ? GetLastError() : 0;
    free(expected_path);
    if (file != INVALID_HANDLE_VALUE)
    {
        expected_size = GetFileSize(file, NULL);
        saved_error = expected_size == INVALID_FILE_SIZE ? GetLastError() : 0;
        if (expected_size == INVALID_FILE_SIZE || expected_size > ORIGINAL_ENVIRONMENT_LIMIT) {
            CloseHandle(file);
            environment_record_io("expected-size", saved_error, size, expected_size); return 0;
        }
        bytes = malloc(expected_size ? expected_size : 1);
        if (!bytes) {
            CloseHandle(file);
            environment_record_io("expected-allocation", 0, expected_size, 0); return 0;
        }
        ok = ReadFile(file, bytes, expected_size, &got, NULL);
        saved_error = ok ? 0 : GetLastError();
        CloseHandle(file);
        if (!ok || got != expected_size) {
            free(bytes);
            environment_record_io("expected-read", saved_error, expected_size, got); return 0;
        }
        ok = expected_size == size && !memcmp(bytes, environment, size);
        if (!ok) environment_record_difference(bytes, expected_size, environment, size);
        free(bytes);
        if (!ok) return 0;
    }
    else if (open_error != ERROR_FILE_NOT_FOUND) {
        environment_record_io("expected-open", open_error, 0, 0); return 0;
    }
    file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        saved_error = GetLastError();
        environment_record_io("receipt-create", saved_error, size, 0); return 0;
    }
    got = 0;
    ok = WriteFile(file, environment, size, &got, NULL);
    saved_error = ok ? 0 : GetLastError();
    CloseHandle(file);
    if (!ok || got != size) environment_record_io("receipt-write", saved_error, size, got);
    return ok && got == size;
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
    char *command,*environment=NULL;
    const char *process_environment_receipt = NULL;
    int callback_argument, environment_argument;
    while (argc > 1)
    {
        if (!strcmp(argv[argc - 1], "pe-self-child=1"))
        {
            if (self_mode) return 2;
            self_mode = 1; --argc;
        }
        else if (!strncmp(argv[argc - 1], "process-os-environment=", 23))
        {
            if (process_environment_receipt || !argv[argc - 1][23]) return 2;
            process_environment_receipt = argv[--argc] + 23;
        }
        else break;
    }
    callback_argument = argc >= 15 && !strncmp(argv[14], "callback=", 9);
    environment_argument = callback_argument ? (argc == 16 || argc == 17) :
        (argc == 11 || argc == 12 || argc == 15 || argc == 16);
    provider_required=callback_argument?argc==17:(argc==12 || argc==16);
    if (argc != 10 && argc != 11 && argc != 12 && argc != 14 && argc != 15 && argc != 16 && !(argc == 17 && callback_argument)) {
        fprintf(stderr,"original broker argument count: %d\n",argc);
        return 2;
    }
    typed_helper = argc == 14 || argc == 15 || argc == 16 || (argc == 17 && callback_argument);
    if(environment_argument) {
        environment=target_environment(argv[argc-1-provider_required]);
        if(!environment) {
            fputs("invalid, oversized or non-ASCII original target environment\n",stderr);
            return 2;
        }
    }
    rpc_input = GetStdHandle(STD_INPUT_HANDLE);
    rpc_output = GetStdHandle(STD_OUTPUT_HANDLE);
    handle_flags = (HandleFlags)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                               "SetHandleInformation");
    if (!handle_flags || !handle_flags(rpc_input, 1, 0) ||
        !handle_flags(rpc_output, 1, 0)) {
        fprintf(stderr,"original broker RPC handle setup: %lu\n",GetLastError());
        return 3;
    }
    target_dll = strcmp(argv[9], "-") != 0;
    if (self_mode && (target_dll || typed_helper)) return 2;
    if (process_environment_receipt &&
        (target_dll || typed_helper || !provider_required || !environment)) return 2;
    if (target_dll || self_mode)
    {
        HANDLE module =
            CreateFileA(argv[self_mode ? 7 : 9], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
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
        if (callback_argument) {
            char *end;
            helper_callback_rva = strtoul(argv[14]+9,&end,0);
            if (!helper_callback_rva || *end) return 3;
        }
        if (!ok || target_dll || !helper_state_rva || !helper_completion_rva ||
            !helper_marker_rva)
            return 3;
    }
    if (provider_required)
    {
        HANDLE module = CreateFileA(argv[argc-1], GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (module == INVALID_HANDLE_VALUE) {
            fprintf(stderr,"original provider open %s: %lu\n",argv[argc-1],GetLastError());
            return 3;
        }
        ok = GetFileInformationByHandle(module, &provider_info);
        CloseHandle(module);
        if (!ok) {
            fprintf(stderr,"original provider file identity: %lu\n",GetLastError());
            return 3;
        }
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
        error_output == INVALID_HANDLE_VALUE) {
        fprintf(stderr,"original stdio open: %lu input=%s output=%s error=%s\n",GetLastError(),argv[4],argv[5],argv[6]);
        return 4;
    }
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input;
    startup.hStdOutput = output;
    startup.hStdError = error_output;
    command = malloc(strlen(argv[1]) + 1);
    if (!command) { fputs("original command allocation failed\n",stderr); return 5; }
    strcpy(command, argv[1]);
    if (process_environment_receipt &&
        !record_process_environment(process_environment_receipt, environment)) {
        fputs("actual target OS environment input differs or receipt failed before launch\n", stderr);
        free(environment); free(command);
        return 6;
    }
    if (!CreateProcessA(argv[7], command, NULL, NULL, TRUE,
                        self_mode ? DEBUG_PROCESS : DEBUG_ONLY_THIS_PROCESS, environment, argv[3], &startup, &launch)) {
        fprintf(stderr,"original CreateProcess %s: %lu\n",argv[7],GetLastError());
        free(environment);
        return 6;
    }
    free(environment);
    process = launch.hProcess;
    thread = launch.hThread;
    pid = launch.dwProcessId;
    tid = launch.dwThreadId;
    if (self_mode)
    {
        root_pid = pid; family_count = 1; owner_index = 0;
        family[0].qualified = 1;
        save_owner();
    }
    where = 0;
    kind = next(&where, &r);
    if (kind != 1 || (provider_required && !provider_seen) || (typed_helper && !helper_base) || where != base + entry_rva ||
        !event_response(1, base, entry_rva, &r, sizeof(r))) {
        fprintf(stderr,"original startup before target entry: kind=%d status=%08lx base=%08lx expected=%08lx provider_seen=%d\n",kind,where,base,entry_rva,provider_seen);
        goto done;
    }
    while (transfer(rpc_input, h, sizeof(h), 0))
    {
        free(data);
        data = NULL;
        if (h[4] > 16777216UL)
            break;
        if (self_mode && h[0] != 1 && h[0] != 6 && h[0] != 7 && h[0] != 9) break;
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
            if (self_mode)
            {
                if (trace_configured || h[4] > 400000 || pid != root_pid) break;
                trace_rvas = malloc(h[4] ? h[4] : 1);
                if (!trace_rvas) break;
                if (h[4]) memcpy(trace_rvas, data, h[4]);
                self_trace_count = h[4] / 4; trace_configured = 1;
            }
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
            if (kind == 0 && typed_helper && helper_callback) {
                DWORD proof[4];
                proof[0]=callback_entered;proof[1]=helper_callback;proof[2]=pid;proof[3]=tid;
                if (!response(ok,0,where,proof,sizeof(proof))) break;
                continue;
            }
            if (!event_response(ok, (DWORD)kind,
                          kind == 0 || kind == 3 || kind == 5 ? where : where - base, &r,
                          kind > 0 && kind != 5 ? sizeof(r) : 0))
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
    if (self_mode) { if (!stop_family()) ok = 0; }
    else stop();
    free(data);
    free(points);
    free(trace_rvas);
    return ok ? 0 : 7;
}
