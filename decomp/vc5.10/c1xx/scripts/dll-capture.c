/* Explicit original-only debugger transport. Never linked to a fixture. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "invoke-arguments.h"
#include <recon.h>

typedef struct Range { DWORD first, last; int entry; BYTE saved; } Range;
static HANDLE child;
static FILE *events;
static DWORD base, entry_rva, code_rva, code_size, image_size;
static DWORD crt_base, return_address, return_reason;
static DWORD kernel_base, kernelbase_base;
static unsigned allocation_api_count, allocation_helper_count;
static DWORD allocation_return_pc;
static BYTE allocation_return_byte;
static int allocation_return_active;
static int allocation_observers;
static DWORD option_return_pc;
static BYTE option_return_byte;
static unsigned option_calls;
static int option_return_active;
static int system_observer_active, system_observer_stage;
static DWORD system_observer_esp, system_observer_provider;
static BYTE system_before_byte, system_after_byte;
static BYTE allocation_before_byte[2], allocation_after_byte[2];
static BYTE return_saved;
static DWORD pch_return_address;
static BYTE pch_return_saved;
static int pch_return_active;
static int return_active, restore_observer, pool_return_observer;
static BYTE restore_observer_byte;
static DWORD aux_old_base, aux_new_base;
static int aux_returned;
static unsigned sequence;
static BYTE *snapshot;
static Range ranges[512];
static unsigned count;
static HANDLE threads[64];
static DWORD thread_ids[64], thread_count;
static int baseline, initialized, pending = -1, target_seen, crt_seen;
static char target_path[MAX_PATH], crt_path[MAX_PATH];

static void fail(const char *message)
{
    if (events) { fprintf(events, "failure %s error=%lu\n", message, GetLastError()); fflush(events); }
    if (child) TerminateProcess(child, 126);
    ExitProcess(126);
}

static void readmem(DWORD address, void *buffer, DWORD size)
{
    DWORD done;
    if (!ReadProcessMemory(child, (void *)address, buffer, size, &done) || done != size)
        fail("ReadProcessMemory");
}

static void write_mem(DWORD address, const void *buffer, DWORD size)
{
    DWORD done, old;
    if (!VirtualProtectEx(child, (void *)address, size, PAGE_EXECUTE_READWRITE, &old)) fail("VirtualProtectEx");
    if (!WriteProcessMemory(child, (void *)address, (LPVOID)buffer, size, &done) || done != size)
        fail("WriteProcessMemory");
    if (!VirtualProtectEx(child, (void *)address, size, old, &old)) fail("restore protection");
    if (!FlushInstructionCache(child, (void *)address, size)) fail("FlushInstructionCache");
}

static void patch(DWORD address, BYTE byte) { write_mem(address, &byte, 1); }

static void remote_string(DWORD address, char *text, DWORD capacity)
{
    DWORD i;
    for (i = 0; i < capacity; ++i) {
        readmem(address + i, text + i, 1);
        if (!text[i]) return;
    }
    fail("long module name");
}

static int same_file(HANDLE loaded, const char *reference, const char *output)
{
    HANDLE expected, saved;
    BYTE actual[8192], wanted[8192];
    DWORD a, b, wrote;
    int same = 1;
    expected = CreateFileA(reference, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (expected == INVALID_HANDLE_VALUE) fail("open expected file");
    saved = CreateFileA(output, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    if (saved == INVALID_HANDLE_VALUE) fail("save loaded file");
    if (SetFilePointer(loaded, 0, NULL, FILE_BEGIN) == 0xffffffffUL) fail("rewind loaded file");
    do {
        if (!ReadFile(loaded, actual, sizeof(actual), &a, NULL) ||
            !ReadFile(expected, wanted, sizeof(wanted), &b, NULL)) fail("read file identity");
        if (a != b || (a && memcmp(actual, wanted, a))) same = 0;
        if (!WriteFile(saved, actual, a, &wrote, NULL) || wrote != a) fail("save identity");
    } while (a || b);
    CloseHandle(saved); CloseHandle(expected);
    return same;
}

static void module_loaded(DWORD address, HANDLE file)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_EXPORT_DIRECTORY exports;
    IMAGE_SECTION_HEADER section;
    char name[MAX_PATH];
    DWORD i, at;
    BYTE first;
    readmem(address, &dos, sizeof(dos));
    readmem(address + dos.e_lfanew, &nt, sizeof(nt));
    strcpy(name, "<none>");
    if (nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress) {
        readmem(address + nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
                &exports, sizeof(exports));
        remote_string(address + exports.Name, name, sizeof(name));
    }
    fprintf(events, "module name=%s actual=%08lx preferred=%08lx entry=%08lx\n",
            name, address, nt.OptionalHeader.ImageBase, nt.OptionalHeader.AddressOfEntryPoint);
    if (getenv("C1XX_OBSERVE_ALLOCATION") && !_stricmp(name, "kernel32.dll")) {
        if (!file || kernel_base || !same_file(file, "C:\\windows\\system32\\kernel32.dll", "loaded-kernel32.bin")) fail("kernel32 provider identity");
        kernel_base = address;
    }
    if (getenv("C1XX_OBSERVE_ALLOCATION") && !_stricmp(name, "kernelbase.dll")) {
        if (!file || kernelbase_base || !same_file(file, "C:\\windows\\system32\\kernelbase.dll", "loaded-kernelbase.bin")) fail("kernelbase provider identity");
        kernelbase_base = address;
    }
    if (!_stricmp(name, "msvcrt.dll")) {
        if (!file || crt_seen || !same_file(file, crt_path, "loaded-msvcrt.bin")) fail("canonical MSVCRT identity");
        crt_seen = 1;
        crt_base = address;
    }
    if (strcmp(name, "c1n32_l.dll")) return;
    if (!file || target_seen || !same_file(file, target_path, "loaded-target.bin")) fail("target file identity");
    target_seen = 1; base = address;
    entry_rva = nt.OptionalHeader.AddressOfEntryPoint;
    image_size = nt.OptionalHeader.SizeOfImage;
    if (nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress) fail("unexpected TLS");
    at = address + dos.e_lfanew + 4 + sizeof(IMAGE_FILE_HEADER) + nt.FileHeader.SizeOfOptionalHeader;
    for (i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
        readmem(at + i * sizeof(section), &section, sizeof(section));
        if (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            if (code_size) fail("multiple executable sections unsupported");
            code_rva = section.VirtualAddress; code_size = section.Misc.VirtualSize;
        }
    }
    if (entry_rva != 0x2e3c6 || code_rva != 0x1000 || code_size != 0xb7670)
        fail("unexpected C1XX layout");
    /* The Python preflight proves this byte is outside every relocation site. */
    readmem(base + entry_rva, &first, 1);
    if (first != 0x53) fail("entry guard byte");
    patch(base + entry_rva, 0xcc);
    fprintf(events, "guard rva=%08lx original=%02x\n", entry_rva, first);
    fflush(events);
}

static void save_blob(const char *name, const void *bytes, unsigned length);

static void prepare_mask(void)
{
    BYTE *masked;
    HANDLE saved;
    DWORD wrote;
    unsigned i;
    DWORD resolved, adjust_value, initfe[43];
    static DWORD import_slots[4] = { 0xc9114, 0xc90a8, 0xc9140, 0xc9118 };
    static DWORD provider_rvas[4] = { 0x1715, 0x1498, 0x138a, 0x3f928 };
    for (i = 0; i < 4; ++i) {
        readmem(base + import_slots[i], &resolved, sizeof(resolved));
        if (resolved != crt_base + provider_rvas[i]) fail("canonical CRT import resolution");
        fprintf(events, "provider iat=%08lx resolved=%08lx\n", import_slots[i], resolved);
        if (i == 3) {
            readmem(resolved, &adjust_value, sizeof(adjust_value));
            fprintf(events, "provider-adjust-fdiv=%lu\n", adjust_value);
        }
    }
    readmem(base + 0xdf000, initfe, sizeof(initfe));
    save_blob("initfe.bin", initfe, sizeof(initfe));
    snapshot = (BYTE *)malloc(code_size);
    masked = (BYTE *)malloc(code_size);
    if (!snapshot || !masked) fail("snapshot allocation");
    readmem(base + code_rva, snapshot, code_size);
    snapshot[entry_rva - code_rva] = 0x53;
    saved = CreateFileA("relocated-text.bin", GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    if (saved == INVALID_HANDLE_VALUE || !WriteFile(saved, snapshot, code_size, &wrote, NULL) || wrote != code_size)
        fail("save relocated snapshot");
    CloseHandle(saved);
    memset(masked, 0xcc, code_size);
    if (baseline) memcpy(masked, snapshot, code_size);
    for (i = 0; i < count; ++i) {
        if (ranges[i].first < code_rva || ranges[i].last > code_rva + code_size ||
            ranges[i].first >= ranges[i].last) fail("range outside code");
        memcpy(masked + ranges[i].first - code_rva,
                snapshot + ranges[i].first - code_rva, ranges[i].last - ranges[i].first);
        if (ranges[i].entry) {
            ranges[i].saved = snapshot[ranges[i].first - code_rva];
            masked[ranges[i].first - code_rva] = 0xcc;
        }
    }
    write_mem(base + code_rva, masked, code_size);
    readmem(base + code_rva, masked, code_size);
    saved = CreateFileA("masked-text.bin", GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    if (saved == INVALID_HANDLE_VALUE || !WriteFile(saved, masked, code_size, &wrote, NULL) || wrote != code_size)
        fail("save actual masked image");
    CloseHandle(saved);
    free(masked); initialized = 1;
    if ((getenv("C1XX_TEST_INIT_RESTORE") || getenv("C1XX_OBSERVE_INIT_COPY")) && !baseline) {
        restore_observer_byte = snapshot[0x385be - code_rva];
        if (restore_observer_byte != 0x89) fail("restore observer instruction changed");
        patch(base + 0x385be, 0xcc); restore_observer = 1;
        fprintf(events, "restore-observer rva=000385be byte=89 after-mask-readback=1\n");
    }
    if (getenv("C1XX_OBSERVE_POOL_RETURN") && !baseline) {
        if (snapshot[0x389d4 - code_rva] != 0xc3) fail("pool return instruction changed");
        patch(base + 0x389d4, 0xcc); pool_return_observer = 1;
        fprintf(events, "pool-return-observer rva=000389d4 byte=c3 after-mask-readback=1\n");
    }
    if (getenv("C1XX_OBSERVE_ALLOCATION") && !baseline) {
        static const DWORD before[2] = { 0x38cdf, 0x38cf8 };
        static const DWORD after[2] = { 0x38ce1, 0x38cfa };
        if (!kernel_base || !kernelbase_base) fail("missing allocation providers");
        readmem(base + 0xc9204, &resolved, 4);
        fprintf(events, "allocation-provider iat=000c9204 resolved=%08lx\n", resolved);
        for (i = 0; i < 2; ++i) {
            allocation_before_byte[i] = snapshot[before[i] - code_rva];
            allocation_after_byte[i] = snapshot[after[i] - code_rva];
            if (allocation_before_byte[i] != 0xff || allocation_after_byte[i] != (i ? 0x85 : 0x8b)) fail("allocation observer instruction");
            patch(base + before[i], 0xcc);
        }
        allocation_observers = 1;
        fprintf(events, "allocation-observers before=00038cdf,00038cf8 after=00038ce1,00038cfa dynamic-return=actual-stack no-code-mock=1\n");
    }
    if (getenv("C1XX_OBSERVE_SYSTEM_INFO") && !baseline) {
        BYTE instruction[6];
        memcpy(instruction,snapshot + 0x38c42-code_rva,6);
        if (instruction[0]!=0xff || instruction[1]!=0x15 || instruction[2]!=0xf4 || instruction[3]!=0x91 || instruction[4]!=0x4c || instruction[5]!=0x10 || snapshot[0x38c48-code_rva]!=0xe8) fail("system info observer instruction");
        system_before_byte=instruction[0];system_after_byte=snapshot[0x38c48-code_rva];
        readmem(base+0xc91f4,&system_observer_provider,4);
        patch(base+0x38c42,0xcc);system_observer_active=1;
        fprintf(events,"system-info-observer before=00038c42 after=00038c48 iat=000c91f4 provider=%08lx after-mask-readback=1\n",system_observer_provider);
    }
    fprintf(events, "mask snapshot=relocated-text.bin baseline=%d ranges=%u\n", baseline, count);
}

static DWORD thread_error(CONTEXT *context, HANDLE thread)
{
    DWORD teb, last_error;
    LDT_ENTRY selector;
    if (!GetThreadSelectorEntry(thread, context->SegFs, &selector)) fail("thread TEB selector");
    teb = selector.BaseLow | ((DWORD)selector.HighWord.Bytes.BaseMid << 16) |
            ((DWORD)selector.HighWord.Bytes.BaseHi << 24);
    readmem(teb + 0x34, &last_error, sizeof(last_error));
    return last_error;
}

static void save_blob(const char *name, const void *bytes, unsigned length)
{
    FILE *file = fopen(name, "wb");
    if (!file || fwrite(bytes, 1, length, file) != length || fclose(file)) fail("save frontend state");
}

static void save_frontend_state(const char *suffix)
{
    DWORD values[36];
    BYTE data[8520];
    char name[80];
    readmem(base + 0xcd7e0, values, 4);
    readmem(base + 0xc4d50, values + 1, 8);
    readmem(base + 0xcb06c, values + 3, 4);
    readmem(base + 0xc4d58, values + 4, 128);
    sprintf(name, "frontend-state-%s.bin", suffix); save_blob(name, values, sizeof(values));
    readmem(base + 0xba900, data, 12);
    sprintf(name, "context-stack-%s.bin", suffix); save_blob(name, data, 12);
    readmem(base + 0xba8f0, data, 12);
    sprintf(name, "saved-context-stack-%s.bin", suffix); save_blob(name, data, 12);
    readmem(base + 0xba858, data, 108);
    sprintf(name, "nested-context-stacks-%s.bin", suffix); save_blob(name, data, 108);
    readmem(base + 0xbecb0, data, 8);
    sprintf(name, "source-position-%s.bin", suffix); save_blob(name, data, 8);
    readmem(base + 0xbf5c0, data, 28);
    readmem(base + 0xbf6e0, data + 28, 4);
    readmem(base + 0xbf5dc, data + 32, 1);
    sprintf(name, "output-buffer-01-%s.bin", suffix); save_blob(name, data, 33);
    readmem(base + 0xc0590, data, 28);
    readmem(base + 0xc06b0, data + 28, 4);
    readmem(base + 0xc05ac, data + 32, 1);
    sprintf(name, "output-buffer-02-%s.bin", suffix); save_blob(name, data, 33);
    readmem(base + 0xbf358, data, 28);
    readmem(base + 0xbf478, data + 28, 4);
    readmem(base + 0xbf374, data + 32, 1);
    sprintf(name, "output-buffer-03-%s.bin", suffix); save_blob(name, data, 33);
    readmem(base + 0xc0360, data, 28);
    readmem(base + 0xc0480, data + 28, 4);
    readmem(base + 0xc037c, data + 32, 1);
    sprintf(name, "output-buffer-04-%s.bin", suffix); save_blob(name, data, 33);
    readmem(base + 0xc0840, data, 28);
    readmem(base + 0xc0960, data + 28, 4);
    readmem(base + 0xc085c, data + 32, 1);
    sprintf(name, "output-buffer-05-%s.bin", suffix); save_blob(name, data, 33);
    readmem(base + 0xc0230, data, 28);
    readmem(base + 0xc0350, data + 28, 4);
    readmem(base + 0xc024c, data + 32, 1);
    sprintf(name, "output-buffer-06-%s.bin", suffix); save_blob(name, data, 33);
    readmem(base + 0xc00f8, data, 28);
    readmem(base + 0xc0218, data + 28, 4);
    readmem(base + 0xc0114, data + 32, 1);
    sprintf(name, "output-buffer-07-%s.bin", suffix); save_blob(name, data, 33);
    readmem(base + 0xbf228, data, 28);
    readmem(base + 0xbf348, data + 28, 4);
    readmem(base + 0xbf244, data + 32, 1);
    sprintf(name, "output-buffer-08-%s.bin", suffix); save_blob(name, data, 33);
    readmem(base + 0xc0578, data, 4);
    sprintf(name, "deferred-work-%s.bin", suffix); save_blob(name, data, 4);
    readmem(base + 0xba8c8, data, 40);
    sprintf(name, "token-source-%s.bin", suffix); save_blob(name, data, 40);
    readmem(base + 0xc0b88, data, 40);
    sprintf(name, "sentinel-payload-%s.bin", suffix); save_blob(name, data, 40);
    readmem(base + 0xc0b80, data, 8);
    sprintf(name, "sentinel-header-%s.bin", suffix); save_blob(name, data, 8);
    readmem(base + 0xcb050, data, 28);
    sprintf(name, "pdb-context-02-%s.bin", suffix); save_blob(name, data, 28);
    readmem(base + 0xcb030, data, 28);
    sprintf(name, "pdb-context-01-%s.bin", suffix); save_blob(name, data, 28);
    readmem(base + 0xbf5b0, data, 16);
    sprintf(name, "pool-02-%s.bin", suffix); save_blob(name, data, 16);
    readmem(base + 0xc0498, data, 16);
    sprintf(name, "pool-03-%s.bin", suffix); save_blob(name, data, 16);
    readmem(base + 0xbf210, data, 16);
    sprintf(name, "pool-04-%s.bin", suffix); save_blob(name, data, 16);
    readmem(base + 0xc06e8, data, 16);
    sprintf(name, "pool-05-%s.bin", suffix); save_blob(name, data, 16);
    readmem(base + 0xc0560, data, 16);
    sprintf(name, "pool-06-%s.bin", suffix); save_blob(name, data, 16);
    readmem(base + 0xc04a8, data, 16);
    sprintf(name, "pool-07-%s.bin", suffix); save_blob(name, data, 16);
    readmem(base + 0xbf498, data, 16);
    sprintf(name, "pool-08-%s.bin", suffix); save_blob(name, data, 16);
    readmem(base + 0xc0b00, data, 16);
    sprintf(name, "pool-09-%s.bin", suffix); save_blob(name, data, 16);
    readmem(base + 0xc0810, data, 16);
    sprintf(name, "pool-10-%s.bin", suffix); save_blob(name, data, 16);
    readmem(base + 0xc0ac8, data, 16);
    sprintf(name, "pool-01-%s.bin", suffix); save_blob(name, data, 16);
    readmem(base + 0xc4cc8, data, 65);
    sprintf(name, "primitive-type-rank-%s.bin", suffix); save_blob(name, data, 65);
    readmem(base + 0xc4c80, data, 69);
    sprintf(name, "complex-type-rank-%s.bin", suffix); save_blob(name, data, 69);
    readmem(base + 0xc6b64, data, 4);
    readmem(base + 0xc6ba8, data + 4, 74);
    readmem(base + 0xc6cb8, data + 78, 4);
    readmem(base + 0xc6c44, data + 82, 4);
    sprintf(name, "generated-comment-%s.bin", suffix); save_blob(name, data, 86);
    readmem(base + 0xcd5c0, data, 4);
    readmem(base + 0xcd488, data + 4, 4);
    readmem(base + 0xccf7c, data + 8, 4);
    readmem(base + 0xcd7b8, data + 12, 36);
    readmem(base + 0xc1704, data + 48, 12);
    sprintf(name, "main-fields-%s.bin", suffix); save_blob(name, data, 60);
    readmem(base + 0xba850, data, 4);
    readmem(base + 0xb9020, data + 4, 4);
    readmem(base + 0xba854, data + 8, 4);
    readmem(base + 0xb9028, data + 12, 4);
    sprintf(name, "work-buffers-%s.bin", suffix); save_blob(name, data, 16);
    readmem(base + 0xc6500, data, 288);
    sprintf(name, "arenas-%s.bin", suffix); save_blob(name, data, 288);
    readmem(base + 0xd5af8, data, 180);
    sprintf(name, "arena-descriptors-%s.bin", suffix); save_blob(name, data, 180);
    readmem(base + 0xc6d28, data, 12);
    sprintf(name, "deferred-actions-%s.bin", suffix); save_blob(name, data, 12);
    readmem(base + 0xc6b40, data, 12);
    sprintf(name, "pending-groups-%s.bin", suffix); save_blob(name, data, 12);
    readmem(base + 0xc64b8, data, 68);
    sprintf(name, "pch-context-%s.bin", suffix); save_blob(name, data, 68);
    readmem(base + 0xc64a0, data, 20);
    sprintf(name, "empty-prototype-%s.bin", suffix); save_blob(name, data, 20);
    readmem(base + 0xc4c78, data, 4);
    sprintf(name, "type-ranks-ready-%s.bin", suffix); save_blob(name, data, 4);
    readmem(base + 0xc0834, data, 4);
    sprintf(name, "source-signature-%s.bin", suffix); save_blob(name, data, 4);
    readmem(base + 0xcc000, data, sizeof(data));
    sprintf(name, "frontend-data-%s.bin", suffix); save_blob(name, data, sizeof(data));
    if (values[1]) {
        readmem(values[1], data, sizeof(data));
        sprintf(name, "frontend-snapshot-%s.bin", suffix); save_blob(name, data, sizeof(data));
    }
}

static void save_aux_state(const char *suffix)
{
    BYTE guards[64];
    DWORD fields[4];
    char name[80];
    if (!aux_new_base) return;
    readmem(aux_old_base, guards, 32); readmem(aux_old_base + 32 + 8520, guards + 32, 32);
    sprintf(name, "aux-old-guards-%s.bin", suffix); save_blob(name, guards, 64);
    readmem(aux_new_base, guards, 32); readmem(aux_new_base + 32 + 8520, guards + 32, 32);
    sprintf(name, "aux-new-guards-%s.bin", suffix); save_blob(name, guards, 64);
    fields[0] = aux_old_base + 32; fields[1] = aux_new_base + 32;
    readmem(fields[0] + 0x17e0, fields + 2, 4); readmem(fields[1] + 0x17e0, fields + 3, 4);
    sprintf(name, "aux-owned-fields-%s.bin", suffix); save_blob(name, fields, sizeof(fields));
}

static void seed_output_buffer(unsigned index, const char *role)
{
    DWORD allocated, value;
    static const DWORD objects[] = { 0xbf5c0, 0xc0590, 0xbf358, 0xc0360, 0xc0840, 0xc0230, 0xc00f8, 0xbf228 };
    DWORD object;
    if (index >= 8) fail("unmodeled output seed");
    object = base + objects[index];
    readmem(base + 0xc4d50, &allocated, 4);
    if (!allocated) fail("output seed needs owned snapshot");
    value = allocated; write_mem(object + 0x0, &value, 4);
    value = allocated + 4; write_mem(object + 0x4, &value, 4);
    value = 64; write_mem(object + 0x8, &value, 4);
    value = base + 0x38610; write_mem(object + 0xc, &value, 4);
    value = allocated + 8; write_mem(object + 0x10, &value, 4);
    value = allocated + 12; write_mem(object + 0x14, &value, 4);
    value = 53; write_mem(object + 0x18, &value, 4);
    value = 37; write_mem(object + 0x120, &value, 4);
    value = 23; write_mem(object + 0x1c, &value, 1);
    fprintf(events, "output-seed object=%u role=%s data=snapshot+0 cursor=snapshot+4 context=snapshot+8 file=snapshot+12 unused-relocation=_initialize_frontend_01 capacity=64 ok=53 descriptor=37 mode=23\n", index + 1, role);
}

static void seed_frontend_restore(void)
{
    const char *seed = getenv("C1XX_TEST_INIT_RESTORE");
    BYTE bytes[8520], guards[32];
    DWORD allocated, value, callbacks[32];
    unsigned i;
    int first_copy;
    if (!seed) return;
    first_copy = !strcmp(seed, "first-copy-aux");
    if (!first_copy && strcmp(seed, "restore-nonzero") && strcmp(seed, "restore-zero")) fail("unknown restore seed");
    readmem(base + 0xcc000, bytes, sizeof(bytes));
    value = first_copy ? 23 : 17; memcpy(bytes + 0x17e0, &value, 4);
    for (i = 0; i < 8; ++i) bytes[0x1708 + i * 8 + 4] += 40;
    for (i = 0; i < 9; ++i) bytes[0x1748 + i * 8 + 4] += 40;
    value = 0x20; memcpy(bytes + 0x1708, &value, 4);
    value = 5; memcpy(bytes + 0x1748, &value, 4);
    if (first_copy) {
        aux_old_base = (DWORD)VirtualAllocEx(child, NULL, 8584, MEM_COMMIT, PAGE_READWRITE);
        aux_new_base = (DWORD)VirtualAllocEx(child, NULL, 8584, MEM_COMMIT, PAGE_READWRITE);
        if (!aux_old_base || !aux_new_base) fail("owned aux buffers");
        memset(guards, 0xa7, sizeof(guards));
        write_mem(aux_old_base, guards, 32); write_mem(aux_old_base + 32 + 8520, guards, 32);
        memset(guards, 0xd3, sizeof(guards));
        write_mem(aux_new_base, guards, 32); write_mem(aux_new_base + 32 + 8520, guards, 32);
        allocated = aux_old_base + 32;
        value = 303; write_mem(aux_new_base + 32 + 0x17e0, &value, 4);
        fprintf(events, "aux-owned role=old-snapshot base=%08lx payload=%08lx request=8584 payload-bytes=8520 guards=32 pattern=a7 first=23\n", aux_old_base, allocated);
        fprintf(events, "aux-owned role=mock-return base=%08lx payload=%08lx request=8584 payload-bytes=8520 guards=32 pattern=d3 first=303\n", aux_new_base, aux_new_base + 32);
    } else {
        allocated = (DWORD)VirtualAllocEx(child, NULL, sizeof(bytes), MEM_COMMIT, PAGE_READWRITE);
        if (!allocated) fail("restore-test allocation");
    }
    value = 23; memcpy(bytes + 0x15c0, &value, 4);
    value = 31; memcpy(bytes + 0x109c, &value, 4);
    value = -37; memcpy(bytes + 0x14ac, &value, 4);
    value = 41; memcpy(bytes + 0x1c28, &value, 4);
    value = 43; write_mem(base + 0xcd09c, &value, 4);
    value = -47; write_mem(base + 0xcd4ac, &value, 4);
    value = 53; write_mem(base + 0xcdc28, &value, 4);
    value = 29; memcpy(bytes + 0x1488, &value, 4);
    value = 0x55667788; memcpy(bytes + 0xf7c, &value, 4);
    {
        SYSTEM_INFO info;
        info.dwOemId = 0x11110007; info.dwPageSize = 8192;
        info.lpMinimumApplicationAddress = (void *)(allocated + 4);
        info.lpMaximumApplicationAddress = (void *)(allocated + 8);
        info.dwActiveProcessorMask = 3; info.dwNumberOfProcessors = 2;
        info.dwProcessorType = 586; info.dwAllocationGranularity = 131072;
        info.wProcessorLevel = 6; info.wProcessorRevision = 1234;
        memcpy(bytes + 0x17b8, &info, sizeof(info));
        info.dwOemId = 0x22220003; info.dwPageSize = 12288;
        info.lpMinimumApplicationAddress = (void *)(allocated + 12);
        info.lpMaximumApplicationAddress = (void *)(allocated + 16);
        info.dwActiveProcessorMask = 7; info.dwNumberOfProcessors = 4;
        info.dwProcessorType = 486; info.dwAllocationGranularity = 196608;
        info.wProcessorLevel = 5; info.wProcessorRevision = 4321;
        write_mem(base + 0xcd7b8, &info, sizeof(info));
    }
    value = 17; write_mem(base + 0xcd5c0, &value, 4);
    value = 19; write_mem(base + 0xcd488, &value, 4);
    value = 0x11223344; write_mem(base + 0xccf7c, &value, 4);
    value = 17; write_mem(base + 0xc1704, &value, 4);
    value = 23; write_mem(base + 0xc1708, &value, 4);
    value = allocated + 12; write_mem(base + 0xc170c, &value, 4);
    write_mem(allocated, bytes, sizeof(bytes));
    for (i = 0; i < 8; ++i) {
        value = 31337 + i; write_mem(base + 0xcd708 + i * 8, &value, 4);
        value = 200 + i; write_mem(base + 0xcd70c + i * 8, &value, 1);
    }
    for (i = 0; i < 9; ++i) {
        value = 31437 + i; write_mem(base + 0xcd748 + i * 8, &value, 4);
        value = 210 + i; write_mem(base + 0xcd74c + i * 8, &value, 1);
    }
    memset(bytes, 181, 65); write_mem(base + 0xc4cc8, bytes, 65);
    memset(bytes, 182, 69); write_mem(base + 0xc4c80, bytes, 69);
    write_mem(base + 0xc4d50, &allocated, 4);
    value = first_copy ? 17 : 0; write_mem(base + 0xcd7e0, &value, 4);
    value = 32; write_mem(base + 0xc4d54, &value, 4);
    for (i = 0; i < 32; ++i) callbacks[i] = base + 0x38610;
    write_mem(base + 0xc4d58, callbacks, sizeof(callbacks));
    for (i = 0; i < 3; ++i) {
        value = i == 1 ? 17 : allocated;
        write_mem(base + 0xba900 + 4 * i, &value, 4);
        write_mem(base + 0xba8f0 + 4 * i, &value, 4);
    }
    for (i = 0; i < 27; ++i) {
        value = i % 3 == 1 ? 17 : allocated;
        write_mem(base + 0xba858 + 4 * i, &value, 4);
    }
    write_mem(base + 0xbecb0, &allocated, 4);
    value = 17; write_mem(base + 0xbecb4, &value, 4);
    seed_output_buffer(0, "nonzero-BSS-reset");
    seed_output_buffer(1, "nonzero-BSS-reset");
    seed_output_buffer(2, "nonzero-BSS-reset");
    seed_output_buffer(3, "nonzero-BSS-reset");
    seed_output_buffer(4, "nonzero-BSS-reset");
    seed_output_buffer(5, "nonzero-BSS-reset");
    seed_output_buffer(6, "nonzero-BSS-reset");
    seed_output_buffer(7, "nonzero-BSS-reset");
    write_mem(base + 0xc0578, &allocated, 4);
    {
        DWORD numeric[2];
        numeric[0] = 0x55667788; numeric[1] = 0x11223344;
        write_mem(base + 0xba8c8, numeric, 8);
        numeric[0] = 0xfffffff9; numeric[1] = 0xffffffff;
        write_mem(base + 0xc0b88, numeric, 8);
        value = 165; write_mem(base + 0xc0b80, &value, 1);
    }
    memset(bytes, 165, 20); write_mem(base + 0xc64a0, bytes, 20);
    value = 37; write_mem(base + 0xc4c78, &value, 4);
    value = 37; write_mem(base + 0xc0834, &value, 4);
    value = allocated + 4; write_mem(base + 0xc0ac8, &value, 4);
    value = 23; write_mem(base + 0xc0acc, &value, 4);
    value = 37; write_mem(base + 0xc0ad0, &value, 4);
    value = allocated + 8; write_mem(base + 0xc0ad4, &value, 4);
    value = allocated + 4; write_mem(base + 0xbf5b0, &value, 4);
    value = 23; write_mem(base + 0xbf5b4, &value, 4);
    value = 37; write_mem(base + 0xbf5b8, &value, 4);
    value = allocated + 8; write_mem(base + 0xbf5bc, &value, 4);
    value = allocated + 4; write_mem(base + 0xc0498, &value, 4);
    value = 23; write_mem(base + 0xc049c, &value, 4);
    value = 37; write_mem(base + 0xc04a0, &value, 4);
    value = allocated + 8; write_mem(base + 0xc04a4, &value, 4);
    value = allocated + 4; write_mem(base + 0xbf210, &value, 4);
    value = 23; write_mem(base + 0xbf214, &value, 4);
    value = 37; write_mem(base + 0xbf218, &value, 4);
    value = allocated + 8; write_mem(base + 0xbf21c, &value, 4);
    value = allocated + 4; write_mem(base + 0xc06e8, &value, 4);
    value = 23; write_mem(base + 0xc06ec, &value, 4);
    value = 37; write_mem(base + 0xc06f0, &value, 4);
    value = allocated + 8; write_mem(base + 0xc06f4, &value, 4);
    value = allocated + 4; write_mem(base + 0xc0560, &value, 4);
    value = 23; write_mem(base + 0xc0564, &value, 4);
    value = 37; write_mem(base + 0xc0568, &value, 4);
    value = allocated + 8; write_mem(base + 0xc056c, &value, 4);
    value = allocated + 4; write_mem(base + 0xc04a8, &value, 4);
    value = 23; write_mem(base + 0xc04ac, &value, 4);
    value = 37; write_mem(base + 0xc04b0, &value, 4);
    value = allocated + 8; write_mem(base + 0xc04b4, &value, 4);
    value = allocated + 4; write_mem(base + 0xbf498, &value, 4);
    value = 23; write_mem(base + 0xbf49c, &value, 4);
    value = 37; write_mem(base + 0xbf4a0, &value, 4);
    value = allocated + 8; write_mem(base + 0xbf4a4, &value, 4);
    value = allocated + 4; write_mem(base + 0xc0b00, &value, 4);
    value = 23; write_mem(base + 0xc0b04, &value, 4);
    value = 37; write_mem(base + 0xc0b08, &value, 4);
    value = allocated + 8; write_mem(base + 0xc0b0c, &value, 4);
    value = allocated + 4; write_mem(base + 0xc0810, &value, 4);
    value = 23; write_mem(base + 0xc0814, &value, 4);
    value = 37; write_mem(base + 0xc0818, &value, 4);
    value = allocated + 8; write_mem(base + 0xc081c, &value, 4);
    value = 17; write_mem(base + 0xc64b8, &value, 4);
    value = allocated + 4; write_mem(base + 0xc64bc, &value, 4);
    value = allocated + 8; write_mem(base + 0xc64c0, &value, 4);
    value = 23; write_mem(base + 0xc64c4, &value, 4);
    value = allocated + 12; write_mem(base + 0xc64c8, &value, 4);
    value = -11; write_mem(base + 0xc64cc, &value, 4);
    value = allocated + 16; write_mem(base + 0xc64d0, &value, 4);
    value = allocated + 20; write_mem(base + 0xc64d4, &value, 4);
    value = allocated + 24; write_mem(base + 0xc64d8, &value, 4);
    value = allocated + 28; write_mem(base + 0xc64dc, &value, 4);
    value = allocated + 32; write_mem(base + 0xc64e0, &value, 4);
    value = allocated + 36; write_mem(base + 0xc64e4, &value, 4);
    value = allocated + 40; write_mem(base + 0xc64e8, &value, 4);
    value = 29; write_mem(base + 0xc64ec, &value, 4);
    value = allocated + 44; write_mem(base + 0xc64f0, &value, 4);
    value = allocated + 48; write_mem(base + 0xc64f4, &value, 4);
    value = allocated + 52; write_mem(base + 0xc64f8, &value, 4);
    value = 37; write_mem(base + 0xc6b64, &value, 4);
    memset(bytes, 165, 74); write_mem(base + 0xc6ba8, bytes, 74);
    value = 23; write_mem(base + 0xc6cb8, &value, 4);
    value = 29; write_mem(base + 0xc6c44, &value, 4);
    value = allocated + 4; write_mem(base + 0xc6d28, &value, 4);
    value = allocated + 8; write_mem(base + 0xc6d2c, &value, 4);
    value = 17; write_mem(base + 0xc6d30, &value, 4);
    value = allocated + 100; write_mem(base + 0xba850, &value, 4);
    value = allocated + 200; write_mem(base + 0xb9020, &value, 4);
    value = allocated + 300; write_mem(base + 0xba854, &value, 4);
    value = allocated + 400; write_mem(base + 0xb9028, &value, 4);
    value = allocated + 4; write_mem(base + 0xc6b40, &value, 4);
    value = allocated + 8; write_mem(base + 0xc6b48, &value, 4);
    {
        DWORD arenas[72]; unsigned i;
        for (i = 0; i < 9; ++i) {
            arenas[i*8] = allocated + 64 + 4*i;
            arenas[i*8+1] = allocated + 128 + 4*i;
            arenas[i*8+2] = allocated + 192 + 4*i;
            arenas[i*8+3] = 17;
            arenas[i*8+4] = base + 0xd5af8 + i*20;
            arenas[i*8+5] = 23;
            arenas[i*8+6] = 37;
            arenas[i*8+7] = 29;
        }
        write_mem(base + 0xc6500, arenas, sizeof(arenas));
    }
    value = 9; write_mem(base + 0xcb06c, &value, 4);
    value = !strcmp(seed, "restore-zero") ? 0 : 3; write_mem(base + 0xcb008, &value, 4);
    fprintf(events, "restore-seed mode=%s allocation=%08lx bytes=8520 cleanup-count=32 mocked-cleanup-callback=_initialize_frontend_01 slots=32\n", seed, allocated);
    save_frontend_state("seed"); save_aux_state("seed");
}

static void save_state(void)
{
    BYTE globals[0xa4];
    DWORD begin, first = 0;
    char filename[80], suffix[16];
    FILE *state;
    sprintf(suffix, "%03u", sequence); save_frontend_state(suffix); save_aux_state(suffix);
    readmem(base + 0xcb000, globals, sizeof(globals));
    memcpy(&begin, globals + 0x1c, 4);
    if (begin) readmem(begin, &first, 4);
    fprintf(events, "state seq=%u heap-begin=%08lx heap-first=%08lx\n", sequence, begin, first);
    sprintf(filename, "state-%03u.bin", sequence++);
    state = fopen(filename, "wb");
    if (!state || fwrite(globals, 1, sizeof(globals), state) != sizeof(globals)) fail("save state");
    fclose(state); fflush(events);
}

static int allocation_event(CONTEXT *context, DWORD address, HANDLE thread)
{
    static const DWORD before[2] = { 0x38cdf, 0x38cf8 };
    static const DWORD after[2] = { 0x38ce1, 0x38cfa };
    DWORD words[4], cursor, end;
    MEMORY_BASIC_INFORMATION region;
    unsigned site, index, part;
    char name[80], suffix[40];
    BYTE committed[0x4000];
    if (!allocation_observers) return 0;
    if (allocation_return_active && address == allocation_return_pc) {
        index = allocation_helper_count - 1;
        fprintf(events, "allocation-helper-return index=%u pc=%08lx esp=%08lx eax=%08lx edi=%08lx esi=%08lx\n",
                index, address, context->Esp, context->Eax, context->Edi, context->Esi);
        sprintf(suffix, "reserve-return-%02u", index + 1); save_frontend_state(suffix);
        patch(address, allocation_return_byte); allocation_return_active = 0;
        context->Eip = address; return 1;
    }
    for (site = 0; site < 2; ++site) {
        if (address == base + before[site]) {
            if (allocation_api_count >= 4 || site != allocation_api_count % 2) fail("allocation API order");
            index = allocation_api_count++;
            readmem(context->Esp, words, sizeof(words));
            fprintf(events, "allocation-before index=%u pc=%08lx esp=%08lx edi=%08lx args=%08lx,%08lx,%08lx,%08lx error=%lu fpu=%04lx\n",
                    index, address, context->Esp, context->Edi, words[0], words[1], words[2], words[3], thread_error(context, thread), context->FloatSave.ControlWord);
            patch(address, allocation_before_byte[site]); patch(base + after[site], 0xcc);
            context->Eip = address; return 1;
        }
        if (address == base + after[site]) {
            if (!allocation_api_count || site != (allocation_api_count - 1) % 2) fail("allocation API return order");
            index = allocation_api_count - 1;
            fprintf(events, "allocation-after index=%u pc=%08lx esp=%08lx eax=%08lx edi=%08lx error=%lu fpu=%04lx\n",
                    index, address, context->Esp, context->Eax, context->Edi, thread_error(context, thread), context->FloatSave.ControlWord);
            if (!context->Eax) fail("uncharacterized original allocation failure");
            cursor = context->Eax; end = cursor + 0x500000; part = 0;
            while (cursor < end) {
                if (VirtualQueryEx(child, (void *)cursor, &region, sizeof(region)) != sizeof(region) ||
                        (DWORD)region.BaseAddress != cursor || !region.RegionSize || cursor + region.RegionSize > end) fail("allocation region inventory");
                fprintf(events, "allocation-region index=%u part=%u base=%08lx allocation=%08lx allocation-protect=%08lx bytes=%08lx state=%08lx protect=%08lx type=%08lx\n",
                        index, part++, (DWORD)region.BaseAddress, (DWORD)region.AllocationBase, region.AllocationProtect,
                        region.RegionSize, region.State, region.Protect, region.Type);
                cursor += region.RegionSize;
            }
            if (site) {
                readmem(context->Eax, committed, sizeof(committed));
                sprintf(name, "work-committed-%02u.bin", index / 2 + 1); save_blob(name, committed, sizeof(committed));
            }
            patch(address, allocation_after_byte[site]); patch(base + before[site], 0xcc);
            context->Eip = address; return 1;
        }
    }
    return 0;
}

static void invocation_arguments(CONTEXT *context);

static void boundary(CONTEXT *context, DWORD address, HANDLE thread)
{
    DWORD stack[5];
    readmem(context->Esp, stack, sizeof(stack));
    fprintf(events, "trap seq=%u rva=%08lx esp=%08lx eax=%08lx ecx=%08lx edi=%08lx stack=%08lx,%08lx,%08lx,%08lx,%08lx\n",
            sequence, address - base, context->Esp, context->Eax, context->Ecx, context->Edi,
            stack[0], stack[1], stack[2], stack[3], stack[4]);
    fprintf(events, "last-error=%lu\n", thread_error(context, thread));
    fprintf(events, "control-word=%04lx\n", context->FloatSave.ControlWord);
    save_state();
    if (address == base + 0x384d4) invocation_arguments(context);
    if (address == base + entry_rva) {
        if (return_active) fail("nested target DLL entry unsupported");
        return_address = stack[0]; return_reason = stack[2];
        readmem(return_address, &return_saved, 1);
        patch(return_address, 0xcc); return_active = 1;
    }
}

static void invocation_arguments(CONTEXT *context)
{
    DWORD stack[4], i, pointer, length;
    FILE *file;
    char text[32768];
    readmem(context->Esp, stack, sizeof(stack));
    if (!stack[1] || stack[1] > 512 || !stack[2] || stack[3] != 1) fail("unobserved invocation ABI");
    file = fopen("invoke-entry-argv.bin", "wb");
    if (!file) fail("save invoke argv");
    fwrite(stack + 2, 4, 1, file); fwrite(stack + 1, 4, 1, file);
    for (i = 0; i < stack[1]; ++i) {
        readmem(stack[2] + i * 4, &pointer, 4);
        remote_string(pointer, text, sizeof(text)); length = strlen(text) + 1;
        fwrite(&pointer, 4, 1, file); fwrite(&length, 4, 1, file); fwrite(text, 1, length, file);
    }
    fclose(file);
    file = fopen("invoke-entry-context.bin", "wb");
    if (!file || fwrite(context, 1, sizeof(*context), file) != sizeof(*context)) fail("save invocation context");
    fclose(file);
}

static void save_crt_vector(const char *name, char **values, unsigned count)
{
    FILE *file;
    DWORD address=(DWORD)values,i,pointer,length;
    file=fopen(name,"wb");if(!file)exit(126);
    if(fwrite(&address,4,1,file)!=1 || fwrite(&count,4,1,file)!=1)exit(126);
    for(i=0;i<count;++i) {
        pointer=(DWORD)values[i];length=strlen(values[i])+1;
        if(fwrite(&pointer,4,1,file)!=1 || fwrite(&length,4,1,file)!=1 || fwrite(values[i],1,length,file)!=length)exit(126);
    }
    if(fclose(file))exit(126);
}

static int host(const char *path)
{
    HMODULE module;
    FARPROC invoke, abort;
    const char *argument_file;
    int pass_argc, pass_result;
    char **pass_argv;
    module = LoadLibraryA(path);
    if (!module) return 3;
    puts("attach=1");
    invoke = GetProcAddress(module, "_InvokeCompilerPass@12");
    abort = GetProcAddress(module, "_AbortCompilerPass@4");
    if (!invoke || !abort || invoke != GetProcAddress(module, (LPCSTR)2) ||
            abort != GetProcAddress(module, (LPCSTR)1)) return 4;
    puts("exports=1");
    argument_file = getenv("C1XX_INVOKE_ARGV");
    if (argument_file) {
        unsigned count=0;
        pass_argc = read_invoke_arguments(argument_file, &pass_argv);
        while(_environ[count]) {if(count>=16384)return 126;++count;}
        save_crt_vector("original-crt-argv.bin",__argv,__argc);
        save_crt_vector("original-crt-environment.bin",_environ,count);
        SetLastError(203);
        pass_result = ((int (WINAPI *)(int,char **,int))invoke)(pass_argc,pass_argv,1);
        printf("invoke=%d\n", pass_result);
        if (pass_result) return pass_result;
    }
    if (!FreeLibrary(module)) return 5;
    puts("detach=1");
    return 0;
}

int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    DEBUG_EVENT event;
    CONTEXT context;
    HANDLE thread;
    DWORD status, address, code, result;
    char self[MAX_PATH], command[2048];
    FILE *input;
    unsigned i;
    if (argc == 3 && !strcmp(argv[1], "--host")) return host(argv[2]);
    if (argc != 6 || strcmp(argv[1], "--debug")) return 2;
    strcpy(target_path, argv[2]); strcpy(crt_path, argv[3]);
    baseline = !strcmp(argv[4], "baseline");
    if (!baseline && strcmp(argv[4], "masked")) return 2;
    input = fopen(argv[5], "r");
    if (!input) return 2;
    while (count < 512 && fscanf(input, "%lx %lx %d", &ranges[count].first,
            &ranges[count].last, &ranges[count].entry) == 3) ++count;
    if (!feof(input)) return 2;
    fclose(input);
    events = fopen("events.txt", "wb");
    if (!events || !GetModuleFileNameA(NULL, self, sizeof(self))) return 2;
    sprintf(command, "\"%s\" --host \"%s\"", self, target_path);
    memset(&startup, 0, sizeof(startup)); startup.cb = sizeof(startup);
    if (!CreateProcessA(self, command, NULL, NULL, TRUE, DEBUG_ONLY_THIS_PROCESS,
            NULL, NULL, &startup, &process)) fail("CreateProcess");
    for (;;) {
        if (!WaitForDebugEvent(&event, 30000)) fail("debug event timeout");
        status = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            child = event.u.CreateProcessInfo.hProcess;
            thread_ids[thread_count] = event.dwThreadId;
            threads[thread_count++] = event.u.CreateProcessInfo.hThread;
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            if (thread_count == 64) fail("too many threads");
            thread_ids[thread_count] = event.dwThreadId;
            threads[thread_count++] = event.u.CreateThread.hThread;
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            module_loaded((DWORD)event.u.LoadDll.lpBaseOfDll, event.u.LoadDll.hFile);
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = event.u.Exception.ExceptionRecord.ExceptionCode;
            address = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            thread = NULL;
            for (i = 0; i < thread_count; ++i)
                if (thread_ids[i] == event.dwThreadId) thread = threads[i];
            if (!thread) fail("unknown debug thread");
            memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
            if (!GetThreadContext(thread, &context)) fail("GetThreadContext");
            if (code == EXCEPTION_SINGLE_STEP && pending >= 0) {
                patch(base + ranges[pending].first, 0xcc);
                context.EFlags &= ~0x100; pending = -1;
                if (!SetThreadContext(thread, &context)) fail("single step context");
            } else if (code == EXCEPTION_BREAKPOINT && system_observer_active && (address==base+0x38c42 || address==base+0x38c48)) {
                DWORD argument;BYTE *state_bss;SYSTEM_INFO info;
                state_bss=(BYTE *)malloc(0xf69c);if(!state_bss)fail("system info observation buffer");
                if (!system_observer_stage) {
                    if (address!=base+0x38c42) fail("system info observer order");
                    readmem(context.Esp,&argument,4);if(argument!=base+0xcd7b8) fail("system info destination");
                    system_observer_esp=context.Esp;
                    fprintf(events,"system-info-before pc=%08lx esp=%08lx argument=%08lx provider=%08lx edi=%08lx esi=%08lx ebx=%08lx error=%lu fpu=%04lx\n",address,context.Esp,argument,system_observer_provider,context.Edi,context.Esi,context.Ebx,thread_error(&context,thread),context.FloatSave.ControlWord);
                    readmem(argument,&info,sizeof(info));save_blob("system-info-before.bin",&info,sizeof(info));
                    readmem(base+0xb9000,state_bss,0xf69c);save_blob("system-info-bss-before.bin",state_bss,0xf69c);save_frontend_state("system-info-before");
                    patch(address,system_before_byte);patch(base+0x38c48,0xcc);system_observer_stage=1;
                } else {
                    if (address!=base+0x38c48 || context.Esp!=system_observer_esp+4) fail("system info return ABI");
                    fprintf(events,"system-info-after pc=%08lx esp=%08lx edi=%08lx esi=%08lx ebx=%08lx error=%lu fpu=%04lx\n",address,context.Esp,context.Edi,context.Esi,context.Ebx,thread_error(&context,thread),context.FloatSave.ControlWord);
                    readmem(base+0xcd7b8,&info,sizeof(info));save_blob("system-info-after.bin",&info,sizeof(info));
                    readmem(base+0xb9000,state_bss,0xf69c);save_blob("system-info-bss-after.bin",state_bss,0xf69c);save_frontend_state("system-info-after");
                    patch(address,system_after_byte);system_observer_active=0;
                }
                free(state_bss);context.Eip=address;if(!SetThreadContext(thread,&context))fail("system info observer context");
            } else if (code == EXCEPTION_BREAKPOINT && option_return_active && address == option_return_pc) {
                DWORD fields[3];
                readmem(base + 0xcd09c, fields, 4);
                readmem(base + 0xcd4ac, fields + 1, 4);
                readmem(base + 0xcdc28, fields + 2, 4);
                fprintf(events, "early-option-return index=%u pc=%08lx esp=%08lx eax=%08lx edi=%08lx esi=%08lx ebx=%08lx values=%08lx,%08lx,%08lx error=%lu fpu=%04lx\n", option_calls-1, address, context.Esp, context.Eax, context.Edi, context.Esi, context.Ebx, fields[0], fields[1], fields[2], thread_error(&context,thread), context.FloatSave.ControlWord);
                patch(address, option_return_byte); option_return_active = 0;
                context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("option return observer context");
            } else if (code == EXCEPTION_BREAKPOINT && allocation_event(&context, address, thread)) {
                if (!SetThreadContext(thread, &context)) fail("allocation observer context");
            } else if (code == EXCEPTION_BREAKPOINT && return_active && address == return_address) {
                fprintf(events, "crt-return seq=%u reason=%lu status=%lu pc=%08lx\n",
                        sequence, return_reason, context.Eax, address);
                fprintf(events, "last-error=%lu\n", thread_error(&context, thread));
                fprintf(events, "control-word=%04lx\n", context.FloatSave.ControlWord);
                save_state();
                patch(return_address, return_saved); return_active = 0;
                context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("CRT return context");
            } else if (code == EXCEPTION_BREAKPOINT && restore_observer && address == base + 0x385be) {
                fprintf(events, "restore-phase rva=000385be\n");
                save_frontend_state("phase"); save_aux_state("phase");
                patch(address, restore_observer_byte); restore_observer = 0;
                context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("restore observer context");
            } else if (code == EXCEPTION_BREAKPOINT && pch_return_active && address == pch_return_address) {
                BYTE pch_bss[0xf69c];
                fprintf(events, "pch-return pc=%08lx eax=%08lx esp=%08lx edi=%08lx\n", address, context.Eax, context.Esp, context.Edi);
                fprintf(events, "last-error=%lu\n", thread_error(&context, thread));
                fprintf(events, "control-word=%04lx\n", context.FloatSave.ControlWord);
                save_frontend_state("pch-return");
                readmem(base + 0xb9000, pch_bss, sizeof(pch_bss));
                save_blob("pch-bss-return.bin", pch_bss, sizeof(pch_bss));
                patch(address, pch_return_saved); pch_return_active = 0;
                context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("PCH return context");
            } else if (code == EXCEPTION_BREAKPOINT && pool_return_observer && address == base + 0x389d4) {
                fprintf(events, "pool-return rva=000389d4 eax=%08lx esp=%08lx\n", context.Eax, context.Esp);
                save_frontend_state("pool-return");
                patch(address, 0xc3); pool_return_observer = 0;
                context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("pool return context");
            } else if (code == EXCEPTION_BREAKPOINT && target_seen &&
                    address >= base + code_rva && address < base + code_rva + code_size) {
                if (!initialized) {
                    if (address != base + entry_rva || !crt_seen) fail("unguarded entry/provider");
                    prepare_mask();
                }
                if (address == base + 0x384d4) seed_frontend_restore();
                if (address == base + 0x38af9 || address == base + 0x386ae) {
                    DWORD caller_words[2];
                    BYTE comment_bss[0xf69c];
                    readmem(context.Esp, caller_words, sizeof(caller_words));
                    if (address == base + 0x38af9 || caller_words[1] == base + 0x346d4) {
                        if (address == base + 0x38af9 && getenv("C1XX_TEST_COMMENT")) {
                            DWORD value; BYTE input[74];
                            value = 37; write_mem(base + 0xc6b64, &value, 4);
                            memset(input, 165, sizeof(input)); write_mem(base + 0xc6ba8, input, sizeof(input));
                            value = 23; write_mem(base + 0xc6cb8, &value, 4);
                            value = 29; write_mem(base + 0xc6c44, &value, 4);
                            fprintf(events, "comment-seed actual-entry=00038af9 bytes=74 value=165 com=37 offset=23 length=29 code-mock=0\n");
                        }
                        readmem(base + 0xb9000, comment_bss, sizeof(comment_bss));
                        save_blob(address == base + 0x38af9 ? "comment-bss-entry.bin" : "comment-bss-result.bin", comment_bss, sizeof(comment_bss));
                    }
                }
                if (address == base + 0x38bea || address == base + 0x38bfc) {
                    BYTE action_bss[0xf69c];
                    if (address == base + 0x38bea && getenv("C1XX_TEST_ACTION")) {
                        DWORD owned, values[3];
                        readmem(base + 0xc4d50, &owned, 4);
                        if (!owned) fail("action seed needs snapshot identity");
                        values[0] = owned + 4; values[1] = owned + 8; values[2] = 17;
                        write_mem(base + 0xc6d28, values, sizeof(values));
                        fprintf(events, "action-seed actual-entry=00038bea head=snapshot+4 tail=snapshot+8 depth=17 code-mock=0\n");
                    }
                    readmem(base + 0xb9000, action_bss, sizeof(action_bss));
                    save_blob(address == base + 0x38bea ? "action-bss-entry.bin" : "action-bss-result.bin", action_bss, sizeof(action_bss));
                }
                if (address == base + 0x38e70) {
                    BYTE *member_bss = (BYTE *)malloc(0xf69c);
                    if (!member_bss) fail("allocate arena member BSS observation");
                    readmem(base + 0xb9000, member_bss, 0xf69c);
                    save_blob("arena-bss-member-frontier.bin", member_bss, 0xf69c);
                    free(member_bss);
                }
                if (address == base + 0x38df1 || address == base + 0x38e33) {
                    BYTE *arena_bss = (BYTE *)malloc(0xf69c);
                    if (!arena_bss) fail("allocate arena BSS observation");
                    readmem(base + 0xb9000, arena_bss, 0xf69c);
                    save_blob(address == base + 0x38df1 ? "arena-bss-entry.bin" : "arena-bss-frontier.bin", arena_bss, 0xf69c);
                    free(arena_bss);
                }
                if (address == base + 0x38d07) {
                    BYTE allocation_bss[0xf69c];
                    readmem(base + 0xb9000, allocation_bss, sizeof(allocation_bss));
                    save_blob("allocation-final-bss.bin", allocation_bss, sizeof(allocation_bss));
                }
                if (address == base + 0x38d56) {
                    BYTE options_bss[0xf69c];
                    readmem(base + 0xb9000, options_bss, sizeof(options_bss));
                    save_blob("early-options-final-bss.bin", options_bss, sizeof(options_bss));
                }
                if (address == base + 0x38bfd || address == base + 0x38c86) {
                    BYTE main_bss[0xf69c];
                    readmem(base + 0xb9000, main_bss, sizeof(main_bss));
                    save_blob(address == base + 0x38bfd ? "main-bss-entry.bin" : "main-bss-frontier.bin", main_bss, sizeof(main_bss));
                }
                if (address == base + 0x38cc9 && !allocation_helper_count) {
                    BYTE work_bss[0xf69c];
                    readmem(base + 0xb9000, work_bss, sizeof(work_bss));
                    save_blob("work-bss-first-reserve.bin", work_bss, sizeof(work_bss));
                }
                if (address == base + 0x38ae2 || address == base + 0x38aef) {
                    BYTE group_bss[0xf69c];
                    if (address == base + 0x38af9 || address == base + 0x386ae) {
                    DWORD caller_words[2];
                    BYTE comment_bss[0xf69c];
                    readmem(context.Esp, caller_words, sizeof(caller_words));
                    if (address == base + 0x38af9 || caller_words[1] == base + 0x346d4) {
                        if (address == base + 0x38af9 && getenv("C1XX_TEST_COMMENT")) {
                            DWORD value; BYTE input[74];
                            value = 37; write_mem(base + 0xc6b64, &value, 4);
                            memset(input, 165, sizeof(input)); write_mem(base + 0xc6ba8, input, sizeof(input));
                            value = 23; write_mem(base + 0xc6cb8, &value, 4);
                            value = 29; write_mem(base + 0xc6c44, &value, 4);
                            fprintf(events, "comment-seed actual-entry=00038af9 bytes=74 value=165 com=37 offset=23 length=29 code-mock=0\n");
                        }
                        readmem(base + 0xb9000, comment_bss, sizeof(comment_bss));
                        save_blob(address == base + 0x38af9 ? "comment-bss-entry.bin" : "comment-bss-result.bin", comment_bss, sizeof(comment_bss));
                    }
                }
                if (address == base + 0x38bea || address == base + 0x38bfc) {
                    BYTE action_bss[0xf69c];
                    if (address == base + 0x38bea && getenv("C1XX_TEST_ACTION")) {
                        DWORD owned, values[3];
                        readmem(base + 0xc4d50, &owned, 4);
                        if (!owned) fail("action seed needs snapshot identity");
                        values[0] = owned + 4; values[1] = owned + 8; values[2] = 17;
                        write_mem(base + 0xc6d28, values, sizeof(values));
                        fprintf(events, "action-seed actual-entry=00038bea head=snapshot+4 tail=snapshot+8 depth=17 code-mock=0\n");
                    }
                    readmem(base + 0xb9000, action_bss, sizeof(action_bss));
                    save_blob(address == base + 0x38bea ? "action-bss-entry.bin" : "action-bss-result.bin", action_bss, sizeof(action_bss));
                }
                if (address == base + 0x38e70) {
                    BYTE *member_bss = (BYTE *)malloc(0xf69c);
                    if (!member_bss) fail("allocate arena member BSS observation");
                    readmem(base + 0xb9000, member_bss, 0xf69c);
                    save_blob("arena-bss-member-frontier.bin", member_bss, 0xf69c);
                    free(member_bss);
                }
                if (address == base + 0x38df1 || address == base + 0x38e33) {
                    BYTE *arena_bss = (BYTE *)malloc(0xf69c);
                    if (!arena_bss) fail("allocate arena BSS observation");
                    readmem(base + 0xb9000, arena_bss, 0xf69c);
                    save_blob(address == base + 0x38df1 ? "arena-bss-entry.bin" : "arena-bss-frontier.bin", arena_bss, 0xf69c);
                    free(arena_bss);
                }
                if (address == base + 0x38d07) {
                    BYTE allocation_bss[0xf69c];
                    readmem(base + 0xb9000, allocation_bss, sizeof(allocation_bss));
                    save_blob("allocation-final-bss.bin", allocation_bss, sizeof(allocation_bss));
                }
                if (address == base + 0x38bfd || address == base + 0x38c86) {
                    BYTE main_bss[0xf69c];
                    readmem(base + 0xb9000, main_bss, sizeof(main_bss));
                    save_blob(address == base + 0x38bfd ? "main-bss-entry.bin" : "main-bss-frontier.bin", main_bss, sizeof(main_bss));
                }
                if (address == base + 0x38cc9 && !allocation_helper_count) {
                    BYTE work_bss[0xf69c];
                    readmem(base + 0xb9000, work_bss, sizeof(work_bss));
                    save_blob("work-bss-first-reserve.bin", work_bss, sizeof(work_bss));
                }
                if (address == base + 0x38ae2 && getenv("C1XX_TEST_PENDING_GROUP")) {
                        DWORD owned, value;
                        readmem(base + 0xc4d50, &owned, 4);
                        if (!owned) fail("group seed needs snapshot identity");
                        value = owned + 4; write_mem(base + 0xc6b40, &value, 4);
                        value = owned + 8; write_mem(base + 0xc6b48, &value, 4);
                        fprintf(events, "group-seed actual-entry=00038ae2 head=snapshot+4 active=snapshot+8 tail-untouched=1 code-mock=0\n");
                    }
                    readmem(base + 0xb9000, group_bss, sizeof(group_bss));
                    save_blob(address == base + 0x38ae2 ? "group-bss-entry.bin" : "group-bss-result.bin", group_bss, sizeof(group_bss));
                }
                if (address == base + 0x3bd41) {
                    BYTE pch_bss[0xf69c];
                    if (getenv("C1XX_TEST_PCH")) {
                        DWORD owned, values[17];
                        readmem(base + 0xc4d50, &owned, 4);
                        if (!owned || context.Ecx != base + 0xc64b8) fail("PCH seed receiver/allocation");
                        values[0] = 17;
                        values[1] = owned + 4;
                        values[2] = owned + 8;
                        values[3] = 23;
                        values[4] = owned + 12;
                        values[5] = -11;
                        values[6] = owned + 16;
                        values[7] = owned + 20;
                        values[8] = owned + 24;
                        values[9] = owned + 28;
                        values[10] = owned + 32;
                        values[11] = owned + 36;
                        values[12] = owned + 40;
                        values[13] = 29;
                        values[14] = owned + 44;
                        values[15] = owned + 48;
                        values[16] = owned + 52;
                        write_mem(base + 0xc64b8, values, sizeof(values));
                        fprintf(events, "pch-seed actual-entry=0003bd41 bytes=68 receiver-unchanged=1 code-mock=0\n");
                    }
                    if (getenv("C1XX_OBSERVE_PCH_RETURN")) {
                        if (pch_return_active) fail("duplicate PCH return observer");
                        readmem(context.Esp, &pch_return_address, 4);
                        if (pch_return_address != base + 0x385f3) fail("PCH inherited return PC");
                        readmem(pch_return_address, &pch_return_saved, 1);
                        if (pch_return_saved != 0xeb || snapshot[0x385f3-code_rva] != 0xeb) fail("PCH return instruction");
                        patch(pch_return_address, 0xcc); pch_return_active = 1;
                        fprintf(events, "pch-return-observer actual-caller=%08lx byte=eb armed-at-member-entry=1\n", pch_return_address);
                    }
                    readmem(base + 0xb9000, pch_bss, sizeof(pch_bss));
                    save_blob("pch-bss-entry.bin", pch_bss, sizeof(pch_bss));
                }
                if (address == base + 0x38ab7 || address == base + 0x38ad8) {
                    BYTE bss[0xf69c]; unsigned k;
                    if (address == base + 0x38ab7 && getenv("C1XX_TEST_PROTOTYPE")) {
                        for (k = 0; k < 20; ++k) bss[k] = (BYTE)(k + 1);
                        write_mem(base + 0xc64a0, bss, 20);
                        fprintf(events, "prototype-seed actual-entry=00038ab7 bytes=20 values=1..20 code-mock=0\n");
                    }
                    readmem(base + 0xb9000, bss, sizeof(bss));
                    save_blob(address == base + 0x38ab7 ? "prototype-bss-entry.bin" : "prototype-bss-result.bin", bss, sizeof(bss));
                }
                if (address == base + 0x38550 && getenv("C1XX_TEST_PDB_CONTEXT")) {
                    DWORD owned, fields[6], k, object = atoi(getenv("C1XX_TEST_PDB_OBJECT"));
                    DWORD object_base;
                    if (object < 1 || object > 2) fail("unknown PDB object seed");
                    object_base = base + 0xcb030 + 32*(object-1);
                    readmem(base + 0xc4d50, &owned, 4);
                    if (!owned) fail("PDB seed needs owned snapshot");
                    for (k = 0; k < 5; ++k) fields[k] = owned + 4*k;
                    fields[5] = 37;
                    write_mem(object_base, fields, 16);
                    write_mem(object_base + 20, fields + 4, 8);
                    fprintf(events, "pdb-context-seed allocation=%08lx read-mode=37 never-dereferenced=1\n", owned);
                }
                if (getenv("C1XX_TEST_PDB_CONTEXT") &&
                    address == base + (atoi(getenv("C1XX_TEST_PDB_OBJECT")) == 1 ? 0x3894f : 0x3898f)) {
                    DWORD reuse = atoi(getenv("C1XX_TEST_PDB_CONTEXT"));
                    if (reuse > 1) fail("unknown PDB reuse mode");
                    write_mem(base + 0xcb06c, &reuse, 4);
                    fprintf(events, "pdb-reuse-seed value=%lu\n", reuse);
                }
                if (getenv("C1XX_TEST_OUTPUT_CONSTRUCTOR")) {
                    static const DWORD constructors[] = { 0x38666, 0x386db, 0x3872a, 0x38779, 0x387c8, 0x38817, 0x38866, 0x388b5 };
                    unsigned object = atoi(getenv("C1XX_TEST_OUTPUT_CONSTRUCTOR"));
                    if (!object || object > 8) fail("unknown constructor data seed");
                    if (address == base + constructors[object - 1])
                        seed_output_buffer(object - 1, "constructor-entry");
                }
                if (address == base + 0x389d5 && getenv("C1XX_TEST_POOL_CONSTRUCTOR")) {
                    DWORD owned, values[4];
                    readmem(base + 0xc4d50, &owned, 4);
                    if (!owned) fail("pool constructor seed needs snapshot");
                    values[0] = owned + 4; values[1] = 23; values[2] = 37; values[3] = owned + 8;
                    write_mem(base + 0xc0ac8, values, 16);
                    fprintf(events, "pool-constructor-seed allocation=%08lx argument=1 receiver-unchanged=1\n", owned);
                }
                if (address == base + 0x38904 && getenv("C1XX_TEST_DEFERRED_CONSTRUCTOR")) {
                    DWORD owned;
                    readmem(base + 0xc4d50, &owned, 4);
                    if (!owned) fail("deferred seed needs owned snapshot");
                    write_mem(base + 0xc0578, &owned, 4);
                    fprintf(events, "deferred-seed role=constructor-entry value=owned-snapshot never-dereferenced=1\n");
                }
                if (address == base + 0x3891e && getenv("C1XX_TEST_TOKEN_COPY")) {
                    const char *variant = getenv("C1XX_TEST_TOKEN_COPY");
                    __int64 source, cache = -7;
                    BYTE kind = 165;
                    if (!strcmp(variant, "positive")) source = 0x1122334455667788i64;
                    else if (!strcmp(variant, "negative")) source = -0x112233445566778i64;
                    else fail("unknown token numeric seed");
                    write_mem(base + 0xba8c8, &source, 8);
                    write_mem(base + 0xc0b88, &cache, 8);
                    write_mem(base + 0xc0b80, &kind, 1);
                    fprintf(events, "token-seed role=callback-entry arm=integer sign=%s unknown-tails-seeded=0 pointer-arm-tested=0\n", variant);
                }
                if (allocation_observers && address == base + 0x38cc9) {
                    DWORD call[3];
                    if (allocation_helper_count >= 2 || allocation_return_active) fail("allocation helper order");
                    readmem(context.Esp, call, sizeof(call));
                    if (call[0] != base + (allocation_helper_count ? 0x38cb6 : 0x38c95) || call[1] != 0x500000 || call[2] != 0x4000) fail("allocation helper entry ABI");
                    allocation_return_pc = call[0];
                    readmem(allocation_return_pc, &allocation_return_byte, 1);
                    if (allocation_return_byte != snapshot[allocation_return_pc - base - code_rva]) fail("allocation helper caller bytes");
                    fprintf(events, "allocation-helper-entry index=%u seq=%u pc=%08lx esp=%08lx edi=%08lx esi=%08lx return=%08lx reserve=%08lx commit=%08lx\n",
                            allocation_helper_count, sequence, address, context.Esp, context.Edi, context.Esi, call[0], call[1], call[2]);
                    ++allocation_helper_count; patch(allocation_return_pc, 0xcc); allocation_return_active = 1;
                }
                if (address == base + 0x38d56 && getenv("C1XX_OBSERVE_EARLY_OPTIONS")) {
                    DWORD call[3];
                    readmem(context.Esp, call, sizeof(call));
                    if (option_return_active || option_calls >= 25 || call[0] != base + 0x38d28) fail("unexpected early option caller/count");
                    option_return_pc = call[0];
                    readmem(option_return_pc, &option_return_byte, 1);
                    if (option_return_byte != snapshot[option_return_pc - base - code_rva]) fail("option caller bytes");
                    fprintf(events, "early-option-entry index=%u seq=%u pc=%08lx esp=%08lx edi=%08lx esi=%08lx ebx=%08lx return=%08lx argument=%08lx state=%08lx\n", option_calls,sequence,address,context.Esp,context.Edi,context.Esi,context.Ebx,call[0],call[1],call[2]);
                    ++option_calls;patch(option_return_pc,0xcc);option_return_active=1;
                }
                boundary(&context, address, thread);
                for (i = 0; i < count; ++i)
                    if (ranges[i].entry && address == base + ranges[i].first) break;
                if (i < count) {
                    patch(address, ranges[i].saved); pending = i;
                    context.EFlags |= 0x100;
                } else if (baseline && address == base + entry_rva) {
                    patch(address, 0x53);
                } else if (aux_new_base && !aux_returned && address == base + 0x233ba) {
                    DWORD call[2]; BYTE byte;
                    readmem(context.Esp, call, sizeof(call)); readmem(address, &byte, 1);
                    if (call[1] != 8520 || byte != 0xcc || call[0] != base + 0x38595) fail("aux allocator ABI/trap");
                    fprintf(events, "aux-stub-return role=mock-return rva=000233ba target-byte=cc request=8520 result=%08lx caller=%08lx cdecl-pop=4\n", aux_new_base + 32, call[0]);
                    save_aux_state("call");
                    context.Eax = aux_new_base + 32; context.Eip = call[0]; context.Esp += 4;
                    aux_returned = 1;
                    if (!SetThreadContext(thread, &context)) fail("aux mock return context");
                    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status); continue;
                } else {
                    fprintf(events, "frontier rva=%08lx\n", address - base); fflush(events);
                    TerminateProcess(child, 125); return 125;
                }
                context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("reset instruction pointer");
            } else if (code != EXCEPTION_BREAKPOINT) {
                fprintf(events, "exception code=%08lx pc=%08lx\n", code, address);
                status = DBG_EXCEPTION_NOT_HANDLED;
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            result = event.u.ExitProcess.dwExitCode;
            fprintf(events, "exit=%lu target=%d crt=%d initialized=%d\n",
                    result, target_seen, crt_seen, initialized);
            fclose(events);
            if (!target_seen || !crt_seen || !initialized) return 126;
            return result;
        }
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status)) fail("ContinueDebugEvent");
    }
}
