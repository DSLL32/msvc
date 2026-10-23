/* Original-only diagnostic harness, not reconstructed WLIB code.
   One-shot breakpoint pattern follows vc1.0/cl/src/trace-run.c. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resource-guard.h"
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
static DWORD loaded_cf_base;
typedef struct OwnerSpec { DWORD caller; int esp, displacement; DWORD capacity; BYTE expected; } OwnerSpec;
typedef struct ReturnBreak { DWORD address; BYTE saved; int active; } ReturnBreak;
static OwnerSpec owner_specs[16];
static unsigned owner_spec_count;
static DWORD guard_owner_rva, guard_reader_rva, guard_reader_return_rva;
static BYTE guard_reader_return_byte;
static int guard_mode, guard_cf, guard_owner_site = -1, guard_reader_site = -1, guard_step_site = -1;
static RgState guard_state;
static ReturnBreak owner_return_break, reader_return_break;
static DWORD owned_process_id, event_count;
static DWORD initial_entry_address;
static int final_exit_seen;
static HANDLE child;
static FILE *events;
static int bytes_at(const char *, DWORD, DWORD);

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

static DWORD guard_base(void)
{
    return guard_cf ? loaded_cf_base : image_base;
}

static int guard_return_install(ReturnBreak *point, DWORD address, BYTE expected)
{
    unsigned i;
    if (point->active || !address) return 0;
    for (i = 0; i < site_count; ++i) if (sites[i].address == address) return 0;
    if ((&owner_return_break != point && owner_return_break.active && owner_return_break.address == address) ||
        (&reader_return_break != point && reader_return_break.active && reader_return_break.address == address)) return 0;
    if (!memory(address, &point->saved, 1) || point->saved != expected) return 0;
    if (!patch(address, 0xcc)) return 0;
    point->address = address; point->active = 1;
    return 1;
}

static int guard_restore(ReturnBreak *point, CONTEXT *context)
{
    BYTE actual;
    if (!point->active || context->Eip != point->address + 1 ||
        !memory(point->address, &actual, 1) || actual != 0xcc ||
        !patch(point->address, point->saved)) return 0;
    context->Eip = point->address; point->active = 0;
    return 1;
}

static int guard_armed(void)
{
    BYTE owner, reader;
    return guard_owner_site >= 0 && guard_reader_site >= 0 &&
           sites[guard_owner_site].address && sites[guard_reader_site].address &&
           memory(sites[guard_owner_site].address, &owner, 1) && owner == 0xcc &&
           memory(sites[guard_reader_site].address, &reader, 1) && reader == 0xcc;
}

static int guard_owner(const CONTEXT *context, DWORD tid)
{
    DWORD caller, pointer = context->Edx, expected, base;
    unsigned i;
    int result;
    if (rg_segments(&guard_state, tid, context->SegDs, context->SegGs) != RG_OK) return 0;
    if (!memory(context->Esp, &caller, 4)) return 0;
    for (i = 0; i < owner_spec_count; ++i)
        if (guard_base() + owner_specs[i].caller == caller) break;
    if (i == owner_spec_count) { fprintf(events, "guard-unknown-owner caller=%08lx\n", caller); return 0; }
    base = owner_specs[i].esp ? context->Esp : context->Ebp;
    if (owner_specs[i].displacement < 0) {
        DWORD magnitude = (DWORD)(-(owner_specs[i].displacement + 1)) + 1;
        if (base < magnitude) return 0;
        expected = base - magnitude;
    } else {
        if (base > 0xffffffffUL - (DWORD)owner_specs[i].displacement) return 0;
        expected = base + owner_specs[i].displacement;
    }
    result = rg_begin_owner(&guard_state, tid, context->Esp, caller, pointer,
                            expected, owner_specs[i].capacity);
    fprintf(events, "guard-owner result=%d tid=%lu caller=%08lx esp=%08lx ebp=%08lx actual-pointer=%08lx expected=%08lx actual-object-capacity=%lu\n",
            result, tid, caller, context->Esp, context->Ebp, pointer, expected, owner_specs[i].capacity);
    if (result != RG_OK || !guard_return_install(&owner_return_break, caller, owner_specs[i].expected)) return 0;
    return bytes_at("guard-owner-before", pointer, owner_specs[i].capacity);
}

static int guard_reader(const CONTEXT *context, DWORD tid)
{
    DWORD stack[4], live;
    int result;
    if (!memory(context->Esp, stack, sizeof(stack)) ||
        !memory(guard_state.owner_stack, &live, 4)) return 0;
    result = rg_enter_reader(&guard_state, tid, context->Esp, stack[0],
                             guard_base() + guard_reader_return_rva,
                             stack[1], stack[3], stack[2] & 0xffffUL,
                             context->SegDs, live, context->SegGs);
    fprintf(events, "guard-reader result=%d call=%lu tid=%lu esp=%08lx return=%08lx pointer=%08lx selector-word=%04lx raw-selector-slot=%08lx capacity=%lu actual-ds=%04lx actual-gs=%04lx\n",
            result, guard_state.calls, tid, context->Esp, stack[0], stack[1], stack[2] & 0xffffUL,
            stack[2], stack[3], context->SegDs, context->SegGs);
    return result == RG_OK && guard_return_install(&reader_return_break,
             stack[0], guard_reader_return_byte);
}

static void guard_context(RgContext *packet, const CONTEXT *context)
{
    packet->pc = context->Eip; packet->flags = context->EFlags;
    packet->registers[0] = context->Eax; packet->registers[1] = context->Ecx;
    packet->registers[2] = context->Edx; packet->registers[3] = context->Ebx;
    packet->registers[4] = context->Esp; packet->registers[5] = context->Ebp;
    packet->registers[6] = context->Esi; packet->registers[7] = context->Edi;
    packet->segments[0] = context->SegCs & 0xffffUL; packet->segments[1] = context->SegSs & 0xffffUL;
    packet->segments[2] = context->SegDs & 0xffffUL; packet->segments[3] = context->SegEs & 0xffffUL;
    packet->segments[4] = context->SegFs & 0xffffUL; packet->segments[5] = context->SegGs & 0xffffUL;
}

static int guard_step_begin(unsigned i, CONTEXT *context, DWORD tid)
{
    BYTE actual;
    int result;
    RgContext before;
    if (context->Eip != sites[i].address + 1 ||
        !memory(sites[i].address, &actual, 1) || actual != 0xcc) return 0;
    guard_context(&before, context);
    /* Rewind only an identified owned INT3, validated above against actual EIP. */
    before.pc = sites[i].address;
    result = rg_prepare_step(&guard_state, tid, sites[i].address, sites[i].saved, &before);
    if (result != RG_OK || !patch(sites[i].address, sites[i].saved)) return 0;
    guard_step_site = (int)i;
    context->Eip = sites[i].address; context->EFlags |= 0x100;
    fprintf(events, "guard-step-begin site=%08lx opcode=%02x thread=%lu expected-next=%08lx requested-flags=%08lx expected-nondebug-flags=%08lx\n",
            sites[i].address, sites[i].saved, tid, guard_state.step_pc,
            context->EFlags, guard_state.step_flags);
    return 1;
}

static int guard_arm_readback(HANDLE thread, DWORD tid)
{
    CONTEXT armed;
    RgContext packet;
    int result;
    memset(&armed, 0, sizeof(armed)); armed.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &armed)) return 0;
    guard_context(&packet, &armed);
    result = rg_confirm_arm(&guard_state, tid, &packet);
    fprintf(events, "guard-arm-readback result=%d thread=%lu eip=%08lx esp=%08lx flags=%08lx cs=%04lx ss=%04lx ds=%04lx es=%04lx fs=%04lx gs=%04lx\n",
            result, tid, armed.Eip, armed.Esp, armed.EFlags,
            armed.SegCs, armed.SegSs, armed.SegDs, armed.SegEs, armed.SegFs, armed.SegGs);
    return result == RG_OK;
}

static int guard_step_end(CONTEXT *context, DWORD tid, DWORD code, int first_chance)
{
    DWORD pushed;
    BYTE actual;
    int result;
    RgContext after;
    if (guard_step_site < 0 || !memory(context->Esp, &pushed, 4)) return 0;
    guard_context(&after, context);
    result = rg_finish_step(&guard_state, tid, code, first_chance, &after, pushed);
    fprintf(events, "guard-step-result=%d tid=%lu actual-pc=%08lx expected-pc=%08lx actual-esp=%08lx expected-esp=%08lx actual-push=%08lx expected-push=%08lx actual-flags=%08lx expected-nondebug-flags=%08lx actual-ds=%04lx expected-ds=%04lx actual-gs=%04lx expected-gs=%04lx\n",
            result, tid, context->Eip, guard_state.step_pc, context->Esp, guard_state.step_stack,
            pushed, guard_state.step_value, context->EFlags, guard_state.step_flags,
            context->SegDs, guard_state.step_ds, context->SegGs, guard_state.step_gs);
    if (result != RG_OK || !memory(sites[guard_step_site].address, &actual, 1) ||
        actual != sites[guard_step_site].saved || !patch(sites[guard_step_site].address, 0xcc)) return 0;
    fprintf(events, "guard-step-rearmed site=%08lx thread=%lu pushed=%08lx\n",
            sites[guard_step_site].address, tid, pushed);
    context->EFlags &= ~0x100UL;
    guard_step_site = -1;
    return 1;
}

static int bytes_at(const char *kind, DWORD address, DWORD size)
{
    BYTE bytes[512];
    DWORD i;
    if (size > sizeof(bytes)) { fprintf(events, "%s span-held=%lu\n", kind, size); return 0; }
    if (!memory(address, bytes, size)) { fprintf(events, "%s read-failed=%08lx size=%lu\n", kind, address, size); return 0; }
    fprintf(events, "%s address=%08lx size=%lu bytes=", kind, address, size);
    for (i = 0; i < size; ++i) fprintf(events, "%02x", bytes[i]);
    fprintf(events, "\n");
    return 1;
}

static int input_string(const char *kind, DWORD address, DWORD limit)
{
    DWORD i;
    BYTE value;
    fprintf(events, "%s address=%08lx bytes=", kind, address);
    for (i = 0; i < limit; ++i) {
        if (!memory(address + i, &value, 1)) { fprintf(events, " READ-FAILED\n"); return 0; }
        fprintf(events, "%02x", value);
        if (!value) { fprintf(events, " length=%lu\n", i); return 1; }
    }
    fprintf(events, " UNTERMINATED\n"); return 0;
}

static int observe_inputs(const char *name, const CONTEXT *context)
{
    DWORD values[7], i, pointer;
    if (!strcmp(name, "observe_CreateFileA")) {
        if (!memory(context->Esp + 4, values, sizeof(values))) return 0;
        fprintf(events, "actual-CreateFileA-arguments");
        for (i = 0; i < 7; ++i) fprintf(events, " %08lx", values[i]);
        fprintf(events, "\n");
        if (!input_string("actual-CreateFileA-filename", values[0], 1024)) return 0;
    } else if (!strcmp(name, "observe_module_filename_return")) {
        fprintf(events, "actual-module-filename-return length=%lu buffer=%08lx\n", context->Eax, context->Esp);
        if (context->Eax >= 260 || !input_string("actual-module-filename", context->Esp, context->Eax + 1)) return 0;
    } else if (!strcmp(name, "cf_launch")) {
        if (!memory(context->Edi, values, 20)) return 0;
        fprintf(events, "actual-CF-startup address=%08lx fields=%08lx,%08lx,%08lx,%08lx,%08lx\n",
                context->Edi, values[0], values[1], values[2], values[3], values[4]);
        if (!input_string("actual-CF-program-name", values[0], 1024) ||
            !input_string("actual-CF-command-tail", values[1], 4096)) return 0;
    } else if (!strcmp(name, "main")) {
        fprintf(events, "actual-main-argc=%lu argv=%08lx\n", context->Eax, context->Edx);
        if (context->Eax > 256) return 0;
        for (i = 0; i < context->Eax; ++i) {
            if (!memory(context->Edx + i * 4, &pointer, 4) || !input_string("actual-main-argument", pointer, 4096)) return 0;
        }
    } else if (!strcmp(name, "guard_final_exit")) {
        if (!memory(context->Esp + 4, &pointer, 4)) return 0;
        fprintf(events, "actual-ExitProcess-status=%lu\n", pointer);
    }
    return 1;
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
    loaded_cf_base = base;
    for (i = 0; i < site_count; ++i) if (sites[i].cf) {
        if (!sites[i].guarded || sites[i].rva >= cf_limit ||
            !install_site(i, base + sites[i].rva)) return 0;
    }
    return 1;
}

static int read_guard_config(const char *path)
{
    FILE *file;
    char domain[8], kind[8], extra[8];
    unsigned byte, i;
    int pe_site = 0, exit_site = 0;
    file = fopen(path, "rt");
    if (!file) return 0;
    if (fscanf(file, "%7s %lx %lx %lx %x %lu", domain, &guard_owner_rva,
               &guard_reader_rva, &guard_reader_return_rva, &byte, &guard_state.max_calls) != 6 ||
        byte > 255 || !guard_state.max_calls || guard_state.max_calls > 4096 ||
        fscanf(file, "%u", &owner_spec_count) != 1 || !owner_spec_count || owner_spec_count > 16) {
        fclose(file); return 0;
    }
    if (strcmp(domain, "pe") && strcmp(domain, "cf")) { fclose(file); return 0; }
    guard_cf = !strcmp(domain, "cf"); guard_reader_return_byte = (BYTE)byte;
    for (i = 0; i < owner_spec_count; ++i) {
        if (fscanf(file, "%lx %7s %d %lu %x", &owner_specs[i].caller, kind,
                   &owner_specs[i].displacement, &owner_specs[i].capacity, &byte) != 5 ||
            (strcmp(kind, "ebp") && strcmp(kind, "esp")) || byte > 255 ||
            !owner_specs[i].capacity || owner_specs[i].capacity > 512) { fclose(file); return 0; }
        owner_specs[i].esp = !strcmp(kind, "esp"); owner_specs[i].expected = (BYTE)byte;
    }
    if (fscanf(file, "%7s", extra) != EOF) { fclose(file); return 0; }
    fclose(file);
    for (i = 0; i < site_count; ++i) {
        unsigned j;
        for (j = 0; j < i; ++j)
            if (sites[i].cf == sites[j].cf && sites[i].rva == sites[j].rva) return 0;
        if (!strcmp(sites[i].name, "guard_owner_begin")) {
            if (guard_owner_site >= 0 || sites[i].cf != guard_cf || sites[i].rva != guard_owner_rva) return 0;
            guard_owner_site = (int)i;
        } else if (!strcmp(sites[i].name, "guard_reader_entry")) {
            if (guard_reader_site >= 0 || sites[i].cf != guard_cf || sites[i].rva != guard_reader_rva) return 0;
            guard_reader_site = (int)i;
        } else if (!strcmp(sites[i].name, "pe_entry")) {
            if (sites[i].cf || pe_site++) return 0;
        } else if (!strcmp(sites[i].name, "guard_final_exit")) {
            if (exit_site++) return 0;
        }
    }
    if (guard_owner_site < 0 || guard_reader_site < 0 || !pe_site || !exit_site) return 0;
    for (i = 0; i < 2; ++i) {
        int index = i ? guard_reader_site : guard_owner_site;
        if (!sites[index].guarded || sites[index].expected < 0x50 || sites[index].expected > 0x57) return 0;
    }
    guard_mode = 1;
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
        fprintf(events, "%s tid=%lu eip=%08lx esp=%08lx eax=%08lx edx=%08lx ebx=%08lx ecx=%08lx esi=%08lx edi=%08lx ebp=%08lx cs=%04lx ds=%04lx es=%04lx fs=%04lx gs=%04lx ss=%04lx eflags=%08lx\n",
                why, tid, context.Eip, context.Esp, context.Eax, context.Edx,
                context.Ebx, context.Ecx, context.Esi, context.Edi, context.Ebp,
                context.SegCs, context.SegDs, context.SegEs, context.SegFs, context.SegGs, context.SegSs, context.EFlags);
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
    DWORD actual = STILL_ACTIVE, began, i, value;
    DEBUG_EVENT event;
    int exited = 0;
    if (child) {
        for (i = 0; i < read_count; ++i)
            if (memory(image_base + reads[i], &value, 4))
                fprintf(events, "failure-read-only-dword rva=%08lx value=%08lx\n", reads[i], value);
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
    if (argc != 12 && argc != 13) return 120;
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
    if (argc == 13 && !read_guard_config(argv[12])) return 121;
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
    owned_process_id = process.dwProcessId;
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
        ++event_count;
        if (guard_mode && (event_count > 100000 || event.dwProcessId != owned_process_id ||
            (guard_state.step && event.dwDebugEventCode != EXCEPTION_DEBUG_EVENT))) return fail_owned(140);
        fprintf(events, "event code=%lu pid=%lu tid=%lu elapsed=%lu\n",
                event.dwDebugEventCode, event.dwProcessId, event.dwThreadId, GetTickCount() - start);
        status = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            image_base = base;
            initial_entry_address = (DWORD)event.u.CreateProcessInfo.lpStartAddress;
            guard_state.thread = event.dwThreadId;
            threads[thread_count].id = event.dwThreadId;
            threads[thread_count++].handle = event.u.CreateProcessInfo.hThread;
            fprintf(events, "create base=%08lx entry=%08lx teb=%08lx\n", base,
                    (DWORD)event.u.CreateProcessInfo.lpStartAddress,
                    (DWORD)event.u.CreateProcessInfo.lpThreadLocalBase);
            for (i = 0; i < site_count; ++i) if (!sites[i].cf) {
                address = base + sites[i].rva;
                if (guard_mode && !strcmp(sites[i].name, "pe_entry") && address != initial_entry_address)
                    return fail_owned(140);
                if (!install_site(i, address)) return fail_owned(127);
            }
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            if (guard_mode) return fail_owned(140);
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
            if (guard_mode && event.dwThreadId != guard_state.thread) return fail_owned(140);
            registers(threads[t].handle, "exception-context", event.dwThreadId, 0);
            memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL;
            if (!GetThreadContext(threads[t].handle, &context)) return fail_owned(130);
            if (guard_mode && guard_state.step) {
                if (code != EXCEPTION_SINGLE_STEP || address != context.Eip ||
                    !guard_step_end(&context, event.dwThreadId, code, event.u.Exception.dwFirstChance) ||
                    !SetThreadContext(threads[t].handle, &context)) return fail_owned(141);
                fflush(events);
                if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE)) return fail_owned(131);
                event_pending = 0;
                continue;
            }
            if (guard_mode && code == EXCEPTION_BREAKPOINT &&
                ((reader_return_break.active && address == reader_return_break.address) ||
                 (owner_return_break.active && address == owner_return_break.address))) {
                int reader = reader_return_break.active && address == reader_return_break.address;
                ReturnBreak *point = reader ? &reader_return_break : &owner_return_break;
                int result;
                if (!guard_armed() || rg_segments(&guard_state, event.dwThreadId, context.SegDs, context.SegGs) != RG_OK)
                    return fail_owned(142);
                result = reader ? rg_leave_reader(&guard_state, event.dwThreadId, context.Esp, address) :
                                  rg_end_owner(&guard_state, event.dwThreadId, context.Esp, address);
                fprintf(events, "guard-%s-return result=%d tid=%lu pc=%08lx esp=%08lx eax=%08lx ds=%04lx gs=%04lx\n",
                        reader ? "reader" : "owner", result, event.dwThreadId, address,
                        context.Esp, context.Eax, context.SegDs, context.SegGs);
                if (result != RG_OK || !guard_restore(point, &context) ||
                    !SetThreadContext(threads[t].handle, &context)) return fail_owned(142);
                if (!bytes_at(reader ? "guard-reader-after" : "guard-owner-after", guard_state.pointer, guard_state.capacity))
                    return fail_owned(142);
                fflush(events);
                if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE)) return fail_owned(131);
                event_pending = 0;
                continue;
            }
            for (i = 0; i < site_count; ++i)
                if (sites[i].address && address == sites[i].address && !sites[i].hit) break;
            if (code == EXCEPTION_BREAKPOINT && i < site_count) {
                fprintf(events, "%s-site name=%s rva=%08lx\n",
                        guard_mode && ((int)i == guard_owner_site || (int)i == guard_reader_site) ? "persistent" : "one-shot",
                        sites[i].name, sites[i].rva);
                if (!strcmp(sites[i].name, "pe_entry")) {
                    pe_entry_seen = 1;
                    if (guard_mode && rg_initial_segments(&guard_state, event.dwThreadId,
                                                           context.SegDs, context.SegGs) != RG_OK) return fail_owned(143);
                    if (guard_mode) fprintf(events, "guard-initial-segments ds=%04lx gs=%04lx\n", context.SegDs, context.SegGs);
                }
                memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL;
                if (!GetThreadContext(threads[t].handle, &context)) return fail_owned(130);
                if (guard_mode && ((int)i == guard_owner_site || (int)i == guard_reader_site)) {
                    if (context.Eip != address + 1 ||
                        !((int)i == guard_owner_site ? guard_owner(&context, event.dwThreadId) : guard_reader(&context, event.dwThreadId)) ||
                        !guard_step_begin(i, &context, event.dwThreadId) ||
                        !SetThreadContext(threads[t].handle, &context)) return fail_owned(144);
                    registers(threads[t].handle, "guard-step-armed-readback", event.dwThreadId, 0);
                    if (!guard_arm_readback(threads[t].handle, event.dwThreadId)) return fail_owned(149);
                    fflush(events);
                    if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE)) return fail_owned(131);
                    event_pending = 0;
                    continue;
                }
                if (guard_mode && !strcmp(sites[i].name, "guard_final_exit")) {
                    if (rg_segments(&guard_state, event.dwThreadId, context.SegDs, context.SegGs) != RG_OK ||
                        guard_state.owner || guard_state.reader || guard_state.step ||
                        owner_return_break.active || reader_return_break.active ||
                        !guard_state.calls || !guard_armed()) return fail_owned(145);
                    final_exit_seen = 1;
                    fprintf(events, "guard-final-segments ds=%04lx gs=%04lx calls=%lu\n",
                            context.SegDs, context.SegGs, guard_state.calls);
                }
                if (!observe_inputs(sites[i].name, &context)) return fail_owned(148);
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
            else if (guard_mode) return fail_owned(146);
            else status = DBG_EXCEPTION_NOT_HANDLED;
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            DWORD continued, signaled, actual_exit = STILL_ACTIVE;
            target_status = event.u.ExitProcess.dwExitCode;
            if (guard_mode && (!final_exit_seen || guard_state.owner || guard_state.reader || guard_state.step ||
                owner_return_break.active || reader_return_break.active)) {
                fprintf(events, "guard-incomplete-exit-held calls=%lu\n", guard_state.calls);
                return fail_owned(147);
            }
            continued = ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status);
            fprintf(events, "normal-exit-continue result=%lu error=%lu\n", continued,
                    continued ? 0 : GetLastError());
            if (!continued) return fail_owned(150);
            event_pending = 0;
            signaled = WaitForSingleObject(child, 1000);
            if (!GetExitCodeProcess(child, &actual_exit)) return fail_owned(150);
            fprintf(events, "normal-owned-exit signaled=%d wait-result=%08lx actual-status=%lu event-status=%lu\n",
                    signaled == WAIT_OBJECT_0, signaled, actual_exit, target_status);
            if (signaled != WAIT_OBJECT_0 || actual_exit != target_status) return fail_owned(150);
            fprintf(events, "exit status=%lu\n", target_status);
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
