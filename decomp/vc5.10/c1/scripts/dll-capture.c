/* Explicit original-only debugger transport. Never linked to a fixture. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

typedef struct Range { DWORD first, last; int entry; BYTE saved; } Range;
static HANDLE child;
static FILE *events;
static DWORD base, entry_rva, code_rva, code_size, image_size;
static DWORD crt_base, return_address, return_reason;
static DWORD mspdb_base;
static int vector_probe,vector_started;
static void prepare_vector_observer(void);
static BYTE return_saved;
static int return_active;
static unsigned sequence;
static BYTE *snapshot;
static Range ranges[512];
static unsigned count;
static HANDLE threads[64];
static DWORD thread_ids[64], thread_count;
static int baseline, initialized, pending = -1, target_seen, crt_seen;
static int restore_probe, restore_seeded, context_seeded;
static DWORD restore_observer;
static BYTE restore_observer_saved;
static char target_path[MAX_PATH], crt_path[MAX_PATH];
typedef struct BufferProbe { DWORD object, constructor, destructor; int seeded; } BufferProbe;
static BufferProbe buffer_probes[16];
static unsigned buffer_probe_count;
static DWORD value_words[9];
static int value_probe, value_seeded;
static DWORD signature_words[2];
static int signature_probe, signature_seeded;
static DWORD pdb_reuse, pdb_context_count;
static int pdb_probe, pdb_seeded;
static DWORD pool_objects[16];
static unsigned pool_count;
static int prototype_probe, prototype_seeded;
static int main_fields_probe;
typedef struct CommandLineSeed {
    char region;
    DWORD rva, bytes;
    unsigned kind, index;
} CommandLineSeed;
static CommandLineSeed command_line_seeds[96];
static unsigned command_line_seed_count;
static int pch_state_probe, pch_member_probe, pch_member_seeded, pch_return_active;
static DWORD pch_return_address, pch_entry_esp, pch_entry_edi;
static BYTE pch_return_saved;
static int pool_member_probe, pool_member_seeded, pool_return_active;
static DWORD pool_return_address, pool_entry_esp;
static BYTE pool_return_saved;
/* Source-bound observation points remain inside the unchanged candidate body. */
static Range allocation_observers[6];
static DWORD allocation_provider;
static unsigned allocation_observer_count, allocation_call_count;
static DWORD allocation_before_esp, allocation_before_edi;
static DWORD allocation_regions[2];
static unsigned allocation_region_count;
static DWORD parser_return_address, parser_entry_esp, parser_calls;
static BYTE parser_return_saved;
static int parser_return_active, parser_probe;
static Range getenv_observer;
static Range system_observers[2];
static int system_probe;
static Range scaler_observers[10];
static unsigned scaler_observer_count;
static int arena_reset_probe;
static unsigned arena_reset_count;
static int scaler_probe;
static DWORD arena_allocation_esp, arena_allocation_error;
static Range pool_observers[14];
static int pool_cycle_probe;
static int include_alias_probe;
static int hfile_offset_probe;
static int checksum_tables_probe, locale_entry_active, locale_return_active;
static DWORD locale_entry_address, locale_return_address, locale_entry_esp;
static BYTE locale_entry_saved, locale_return_saved;
static int scanner_tables_probe;
static int isolated_locale_probe,locale_complete;
static DWORD locale_old_pctype,locale_result;
static int mb_entry_active,mb_return_active,mb_complete;
static DWORD mb_entry_address,mb_return_address,mb_entry_esp;
static BYTE mb_entry_saved,mb_return_saved;
static void prepare_isolated_locale_observer(void);
static void path_fold_prepare(void);
static void path_fold_seed(const char *,WORD);
static int checksum_return_active;
static BYTE checksum_return_saved;
static DWORD pool_commit_address, pool_commit_bytes, pool_commit_esp, pool_commit_error;
static DWORD system_before_esp, system_before_error;




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

/* Keep the actual debug-event HANDLE. Duplicating it would retain its shared
 * file position; reopening a pathname would lose loaded-file identity. */
static BOOL read_file_at(HANDLE file, DWORD offset, void *buffer, DWORD bytes,
                         DWORD *received, DWORD *error)
{
    OVERLAPPED position;
    BOOL status;
    memset(&position, 0, sizeof(position));
    position.Offset = offset;
    *received = 0;
    status = ReadFile(file, buffer, bytes, received, &position);
    *error = GetLastError();
    return status;
}

/* Owned original-only transport probe, before target code can run. It forces
 * the SAME actual debug HANDLE's current position to disagree with each
 * request and records real completion, byte counts, EOF and pointer effects. */
static void probe_hfile_offsets(HANDLE loaded, const char *reference)
{
    HANDLE expected;
    BYTE actual[32], wanted[32];
    DWORD size, offsets[5], i, wrong, before, after, a, b, error;
    BOOL status;
    expected = CreateFileA(reference, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (expected == INVALID_HANDLE_VALUE) fail("probe expected handle");
    size = GetFileSize(loaded, NULL);
    if (size == 0xffffffffUL || size < 65568) fail("probe actual file size");
    SetFilePointer(loaded, 8192, NULL, FILE_BEGIN);
    if (!ReadFile(loaded, actual, 32, &a, NULL) || a != 32 ||
        !ReadFile(expected, wanted, 32, &b, NULL) || b != 32) fail("probe implicit read");
    fprintf(events, "hfile-implicit actual-offset=00002000 requested-reference=00000000 bytes=%lu matches=%d\n",
            a, !memcmp(actual, wanted, 32));
    if (!memcmp(actual, wanted, 32)) fail("probe implicit control was not discriminating");
    offsets[0] = 0; offsets[1] = 8192; offsets[2] = 65536;
    offsets[3] = size - 13; offsets[4] = size;
    for (i = 0; i < 5; ++i) {
        wrong = offsets[i] ? 0 : 8192;
        if (SetFilePointer(loaded, wrong, NULL, FILE_BEGIN) != wrong ||
            SetFilePointer(expected, offsets[i], NULL, FILE_BEGIN) != offsets[i]) fail("probe seek");
        before = SetFilePointer(loaded, 0, NULL, FILE_CURRENT);
        SetLastError(0x6abc);
        status = read_file_at(loaded, offsets[i], actual, 32, &a, &error);
        after = SetFilePointer(loaded, 0, NULL, FILE_CURRENT);
        if (!ReadFile(expected, wanted, 32, &b, NULL) || a > 32) fail("probe expected bytes");
        fprintf(events, "hfile-explicit index=%lu offset=%08lx before=%08lx after=%08lx requested=32 status=%d error=%lu bytes=%lu matches=%d\n",
                i, offsets[i], before, after, status, error, a,
                a == b && (!a || !memcmp(actual, wanted, a)));
        if ((!status && (error != ERROR_HANDLE_EOF || a)) || a != b ||
            (a && memcmp(actual, wanted, a))) fail("probe explicit actual bytes");
        {
            FILE *raw;
            char name[64];
            sprintf(name, "hfile-sample-%lu.bin", i);
            raw = fopen(name, "wb");
            if (!raw || fwrite(actual, 1, a, raw) != a) fail("probe record bytes");
            fclose(raw);
        }
    }
    CloseHandle(expected);
}

static int same_file(HANDLE loaded, const char *reference, const char *output)
{
    HANDLE expected, saved;
    BYTE actual[8192], wanted[8192];
    DWORD a, b, wrote, offset = 0, error, bytes, size, expected_size, high;
    int same = 1;
    expected = CreateFileA(reference, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (expected == INVALID_HANDLE_VALUE) fail("open expected file");
    size = GetFileSize(loaded, &high);
    if (high || !size || size == 0xffffffffUL) fail("actual identity size");
    expected_size = GetFileSize(expected, &high);
    if (high || expected_size == 0xffffffffUL) fail("expected identity size");
    if (size != expected_size) same = 0;
    saved = CreateFileA(output, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    if (saved == INVALID_HANDLE_VALUE) fail("save loaded file");
    while (offset < size) {
        bytes = size - offset;
        if (bytes > sizeof(actual)) bytes = sizeof(actual);
        if (!read_file_at(loaded, offset, actual, bytes, &a, &error) || a != bytes)
            fail("read actual file identity");
        if (!ReadFile(expected, wanted, bytes, &b, NULL)) fail("read expected identity");
        if (a != b) same = 0;
        else if (a && memcmp(actual, wanted, a)) same = 0;
        if (!WriteFile(saved, actual, a, &wrote, NULL) || wrote != a) fail("save identity");
        if (offset > 0xffffffffUL - a) fail("file identity offset overflow");
        offset += a;
    }
    if (offset != size || GetFileSize(loaded, &high) != size || high)
        fail("actual identity length changed");
    if (GetFileSize(expected, &high) != expected_size || high)
        fail("expected identity length changed");
    CloseHandle(saved); CloseHandle(expected);
    fprintf(events, "file-identity output=%s actual-bytes=%lu explicit-offset=1 equal=%d\n", output, offset, same);
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
    if (!_stricmp(name, "msvcrt.dll")) {
        if (!file || crt_seen || !same_file(file, crt_path, "loaded-msvcrt.bin")) fail("canonical MSVCRT identity");
        crt_seen = 1;
        crt_base = address;
    }
    if (!_stricmp(name, "kernel32.dll")) {
        if (hfile_offset_probe) probe_hfile_offsets(file, "C:\\windows\\system32\\kernel32.dll");
        if (!file || !same_file(file, "C:\\windows\\system32\\kernel32.dll", "loaded-kernel32.bin")) fail("KERNEL32 loaded file identity");
        if (hfile_offset_probe) {
            fprintf(events, "hfile-probe-complete target-loaded=%d explicit-offset-identity=1\n", target_seen);
            fflush(events); TerminateProcess(child, 125); ExitProcess(125);
        }
    }
    if (!_stricmp(name, "kernelbase.dll")) {
        if (!file || !same_file(file, "C:\\windows\\system32\\kernelbase.dll", "loaded-kernelbase.bin")) fail("KernelBase loaded file identity");
    }
    if (!_stricmp(name,"mspdb50.dll")) {
        if(!file || mspdb_base || !same_file(file,"R:\\toolchains\\5.10\\bin\\MSPDB50.DLL","loaded-mspdb50.bin"))fail("MSPDB loaded identity");
        mspdb_base=address;
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
    if (entry_rva != 0x1202a || code_rva != 0x1000 || code_size != 0x4eaf0)
        fail("unexpected C1 layout");
    /* The Python preflight proves this byte is outside every relocation site. */
    readmem(base + entry_rva, &first, 1);
    if (first != 0x53) fail("entry guard byte");
    patch(base + entry_rva, 0xcc);
    fprintf(events, "guard rva=%08lx original=%02x\n", entry_rva, first);
    fflush(events);
}

static void save_remote_state(const char *, unsigned, DWORD, DWORD);
static DWORD exported(DWORD, const char *);

static void prepare_mask(void)
{
    BYTE *masked;
    HANDLE saved;
    DWORD wrote;
    unsigned i;
    DWORD resolved;
    BYTE first;
    static DWORD import_slots[4] = { 0x5d0e8, 0x5d0a8, 0x5d114, 0x5d0ec };
    static DWORD provider_rvas[4] = { 0x1715, 0x1498, 0x138a, 0x3f928 };
    for (i = 0; i < 4; ++i) {
        readmem(base + import_slots[i], &resolved, sizeof(resolved));
        if (resolved != crt_base + provider_rvas[i]) fail("canonical CRT import resolution");
        fprintf(events, "provider iat=%08lx resolved=%08lx\n", import_slots[i], resolved);
    }
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
    free(masked); initialized = 1;
    if (restore_probe) {
        BYTE current;
        restore_observer = base + 0x303d5;
        restore_observer_saved = snapshot[0x303d5 - code_rva];
        readmem(restore_observer, &current, 1);
        if (current != restore_observer_saved) fail("restore observer outside candidate ownership");
        patch(restore_observer, 0xcc);
    }
    {
        FILE *probe = fopen("allocation-observers.txt", "r");
        if (probe) {
            DWORD rva, index;
            for (index = 0; index < 6; ++index) {
                if (fscanf(probe, "%lx", &rva) != 1 || rva < code_rva || rva >= code_rva + code_size) fail("allocation observer metadata");
                readmem(base + rva, &first, 1);
                if (first != snapshot[rva - code_rva] || first == 0xcc) fail("allocation observer outside candidate");
                allocation_observers[index].first = rva;
                allocation_observers[index].saved = first;
                patch(base + rva, 0xcc);
            }
            if (fscanf(probe, "%lx", &rva) != EOF) fail("extra allocation observer metadata");
            fclose(probe); allocation_observer_count = 6;
            readmem(base + 0x5d1c4, &allocation_provider, 4);
            fprintf(events, "allocation-provider iat=0005d1c4 resolved=%08lx\n", allocation_provider);
        }
    }
    {
        FILE *probe = fopen("internal-switch-observers.txt", "r");
        DWORD rva;
        if (probe) {
            if (fscanf(probe, "%lx", &rva) != 1 || rva != 0x128ec) fail("internal getenv observer metadata");
            getenv_observer.first = rva;
            readmem(base + rva, &getenv_observer.saved, 1);
            if (getenv_observer.saved != snapshot[rva - code_rva] || getenv_observer.saved != 0x83) fail("internal getenv observer outside owned source");
            if (fscanf(probe, "%lx", &rva) != EOF) fail("extra getenv observer metadata");
            patch(base + getenv_observer.first, 0xcc); parser_probe = 1; fclose(probe);
        }
    }
    {
        FILE *probe = fopen("system-info-observers.txt", "r");
        DWORD rva;
        unsigned index;
        if (probe) {
            for (index = 0; index < 2; ++index) {
                if (fscanf(probe, "%lx", &rva) != 1 || rva != (index ? 0x127f2 : 0x127ec)) fail("systeminfo observer metadata");
                system_observers[index].first = rva;
                readmem(base + rva, &system_observers[index].saved, 1);
                if (system_observers[index].saved != snapshot[rva - code_rva] || system_observers[index].saved != (index ? 0xe8 : 0xff)) fail("systeminfo observer outside owned source");
                patch(base + rva, 0xcc);
            }
            if (fscanf(probe, "%lx", &rva) != EOF) fail("extra systeminfo observer metadata");
            fclose(probe); system_probe = 1;
        }
    }
    {
        FILE *probe = fopen("scaler-observers.txt", "r");
        static DWORD const expected[10] = {0x12a7f,0x12a85,0x12a8b,0x12a96,0x12a29,0x12a51,0x12a57,0x12a70,0x12a0f,0x129bc};
        DWORD rva, provider;
        unsigned index;
        if (probe) {
            for (index = 0; index < 10; ++index) {
                if (fscanf(probe, "%lx", &rva) != 1) {
                    if (index == 7 && feof(probe)) break;
                    fail("scaler observer metadata");
                }
                if (rva != expected[index]) fail("scaler observer metadata");
                readmem(base + rva, &first, 1);
                if (first != snapshot[rva - code_rva] || first == 0xcc) fail("scaler observer outside owned source");
                scaler_observers[index].first = rva; scaler_observers[index].saved = first; patch(base + rva, 0xcc);
            }
            if (fscanf(probe, "%lx", &rva) != EOF) fail("extra scaler observer");
            scaler_observer_count = index;
            fclose(probe); scaler_probe = 1;
            probe = fopen("arena-reset-probe.txt", "r");
            if (probe) {
                DWORD entry, returned, objects;
                if (fscanf(probe, "%lx %lx %lu", &entry, &returned, &objects) != 3 || entry != 0x12ab2 || returned != 0x12a70 || objects != 6 || scaler_observer_count != 10) fail("arena Reset observer metadata");
                arena_reset_probe = 1; fclose(probe);
            }
            readmem(base + 0x5d0cc, &provider, 4);
            if (provider != exported(crt_base, "_ftol")) fail("canonical ftol provider");
            fprintf(events, "ftol-provider iat=0005d0cc resolved=%08lx\n", provider);
            save_remote_state("ftol-provider-code", 0, provider, 39);
        }
    }
    {
        FILE *probe = fopen("pool-cycle-observers.txt", "r");
        static DWORD const expected[14] = {0x98b9,0x98bf,0xab12,0xaac5,0x12b46,0x12acf,0x12adb,0x12ae7,0x12af3,0x12aff,0x12b0b,0x12b17,0x12b23,0x12b2f};
        DWORD rva;
        unsigned index, owner;
        if (probe) {
            for (index = 0; index < 14; ++index) {
                if (fscanf(probe, "%lx", &rva) != 1 || rva != expected[index]) fail("pool cycle observer metadata");
                for (owner = 0; owner < count; ++owner)
                    if (ranges[owner].first <= rva && rva < ranges[owner].last) break;
                if (owner == count) fail("pool observer outside accepted source");
                readmem(base + rva, &first, 1);
                if (first != snapshot[rva-code_rva] || first == 0xcc) fail("pool observer relocated byte");
                pool_observers[index].first = rva; pool_observers[index].saved = first;
                patch(base + rva, 0xcc);
            }
            if (fscanf(probe, "%lx", &rva) != EOF) fail("extra pool observer metadata");
            fclose(probe); pool_cycle_probe = 1;
        }
    }
    {
        FILE *probe = fopen("include-alias-observers.txt", "r");
        DWORD entry, frontier, pointer, bytes;
        if (probe) {
            if (fscanf(probe, "%lx %lx %lx %lx", &entry, &frontier, &pointer, &bytes) != 4 ||
                entry != 0x12df6 || frontier != 0x12e1c || pointer != 0x5e414 || bytes != 4)
                fail("include alias observer metadata");
            fclose(probe); include_alias_probe = 1;
        }
    }
    {
        FILE *probe = fopen("checksum-locale-observers.txt", "r");
        DWORD entry, iat, provider;
        if (probe) {
            if (fscanf(probe, "%lx %lx %lx", &entry, &iat, &provider) != 3 ||
                    entry != 0x12e1c || iat != 0x5d120 || provider != 0x5729)
                fail("checksum locale observer metadata");
            fclose(probe); checksum_tables_probe = 1;
            readmem(base + iat, &locale_entry_address, 4);
            if (locale_entry_address != crt_base + provider ||
                    locale_entry_address != exported(crt_base, "setlocale"))
                fail("canonical setlocale provider");
            readmem(locale_entry_address, &locale_entry_saved, 1);
            if (locale_entry_saved != 0x81) fail("canonical setlocale first byte");
            save_remote_state("setlocale-provider-entry", 0, locale_entry_address, 6);
            patch(locale_entry_address, 0xcc); locale_entry_active = 1;
            readmem(base + 0x12b66, &checksum_return_saved, 1);
            if (checksum_return_saved != 0x68 || checksum_return_saved != snapshot[0x12b66-code_rva])
                fail("checksum return ownership");
            patch(base + 0x12b66, 0xcc); checksum_return_active = 1;
            prepare_isolated_locale_observer();
            path_fold_prepare();
            prepare_vector_observer();
        }
    }
    save_remote_state("compiler-callbacks", 0, base + 0x69000, 34 * 4);
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

static void save_remote_state(const char *prefix, unsigned index, DWORD address, DWORD size)
{
    char filename[100];
    BYTE *bytes;
    FILE *file;
    bytes = (BYTE *)malloc(size);
    if (!bytes) fail("capture compiler state allocation");
    readmem(address, bytes, size);
    sprintf(filename, "%s-%03u.bin", prefix, index);
    file = fopen(filename, "wb");
    if (!file || fwrite(bytes, 1, size, file) != size) fail("capture compiler state write");
    fclose(file);
    free(bytes);
}

static void save_state(void)
{
    BYTE globals[0xa4];
    DWORD begin, first = 0, saved_data;
    unsigned index = sequence;
    char filename[80];
    FILE *state;
    readmem(base + 0x5c000, globals, sizeof(globals));
    memcpy(&begin, globals + 0x98, 4);
    if (begin) readmem(begin, &first, 4);
    fprintf(events, "state seq=%u heap-begin=%08lx heap-first=%08lx\n", sequence, begin, first);
    sprintf(filename, "state-%03u.bin", sequence++);
    state = fopen(filename, "wb");
    if (!state || fwrite(globals, 1, sizeof(globals), state) != sizeof(globals)) fail("save state");
    fclose(state);
    save_remote_state("compiler-data", index, base + 0x5e000, 6548);
    save_remote_state("compiler-bss", index, base + 0x50000, 0xa06c);
    readmem(base + 0x57818, &saved_data, 4);
    if (saved_data) save_remote_state("compiler-snapshot", index, saved_data, 6548);
    fflush(events);
}

static void save_vector(DWORD address, DWORD count, const char *name)
{
    FILE *file;
    DWORD i, pointer, length, actual=0;
    char text[32768];
    for (i=0;i<count;++i) { readmem(address+i*4,&pointer,4); if (!pointer) break; ++actual; }
    file=fopen(name,"wb"); if (!file) fail("capture environment vector");
    fwrite(&address,4,1,file); fwrite(&actual,4,1,file);
    for (i=0;i<actual;++i) {
        readmem(address+i*4,&pointer,4); remote_string(pointer,text,sizeof(text)); length=strlen(text)+1;
        fwrite(&pointer,4,1,file); fwrite(&length,4,1,file); fwrite(text,1,length,file);
    }
    fclose(file);
}
static DWORD exported(DWORD module, const char *wanted)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_EXPORT_DIRECTORY table;
    DWORD i, rva, address;
    WORD ordinal;
    char name[256];
    readmem(module,&dos,sizeof(dos)); readmem(module+dos.e_lfanew,&nt,sizeof(nt));
    readmem(module+nt.OptionalHeader.DataDirectory[0].VirtualAddress,&table,sizeof(table));
    for (i=0;i<table.NumberOfNames;++i) {
        readmem(module+(DWORD)table.AddressOfNames+i*4,&rva,4); remote_string(module+rva,name,sizeof(name));
        if (!strcmp(name,wanted)) {
            readmem(module+(DWORD)table.AddressOfNameOrdinals+i*2,&ordinal,2);
            readmem(module+(DWORD)table.AddressOfFunctions+ordinal*4,&address,4); return module+address;
        }
    }
    fail("missing CRT environment export"); return 0;
}

static void save_blob(const char *name,const void *bytes,unsigned length)
{
    FILE *file=fopen(name,"wb");
    if(!file || fwrite(bytes,1,length,file)!=length || fclose(file))fail("isolated observer write");
}
static void save_frontend_state(const char *phase)
{
    char name[100];BYTE *bytes;DWORD saved;
    bytes=(BYTE *)malloc(0xa06c);if(!bytes)fail("isolated observer target buffer");
    readmem(base+0x5e000,bytes,6548);sprintf(name,"%s-target-data.bin",phase);save_blob(name,bytes,6548);
    readmem(base+0x57818,&saved,4);if(!saved)fail("isolated observer saved state");
    readmem(saved,bytes,6548);sprintf(name,"%s-target-snapshot.bin",phase);save_blob(name,bytes,6548);
    readmem(base+0x50000,bytes,0xa06c);sprintf(name,"%s-target-BSS.bin",phase);save_blob(name,bytes,0xa06c);free(bytes);
}
#include "locale-observer.inc"
#include "path-fold-observer.inc"
#include "vector-observer.inc"

/* Labeled auxiliary values: the stream and relocation callback are never
 * dereferenced/invoked. Only their mapped pointer identities are exercised. */
static void read_buffer_probes(void)
{
    FILE *file;
    DWORD object, constructor, destructor;
    file = fopen("output-buffers.txt", "r");
    if (!file) fail("missing typed buffer-probe metadata");
    while (fscanf(file, "%lx %lx %lx", &object, &constructor, &destructor) == 3) {
        if (buffer_probe_count == 16 || object < 0x50000 || object + 0x124 > 0x5a06c ||
                constructor < 0x1000 || constructor >= 0x4faf0 ||
                destructor < 0x1000 || destructor >= 0x4faf0)
            fail("invalid typed buffer-probe metadata");
        buffer_probes[buffer_probe_count].object = object;
        buffer_probes[buffer_probe_count].constructor = constructor;
        buffer_probes[buffer_probe_count].destructor = destructor;
        ++buffer_probe_count;
    }
    if (!feof(file) || !buffer_probe_count) fail("incomplete typed buffer-probe metadata");
    fclose(file);
}

/* Explicit typed signed-64 auxiliary inputs. No original value tail is seeded. */
static void read_value_probe(void)
{
    FILE *file;
    unsigned index;
    file = fopen("value-probe.txt", "r");
    if (!file) fail("missing typed value-probe metadata");
    for (index = 0; index < 9; ++index)
        if (fscanf(file, "%lx", &value_words[index]) != 1) fail("incomplete typed value-probe metadata");
    if (value_words[8] > 255 || fscanf(file, "%lx", &index) != EOF) fail("invalid typed value-probe metadata");
    fclose(file);
    value_probe = 1;
}

static void seed_value(DWORD rva, unsigned start)
{
    BYTE flag = (BYTE)value_words[8];
    write_mem(base + 0x518a0, value_words + start, 8);
    write_mem(base + 0x55108, value_words + start + 2, 8);
    write_mem(base + 0x55100, &flag, 1);
    fprintf(events, "value-seed rva=%08lx source=%08lx%08lx cached=%08lx%08lx flag=%u\n",
            rva, value_words[start + 1], value_words[start], value_words[start + 3], value_words[start + 2], (unsigned)flag);
}

/* One unsigned rolling signature, not an opaque data block. */
static void read_signature_probe(void)
{
    FILE *file;
    DWORD extra;
    file = fopen("signature-probe.txt", "r");
    if (!file) return;
    if (fscanf(file, "%lx %lx", &signature_words[0], &signature_words[1]) != 2 ||
            fscanf(file, "%lx", &extra) != EOF) fail("invalid signature probe");
    fclose(file);
    signature_probe = 1;
}

static void seed_signature(DWORD rva, unsigned index)
{
    write_mem(base + 0x54de0, signature_words + index, 4);
    fprintf(events, "signature-seed rva=%08lx value=%08lx\n", rva, signature_words[index]);
}

static void read_pdb_probe(void)
{
    FILE *file;
    DWORD extra;
    file = fopen("pdb-context-probe.txt", "r");
    if (!file) return;
    if (fscanf(file, "%lx %lx", &pdb_reuse, &pdb_context_count) != 2 || pdb_reuse > 1 ||
            pdb_context_count != 2 ||
            fscanf(file, "%lx", &extra) != EOF) fail("invalid PDB helper probe");
    fclose(file); pdb_probe = 1;
}

static void seed_pdb_context(void)
{
    DWORD saved, value, object, offsets[5] = {0,4,8,12,20};
    unsigned index, instance;
    readmem(base + 0x57818, &saved, 4);
    if (!saved) fail("PDB probe missing owned snapshot");
    for (instance = 0; instance < pdb_context_count; ++instance) {
        object = instance ? 0x5c070 : 0x5c048;
        for (index = 0; index < 5; ++index) {
            value = saved + instance * 64 + index * 8;
            write_mem(base + object + offsets[index], &value, 4);
        }
        value = 37; write_mem(base + object + 24, &value, 4);
        fprintf(events, "%spdb-context-seed rva=000122a5 allocation=%08lx read_mode=37\n",
                instance ? "output-" : "", saved);
    }
}

static void read_pool_probes(void)
{
    FILE *file;
    DWORD object;
    file = fopen("pools-probe.txt", "r");
    if (!file) return;
    while (fscanf(file, "%lx", &object) == 1) {
        if (pool_count == 16 || object < 0x50000 || object + 16 > 0x5a06c)
            fail("invalid typed pool probe");
        pool_objects[pool_count++] = object;
    }
    if (!feof(file)) fail("incomplete pool probe");
    fclose(file);
    file = fopen("pool-member-probe.txt", "r");
    if (file) {
        DWORD member, index;
        if (fscanf(file, "%lx %lx", &member, &index) != 2 || member != 0x126fc || index != pool_count) fail("pool member probe");
        pool_member_probe = 1; fclose(file);
    }
}

static void seed_pool(unsigned index, DWORD saved, const char *label)
{
    DWORD values[4];
    values[0] = saved + 128 + index * 16;
    values[1] = 91 + index; values[2] = 7 + index;
    values[3] = saved + 512 + index * 16;
    write_mem(base + pool_objects[index], values, sizeof(values));
    fprintf(events, "%s index=%u object=%08lx allocation=%08lx\n", label, index, pool_objects[index], saved);
}

static void seed_pools(DWORD saved)
{
    unsigned index;
    for (index = 0; index < pool_count; ++index) seed_pool(index, saved, "pool-seed");
}

static void seed_pch_state(DWORD saved, DWORD rva, const char *tag)
{
    static DWORD pointer_offsets[13] = {4,8,16,24,28,32,36,40,44,48,56,60,64};
    DWORD value;
    unsigned i;
    value = 37; write_mem(base + 0x58f68, &value, 4);
    value = 19; write_mem(base + 0x58f68 + 52, &value, 4);
    value = 0x11223344; write_mem(base + 0x58f68 + 12, &value, 4);
    value = 0x55667788; write_mem(base + 0x58f68 + 20, &value, 4);
    for (i = 0; i < 13; ++i) {
        value = saved + 1024 + i * 8;
        write_mem(base + 0x58f68 + pointer_offsets[i], &value, 4);
    }
    fprintf(events, "%s rva=%08lx object=00058f68 allocation=%08lx bytes=68\n", tag, rva, saved);
}

static void seed_prototype(DWORD rva, unsigned first)
{
    BYTE value[20];
    unsigned i;
    for (i = 0; i < 20; ++i) value[i] = (BYTE)(first + i);
    write_mem(base + 0x58f50, value, sizeof(value));
    fprintf(events, "prototype-seed rva=%08lx bytes=20 first=%02x\n", rva, first);
}

static void seed_output_buffer(unsigned index, DWORD rva)
{
    DWORD saved, start, object, values[7], descriptor;
    BYTE mode;
    if (index >= buffer_probe_count) fail("unknown typed buffer probe");
    object = buffer_probes[index].object;
    readmem(base + 0x57818, &saved, 4);
    if (!saved) fail("buffer probe missing snapshot allocation");
    start = saved + index * 16;
    values[0] = start; values[1] = start + 4; values[2] = 47 + index;
    values[3] = base + 0x12365; values[4] = start;
    values[5] = start + 8; values[6] = 9;
    mode = (BYTE)(165 + index); descriptor = 57 + index;
    write_mem(base + object, values, sizeof(values));
    write_mem(base + object + 28, &mode, 1);
    write_mem(base + object + 0x120, &descriptor, 4);
    fprintf(events, "output-buffer-seed index=%u rva=%08lx allocation=%08lx callback=%08lx\n",
            index, rva, saved, values[3]);
}

/* Explicit semantic field inputs, not an initialized-section replacement.
 * Unknown original bytes remain in their original snapshot unchanged. */
static void read_command_line_seeds(void)
{
    FILE *file = fopen("command-line-seeds.txt", "r");
    CommandLineSeed row;
    int result;
    if (!file) return;
    while ((result = fscanf(file, " %c %lx %lx %u %u", &row.region,
                            &row.rva, &row.bytes, &row.kind, &row.index)) == 5) {
        DWORD first = row.region == 'D' ? 0x5e000 : 0x50000;
        DWORD last = row.region == 'D' ? 0x5f994 : 0x5a06c;
        if ((row.region != 'D' && row.region != 'B') || !row.bytes ||
            row.bytes > 1000 || row.kind > 2 || row.index > 80 ||
            row.rva < first || row.rva >= last || row.bytes > last - row.rva ||
            (row.kind != 2 && row.bytes != 4) || command_line_seed_count == 96)
            fail("command line seed metadata");
        command_line_seeds[command_line_seed_count++] = row;
    }
    if (result != EOF || !command_line_seed_count) fail("command line seed file");
    fclose(file);
}

static void seed_command_line_initialized(BYTE *data, DWORD saved, int snapshot_copy)
{
    unsigned i, j;
    DWORD value;
    CommandLineSeed *row;
    for (i = 0; i < command_line_seed_count; ++i) {
        row = &command_line_seeds[i];
        if (row->region != 'D') continue;
        if (row->kind == 2) {
            for (j = 0; j < row->bytes; ++j)
                data[row->rva - 0x5e000 + j] =
                    (BYTE)((row->index + 1) * 7 + j + (snapshot_copy ? 11 : 31));
        } else {
            if (row->kind == 1)
                value = snapshot_copy && row->rva == 0x5e414 ? 0 :
                    saved + 5200 + row->index * 4 + (snapshot_copy ? 0 : 400);
            else
                value = 101 + row->index * 3 + (snapshot_copy ? 0 : 1000);
            memcpy(data + row->rva - 0x5e000, &value, 4);
        }
    }
}

static void seed_command_line_live(BYTE *data, DWORD saved)
{
    unsigned i;
    DWORD value;
    CommandLineSeed *row;
    if (!command_line_seed_count) return;
    seed_command_line_initialized(data, saved, 0);
    if(scanner_tables_probe) {
        unsigned table,byte;for(table=0;table<2;++table)for(byte=0;byte<256;++byte)
            data[8+table*256+byte]=(BYTE)((69+table)*7+byte+31);
        write_mem(base+0x5e008,data+8,512);
        fprintf(events,"scanner-state-seed fields=2 bytes=512 snapshot=%08lx\n",saved);
    }
    for (i = 0; i < command_line_seed_count; ++i) {
        row = &command_line_seeds[i];
        if (row->region == 'D')
            write_mem(base + row->rva, data + row->rva - 0x5e000, row->bytes);
        else {
            value = row->kind == 1 ? saved + 5800 + row->index * 4 :
                301 + row->index * 3;
            write_mem(base + row->rva, &value, 4);
        }
    }
    value = base + 0x5c048;
    write_mem(base + 0x5c068, &value, 4);
    fprintf(events, "command-line-seed fields=%u snapshot=%08lx persistent=normal-pdb\n",
            command_line_seed_count, saved);
}

static void seed_main_initialized(BYTE *data, DWORD saved, int snapshot_copy)
{
    DWORD phase = snapshot_copy ? 7 : 11;
    DWORD c9 = snapshot_copy ? 9 : 13;
    DWORD flags = snapshot_copy ? 0x13572468 : 0x24681357;
    SYSTEM_INFO info;
    if (sizeof(info) != 36) fail("SYSTEM_INFO layout");
    memset(&info, 0, sizeof(info));
    info.dwOemId = snapshot_copy ? 0x56780003 : 0x12340002;
    info.dwPageSize = snapshot_copy ? 8192 : 16384;
    info.lpMinimumApplicationAddress = (LPVOID)(saved + (snapshot_copy ? 3000 : 3500));
    info.lpMaximumApplicationAddress = (LPVOID)(saved + (snapshot_copy ? 3016 : 3516));
    info.dwActiveProcessorMask = snapshot_copy ? 0x1122 : 0x3344;
    info.dwNumberOfProcessors = snapshot_copy ? 7 : 9;
    info.dwProcessorType = snapshot_copy ? 586 : 486;
    info.dwAllocationGranularity = snapshot_copy ? 131072 : 262144;
    info.wProcessorLevel = (WORD)(snapshot_copy ? 6 : 5);
    info.wProcessorRevision = (WORD)(snapshot_copy ? 0x1234 : 0x4321);
    memcpy(data + 0x13d8, &phase, 4);
    memcpy(data + 0x12e0, &c9, 4);
    memcpy(data + 0xde4, &flags, 4);
    memcpy(data + 0x1508, &info, sizeof(info));
    {
        int value = snapshot_copy ? 37 : 41;
        memcpy(data + 0x12fc, &value, 4);
        value = snapshot_copy ? 61 : 67;
        memcpy(data + 0xf04, &value, 4);
        value = snapshot_copy ? 83 : 89;
        memcpy(data + 0x1978, &value, 4);
    }
}

/* Explicit code-probe input: unknown original bytes stay on the ORIGINAL
 * side only. The native probe receives just the mapped semantic fields. */
static void seed_restore_state(void)
{
    BYTE *data;
    DWORD saved, first = 0x13579bdfUL, zero = 0, count = 7;
    DWORD callbacks[2], context[3], mode = 1, reuse = 9;
    unsigned index;
    FILE *file;
    data = (BYTE *)malloc(6548);
    if (!data) fail("restore seed allocation");
    readmem(base + 0x5e000, data, 6548);
    memcpy(data + 0x1530, &first, 4);
    saved = (DWORD)VirtualAllocEx(child, NULL, 6548, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!saved) fail("restore seed remote allocation");
    if (main_fields_probe) seed_main_initialized(data, saved, 1);
    seed_command_line_initialized(data, saved, 1);
    if(scanner_tables_probe) {
        unsigned table,byte;for(table=0;table<2;++table)for(byte=0;byte<256;++byte)
            data[8+table*256+byte]=(BYTE)((69+table)*7+byte+11);
    }
    file = fopen("restore-original-input.bin", "wb");
    if (!file || fwrite(data, 1, 6548, file) != 6548) fail("restore seed record");
    fclose(file);
    write_mem(saved, data, 6548);
    if (main_fields_probe) {
        seed_main_initialized(data, saved, 0);
        write_mem(base + 0x5f3d8, data + 0x13d8, 4);
        write_mem(base + 0x5f2e0, data + 0x12e0, 4);
        write_mem(base + 0x5ede4, data + 0xde4, 4);
        write_mem(base + 0x5f508, data + 0x1508, 36);
        write_mem(base + 0x5f2fc, data + 0x12fc, 4);
        write_mem(base + 0x5ef04, data + 0xf04, 4);
        write_mem(base + 0x5f978, data + 0x1978, 4);
        fprintf(events, "main-fields-seed bytes=64 snapshot-phase=7 live-phase=11 snapshot=%08lx pointers=owned\n", saved);
    }
    seed_command_line_live(data, saved);
    if(path_fold_probe)path_fold_seed("reset",0x8101);
    if (checksum_tables_probe) {
        BYTE table[256];
        unsigned table_index, byte_index;
        for (table_index = 0; table_index < 3; ++table_index) {
            for (byte_index = 0; byte_index < 256; ++byte_index)
                table[byte_index] = (BYTE)(31 + table_index * 53 + byte_index * 7);
            write_mem(base + 0x538b8 + table_index * 256, table, 256);
        }
        fprintf(events, "checksum-reset-seed rva=000122a5 tables=3 bytes=768 pattern=31+53*table+7*byte\n");
    }
    free(data);
    write_mem(base + 0x57818, &saved, 4);
    write_mem(base + 0x5f530, &zero, 4);
    if (main_fields_probe) {
        DWORD command[3];
        command[0] = 17; command[1] = 23; command[2] = saved + 4000;
        write_mem(base + 0x55c34, command, sizeof(command));
    }
    if (main_fields_probe) {
        DWORD pointer;
        pointer = saved + 4200; write_mem(base + 0x51850, &pointer, 4);
        pointer = saved + 4300; write_mem(base + 0x5001c, &pointer, 4);
        pointer = saved + 4400; write_mem(base + 0x51854, &pointer, 4);
        pointer = saved + 4500; write_mem(base + 0x50024, &pointer, 4);
        fprintf(events, "work-buffers-seed pointers=owned offsets=4200,4300,4400,4500\n");
    }
    if (main_fields_probe) {
        DWORD arena[8];
        unsigned item;
        for (item = 0; item < 6; ++item) {
            arena[0] = saved + 4600 + item * 4;
            arena[1] = saved + 4700 + item * 4;
            arena[2] = saved + 4800 + item * 4;
            arena[3] = 71 + item * 4;
            arena[4] = saved + 4900 + item * 4;
            arena[5] = 72 + item * 4;
            arena[6] = 73 + item * 4;
            arena[7] = 74 + item * 4;
            write_mem(base + 0x58fb0 + item * 32, arena, sizeof(arena));
        }
        fprintf(events, "arena-seed count=6 stride=32 pointers=owned integer-base=71\n");
    }
    write_mem(base + 0x5781c, &count, 4);
    callbacks[0] = base + 0x12365; callbacks[1] = base + 0x12377;
    write_mem(base + 0x57820, callbacks, 8);
    context[0] = base + 0x5f530; context[1] = 23; context[2] = saved;
    write_mem(base + 0x518c8, context, sizeof(context));
    context[1] = 29;
    write_mem(base + 0x518b8, context, sizeof(context));
    for (index = 0; index < 6; ++index) {
        context[1] = 31 + index;
        write_mem(base + 0x51858 + index * 12, context, sizeof(context));
    }
    for (index = 0; index < buffer_probe_count; ++index) seed_output_buffer(index, 0x122a5);
    if (value_probe) seed_value(0x122a5, 0);
    if (signature_probe) seed_signature(0x122a5, 0);
    if (pdb_probe) seed_pdb_context();
    seed_pools(saved);
    if (prototype_probe) seed_prototype(0x122a5, 1);
    if (pch_state_probe) seed_pch_state(saved, 0x122a5, "pch-state-seed");
    write_mem(base + 0x5c08c, &mode, 4);
    write_mem(base + 0x5c064, &reuse, 4);
    fprintf(events, "restore-seed snapshot=%08lx bytes=6548 first=%08lx count=7 mode=1 reuse=9\n", saved, first);
    restore_seeded = 1;
}

/* Auxiliary callback-body check: the object/value/frame are owned typed
 * sentinels, not input to the normal compilation case. */
static void seed_context_callback(DWORD rva)
{
    DWORD context[3];
    context[0] = base + 0x5f530; context[1] = rva == 0x12365 ? 23 : 29;
    readmem(base + 0x57818, &context[2], 4);
    if (!context[2]) fail("context probe missing owned snapshot");
    write_mem(base + (rva == 0x12365 ? 0x518c8 : 0x518b8), context, sizeof(context));
    fprintf(events, "context-seed rva=%08lx object=%08lx value=%lu frame=%08lx\n",
            rva, context[0], context[1], context[2]);
    context_seeded |= rva == 0x12365 ? 1 : 2;
}

static void seed_contexts_callback(void)
{
    DWORD context[3];
    unsigned index;
    context[0] = base + 0x5f530;
    readmem(base + 0x57818, &context[2], 4);
    if (!context[2]) fail("context array probe missing owned snapshot");
    for (index = 0; index < 6; ++index) {
        context[1] = 31 + index;
        write_mem(base + 0x51858 + index * 12, context, sizeof(context));
    }
    fprintf(events, "contexts-seed rva=00012389 count=6 object=%08lx frame=%08lx\n",
            context[0], context[2]);
    context_seeded |= 4;
}

static void boundary(CONTEXT *context, DWORD address, HANDLE thread)
{
    DWORD stack[5], argument, pointer, environment_address;
    FILE *saved;
    char filename[80], text[4096];
    readmem(context->Esp, stack, sizeof(stack));
    fprintf(events, "trap seq=%u rva=%08lx esp=%08lx eax=%08lx ecx=%08lx stack=%08lx,%08lx,%08lx,%08lx,%08lx\n",
            sequence, address - base, context->Esp, context->Eax, context->Ecx,
            stack[0], stack[1], stack[2], stack[3], stack[4]);
    fprintf(events, "last-error=%lu\n", thread_error(context, thread));
    fprintf(events, "x87-control=%04lx\n", context->FloatSave.ControlWord);
    if (pool_cycle_probe)
        fprintf(events, "pool-registers seq=%u ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx\n", sequence, context->Ebx, context->Esi, context->Edi, context->Ebp);
    if (address == base + 0x12229) {
        environment_address = exported(crt_base, "_environ");
        readmem(environment_address, &environment_address, 4);
        save_vector(environment_address, 4096, "invoke-environment.bin");
        save_vector(environment_address, 4096, "original-crt-environment.bin");
        {
            DWORD argc_address, argv_address, host_argc, host_argv;
            argc_address = exported(crt_base, "__argc");
            argv_address = exported(crt_base, "__argv");
            readmem(argc_address, &host_argc, 4);
            readmem(argv_address, &host_argv, 4);
            if (!host_argc || host_argc > 256 || !host_argv) fail("invalid original HOST CRT argv");
            save_vector(host_argv, host_argc + 1, "original-crt-argv.bin");
            fprintf(events, "host-crt argc=%lu argv=%08lx environment=%08lx argc-export=%08lx argv-export=%08lx environment-export=%08lx\n",
                    host_argc, host_argv, environment_address, argc_address, argv_address, exported(crt_base, "_environ"));
        }
        if (stack[1] > 256) fail("Invoke argc too large");
        if(isolated_locale_probe)save_vector(stack[2],stack[1]+1,"invoke-entry-argv.bin");
        for (argument = 0; argument <= stack[1]; ++argument) {
            readmem(stack[2] + argument * 4, &pointer, 4);
            if (argument == stack[1]) {
                if (pointer) fail("Invoke argv missing terminator");
                break;
            }
            if (!pointer) fail("Invoke argv early terminator");
            remote_string(pointer, text, sizeof(text));
            sprintf(filename, "invoke-arg-%03lu.bin", argument);
            saved = fopen(filename, "wb");
            if (!saved || fwrite(text, 1, strlen(text) + 1, saved) != strlen(text) + 1)
                fail("save actual Invoke argv");
            fclose(saved);
            fprintf(events, "invoke-arg index=%lu address=%08lx bytes=%u\n", argument, pointer, strlen(text) + 1);
        }
    }
    if (address == base + 0x12873) {
        fprintf(events, "allocation-helper-entry index=%u pc=%08lx esp=%08lx edi=%08lx esi=%08lx\n", allocation_call_count / 2, address, context->Esp, context->Edi, context->Esi);
    }
    if (pool_cycle_probe && address == base + 0x12b49) {
        DWORD address, bytes;
        readmem(base + 0x58fb8, &address, 4); readmem(base + 0x58fbc, &bytes, 4);
        if (!address || bytes != 0x10000) fail("final pool committed extent");
        save_remote_state("pool-final-committed", 0, address, bytes);
    }
    if (include_alias_probe && address == base + 0x12e1c) {
        DWORD allocation, bytes, root, head;
        readmem(base + 0x58fb8, &allocation, 4);
        readmem(base + 0x58fbc, &bytes, 4);
        readmem(base + 0x5e414, &root, 4);
        if (!allocation || bytes != 0x10000 || root < allocation ||
            root - allocation > bytes - 4) fail("include alias root ownership");
        readmem(root, &head, 4);
        save_remote_state("include-alias-root", 0, root, 4);
        save_remote_state("include-alias-committed", 0, allocation, bytes);
        fprintf(events, "include-aliases seq=%u root=%08lx head=%08lx allocation=%08lx bytes=%08lx\n",
                sequence, root, head, allocation, bytes);
    }
    save_state();
    if (address == base + 0x126fc) {
        unsigned i;
        for (i = 0; i < count; ++i) if (ranges[i].entry && ranges[i].first == 0x126fc) break;
        if (i < count) {
            if (pool_return_active) fail("nested pool member call");
            pool_return_address = stack[0]; pool_entry_esp = context->Esp;
            readmem(pool_return_address, &pool_return_saved, 1);
            if (pool_return_saved != 0xc3 || pool_return_address < base + code_rva + 5 || pool_return_address >= base + code_rva + code_size) fail("pool member return identity");
            {
                DWORD site = pool_return_address - base - code_rva;
                LONG displacement;
                if (snapshot[site - 5] != 0xe8) fail("pool member caller opcode");
                memcpy(&displacement, snapshot + site - 4, 4);
                if (pool_return_address + displacement != base + 0x126fc) fail("pool member caller destination");
            }
            patch(pool_return_address, 0xcc); pool_return_active = 1;
        }
    }
    if (address == base + 0x11e2a) {
        unsigned i;
        for (i = 0; i < count; ++i) if (ranges[i].entry && ranges[i].first == 0x11e2a) break;
        if (i < count) {
            if (pch_return_active || stack[0] != base + 0x12348 || context->Ecx != base + 0x58f68) fail("PCH member inherited caller/receiver");
            pch_return_address = stack[0]; pch_entry_esp = context->Esp; pch_entry_edi = context->Edi;
            readmem(pch_return_address, &pch_return_saved, 1);
            if (pch_return_saved != snapshot[0x12348 - code_rva] || pch_return_saved != 0xeb) fail("PCH inherited return opcode");
            fprintf(events, "pch-member-entry seq=%u edi=%08lx inherited-return=%08lx\n", sequence - 1, pch_entry_edi, pch_return_address);
            patch(pch_return_address, 0xcc); pch_return_active = 1;
        }
    }
    if (parser_probe && address == base + 0x12900) {
        if (parser_return_active) fail("nested internal parser call");
        parser_return_address = stack[0]; parser_entry_esp = context->Esp;
        readmem(parser_return_address, &parser_return_saved, 1);
        if (parser_return_address != base + 0x128d2 || parser_return_saved != snapshot[0x128d2 - code_rva]) fail("internal parser caller return identity");
        fprintf(events, "internal-parser-entry index=%lu seq=%u argument=%08lx mode=%lu esp=%08lx\n", parser_calls, sequence - 1, stack[1], stack[2], context->Esp);
        patch(parser_return_address, 0xcc); parser_return_active = 1;
    }
    if (address == base + entry_rva) {
        if (return_active) fail("nested target DLL entry unsupported");
        return_address = stack[0]; return_reason = stack[2];
        readmem(return_address, &return_saved, 1);
        patch(return_address, 0xcc); return_active = 1;
    }
}

static void observe_allocation(CONTEXT *context, DWORD address, HANDLE thread)
{
    DWORD args[4], cursor, end;
    MEMORY_BASIC_INFORMATION region;
    unsigned index;
    int before = address == base + 0x12889 || address == base + 0x128a2;
    if (before) {
        readmem(context->Esp, args, sizeof(args));
        fprintf(events, "allocation-before index=%u pc=%08lx esp=%08lx edi=%08lx args=%08lx,%08lx,%08lx,%08lx\n",
            allocation_call_count, address, context->Esp, context->Edi, args[0], args[1], args[2], args[3]);
        allocation_before_esp = context->Esp; allocation_before_edi = context->Edi;
        if (context->Edi != allocation_provider) fail("allocation cached provider identity");
    } else if (address == base + 0x1288b || address == base + 0x128a4) {
        fprintf(events, "allocation-after index=%u pc=%08lx esp=%08lx eax=%08lx edi=%08lx before-esp=%08lx before-edi=%08lx\n",
            allocation_call_count, address, context->Esp, context->Eax, context->Edi, allocation_before_esp, allocation_before_edi);
        fprintf(events, "allocation-error index=%u value=%lu fpu=%04lx\n", allocation_call_count, thread_error(context, thread), context->FloatSave.ControlWord);
        if (!context->Eax) fail("allocation capture requires actual successful call");
        if (!(allocation_call_count & 1)) {
            if (allocation_region_count == 2) fail("excess work reservations");
            allocation_regions[allocation_region_count++] = context->Eax;
        }
        cursor = context->Eax; end = cursor + 0x500000;
        for (index = 0; cursor < end; ++index) {
            if (VirtualQueryEx(child, (void *)cursor, &region, sizeof(region)) != sizeof(region) ||
                    (DWORD)region.BaseAddress != cursor || !region.RegionSize || cursor + region.RegionSize > end) fail("allocation region query");
            fprintf(events, "allocation-region index=%u part=%u base=%08lx allocation=%08lx allocation-protect=%08lx bytes=%08lx state=%08lx protect=%08lx type=%08lx\n",
                allocation_call_count, index, region.BaseAddress, region.AllocationBase, region.AllocationProtect, region.RegionSize, region.State, region.Protect, region.Type);
            cursor += region.RegionSize;
        }
        if (allocation_call_count & 1) save_remote_state("work-committed", allocation_call_count / 2, context->Eax, 0x4000);
        ++allocation_call_count;
    } else {
        fprintf(events, "allocation-helper-return index=%u pc=%08lx esp=%08lx eax=%08lx edi=%08lx esi=%08lx\n",
                allocation_call_count / 2 - 1, address, context->Esp, context->Eax, context->Edi, context->Esi);
    }
}

static int pool_observer_index(DWORD address)
{
    unsigned index;
    for (index = 0; index < 14; ++index)
        if (address == base + pool_observers[index].first) return index;
    return -1;
}

static void observe_pool_commit(CONTEXT *context, DWORD address, HANDLE thread)
{
    DWORD arguments[4], provider, reserve, cursor;
    MEMORY_BASIC_INFORMATION region;
    unsigned part;
    if (address == base + 0x98b9) {
        readmem(context->Esp, arguments, sizeof(arguments)); readmem(base + 0x5d1c4, &provider, 4);
        pool_commit_address = arguments[0]; pool_commit_bytes = arguments[1];
        pool_commit_esp = context->Esp; pool_commit_error = thread_error(context, thread);
        fprintf(events, "pool-commit-before seq=%u esp=%08lx address=%08lx bytes=%08lx kind=%08lx protection=%08lx provider=%08lx error=%lu\n", sequence, context->Esp, arguments[0], arguments[1], arguments[2], arguments[3], provider, pool_commit_error);
    } else if (address == base + 0x98bf) {
        if (context->Eax != pool_commit_address || pool_commit_bytes != 0x10000) fail("actual pool commit success/size");
        fprintf(events, "pool-commit-after seq=%u esp=%08lx before-esp=%08lx result=%08lx error=%lu before-error=%lu\n", sequence, context->Esp, pool_commit_esp, context->Eax, thread_error(context, thread), pool_commit_error);
        readmem(base + 0x58fc4, &reserve, 4); cursor = pool_commit_address;
        for (part = 0; part < 2; ++part) {
            if (VirtualQueryEx(child, (LPVOID)cursor, &region, sizeof(region)) != sizeof(region)) fail("pool commit region query");
            fprintf(events, "pool-commit-region part=%u base=%08lx allocation=%08lx allocation-protect=%08lx bytes=%08lx state=%08lx protect=%08lx type=%08lx\n", part, (DWORD)region.BaseAddress, (DWORD)region.AllocationBase, region.AllocationProtect, region.RegionSize, region.State, region.Protect, region.Type);
            cursor += region.RegionSize;
        }
        if (cursor != pool_commit_address + reserve) fail("pool committed/reserved extent closure");
        save_remote_state("pool-committed", 0, pool_commit_address, pool_commit_bytes);
    }
}

static int host(const char *path, int compile)
{
    HMODULE module;
    FARPROC invoke, abort;
    int pass_result;
    typedef int (WINAPI *CompilerPass)(int, char **, BOOL);
    char *pass_args[] = { "c1.dll", "-il", ".\\minimal", "-f", "test.c", "-W", "1", "-G5", "-Ot", "-Ob0", "-nologo", NULL };
    FILE *capture;
    unsigned argument;
    module = LoadLibraryA(path);
    if (!module) return 3;
    puts("attach=1");
    invoke = GetProcAddress(module, "_InvokeCompilerPass@12");
    abort = GetProcAddress(module, "_AbortCompilerPass@4");
    if (!invoke || !abort || invoke != GetProcAddress(module, (LPCSTR)2) ||
            abort != GetProcAddress(module, (LPCSTR)1)) return 4;
    puts("exports=1");
    if (compile) {
        capture = fopen("invoke-args.txt", "wb");
        if (!capture) return 6;
        fprintf(capture, "argc=11\nBOOL=1\n");
        for (argument = 0; argument < 11; ++argument)
            fprintf(capture, "%u=%s\n", argument, pass_args[argument]);
        fclose(capture);
        pass_result = ((CompilerPass)invoke)(11, pass_args, TRUE);
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
    if (argc == 3 && !strcmp(argv[1], "--host")) return host(argv[2], 0);
    if (argc == 3 && !strcmp(argv[1], "--host-minimal")) return host(argv[2], 1);
    if ((argc != 6 && argc != 7) || strcmp(argv[1], "--debug")) return 2;
    {
        FILE *probe = fopen("hfile-offset-probe.txt", "r");
        if (probe) { fclose(probe); hfile_offset_probe = 1; }
    }
    if (argc == 7 && strcmp(argv[6], "minimal") && strcmp(argv[6], "minimal-restore")) return 2;
    restore_probe = argc == 7 && !strcmp(argv[6], "minimal-restore");
    if (restore_probe) {
        FILE *probe;
        DWORD object, bytes, first, callback;
        read_buffer_probes(); read_value_probe(); read_signature_probe(); read_pdb_probe(); read_pool_probes();
        read_command_line_seeds();
        probe = fopen("prototype-probe.txt", "r");
        if (probe) {
            if (fscanf(probe, "%lx %lx %lx %lx", &object, &bytes, &first, &callback) != 4 || object != 0x58f50 || bytes != 20 || first != 1 || callback != 0xa0) fail("prototype probe metadata");
            prototype_probe = 1; fclose(probe);
        }
        probe = fopen("pch-state-probe.txt", "r");
        if (probe) {
            if (fscanf(probe, "%lx %lx", &object, &bytes) != 2 || object != 0x58f68 || bytes != 68) fail("PCH state probe metadata");
            pch_state_probe = 1; fclose(probe);
        }
        probe = fopen("main-fields-probe.txt", "r");
        if (probe) {
            DWORD phase, c9, flags, info, known, command;
            if (fscanf(probe, "%lx %lx %lx %lx %lx %lx", &phase, &c9, &flags, &info, &known, &command) != 6 || phase != 0x5f3d8 || c9 != 0x5f2e0 || flags != 0x5ede4 || info != 0x5f508 || known != 64 || command != 0x55c34) fail("main fields probe metadata");
            main_fields_probe = 1; fclose(probe);
        }
        probe = fopen("pch-member-probe.txt", "r");
        if (probe) {
            if (fscanf(probe, "%lx %lx %lx", &first, &object, &bytes) != 3 || first != 0x11e2a || object != 0x58f68 || bytes != 68) fail("PCH member probe metadata");
            pch_member_probe = 1; fclose(probe);
        }
    }
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
    sprintf(command, "\"%s\" %s \"%s\"", self, argc == 7 ? "--host-minimal" : "--host", target_path);
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
            if (code == EXCEPTION_SINGLE_STEP && pending != -1) {
                if (pending <= -60 && pending >= -73) patch(base + pool_observers[-pending - 60].first, 0xcc);
                else if (pending <= -30 && pending >= -39) patch(base + scaler_observers[-pending - 30].first, 0xcc);
                else if (pending == -21 || pending == -22) patch(base + system_observers[-pending - 21].first, 0xcc);
                else if (pending == -20) patch(base + getenv_observer.first, 0xcc);
                else if (pending <= -3) patch(base + allocation_observers[-pending - 3].first, 0xcc);
                else if (pending == -2) patch(restore_observer, 0xcc);
                else patch(base + ranges[pending].first, 0xcc);
                context.EFlags &= ~0x100; pending = -1;
                if (!SetThreadContext(thread, &context)) fail("single step context");
            } else if (code == EXCEPTION_BREAKPOINT && vector_boundary(address,&context,thread,event.dwThreadId)) {
            } else if (code == EXCEPTION_BREAKPOINT && path_fold_boundary(address,&context,thread,event.dwThreadId)) {
                /* Genuine public call/return or owned helper return. */
            } else if (code == EXCEPTION_BREAKPOINT && checksum_return_active && address == base + 0x12b66) {
                char name[64];
                FILE *floating;
                fprintf(events, "checksum-return seq=%u pc=%08lx esp=%08lx error=%lu\n", sequence, address, context.Esp, thread_error(&context, thread));
                fprintf(events, "pool-registers seq=%u ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx\n", sequence, context.Ebx, context.Esi, context.Edi, context.Ebp);
                sprintf(name, "fpu-%03u.bin", sequence); floating = fopen(name, "wb");
                if (!floating || fwrite(&context.FloatSave, 1, sizeof(context.FloatSave), floating) != sizeof(context.FloatSave)) fail("checksum return FP record");
                fclose(floating); save_state(); patch(address, checksum_return_saved); checksum_return_active = 0;
                context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("checksum return context");
            } else if (code == EXCEPTION_BREAKPOINT && locale_entry_active && address == locale_entry_address) {
                DWORD words[3];
                FILE *floating;
                char name[64], text[64];
                readmem(context.Esp, words, sizeof(words));
                if (words[0] != base + 0x12b73 || words[1] != 2 || words[2] != base + 0x601c1)
                    fail("actual setlocale call ABI");
                remote_string(words[2], text, sizeof(text));
                if (text[0]) fail("actual setlocale empty string");
                fprintf(events, "public-entry seq=%u module=MSVCRT.dll name=setlocale provider=%08lx rva=00005729 iat=0005d120 esp=%08lx return=%08lx category=%lu locale=%08lx error=%lu\n",
                        sequence, address, context.Esp, words[0], words[1], words[2], thread_error(&context, thread));
                fprintf(events, "pool-registers seq=%u ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx\n", sequence, context.Ebx, context.Esi, context.Edi, context.Ebp);
                sprintf(name, "fpu-%03u.bin", sequence); floating = fopen(name, "wb");
                if (!floating || fwrite(&context.FloatSave, 1, sizeof(context.FloatSave), floating) != sizeof(context.FloatSave)) fail("locale entry FP record");
                fclose(floating); save_remote_state("setlocale-input", 0, words[2], 1);
                if(isolated_locale_probe)locale_snapshot("before",&context,thread,event.dwThreadId);
                save_state();
                locale_entry_esp = context.Esp; locale_return_address = words[0];
                readmem(locale_return_address, &locale_return_saved, 1);
                if (locale_return_saved != snapshot[0x12b73-code_rva]) fail("locale return ownership");
                patch(locale_return_address, 0xcc); locale_return_active = 1;
                patch(address, locale_entry_saved); locale_entry_active = 0; context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("locale entry context");
            } else if (code == EXCEPTION_BREAKPOINT && locale_return_active && address == locale_return_address) {
                char text[256], name[64];
                FILE *floating, *output;
                fprintf(events, "public-return seq=%u module=MSVCRT.dll name=setlocale pc=%08lx esp=%08lx entry-esp=%08lx result=%08lx error=%lu\n",
                        sequence, address, context.Esp, locale_entry_esp, context.Eax, thread_error(&context, thread));
                if (context.Eax) {
                    remote_string(context.Eax, text, sizeof(text)); output = fopen("setlocale-result.bin", "wb");
                    if (!output || fwrite(text, 1, strlen(text)+1, output) != strlen(text)+1) fail("locale result record");
                    fclose(output);
                }
                sprintf(name, "fpu-%03u.bin", sequence); floating = fopen(name, "wb");
                if (!floating || fwrite(&context.FloatSave, 1, sizeof(context.FloatSave), floating) != sizeof(context.FloatSave)) fail("locale return FP record");
                fclose(floating);
                if(isolated_locale_probe){locale_result=context.Eax;locale_snapshot("after",&context,thread,event.dwThreadId);locale_complete=1;}
                save_state();
                patch(address, locale_return_saved); locale_return_active = 0; context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("locale return context");
            } else if(code==EXCEPTION_BREAKPOINT && mb_entry_active && address==mb_entry_address) {
                DWORD words[2];readmem(context.Esp,words,sizeof(words));
                if(!locale_complete || words[0]!=base+0x12f12 || words[1]!=(DWORD)-4)fail("actual _setmbcp caller/arguments");
                fprintf(events,"mb-entry seq=%u pc=%08lx esp=%08lx return=%08lx codepage=%ld ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx\n",sequence,address,context.Esp,words[0],(LONG)words[1],context.Ebx,context.Esi,context.Edi,context.Ebp);
                locale_snapshot("mb-before",&context,thread,event.dwThreadId);mb_entry_esp=context.Esp;mb_return_address=words[0];
                mb_return_saved=snapshot[0x12f12-code_rva];patch(mb_return_address,0xcc);mb_return_active=1;
                patch(address,mb_entry_saved);mb_entry_active=0;context.Eip=address;if(!SetThreadContext(thread,&context))fail("mb entry context");
            } else if(code==EXCEPTION_BREAKPOINT && mb_return_active && address==mb_return_address) {
                fprintf(events,"mb-return seq=%u pc=%08lx esp=%08lx entry-esp=%08lx result=%08lx ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx\n",sequence,address,context.Esp,mb_entry_esp,context.Eax,context.Ebx,context.Esi,context.Edi,context.Ebp);
                locale_snapshot("mb-after",&context,thread,event.dwThreadId);mb_complete=1;patch(address,mb_return_saved);mb_return_active=0;
                context.Eip=address;if(!SetThreadContext(thread,&context))fail("mb return context");
            } else if (code == EXCEPTION_BREAKPOINT && parser_return_active && address == parser_return_address) {
                fprintf(events, "internal-parser-return index=%lu seq=%u pc=%08lx esp=%08lx entry-esp=%08lx eax=%08lx\n", parser_calls, sequence, address, context.Esp, parser_entry_esp, context.Eax);
                fprintf(events, "last-error=%lu\n", thread_error(&context, thread));
                fprintf(events, "x87-control=%04lx\n", context.FloatSave.ControlWord);
                save_state(); patch(address, parser_return_saved); parser_return_active = 0; ++parser_calls;
                context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("internal parser return context");
            } else if (code == EXCEPTION_BREAKPOINT && pch_return_active && address == pch_return_address) {
                fprintf(events, "pch-member-return seq=%u pc=%08lx eax=%08lx esp=%08lx entry-esp=%08lx edi=%08lx entry-edi=%08lx\n", sequence, address, context.Eax, context.Esp, pch_entry_esp, context.Edi, pch_entry_edi);
                fprintf(events, "last-error=%lu\n", thread_error(&context, thread));
                fprintf(events, "x87-control=%04lx\n", context.FloatSave.ControlWord);
                save_state(); patch(pch_return_address, pch_return_saved); pch_return_active = 0;
                context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("PCH return context");
            } else if (code == EXCEPTION_BREAKPOINT && pool_return_active && address == pool_return_address) {
                fprintf(events, "pool-return seq=%u pc=%08lx eax=%08lx esp=%08lx entry-esp=%08lx\n", sequence, address, context.Eax, context.Esp, pool_entry_esp);
                fprintf(events, "last-error=%lu\n", thread_error(&context, thread));
                fprintf(events, "x87-control=%04lx\n", context.FloatSave.ControlWord);
                save_state(); patch(pool_return_address, pool_return_saved); pool_return_active = 0;
                context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("pool return context");
            } else if (code == EXCEPTION_BREAKPOINT && return_active && address == return_address) {
                fprintf(events, "crt-return seq=%u reason=%lu status=%lu pc=%08lx\n",
                        sequence, return_reason, context.Eax, address);
                fprintf(events, "last-error=%lu\n", thread_error(&context, thread));
                fprintf(events, "x87-control=%04lx\n", context.FloatSave.ControlWord);
                save_state();
                patch(return_address, return_saved); return_active = 0;
                context.Eip = address;
                if (!SetThreadContext(thread, &context)) fail("CRT return context");
            } else if (code == EXCEPTION_BREAKPOINT && target_seen &&
                    address >= base + code_rva && address < base + code_rva + code_size) {
                if (!initialized) {
                    if (address != base + entry_rva || !crt_seen) fail("unguarded entry/provider");
                    prepare_mask();
                }
                if (restore_probe && !restore_seeded && address == base + 0x122a5) seed_restore_state();
                if (restore_probe && !(context_seeded & 1) && address == base + 0x12365) seed_context_callback(0x12365);
                if (restore_probe && !(context_seeded & 2) && address == base + 0x12377) seed_context_callback(0x12377);
                if (restore_probe && !(context_seeded & 4) && address == base + 0x12389) seed_contexts_callback();
                if (restore_probe) {
                    for (i = 0; i < buffer_probe_count; ++i) {
                        if (!buffer_probes[i].seeded && address == base + buffer_probes[i].constructor) {
                            seed_output_buffer(i, buffer_probes[i].constructor);
                            buffer_probes[i].seeded = 1;
                        }
                    }
                }
                if (restore_probe && value_probe && !value_seeded && address == base + 0x12646) {
                    seed_value(0x12646, 4); value_seeded = 1;
                }
                if (restore_probe && signature_probe && !signature_seeded && address == base + 0x12664) {
                    seed_signature(0x12664, 1); signature_seeded = 1;
                }
                if (restore_probe && pdb_probe && !pdb_seeded && address == base + 0x12676) {
                    write_mem(base + 0x5c064, &pdb_reuse, 4);
                    fprintf(events, "pdb-helper-seed rva=00012676 reuse=%lu\n", pdb_reuse); pdb_seeded = 1;
                }
                if (restore_probe && pool_member_probe && address == base + 0x126fc) {
                    DWORD saved;
                    unsigned pool;
                    for (pool = 0; pool < pool_count; ++pool) if (context.Ecx == base + pool_objects[pool]) break;
                    if (pool == pool_count || (pool_member_seeded & (1 << pool))) fail("pool member receiver/seed identity");
                    readmem(base + 0x57818, &saved, 4);
                    seed_pool(pool, saved, "pool-member-field-seed"); pool_member_seeded |= 1 << pool;
                    fprintf(events, "pool-member-seed rva=000126fc index=%u object=%08lx allocation=%08lx\n", pool, pool_objects[pool], saved);
                }
                if (restore_probe && prototype_probe && !prototype_seeded && address == base + 0x1277b) {
                    seed_prototype(0x1277b, 0xa0); prototype_seeded = 1;
                }
                if (restore_probe && pch_member_probe && !pch_member_seeded && address == base + 0x11e2a) {
                    DWORD saved;
                    if (context.Ecx != base + 0x58f68) fail("PCH member seed receiver");
                    readmem(base + 0x57818, &saved, 4);
                    seed_pch_state(saved, 0x11e2a, "pch-member-seed"); pch_member_seeded = 1;
                }
                for (i = 0; i < allocation_observer_count; ++i)
                    if (address == base + allocation_observers[i].first) break;
                if (i < allocation_observer_count) observe_allocation(&context, address, thread);
                if (parser_probe && address == base + getenv_observer.first) {
                    DWORD name, resolved;
                    char text[64];
                    readmem(context.Esp, &name, 4); remote_string(name, text, sizeof(text));
                    readmem(base + 0x5d050, &resolved, 4);
                    if (resolved != exported(crt_base, "getenv")) fail("canonical getenv provider");
                    fprintf(events, "internal-getenv seq=%u pc=%08lx provider=%08lx name=%s name-address=%08lx result=%08lx parser-calls=%lu\n", sequence, address, resolved, text, name, context.Eax, parser_calls);
                }
                if (system_probe && (address == base + 0x127ec || address == base + 0x127f2)) {
                    DWORD argument, provider;
                    SYSTEM_INFO info;
                    if (address == base + 0x127ec) {
                        readmem(context.Esp, &argument, 4); readmem(base + 0x5d1b4, &provider, 4);
                        if (argument != base + 0x5f508) fail("GetSystemInfo output identity");
                        system_before_esp = context.Esp; system_before_error = thread_error(&context, thread);
                        fprintf(events, "systeminfo-before seq=%u pc=%08lx esp=%08lx argument=%08lx provider=%08lx error=%lu fpu=%04lx\n", sequence, address, context.Esp, argument, provider, system_before_error, context.FloatSave.ControlWord);
                        save_remote_state("system-info-before", 0, argument, sizeof(info));
                    } else {
                        fprintf(events, "systeminfo-after seq=%u pc=%08lx esp=%08lx before-esp=%08lx error=%lu before-error=%lu fpu=%04lx\n", sequence, address, context.Esp, system_before_esp, thread_error(&context, thread), system_before_error, context.FloatSave.ControlWord);
                        save_remote_state("system-info-after", 0, base + 0x5f508, sizeof(info));
                    }
                }
                if (restore_probe && arena_reset_probe && address == base + 0x12ab2) {
                    DWORD value, object, allocation;
                    DWORD written;
                    object = context.Ecx;
                    if (arena_reset_count >= 6 || object != base + 0x58fb0 + arena_reset_count * 32) fail("Reset auxiliary receiver order");
                    save_remote_state("arena-reset-preseed", arena_reset_count, base + 0x58fb0, 192);
                    readmem(object + 8, &allocation, 4);
                    if (!allocation) fail("Reset auxiliary requires actual owned reservation");
                    value = allocation + 0x1000;
                    if (!WriteProcessMemory(child, (void *)object, &value, 4, &written) || written != 4) fail("Reset cursor seed");
                    value = allocation + 0x2000;
                    if (!WriteProcessMemory(child, (void *)(object + 4), &value, 4, &written) || written != 4) fail("Reset limit seed");
                    value = 117 + arena_reset_count;
                    if (!WriteProcessMemory(child, (void *)(object + 12), &value, 4, &written) || written != 4) fail("Reset committed size seed");
                    value = 37 + arena_reset_count;
                    if (!WriteProcessMemory(child, (void *)(object + 24), &value, 4, &written) || written != 4) fail("Reset value seed");
                    fprintf(events, "arena-reset-seed index=%u object=%08lx allocation=%08lx cursor-offset=00001000 limit-offset=00002000 committed=%lu reset=%lu\n", arena_reset_count, object, allocation, 117 + arena_reset_count, 37 + arena_reset_count);
                    ++arena_reset_count;
                }
                if (scaler_probe) {
                    FILE *floating;
                    char name[64];
                    sprintf(name, "fpu-%03u.bin", sequence);
                    floating = fopen(name, "wb");
                    if (!floating || fwrite(&context.FloatSave, 1, sizeof(context.FloatSave), floating) != sizeof(context.FloatSave)) fail("floating context record");
                    fclose(floating);
                    fprintf(events, "fpu seq=%u rva=%08lx eax=%08lx edx=%08lx esp=%08lx cw=%08lx sw=%08lx tag=%08lx\n", sequence, address-base, context.Eax, context.Edx, context.Esp, context.FloatSave.ControlWord, context.FloatSave.StatusWord, context.FloatSave.TagWord);
                    if (address == base + 0x12a51) {
                        DWORD arguments[4], provider;
                        readmem(context.Esp, arguments, sizeof(arguments)); readmem(base + 0x5d1c4, &provider, 4);
                        arena_allocation_esp = context.Esp; arena_allocation_error = thread_error(&context, thread);
                        fprintf(events, "arena-allocation-before seq=%u esp=%08lx address=%08lx bytes=%08lx kind=%08lx protection=%08lx provider=%08lx error=%lu\n", sequence, context.Esp, arguments[0], arguments[1], arguments[2], arguments[3], provider, arena_allocation_error);
                    } else if (address == base + 0x12a57) {
                        MEMORY_BASIC_INFORMATION region;
                        DWORD size;
                        fprintf(events, "arena-allocation-after seq=%u esp=%08lx before-esp=%08lx result=%08lx error=%lu before-error=%lu\n", sequence, context.Esp, arena_allocation_esp, context.Eax, thread_error(&context, thread), arena_allocation_error);
                        if (context.Eax) {
                            size = VirtualQueryEx(child, (LPVOID)context.Eax, &region, sizeof(region));
                            if (size != sizeof(region)) fail("arena reservation region query");
                            fprintf(events, "arena-region base=%08lx allocation=%08lx allocation-protect=%08lx bytes=%08lx state=%08lx protect=%08lx type=%08lx\n", (DWORD)region.BaseAddress, (DWORD)region.AllocationBase, region.AllocationProtect, region.RegionSize, region.State, region.Protect, region.Type);
                        }
                    }
                }
                if (pool_cycle_probe && pool_observer_index(address) >= 0) {
                    if(vector_started)observe_vector_commit(&context,address,thread);
                    else observe_pool_commit(&context,address,thread);
                }
                if(isolated_locale_probe && mb_complete && address==base+0x12f3e)locale_snapshot("next",&context,thread,event.dwThreadId);
                if(path_fold_probe && address==base+0x12f3e)path_fold_enter(&context);
                if(path_fold_probe && address==base+0x12f87)environment_enter(&context);
                if(vector_probe && address==base+0x11e3c)vector_enter(&context);
                if(vector_probe && address==base+0x9883)vector_member_enter(&context);
                boundary(&context, address, thread);
                for (i = 0; i < count; ++i)
                    if (ranges[i].entry && address == base + ranges[i].first) break;
                if (i < count) {
                    patch(address, ranges[i].saved); pending = i;
                    context.EFlags |= 0x100;
                } else if (pool_cycle_probe && pool_observer_index(address) >= 0) {
                    i = pool_observer_index(address); patch(address, pool_observers[i].saved);
                    pending = -(int)i - 60; context.EFlags |= 0x100;
                } else if (scaler_probe && (address == base + 0x12a7f || address == base + 0x12a85 || address == base + 0x12a8b || address == base + 0x12a96 || address == base + 0x12a29 || address == base + 0x12a51 || address == base + 0x12a57 || (arena_reset_probe && (address == base + 0x12a70 || address == base + 0x12a0f || address == base + 0x129bc)))) {
                    for (i = 0; i < scaler_observer_count; ++i) if (address == base + scaler_observers[i].first) break;
                    if (i == scaler_observer_count) fail("unknown scaler observer");
                    patch(address, scaler_observers[i].saved); pending = -(int)i - 30; context.EFlags |= 0x100;
                } else if (restore_probe && address == restore_observer) {
                    patch(address, restore_observer_saved); pending = -2;
                    context.EFlags |= 0x100;
                } else if (system_probe && (address == base + 0x127ec || address == base + 0x127f2)) {
                    i = address == base + 0x127ec ? 0 : 1;
                    patch(address, system_observers[i].saved); pending = -(int)i - 21; context.EFlags |= 0x100;
                } else if (parser_probe && address == base + getenv_observer.first) {
                    patch(address, getenv_observer.saved); pending = -20; context.EFlags |= 0x100;
                } else {
                    for (i = 0; i < allocation_observer_count; ++i)
                        if (address == base + allocation_observers[i].first) break;
                    if (i < allocation_observer_count) {
                        patch(address, allocation_observers[i].saved); pending = -(int)i - 3;
                        context.EFlags |= 0x100;
                    } else if (baseline && address == base + entry_rva) {
                        patch(address, 0x53);
                    } else {
                        if(signature_reference && (!signature_reference_complete || address!=base+0x12f9c))fail("signature reference controlled next_argument stop");
                        fprintf(events, "frontier rva=%08lx\n", address - base); fflush(events);
                        TerminateProcess(child, 125); return 125;
                    }
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
