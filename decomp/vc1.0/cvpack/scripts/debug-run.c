/* Original-reference capture only. Never linked into the reconstruction. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <recon.h>

typedef struct RuntimeRange {
    DWORD start, end, offset;
} RuntimeRange;

static RuntimeRange ranges[512];
static unsigned range_count;
static unsigned char original_bytes[65536];
static unsigned char walk_bytes[524288];
static unsigned char array_bytes[65536];
static char object_line[1048704];
static DWORD watched_spawn, spawn_return;
static unsigned char spawn_return_byte;

static int watch_spawn(HANDLE child, HANDLE thread, HANDLE log)
{
    CONTEXT context;
    DWORD arguments[24], bytes, written, i, j;
    unsigned char byte, trap = 0xcc;
    char line[2048];
    int length;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context) ||
        !ReadProcessMemory(child, (void *)context.Esp, arguments, sizeof(arguments), &bytes) ||
        bytes != sizeof(arguments)) return 0;
    sprintf(line, "spawn-mode %lu\n", arguments[1]);
    WriteFile(log, line, strlen(line), &written, 0);
    for (i = 2; i < 24 && arguments[i]; ++i) {
        length = sprintf(line, "spawn-string %lu ", i-2);
        for (j = 0; j < 512; ++j) {
            if (!ReadProcessMemory(child, (void *)(arguments[i]+j), &byte, 1, &bytes) || bytes != 1)
                return 0;
            if (!byte) break;
            length += sprintf(line+length, "%02x", byte);
        }
        if (j == 512) return 0;
        line[length++] = '\n';
        WriteFile(log, line, length, &written, 0);
    }
    if (i == 24) return 0;
    spawn_return = arguments[0];
    if (!ReadProcessMemory(child, (void *)spawn_return, &spawn_return_byte, 1, &bytes) || bytes != 1 ||
        !WriteProcessMemory(child, (void *)spawn_return, &trap, 1, &bytes) || bytes != 1)
        return 0;
    return 1;
}

static int finish_spawn(HANDLE child, HANDLE thread, HANDLE log)
{
    CONTEXT context;
    DWORD old_errno, bytes, written;
    char line[128];
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context) ||
        !ReadProcessMemory(child, (void *)0x39614, &old_errno, 4, &bytes) || bytes != 4)
        return 0;
    sprintf(line, "spawn-return %ld errno %lu\n", (long)context.Eax, old_errno);
    WriteFile(log, line, strlen(line), &written, 0);
    if (!WriteProcessMemory(child, (void *)spawn_return, &spawn_return_byte, 1, &bytes) || bytes != 1)
        return 0;
    FlushInstructionCache(child, (void *)spawn_return, 1);
    context.Eip = spawn_return;
    spawn_return = 0;
    return SetThreadContext(thread, &context);
}

/* Case-specific original CRT observation. Not reconstruction source. */
static DWORD sopen_start, sopen_end, sopen_offset, sopen_return, sopen_step;
static DWORD sopen_saved_tf, sopen_create_step, sopen_create_tf, sopen_create_seen;
static DWORD child_teb, sopen_access, last_sopen_fd;
static char sopen_pdb_name[260], sopen_pdb_master[1024];
static const char *sopen_active_input, *sopen_active_master;

static unsigned char sopen_return_byte;
static char sopen_name[260], sopen_input[1024], sopen_master[1024];

static int identical_files(const char *left, const char *right)
{
    HANDLE a, b;
    DWORD na, nb;
    int same = 0;
    a = CreateFileA(left, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    b = CreateFileA(right, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (a != INVALID_HANDLE_VALUE && b != INVALID_HANDLE_VALUE) {
        do {
            if (!ReadFile(a, original_bytes, sizeof(original_bytes), &na, 0) ||
                !ReadFile(b, array_bytes, sizeof(array_bytes), &nb, 0) ||
                na != nb || memcmp(original_bytes, array_bytes, na)) break;
            if (!na) { same = 1; break; }
        } while (1);
    }
    if (a != INVALID_HANDLE_VALUE) CloseHandle(a);
    if (b != INVALID_HANDLE_VALUE) CloseHandle(b);
    return same;
}

static int denied_sopen(HANDLE log, const char *reason)
{
    char line[160];
    DWORD written;
    sprintf(line, "crt-denied %08lx %s\n", sopen_start, reason);
    WriteFile(log, line, strlen(line), &written, 0);
    return -1;
}

static int begin_sopen(HANDLE child, HANDLE original, HANDLE log, HANDLE thread)
{
    CONTEXT context;
    DWORD words[5], bytes, written, i, length;
    char name[260], line[1024];
    unsigned char trap = 0xcc;
    int valid_return = 0, used;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context) ||
        !ReadProcessMemory(child, (void *)context.Esp, words, sizeof(words), &bytes) ||
        bytes != sizeof(words)) return denied_sopen(log, "entry-state");
    for (i = 0; i < sizeof(name); ++i) {
        if (!ReadProcessMemory(child, (void *)(words[1]+i), name+i, 1, &bytes) || bytes != 1)
            return denied_sopen(log, "name-read");
        if (!name[i]) break;
    }
    if (i == sizeof(name)) return denied_sopen(log, "name-length");
    used = sprintf(line, "crt-entry %08lx %08lx %08lx %08lx ", sopen_start, words[2], words[3], words[4]);
    for (i = 0; name[i]; ++i) used += sprintf(line+used, "%02x", (unsigned char)name[i]);
    line[used++] = '\n';
    WriteFile(log, line, used, &written, 0);
    if (sopen_return || sopen_step || (context.EFlags & 0x100)) return denied_sopen(log, "nested-or-TF");
    if (!strcmp(name, sopen_name)) {
        if (words[2] != 0x8002) return denied_sopen(log, "mode");
        sopen_active_input = sopen_input;
        sopen_active_master = sopen_master;
        sopen_access = GENERIC_READ|GENERIC_WRITE;
    } else if (sopen_pdb_name[0] && !strcmp(name, sopen_pdb_name)) {
        if (words[2] != 0x8000) return denied_sopen(log, "mode");
        sopen_active_input = sopen_pdb_name;
        sopen_active_master = sopen_pdb_master;
        sopen_access = GENERIC_READ;
    } else return denied_sopen(log, "name");
    if (words[3] != 0x40) return denied_sopen(log, "share");
    if (!identical_files(sopen_active_input, sopen_active_master)) return denied_sopen(log, "input");
    sopen_create_seen = 0;
    for (i = 0; i < range_count; ++i)
        if (ranges[i].start <= words[0] && words[0] < ranges[i].end) valid_return = 1;
    if (!valid_return || !ReadProcessMemory(child, (void *)words[0], &sopen_return_byte, 1, &bytes) ||
        bytes != 1 || sopen_return_byte == trap) return denied_sopen(log, "return-site");
    length = sopen_end-sopen_start;
    if (SetFilePointer(original, sopen_offset, 0, FILE_BEGIN) == 0xffffffff ||
        !ReadFile(original, original_bytes, length, &bytes, 0) || bytes != length ||
        original_bytes[0] != 0x83 || original_bytes[1] != 0xec || original_bytes[2] != 0x20)
        return denied_sopen(log, "original-body");
    if (!WriteProcessMemory(child, (void *)sopen_start, original_bytes, length, &bytes) || bytes != length ||
        !WriteProcessMemory(child, (void *)words[0], &trap, 1, &bytes) || bytes != 1)
        return denied_sopen(log, "restore");
    if (!WriteProcessMemory(child, (void *)0x2df3c, &trap, 1, &bytes) || bytes != 1)
        return denied_sopen(log, "create-probe");
    FlushInstructionCache(child, 0, 0);
    sopen_return = words[0];
    sopen_step = sopen_start+3;
    sopen_saved_tf = context.EFlags & 0x100;
    context.Eip = sopen_start;
    context.EFlags |= 0x100;
    if (!SetThreadContext(thread, &context)) return denied_sopen(log, "context");
    sprintf(line, "crt-restored %08lx %08lx\n", sopen_start, sopen_end);
    WriteFile(log, line, strlen(line), &written, 0);
    return 1;
}

static int rearm_sopen(HANDLE child, HANDLE log, HANDLE thread, DWORD address)
{
    CONTEXT context;
    DWORD bytes, written;
    char line[128];
    unsigned char trap = 0xcc;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_CONTROL;
    if (!sopen_step || address != sopen_step || !GetThreadContext(thread, &context) ||
        context.Eip != sopen_step) return denied_sopen(log, "unexpected-step");
    if (!WriteProcessMemory(child, (void *)sopen_start, &trap, 1, &bytes) || bytes != 1)
        return denied_sopen(log, "rearm");
    FlushInstructionCache(child, (void *)sopen_start, 1);
    context.EFlags = (context.EFlags & ~0x100UL) | sopen_saved_tf;
    if (!SetThreadContext(thread, &context)) return denied_sopen(log, "step-context");
    sprintf(line, "crt-rearm %08lx %08lx\n", sopen_start, sopen_step);
    WriteFile(log, line, strlen(line), &written, 0);
    sopen_step = 0;
    return 1;
}

static int begin_sopen_create(HANDLE child, HANDLE log, HANDLE thread)
{
    CONTEXT context;
    DWORD args[7], security[3], bytes, written, i;
    char name[260], line[1024];
    unsigned char call = 0xe8;
    int used;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!sopen_return || sopen_step || sopen_create_step || sopen_create_seen ||
        !GetThreadContext(thread, &context) || (context.EFlags & 0x100) ||
        !ReadProcessMemory(child, (void *)context.Esp, args, sizeof(args), &bytes) || bytes != sizeof(args))
        return denied_sopen(log, "create-state");
    for (i = 0; i < sizeof(name); ++i) {
        if (!ReadProcessMemory(child, (void *)(args[0]+i), name+i, 1, &bytes) || bytes != 1)
            return denied_sopen(log, "create-name");
        if (!name[i]) break;
    }
    if (i == sizeof(name) || strcmp(name, sopen_access == GENERIC_READ ? sopen_pdb_name : sopen_name) ||
        args[1] != sopen_access || args[2] != 3 || args[4] != OPEN_EXISTING ||
        args[5] != FILE_ATTRIBUTE_NORMAL || args[6] ||
        !ReadProcessMemory(child, (void *)args[3], security, 12, &bytes) || bytes != 12 ||
        security[0] != 12 || security[1] || security[2] != TRUE)
        return denied_sopen(log, "create-arguments");
    used = sprintf(line, "crt-createfile %08lx access %08lx share %lu disposition %lu attrs %lu inherit %lu name ",
        sopen_start, args[1], args[2], args[4], args[5], security[2]);
    for (i = 0; name[i]; ++i) used += sprintf(line+used, "%02x", (unsigned char)name[i]);
    line[used++] = '\n';
    WriteFile(log, line, used, &written, 0);
    if (!WriteProcessMemory(child, (void *)0x2df3c, &call, 1, &bytes) || bytes != 1)
        return denied_sopen(log, "create-restore");
    FlushInstructionCache(child, (void *)0x2df3c, 1);
    sopen_create_seen = 1;
    sopen_create_step = 0x2be52;
    sopen_create_tf = context.EFlags & 0x100;
    context.Eip = 0x2df3c;
    context.EFlags |= 0x100;
    return SetThreadContext(thread, &context) ? 1 : denied_sopen(log, "create-context");
}

static int rearm_sopen_create(HANDLE child, HANDLE log, HANDLE thread, DWORD address)
{
    CONTEXT context;
    DWORD bytes, written;
    unsigned char trap = 0xcc;
    char line[128];
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_CONTROL;
    if (address != sopen_create_step || !GetThreadContext(thread, &context) || context.Eip != sopen_create_step ||
        !WriteProcessMemory(child, (void *)0x2df3c, &trap, 1, &bytes) || bytes != 1)
        return denied_sopen(log, "create-rearm");
    FlushInstructionCache(child, (void *)0x2df3c, 1);
    context.EFlags = (context.EFlags & ~0x100UL)|sopen_create_tf;
    if (!SetThreadContext(thread, &context)) return denied_sopen(log, "create-rearm-context");
    sprintf(line, "crt-createfile-rearm %08lx 0002be52\n", sopen_start);
    WriteFile(log, line, strlen(line), &written, 0);
    sopen_create_step = 0;
    return 1;
}

static int finish_sopen(HANDLE child, HANDLE log, HANDLE thread)
{
    CONTEXT context;
    DWORD bytes, written, fd, error, flags, handle, type, position, last_error;
    HANDLE duplicate, expected;
    BY_HANDLE_FILE_INFORMATION actual_info, expected_info;
    char line[256];
    int owned, input_matches;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (sopen_step || sopen_create_step || sopen_create_seen != 1 || !GetThreadContext(thread, &context)) return denied_sopen(log, "return-context");
    fd = context.Eax;
    if (fd >= 64 || !ReadProcessMemory(child, (void *)0x39614, &error, 4, &bytes) || bytes != 4)
        return denied_sopen(log, "unsuccessful-return");
    if (!child_teb || !ReadProcessMemory(child, (void *)(child_teb+0x34), &last_error, 4, &bytes) || bytes != 4)
        return denied_sopen(log, "last-error-state");
    flags = 0;
    if (!ReadProcessMemory(child, (void *)(0x39700+fd), &flags, 1, &bytes) || bytes != 1 ||
        !ReadProcessMemory(child, (void *)(0x39740+fd*4), &handle, 4, &bytes) || bytes != 4 ||
        !DuplicateHandle(child, (HANDLE)handle, GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS))
        return denied_sopen(log, "return-handle");
    type = GetFileType(duplicate);
    SetLastError(0);
    position = SetFilePointer(duplicate, 0, 0, FILE_CURRENT);
    if (position == 0xffffffff && GetLastError()) { CloseHandle(duplicate); return denied_sopen(log, "position"); }
    expected = CreateFileA(sopen_active_input, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    owned = expected != INVALID_HANDLE_VALUE && GetFileInformationByHandle(duplicate, &actual_info) &&
        GetFileInformationByHandle(expected, &expected_info) &&
        actual_info.dwVolumeSerialNumber == expected_info.dwVolumeSerialNumber &&
        actual_info.nFileIndexHigh == expected_info.nFileIndexHigh &&
        actual_info.nFileIndexLow == expected_info.nFileIndexLow;
    if (expected != INVALID_HANDLE_VALUE) CloseHandle(expected);
    CloseHandle(duplicate);
    input_matches = identical_files(sopen_active_input, sopen_active_master);
    last_sopen_fd = fd;
    sprintf(line, "crt-return %08lx fd %lu errno %lu osfile %02lx type %lu position %lu owned %u input %u last_error %lu\n",
        sopen_start, fd, error, flags, type, position, owned, input_matches, last_error);
    WriteFile(log, line, strlen(line), &written, 0);
    if (flags != 1 || type != FILE_TYPE_DISK || position != 0 || !owned ||
        !input_matches) return denied_sopen(log, "return-state");
    if (!WriteProcessMemory(child, (void *)sopen_return, &sopen_return_byte, 1, &bytes) || bytes != 1)
        return denied_sopen(log, "return-restore");
    FlushInstructionCache(child, (void *)sopen_return, 1);
    context.Eip = sopen_return;
    sopen_return = 0;
    return SetThreadContext(thread, &context) ? 1 : denied_sopen(log, "return-resume");
}

static int dump_file_descriptor(HANDLE child, HANDLE log, DWORD root, DWORD index)
{
    DWORD fd, flags = 0, handle, bytes, written, type = 0, position = 0;
    HANDLE duplicate, expected;
    BY_HANDLE_FILE_INFORMATION actual_info, expected_info;
    char line[256];
    int owned = 0;
    memcpy(&fd, original_bytes+16, 4);
    /* Current case has all three standard descriptors open. A zero source
       word before _open is not yet an allocated FileSlot descriptor. */
    if (fd < 3 || fd == 0xffffffff) return 1;
    if (fd >= 64 || !ReadProcessMemory(child, (void *)(0x39700+fd), &flags, 1, &bytes) || bytes != 1)
        return 0;
    if (flags & 1) {
        if (!ReadProcessMemory(child, (void *)(0x39740+fd*4), &handle, 4, &bytes) || bytes != 4 ||
            !DuplicateHandle(child, (HANDLE)handle, GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) return 0;
        type = GetFileType(duplicate);
        SetLastError(0);
        position = SetFilePointer(duplicate, 0, 0, FILE_CURRENT);
        if (position == 0xffffffff && GetLastError()) { CloseHandle(duplicate); return 0; }
        expected = CreateFileA(sopen_input, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        owned = expected != INVALID_HANDLE_VALUE && GetFileInformationByHandle(duplicate, &actual_info) &&
            GetFileInformationByHandle(expected, &expected_info) &&
            actual_info.dwVolumeSerialNumber == expected_info.dwVolumeSerialNumber &&
            actual_info.nFileIndexHigh == expected_info.nFileIndexHigh &&
            actual_info.nFileIndexLow == expected_info.nFileIndexLow;
        if (expected != INVALID_HANDLE_VALUE) CloseHandle(expected);
        CloseHandle(duplicate);
        if (!owned || type != FILE_TYPE_DISK) return 0;
    }
    sprintf(line, "node-fd %08lx %lu 16 %lu %02lx %lu %lu %u\n", root, index, fd, flags, type, position, owned);
    return WriteFile(log, line, strlen(line), &written, 0) && written == strlen(line);
}

static int load_plan(char *path)
{
    FILE *plan;
    RuntimeRange *range;
    plan = fopen(path, "r");
    if (!plan) return 0;
    while (range_count < sizeof(ranges)/sizeof(ranges[0])) {
        range = ranges + range_count;
        if (fscanf(plan, "%lx %lx %lx", &range->start, &range->end, &range->offset) != 3)
            break;
        if (range->start >= range->end || range->end-range->start > sizeof(original_bytes)) {
            fclose(plan);
            return 0;
        }
        ++range_count;
    }
    if (!feof(plan)) { fclose(plan); return 0; }
    fclose(plan);
    return 1;
}

typedef struct ReadSite { DWORD call, returned; } ReadSite;
static ReadSite read_sites[32];
static DWORD symbol_base,symbol_size,symbol_input_base,symbol_module,symbol_entry;
static DWORD symbol_padding,symbol_skipped,symbol_step,symbol_tf,symbol_thread;
static unsigned char symbol_before[8192];
static DWORD type_page_base,type_page_step,type_page_tf,type_page_thread;
static int buffer_enabled;
static unsigned char type_page_before[2048];
static DWORD pdb_read_enabled, pdb_read_step, pdb_read_tf, pdb_read_site;
static DWORD read_site_count, read_step_site, read_step_address, read_saved_tf;
static DWORD read_active_site, read_address, read_size, read_handle;
static unsigned char read_step_byte, read_before[8192];

static int restore_runtime(HANDLE child, HANDLE original, HANDLE log,
                           HANDLE thread, DWORD address)
{
    unsigned i;
    DWORD bytes, length, written, j;
    unsigned char trap = 0xcc;
    CONTEXT context;
    char line[128];
    for (i = 0; i < range_count; ++i) {
        if (ranges[i].start != address) continue;
        length = ranges[i].end-ranges[i].start;
        if (SetFilePointer(original, ranges[i].offset, 0, FILE_BEGIN) == 0xffffffff ||
            !ReadFile(original, original_bytes, length, &bytes, 0) || bytes != length ||
            !WriteProcessMemory(child, (void *)address, original_bytes, length, &bytes) ||
            bytes != length) return -1;
        for (j = 0; j < read_site_count; ++j) {
            if (address <= read_sites[j].call && read_sites[j].returned < address+length) {
                if (!WriteProcessMemory(child, (void *)read_sites[j].call, &trap, 1, &bytes) || bytes != 1 ||
                    !WriteProcessMemory(child, (void *)read_sites[j].returned, &trap, 1, &bytes) || bytes != 1) return -1;
            }
        }
        if (pdb_read_enabled) {
            DWORD sites[3]; sites[0]=0x2af51;sites[1]=0x2b1a7;sites[2]=0x2b299;
            for (j=0;j<3;++j) if (address<=sites[j] && sites[j]<address+length)
                if (!WriteProcessMemory(child,(void *)sites[j],&trap,1,&bytes) || bytes!=1) return -1;
        }
        if (read_site_count && address<=0x2504e && 0x2504e<address+length)
            if (!WriteProcessMemory(child,(void *)0x2504e,&trap,1,&bytes) || bytes!=1) return -1;
        if (read_site_count && address<=0x199d7 && 0x199d7<address+length)
            if (!WriteProcessMemory(child,(void *)0x199d7,&trap,1,&bytes) || bytes!=1) return -1;
        if (buffer_enabled) {
            DWORD sites[2];sites[0]=0x22f6d;sites[1]=0x19a65;
            for(j=0;j<2;++j)if(address<=sites[j] && sites[j]<address+length)
                if(!WriteProcessMemory(child,(void *)sites[j],&trap,1,&bytes) || bytes!=1)return -1;
        }
        FlushInstructionCache(child, (void *)address, length);
        memset(&context, 0, sizeof(context));
        context.ContextFlags = CONTEXT_CONTROL;
        if (!GetThreadContext(thread, &context)) return -1;
        context.Eip = address;
        if (!SetThreadContext(thread, &context)) return -1;
        sprintf(line, "runtime %08lx %08lx\n", ranges[i].start, ranges[i].end);
        WriteFile(log, line, strlen(line), &written, 0);
        return 1;
    }
    return 0;
}

static void dump_arguments(HANDLE child, HANDLE log, DWORD *words, DWORD word_count)
{
    DWORD i, j, pointer, argument, bytes, written;
    unsigned char value;
    char line[2048];
    int length;
    if (word_count < 3 || words[1] > 64) return;
    for (i = 0; i < words[1]; ++i) {
        pointer = words[2] + 4*i;
        if (!ReadProcessMemory(child, (void *)pointer, &argument, 4, &bytes) || bytes != 4)
            return;
        length = sprintf(line, "argv %lu ", i);
        for (j = 0; j < 512; ++j) {
            if (!ReadProcessMemory(child, (void *)(argument+j), &value, 1, &bytes) || bytes != 1)
                return;
            if (!value) break;
            length += sprintf(line+length, "%02x", value);
        }
        line[length++] = '\n';
        WriteFile(log, line, length, &written, 0);
    }
}

static void dump_inspections(HANDLE child, HANDLE log)
{
    char specifications[4096], *cursor, *next;
    char line[160];
    DWORD address, length, count, i, j, written;
    int used;
    if (!GetEnvironmentVariableA("CVPACK_CAPTURE_INSPECT", specifications, sizeof(specifications)))
        return;
    cursor = specifications;
    while (*cursor) {
        address = strtoul(cursor, &next, 16);
        if (next == cursor || *next != ':') return;
        cursor = next+1;
        length = strtoul(cursor, &next, 10);
        if (next == cursor || length > sizeof(original_bytes)) return;
        if (!ReadProcessMemory(child, (void *)address, original_bytes, length, &count) ||
            count != length) return;
        for (i = 0; i < length; i += 32) {
            used = sprintf(line, "memory %08lx ", address+i);
            for (j = i; j < length && j < i+32; ++j)
                used += sprintf(line+used, "%02x", original_bytes[j]);
            line[used++] = '\n';
            WriteFile(log, line, used, &written, 0);
        }
        if (*next && *next != ',') return;
        cursor = *next ? next+1 : next;
    }
}

/* Typed header reads observed before and immediately after the original call. */

static int read_state(HANDLE child, DWORD handle, DWORD *position, DWORD *fd, DWORD *flags, DWORD *host_position)
{
    DWORD table, slot, page, raw_handle, bytes;
    HANDLE duplicate;
    if (handle >= 64 || !ReadProcessMemory(child, (void *)0x33000, &table, 4, &bytes) || bytes != 4 ||
        !ReadProcessMemory(child, (void *)(table+handle*4), &slot, 4, &bytes) || bytes != 4 ||
        !ReadProcessMemory(child, (void *)(slot+4), flags, 4, &bytes) || bytes != 4 || (*flags & 8) ||
        !ReadProcessMemory(child, (void *)(slot+16), fd, 4, &bytes) || bytes != 4 || *fd >= 64 ||
        !ReadProcessMemory(child, (void *)(slot+32), &page, 4, &bytes) || bytes != 4 || !page ||
        !ReadProcessMemory(child, (void *)(page+12), position, 4, &bytes) || bytes != 4 ||
        !ReadProcessMemory(child, (void *)(0x39740+*fd*4), &raw_handle, 4, &bytes) || bytes != 4)
        return 0;
    *flags = 0;
    if (!ReadProcessMemory(child, (void *)(0x39700+*fd), flags, 1, &bytes) || bytes != 1 ||
        !DuplicateHandle(child, (HANDLE)raw_handle, GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) return 0;
    SetLastError(0);
    *host_position = SetFilePointer(duplicate, 0, 0, FILE_CURRENT);
    CloseHandle(duplicate);
    return *host_position != 0xffffffff || !GetLastError();
}

static int observe_read(HANDLE child, HANDLE thread, HANDLE log, DWORD address)
{
    CONTEXT context;
    DWORD i, words[3], bytes, written, error, position, fd, flags, host_position;
    unsigned char byte, expected[5];
    int before, used;
    for (i = 0; i < read_site_count; ++i) {
        before = read_sites[i].call == address;
        if (!before && read_sites[i].returned != address) continue;
        if (read_step_site) return -1;
        memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(thread, &context) || (context.EFlags & 0x100) ||
            !ReadProcessMemory(child, (void *)context.Esp, words, sizeof(words), &bytes) || bytes != sizeof(words) ||
            !ReadProcessMemory(child, (void *)0x39614, &error, 4, &bytes) || bytes != 4) return -1;
        if (before) {
            if (read_active_site || words[2] > sizeof(read_before) || !words[2]) return -1;
            read_active_site = address; read_handle = words[0]; read_address = words[1]; read_size = words[2];
            if (!ReadProcessMemory(child, (void *)read_address, read_before, read_size, &bytes) || bytes != read_size ||
                !read_state(child, read_handle, &position, &fd, &flags, &host_position)) return -1;
            used = sprintf(object_line, "read-entry %08lx %lu %08lx %lu %lu %ld %lu %02lx %lu ",
                address, read_handle, read_address, read_size, error, (long)position, fd, flags, host_position);
            for (i = 0; i < read_size; ++i) used += sprintf(object_line+used, "%02x", read_before[i]);
            byte = 0xe8;
            read_step_address = 0x2a400; /* CALL's single-step arrives at read_file. */
        } else {
            if (read_active_site != read_sites[i].call || words[0] != read_handle || words[1] != read_address || words[2] != read_size ||
                (long)context.Eax < -1 || (long)context.Eax > (long)read_size ||
                !ReadProcessMemory(child, (void *)read_address, array_bytes, read_size, &bytes) || bytes != read_size ||
                !read_state(child, read_handle, &position, &fd, &flags, &host_position)) return -1;
            used = sprintf(object_line, "read-return %08lx %ld %lu %ld %lu %02lx %lu ",
                read_active_site, (long)context.Eax, error, (long)position, fd, flags, host_position);
            for (i = 0; i < read_size; ++i) used += sprintf(object_line+used, "%02x", array_bytes[i]);
            read_active_site = 0;
            byte = 0x83; read_step_address = address+3;
        }
        object_line[used++] = '\n';
        if (!WriteFile(log, object_line, used, &written, 0) || written != (DWORD)used) return -1;
        if(before && address==0x17dc9 && read_size==4746) {
            DWORD table,slot,page,page_flags,page_position;
            if(!ReadProcessMemory(child,(void *)0x33000,&table,4,&bytes) || bytes!=4 ||
               !ReadProcessMemory(child,(void *)(table+4*read_handle),&slot,4,&bytes) || bytes!=4 ||
               !ReadProcessMemory(child,(void *)(slot+32),&page,4,&bytes) || bytes!=4 ||
               !ReadProcessMemory(child,(void *)(page+20),&page_flags,4,&bytes) || bytes!=4 ||
               !ReadProcessMemory(child,(void *)(page+12),&page_position,4,&bytes) || bytes!=4) return -1;
            used=sprintf(object_line,"read-page-entry site %08lx slot %08lx page %08lx flags %lu position %lu address %08lx\n",address,slot,page,page_flags,page_position,read_address);
            if(!WriteFile(log,object_line,used,&written,0) || written!=(DWORD)used) return -1;
        }
        if (
            !ReadProcessMemory(child, (void *)(address+1), expected, before ? 4 : 2, &bytes) || bytes != (before ? 4 : 2) ||
            (!before && (expected[0] != 0xc4 || expected[1] != 0x0c)) ||
            !WriteProcessMemory(child, (void *)address, &byte, 1, &bytes) || bytes != 1) return -1;
        FlushInstructionCache(child, (void *)address, 1);
        read_step_site = address; read_step_byte = byte; read_saved_tf = context.EFlags & 0x100;
        context.Eip = address; context.EFlags |= 0x100;
        return SetThreadContext(thread, &context) ? 1 : -1;
    }
    return 0;
}

/* The reached public read is suspended after its first page copy.  Retain
   actual loop registers/ABI words and the full pending buffer, without returning
   from the read or initializing its untouched tail. */
static int dump_read_progress(HANDLE child,HANDLE log,DWORD entry,CONTEXT *context,DWORD *words)
{
    DWORD bytes,written,position,fd,flags,host_position,module,module_number=0;
    int used;DWORD i,shift=0,inner_return=0,page,requested,copied,returned;CONTEXT parent;
    if (!read_active_site) return 1;
    if(read_active_site==0x17dc9) {requested=4746;copied=3076;returned=0x17dce;}
    else if(read_active_site==0x17e69) {requested=209;copied=197;returned=0x17e6e;}
    else return 0;
    if (entry==0x2a120 || entry==0x2a850) {
        if((entry==0x2a120 && (read_active_site!=0x17e69 || words[0]!=0x2a606)) ||
           (entry==0x2a850 && (read_active_site!=0x17dc9 || words[0]!=0x2a5ef)) || !ReadProcessMemory(child,(void *)(words[6]+32),&page,4,&bytes) || bytes!=4 || page!=words[1]) return 0;
        inner_return=words[0];shift=5;parent=*context;
        parent.Edi=words[2];parent.Esi=words[3];parent.Ebx=words[4];context=&parent;words+=5;
    } else if(entry!=0x2a5c0) return 0;
    if (read_size!=requested || context->Ebp!=copied ||
        words[0]!=0x2a3dd || words[1]!=context->Ebx || words[3]!=requested || words[8]!=words[1] ||
        words[9]!=read_address || words[10]!=requested || words[11]!=returned ||
        words[12]!=read_handle || words[13]!=read_address || words[14]!=requested ||
        context->Edi!=read_address+copied ||
        !ReadProcessMemory(child,(void *)0x33050,&module,4,&bytes) || bytes!=4 ||
        !ReadProcessMemory(child,(void *)module,&module_number,2,&bytes) || bytes!=2 || module_number!=3 ||
        !ReadProcessMemory(child,(void *)read_address,array_bytes,read_size,&bytes) || bytes!=read_size ||
        !read_state(child,read_handle,&position,&fd,&flags,&host_position)) return 0;
    used=sprintf(object_line,"read-progress site %08lx address %08lx count %lu copied %lu remaining %lu counter %lu position %lu slot %08lx caller %08lx readreturn %08lx destination %08lx cursor %08lx module %lu frontier %08lx innerreturn %08lx shift %lu bytes ",
        read_active_site,read_address,read_size,context->Ebp,words[3]-context->Ebp,words[3],position,words[1],words[0],words[11],words[9],context->Esi,module_number,entry,inner_return,shift);
    for(i=0;i<read_size;++i)used+=sprintf(object_line+used,"%02x",array_bytes[i]);
    object_line[used++]='\n';
    return WriteFile(log,object_line,used,&written,0) && written==(DWORD)used;
}

static int rearm_read(HANDLE child, HANDLE thread, HANDLE log, DWORD address)
{
    CONTEXT context;
    DWORD bytes, written;
    unsigned char trap = 0xcc;
    char line[160];
    memset(&context, 0, sizeof(context));context.ContextFlags = CONTEXT_CONTROL;
    if (!read_step_site || address != read_step_address || !GetThreadContext(thread, &context) ||
        context.Eip != read_step_address || !WriteProcessMemory(child, (void *)read_step_site, &trap, 1, &bytes) || bytes != 1) return -1;
    FlushInstructionCache(child, (void *)read_step_site, 1);
    context.EFlags = (context.EFlags & ~0x100UL) | read_saved_tf;
    if (!SetThreadContext(thread, &context)) return -1;
    sprintf(line, "read-rearm %08lx %08lx\n", read_step_site, read_step_address);
    WriteFile(log, line, strlen(line), &written, 0);
    read_step_site = read_step_address = 0;
    return 1;
}

static int observe_pdb_header(HANDLE child, HANDLE log, HANDLE thread)
{
    CONTEXT context;
    DWORD args[3], bytes, written, error, last_error, flags=0, handle, position, type;
    HANDLE duplicate, expected;
    BY_HANDLE_FILE_INFORMATION actual_info, expected_info;
    unsigned char raw[4140], instruction = 0x83;
    DWORD size,site,parent,block[3];
    char line[9000];
    int i, used, owned, input_matches;
    memset(&context, 0, sizeof(context));context.ContextFlags = CONTEXT_FULL;
    if (pdb_read_step || !GetThreadContext(thread, &context) || (context.EFlags & 0x100) ||
        !ReadProcessMemory(child, (void *)context.Esp, args, sizeof(args), &bytes) || bytes != sizeof(args) ||
        args[0] != last_sopen_fd || sopen_access != GENERIC_READ) return 0;
    site=context.Eip-1; size=site==0x2af51?64:4140;
    if (site!=0x2af51 && site!=0x2b299) return 0;
    if (context.Eax!=size || args[2]!=size ||
        (site==0x2af51 && args[1]!=context.Ebx)) return 0;
    if (site==0x2b299 && (!ReadProcessMemory(child,(void *)(context.Ebp+8),&parent,4,&bytes) || bytes!=4 ||
        !ReadProcessMemory(child,(void *)(parent+76),&block[0],4,&bytes) || bytes!=4 ||
        !ReadProcessMemory(child,(void *)block[0],block,12,&bytes) || bytes!=12 ||
        args[1]!=context.Esi || args[1]+size!=block[1] || block[0]!=12288-size || block[2])) return 0;
    if (!ReadProcessMemory(child, (void *)args[1], raw, size, &bytes) || bytes != size ||
        !ReadProcessMemory(child, (void *)0x39614, &error, 4, &bytes) || bytes != 4 ||
        !ReadProcessMemory(child, (void *)(child_teb+0x34), &last_error, 4, &bytes) || bytes != 4 ||
        !ReadProcessMemory(child, (void *)(0x39700+args[0]), &flags, 1, &bytes) || bytes != 1 ||
        !ReadProcessMemory(child, (void *)(0x39740+args[0]*4), &handle, 4, &bytes) || bytes != 4 ||
        !DuplicateHandle(child, (HANDLE)handle, GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) return 0;
    type = GetFileType(duplicate);
    position = SetFilePointer(duplicate, 0, 0, FILE_CURRENT);
    expected = CreateFileA(sopen_pdb_name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    owned = expected != INVALID_HANDLE_VALUE && GetFileInformationByHandle(duplicate, &actual_info) &&
        GetFileInformationByHandle(expected, &expected_info) && actual_info.dwVolumeSerialNumber == expected_info.dwVolumeSerialNumber &&
        actual_info.nFileIndexHigh == expected_info.nFileIndexHigh && actual_info.nFileIndexLow == expected_info.nFileIndexLow;
    if (expected != INVALID_HANDLE_VALUE) CloseHandle(expected);
    CloseHandle(duplicate);
    input_matches = identical_files(sopen_pdb_name, sopen_pdb_master);
    used = sprintf(line, "pdb-read fd %lu count %lu errno %lu last_error %lu position %lu osfile %lu type %lu owned %u input %u buffer %08lx header ",
        args[0], context.Eax, error, last_error, position, flags, type, owned, input_matches, args[1]);
    for (i = 0; i < (int)size; ++i) used += sprintf(line+used, "%02x", raw[i]);
    line[used++] = '\n';WriteFile(log, line, used, &written, 0);
    if (flags != 1 || type != FILE_TYPE_DISK || position != (size==64?64:4204) || !owned || !input_matches ||
        !WriteProcessMemory(child, (void *)site, &instruction, 1, &bytes) || bytes != 1) return 0;
    FlushInstructionCache(child, (void *)site, 1);
    pdb_read_site=site; pdb_read_step = site+3;pdb_read_tf = context.EFlags&0x100;
    context.Eip = site;context.EFlags |= 0x100;
    return SetThreadContext(thread, &context);
}

/* Observe the real uninitialized incremental member immediately before CMP.
   Retain its value; this observer never writes the object or supplies a default. */
static int observe_pdb_incremental(HANDLE child,HANDLE log,HANDLE thread)
{
    CONTEXT context; DWORD parent,value,bytes,written;unsigned char instruction=0x83;char line[160];
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if (pdb_read_step || !GetThreadContext(thread,&context) || (context.EFlags&0x100) || context.Eip!=0x2b1a8 ||
        !ReadProcessMemory(child,(void *)(context.Ebp+8),&parent,4,&bytes) || bytes!=4 || parent!=context.Ecx ||
        !ReadProcessMemory(child,(void *)(parent+108),&value,4,&bytes) || bytes!=4) return 0;
    sprintf(line,"pdb-consumed address %08lx value %lu site 0002b1a7\n",parent,value);
    if (!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line) ||
        !WriteProcessMemory(child,(void *)0x2b1a7,&instruction,1,&bytes) || bytes!=1) return 0;
    FlushInstructionCache(child,(void *)0x2b1a7,1);
    pdb_read_site=0x2b1a7;pdb_read_step=0x2b1ab;pdb_read_tf=context.EFlags&0x100;
    context.Eip=0x2b1a7;context.EFlags|=0x100;
    return SetThreadContext(thread,&context);
}

static int rearm_pdb_header(HANDLE child, HANDLE log, HANDLE thread, DWORD address)
{
    CONTEXT context;DWORD bytes;unsigned char trap=0xcc;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_CONTROL;
    if (address!=pdb_read_step || !GetThreadContext(thread,&context) || context.Eip!=pdb_read_step ||
        !WriteProcessMemory(child,(void *)pdb_read_site,&trap,1,&bytes) || bytes!=1) return 0;
    FlushInstructionCache(child,(void *)pdb_read_site,1);
    context.EFlags=(context.EFlags&~0x100UL)|pdb_read_tf;
    pdb_read_step=0;
    return SetThreadContext(thread,&context);
}

static int observe_symbol_allocation(HANDLE child,HANDLE log,HANDLE thread,DWORD tid)
{
    CONTEXT context;DWORD words[13],bytes,written,i,header[3],number;int used;
    unsigned char instruction=0x89;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if(symbol_base || symbol_step || !GetThreadContext(thread,&context) || context.Eip!=0x2504f ||
       !ReadProcessMemory(child,(void *)context.Esp,words,sizeof(words),&bytes) || bytes!=sizeof(words) ||
       !ReadProcessMemory(child,(void *)words[9],header,12,&bytes) || bytes!=12 ||
       !ReadProcessMemory(child,(void *)words[10],&number,2,&bytes) || bytes!=2) return 0;
    symbol_size=words[0];symbol_base=context.Eax;symbol_input_base=words[8];symbol_entry=words[9];symbol_module=words[10];
    if(!ReadProcessMemory(child,(void *)words[11],&symbol_padding,2,&bytes) || bytes!=2 ||
       !ReadProcessMemory(child,(void *)words[12],&symbol_skipped,2,&bytes) || bytes!=2) return 0;
    symbol_padding&=0xffff;symbol_skipped&=0xffff;number&=0xffff;
    if(!symbol_base || symbol_size!=5113 || header[0]!=0x30124 || header[2]!=4746 || number!=3 || symbol_padding!=367 || symbol_skipped ||
       symbol_size!=header[2]+symbol_padding-symbol_skipped ||
       !ReadProcessMemory(child,(void *)symbol_base,symbol_before,symbol_size,&bytes) || bytes!=symbol_size) return 0;
    used=sprintf(object_line,"symbol-allocation base %08lx size %lu input %08lx entry %08lx module %08lx number %lu padding %lu skipped %lu esp %08lx before ",
         symbol_base,symbol_size,symbol_input_base,symbol_entry,symbol_module,number,symbol_padding,symbol_skipped,context.Esp);
    for(i=0;i<symbol_size;++i)used+=sprintf(object_line+used,"%02x",symbol_before[i]);
    object_line[used++]='\n';
    if(!WriteFile(log,object_line,used,&written,0) || written!=(DWORD)used ||
       !WriteProcessMemory(child,(void *)0x2504e,&instruction,1,&bytes) || bytes!=1) return 0;
    FlushInstructionCache(child,(void *)0x2504e,1);
    symbol_step=0x25052;symbol_tf=context.EFlags&0x100;symbol_thread=tid;context.Eip=0x2504e;context.EFlags|=0x100;
    return SetThreadContext(thread,&context);
}

static int rearm_symbol_allocation(HANDLE child,HANDLE log,HANDLE thread,DWORD address,DWORD tid)
{
    CONTEXT context;DWORD bytes,written;unsigned char trap=0xcc;
    char line[]="symbol-allocation-rearm 0002504e 00025052\n";
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_CONTROL;
    if(address!=symbol_step || tid!=symbol_thread || !GetThreadContext(thread,&context) || context.Eip!=symbol_step ||
       !WriteProcessMemory(child,(void *)0x2504e,&trap,1,&bytes) || bytes!=1) return 0;
    FlushInstructionCache(child,(void *)0x2504e,1);
    if(!WriteFile(log,line,sizeof(line)-1,&written,0) || written!=sizeof(line)-1) return 0;
    context.EFlags=(context.EFlags&~0x100UL)|symbol_tf;symbol_step=0;
    return SetThreadContext(thread,&context);
}

static int dump_symbol_allocation(HANDLE child,HANDLE log,DWORD frontier)
{
    DWORD cursor,input,defined,offset,bytes,written,i;int used;
    if(!symbol_base)return 1;
    if(frontier==0x1bee0) { defined=4;offset=4; }
    else if(frontier==0x255c0) { defined=16;offset=4; }
    else if(frontier==0x25800 || frontier==0x1b500) { defined=40;offset=28; }
    else if(frontier==0x26520 || frontier==0x27100 || frontier==0x272a0 || frontier==0x19580 || frontier==0x1c1a0 || frontier==0x19860 || frontier==0x22f40 || frontier==0x262d0) { defined=40;offset=202; }
    else if(frontier==0x1ad40 || frontier==0x1af10 || frontier==0x1ae40 || frontier==0x19770) { defined=40;offset=384; }
    else if(frontier==0x256f0) { defined=40;offset=966; }
    else if(frontier==0x1e700) { defined=40;offset=1321; }
    else if(frontier==0x27b10) { defined=40;offset=2227; }
    else if(frontier==0x1ac80 || frontier==0x1c640) { defined=40;offset=2227; }
    else return 0;
    if(symbol_step || symbol_size!=5113 ||
       !ReadProcessMemory(child,(void *)0x335e4,&input,4,&bytes) || bytes!=4 || input!=symbol_input_base+offset ||
       !ReadProcessMemory(child,(void *)0x335f0,&cursor,4,&bytes) || bytes!=4 || cursor!=symbol_base+defined ||
       !ReadProcessMemory(child,(void *)symbol_base,array_bytes,symbol_size,&bytes) || bytes!=symbol_size) return 0;
    used=sprintf(object_line,"symbol-output base %08lx size %lu cursor %08lx frontier %08lx bytes ",symbol_base,symbol_size,cursor,frontier);
    for(i=0;i<symbol_size;++i)used+=sprintf(object_line+used,"%02x",array_bytes[i]);
    object_line[used++]='\n';
    return WriteFile(log,object_line,used,&written,0) && written==(DWORD)used;
}

static int observe_type_page(HANDLE child,HANDLE log,HANDLE thread,DWORD tid)
{
    CONTEXT context;DWORD size,index=0,capacity=0,bytes,written,i,roots[120];int used;
    unsigned char instruction=0x83;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if(type_page_base || type_page_step || !GetThreadContext(thread,&context) || context.Eip!=0x199d8 ||
       !ReadProcessMemory(child,(void *)context.Esp,&size,4,&bytes) || bytes!=4 || size!=2048 ||
       !ReadProcessMemory(child,(void *)0x37398,&index,2,&bytes) || bytes!=2 || index!=0x1000 ||
       !ReadProcessMemory(child,(void *)0x33924,&capacity,2,&bytes) || bytes!=2 || capacity!=512 || !context.Eax ||
       !ReadProcessMemory(child,(void *)context.Eax,type_page_before,2048,&bytes) || bytes!=2048) return 0;
    if(!ReadProcessMemory(child,(void *)0x37198,roots,sizeof(roots),&bytes) || bytes!=sizeof(roots))return 0;
    for(i=0;i<120;++i)if(roots[i])return 0;
    type_page_base=context.Eax;
    used=sprintf(object_line,"type-page-allocation base %08lx size %lu index %lu capacity %lu owner %08lx esp %08lx before ",
        type_page_base,size,index,capacity,context.Ebx,context.Esp);
    for(i=0;i<2048;++i)used+=sprintf(object_line+used,"%02x",type_page_before[i]);
    used+=sprintf(object_line+used," roots_before ");
    for(i=0;i<sizeof(roots);++i)used+=sprintf(object_line+used,"%02x",((unsigned char *)roots)[i]);
    object_line[used++]='\n';
    if(!WriteFile(log,object_line,used,&written,0) || written!=(DWORD)used ||
       !WriteProcessMemory(child,(void *)0x199d7,&instruction,1,&bytes) || bytes!=1) return 0;
    FlushInstructionCache(child,(void *)0x199d7,1);
    type_page_step=0x199da;type_page_tf=context.EFlags&0x100;type_page_thread=tid;
    context.Eip=0x199d7;context.EFlags|=0x100;
    return SetThreadContext(thread,&context);
}

static int rearm_type_page(HANDLE child,HANDLE log,HANDLE thread,DWORD address,DWORD tid)
{
    CONTEXT context;DWORD bytes,written;unsigned char trap=0xcc;
    char line[]="type-page-rearm 000199d7 000199da\n";
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_CONTROL;
    if(address!=type_page_step || tid!=type_page_thread || !GetThreadContext(thread,&context) || context.Eip!=type_page_step ||
       !WriteProcessMemory(child,(void *)0x199d7,&trap,1,&bytes) || bytes!=1) return 0;
    FlushInstructionCache(child,(void *)0x199d7,1);
    if(!WriteFile(log,line,sizeof(line)-1,&written,0) || written!=sizeof(line)-1) return 0;
    context.EFlags=(context.EFlags&~0x100UL)|type_page_tf;type_page_step=0;
    return SetThreadContext(thread,&context);
}

static int dump_type_page(HANDLE child,HANDLE log,DWORD frontier)
{
    DWORD roots[120],bytes,written,i;int used;
    if(!type_page_base)return 1;
    if(type_page_step || (frontier!=0x22f40 && frontier!=0x262d0 && frontier!=0x1ad40 && frontier!=0x1af10 && frontier!=0x1ae40 && frontier!=0x19770 && frontier!=0x256f0 && frontier!=0x1e700 && frontier!=0x27b10 && frontier!=0x1ac80 && frontier!=0x1c640) ||
       !ReadProcessMemory(child,(void *)0x37198,roots,sizeof(roots),&bytes) || bytes!=sizeof(roots) || roots[0]!=type_page_base) return 0;
    for(i=1;i<120;++i)if(roots[i])return 0;
    if(!ReadProcessMemory(child,(void *)type_page_base,array_bytes,2048,&bytes) || bytes!=2048)return 0;
    used=sprintf(object_line,"type-page-output base %08lx size 2048 frontier %08lx bytes ",type_page_base,frontier);
    for(i=0;i<2048;++i)used+=sprintf(object_line+used,"%02x",array_bytes[i]);
    object_line[used++]='\n';
    return WriteFile(log,object_line,used,&written,0) && written==(DWORD)used;
}

static int dump_output_buffers(HANDLE child, HANDLE log, DWORD entry, DWORD *words, CONTEXT *context)
{
    char specifications[2048], *cursor;
    DWORD address, slot, size, bytes, written, i, input, pointer;
    int consumed, used;
    if (!GetEnvironmentVariableA("CVPACK_CAPTURE_OUTPUTS", specifications, sizeof(specifications))) return 1;
    cursor = specifications;
    while (*cursor) {
        if (sscanf(cursor, "%lx:%lu:%lu:%lu%n", &address, &slot, &size, &input, &consumed) != 4) return 0;
        if (address == entry) {
            if (slot>15 && slot!=256) return 0;
            pointer=slot==256?context->Edx:words[slot];
            if (input > 1 || !size || size > 1024 ||
                !ReadProcessMemory(child, (void *)pointer, original_bytes, size, &bytes) || bytes != size) return 0;
            used = sprintf(object_line, "%s-buffer %08lx ", input ? "input" : "output", pointer);
            for (i = 0; i < size; ++i) used += sprintf(object_line+used, "%02x", original_bytes[i]);
            object_line[used++] = '\n';
            if (!WriteFile(log, object_line, used, &written, 0) || written != (DWORD)used) return 0;
        }
        cursor += consumed;
        if (*cursor == ',') ++cursor;
        else if (*cursor) return 0;
    }
    return 1;
}

static int dump_transient_children(HANDLE child, HANDLE log, DWORD entry, DWORD *words)
{
    char specifications[1024], *cursor;
    unsigned char raw[12];
    DWORD address, slot, offset, pointer, bytes, written, i;
    int consumed, used;
    if (!GetEnvironmentVariableA("CVPACK_CAPTURE_TRANSIENTS", specifications, sizeof(specifications))) return 1;
    cursor = specifications;
    while (*cursor) {
        if (sscanf(cursor, "%lx:%lu:%lu%n", &address, &slot, &offset, &consumed) != 3) return 0;
        if (address == entry) {
            if (slot > 15 || offset > 112 ||
                !ReadProcessMemory(child, (void *)(words[slot]+offset), &pointer, 4, &bytes) || bytes != 4 ||
                !pointer || !ReadProcessMemory(child, (void *)pointer, raw, 12, &bytes) || bytes != 12) return 0;
            used = sprintf(object_line, "transient-child %08lx %lu %08lx ", words[slot], offset, pointer);
            for (i = 0; i < 12; ++i) used += sprintf(object_line+used, "%02x", raw[i]);
            object_line[used++] = '\n';
            if (!WriteFile(log, object_line, used, &written, 0) || written != (DWORD)used) return 0;
        }
        cursor += consumed;
        if (*cursor == ',') ++cursor;
        else if (*cursor) return 0;
    }
    return 1;
}

static void dump_process_arguments(HANDLE child, HANDLE log)
{
    DWORD arguments[3], bytes;
    arguments[0] = 0;
    if (!ReadProcessMemory(child, (void *)0x39630, arguments+1, 4, &bytes) || bytes != 4 ||
        !ReadProcessMemory(child, (void *)0x39634, arguments+2, 4, &bytes) || bytes != 4)
        return;
    dump_arguments(child, log, arguments, 3);
}

static int dump_pdb_object(HANDLE child, HANDLE log, DWORD root, DWORD index, const char *key, DWORD address, DWORD size)
{
    DWORD bytes, written, i;int used;
    if (!address || !size || size > sizeof(array_bytes) ||
        !ReadProcessMemory(child,(void *)address,array_bytes,size,&bytes) || bytes!=size) return 0;
    used=sprintf(object_line,"node-object %08lx %lu %s %08lx ",root,index,key,address);
    for (i=0;i<size;++i) used+=sprintf(object_line+used,"%02x",array_bytes[i]);
    object_line[used++]='\n';
    return WriteFile(log,object_line,used,&written,0) && written==(DWORD)used;
}

static int dump_pdb_objects(HANDLE child,HANDLE log,DWORD root,DWORD index,DWORD address)
{
    DWORD header[29],first[3],bytes,capacity,count;
    if (!ReadProcessMemory(child,(void *)address,header,116,&bytes) || bytes!=116 || header[15]!=4140) return 0;
    capacity=(header[15]+0x1fff)&~0xfff;count=(header[14]>>16)-0x1000;
    if (capacity!=12288 || count!=136 || !ReadProcessMemory(child,(void *)header[18],first,12,&bytes) || bytes!=12) return 0;
    return dump_pdb_object(child,log,root,index,"offsets",header[16],count*4) &&
        dump_pdb_object(child,log,root,index,"type-storage-0",header[18],12) &&
        dump_pdb_object(child,log,root,index,"type-storage-1",first[2],capacity+12) &&
        dump_pdb_object(child,log,root,index,"index-storage-0",header[21],12);
}

static int dump_type_cache(HANDLE child,HANDLE log,DWORD root,DWORD index)
{
    DWORD bucket,pointer,record[4],bytes,count,total=0,seen[4096],j;
    char key[48];
    for(bucket=0;bucket<512;++bucket) {
        memcpy(&pointer,walk_bytes+bucket*4,4);
        count=0;
        while(pointer) {
            if(total==4096 || count==4096) return 0;
            for(j=0;j<total;++j) if(seen[j]==pointer) return 0;
            seen[total++]=pointer;
            if(!ReadProcessMemory(child,(void *)pointer,record,16,&bytes) || bytes!=16) return 0;
            sprintf(key,"cache-%lu-%lu",bucket,count++);
            if(!dump_pdb_object(child,log,root,index,key,pointer,16)) return 0;
            pointer=record[3];
        }
    }
    return 1;
}

static int dump_walks(HANDLE child, HANDLE log)
{
    char specifications[4096], strings[1024], arrays[2048], arenas[512], *cursor, *field;
    unsigned char text[1024];
    const char *digits = "0123456789abcdef";
    DWORD root, size, next, limit, count_address, pointer, bytes, written, i, j, elements, count_size, count_limit, zero_allocation;
    int consumed, used;
    DWORD string_root, offset, address, length;
    DWORD count_offset, shift, maximum, flags_offset, flags_mask, flags_value, flags, element_size;
    int field_consumed;
    if (!GetEnvironmentVariableA("CVPACK_CAPTURE_WALKS", specifications, sizeof(specifications)))
        return 1;
    cursor = specifications;
    while (*cursor) {
        if (sscanf(cursor, "%lx:%lu:%lu:%lu:%lx:%lu:%lu:%lu%n", &root, &size, &next, &limit,
                   &count_address, &count_size, &count_limit, &zero_allocation, &consumed) != 8) return 0;
        if (count_address) {
            elements = 0;
            if ((count_size != 2 && count_size != 4) ||
                !ReadProcessMemory(child, (void *)count_address, &elements, count_size, &bytes) ||
                bytes != count_size || !count_limit || count_limit > 65536 || elements > count_limit) return 0;
            size *= elements;
        }
        if (!ReadProcessMemory(child, (void *)root, &pointer, 4, &bytes) || bytes != 4) return 0;
        if (GetEnvironmentVariableA("CVPACK_CAPTURE_ARENAS", arenas, sizeof(arenas))) {
            DWORD arena_root, available_address, capacity, available;
            field = arenas;
            while (*field) {
                if (sscanf(field, "%lx:%lx:%lu%n", &arena_root, &available_address,
                    &capacity, &field_consumed) != 3) return 0;
                if (root == arena_root) {
                    if (capacity != 8100 || size != capacity ||
                        !ReadProcessMemory(child, (void *)available_address, &available, 4, &bytes) ||
                        bytes != 4 || available > capacity || (!pointer && available)) return 0;
                    if (pointer) {
                        if (pointer < capacity-available) return 0;
                        pointer -= capacity-available;
                    }
                }
                field += field_consumed;
                if (*field == ',') ++field;
                else if (*field) return 0;
            }
        }
        if ((!size && pointer && !zero_allocation) || zero_allocation > 1 || size > sizeof(walk_bytes) || !limit || limit > 256) return 0;
        for (i = 0; pointer && i < limit; ++i) {
            if (!ReadProcessMemory(child, (void *)pointer, walk_bytes, size, &bytes) || bytes != size)
                return 0;
            used = sprintf(object_line, "node %08lx %lu %08lx ", root, i, pointer);
            for (j = 0; j < size; ++j) {
                object_line[used++] = digits[walk_bytes[j] >> 4];
                object_line[used++] = digits[walk_bytes[j] & 15];
            }
            object_line[used++] = '\n';
            if (!WriteFile(log, object_line, used, &written, 0) || written != (DWORD)used) return 0;
            if (root==0x373d0 && !dump_pdb_objects(child,log,root,i,pointer)) return 0;
            if ((root==0x37160 || root==0x37164) && !dump_type_cache(child,log,root,i)) return 0;
            if (sopen_start && size == 40 && (root == 0x38d18 || root == 0x38d1c)) {
                DWORD flags;
                memcpy(&flags, walk_bytes+4, 4);
                memcpy(original_bytes, walk_bytes, 40);
                if (!(flags & 8) && !dump_file_descriptor(child, log, root, i)) return 0;
            }
            if (GetEnvironmentVariableA("CVPACK_CAPTURE_STRINGS", strings, sizeof(strings))) {
                field = strings;
                while (*field) {
                    if (sscanf(field, "%lx:%lu%n", &string_root, &offset, &field_consumed) != 2) return 0;
                    if (string_root == root) {
                        if (offset+4 > size) return 0;
                        memcpy(&address, walk_bytes+offset, 4);
                        length = 0;
                        if (address) {
                            do {
                                if (length == sizeof(text) || !ReadProcessMemory(child,
                                    (void *)(address+length), text+length, 1, &bytes) || bytes != 1) return 0;
                            } while (text[length++]);
                        }
                        used = sprintf(object_line, "node-string %08lx %lu %lu %08lx ", root, i, offset, address);
                        for (j = 0; j < length; ++j) {
                            object_line[used++] = digits[text[j] >> 4];
                            object_line[used++] = digits[text[j] & 15];
                        }
                        object_line[used++] = '\n';
                        if (!WriteFile(log, object_line, used, &written, 0) || written != (DWORD)used) return 0;
                    }
                    field += field_consumed;
                    if (*field == ',') ++field;
                    else if (*field) return 0;
                }
            }
            if (GetEnvironmentVariableA("CVPACK_CAPTURE_ARRAYS", arrays, sizeof(arrays))) {
                field = arrays;
                while (*field) {
                    if (sscanf(field, "%lx:%lu:%lu:%lu:%lu:%lu:%lx:%lx:%lu%n", &string_root,
                        &offset, &count_offset, &shift, &maximum, &flags_offset,
                        &flags_mask, &flags_value, &element_size, &field_consumed) != 9) return 0;
                    if (string_root == root) {
                        if (offset > size-4 || (count_offset != 0xffffffff && count_offset > size-4) || flags_offset > size-4 || shift > 31 || (element_size != 1 && element_size != 4)) return 0;
                        memcpy(&flags, walk_bytes+flags_offset, 4);
                        if ((flags & flags_mask) == flags_value) {
                            memcpy(&address, walk_bytes+offset, 4);
                            if (count_offset == 0xffffffff) length = maximum;
                            else {
                                memcpy(&length, walk_bytes+count_offset, 4);
                                length >>= shift;
                            }
                            if (length > maximum || length > sizeof(array_bytes)/element_size) return 0;
                            length = address ? length*element_size : 0;
                            if (length && (!ReadProcessMemory(child, (void *)address, array_bytes,
                                length, &bytes) || bytes != length)) return 0;
                            used = sprintf(object_line, "node-array %08lx %lu %lu %08lx ", root, i, offset, address);
                            for (j = 0; j < length; ++j) {
                                object_line[used++] = digits[array_bytes[j] >> 4];
                                object_line[used++] = digits[array_bytes[j] & 15];
                            }
                            object_line[used++] = '\n';
                            if (!WriteFile(log, object_line, used, &written, 0) || written != (DWORD)used) return 0;
                        }
                    }
                    field += field_consumed;
                    if (*field == ',') ++field;
                    else if (*field) return 0;
                }
            }
            if (next == 0xffffffff) pointer = 0;
            else {
                if (next+4 > size) return 0;
                memcpy(&pointer, walk_bytes+next, 4);
            }
        }
        used = sprintf(object_line, "walk-end %08lx %lu %08lx\n", root, i, pointer);
        WriteFile(log, object_line, used, &written, 0);
        if (pointer) return 0;
        cursor += consumed;
        if (*cursor == ',') ++cursor;
        else if (*cursor) return 0;
    }
    return 1;
}

#include "buffer-observer.h"
#include "resolver-observer.h"

int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION child;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;
    CONTEXT context;
    HANDLE output, error, log, thread, original;
    DWORD base, written, disposition, i, words[96], count;
    char command[2048], line[1024];
    int loader_break, restored;
    char program[1024], child_environment[2048];
    DWORD environment_length;
    if (argc != 7 && argc != 9) return 2;
    original = INVALID_HANDLE_VALUE;
    if (argc == 9) {
        if (!load_plan(argv[7])) return 6;
        original = CreateFileA(argv[8], GENERIC_READ, FILE_SHARE_READ, 0,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (original == INVALID_HANDLE_VALUE) return 7;
    }
    memset(&startup, 0, sizeof(startup));
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    output = CreateFileA(argv[3], GENERIC_WRITE, FILE_SHARE_READ, &security,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    error = CreateFileA(argv[4], GENERIC_WRITE, FILE_SHARE_READ, &security,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    log = CreateFileA(argv[5], GENERIC_WRITE, FILE_SHARE_READ, 0,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (output == INVALID_HANDLE_VALUE || error == INVALID_HANDLE_VALUE ||
        log == INVALID_HANDLE_VALUE) return 3;
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = output;
    startup.hStdError = error;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetEnvironmentVariableA("CVPACK_CAPTURE_ARGV0", program, sizeof(program)))
        strcpy(program, argv[1]);
    sprintf(command, "\"%s\" %s", program, argv[2]);
    if (GetEnvironmentVariableA("CVPACK_CAPTURE_WATCH_SPAWN", program, sizeof(program)))
        watched_spawn = strtoul(program, 0, 16);
    resolver_enabled = GetEnvironmentVariableA("CVPACK_CAPTURE_RESOLVER", program, sizeof(program)) != 0;
    buffer_enabled = GetEnvironmentVariableA("CVPACK_CAPTURE_BUFFER", program, sizeof(program)) != 0;
    if (GetEnvironmentVariableA("CVPACK_CAPTURE_SOPEN_GUARD", program, sizeof(program))) {
        if (sscanf(program, "%lx:%lx:%lx", &sopen_start, &sopen_end, &sopen_offset) != 3 ||
            sopen_start != 0x2dcb0 || sopen_end != 0x2e0a0 ||
            !GetEnvironmentVariableA("CVPACK_CAPTURE_SOPEN_NAME", sopen_name, sizeof(sopen_name)) ||
            strcmp(sopen_name, "hello-debug.dll") ||
            !GetEnvironmentVariableA("CVPACK_CAPTURE_SOPEN_INPUT", sopen_master, sizeof(sopen_master)) ||
            strlen(argv[6])+strlen(sopen_name)+2 >= sizeof(sopen_input)) return 14;
        sprintf(sopen_input, "%s\\%s", argv[6], sopen_name);
        if (GetEnvironmentVariableA("CVPACK_CAPTURE_SOPEN_PDB_NAME", sopen_pdb_name, sizeof(sopen_pdb_name))) {
            if (!GetEnvironmentVariableA("CVPACK_CAPTURE_SOPEN_PDB_INPUT", sopen_pdb_master, sizeof(sopen_pdb_master))) return 14;
        }
    }
    pdb_read_enabled = GetEnvironmentVariableA("CVPACK_CAPTURE_PDB_HEADER", program, sizeof(program)) != 0;
    if (GetEnvironmentVariableA("CVPACK_CAPTURE_READS", program, sizeof(program))) {
        char *cursor = program;
        int consumed;
        while (*cursor) {
            if (read_site_count == 32 || sscanf(cursor, "%lx:%lx%n", &read_sites[read_site_count].call,
                &read_sites[read_site_count].returned, &consumed) != 2) return 18;
            ++read_site_count; cursor += consumed;
            if (*cursor == ',') ++cursor;
            else if (*cursor) return 18;
        }
    }
    strcpy(child_environment, "LIB=");
    environment_length = GetEnvironmentVariableA("LIB", child_environment+4, sizeof(child_environment)-16);
    if (!environment_length || environment_length >= sizeof(child_environment)-16) return 10;
    strcpy(child_environment+4+environment_length+1, "PATH=");
    child_environment[4+environment_length+1+6] = 0;
    if (!CreateProcessA(argv[1], command, 0, 0, TRUE, DEBUG_ONLY_THIS_PROCESS,
                         child_environment, argv[6], &startup, &child)) return 4;
    if (sopen_start) {
        DWORD saved_error, own_teb;
        saved_error = GetLastError();
        __asm { mov eax, fs:[18h] }
        __asm { mov own_teb, eax }
        SetLastError(0x76543210);
        if (*(DWORD *)(own_teb+0x34) != GetLastError()) return 22;
        SetLastError(saved_error);
    }
    base = 0;
    loader_break = 0;
    for (;;) {
        if (!WaitForDebugEvent(&event, 60000)) {
            TerminateProcess(child.hProcess, 98);
            return 5;
        }
        disposition = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            child_teb = (DWORD)event.u.CreateProcessInfo.lpThreadLocalBase;
            if (base != 0x10000) {
                TerminateProcess(child.hProcess, 97);
                return 8;
            }
            if (event.u.CreateProcessInfo.hFile)
                CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            if (!loader_break && event.u.Exception.ExceptionRecord.ExceptionCode ==
                                  EXCEPTION_BREAKPOINT) {
                loader_break = 1;
            } else {
                thread = event.dwThreadId == child.dwThreadId ? child.hThread : 0;
                if(resolver_enabled && resolver_step && thread && event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_SINGLE_STEP) {
                    if(!rearm_resolver(child.hProcess,log,thread,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress,event.dwThreadId))return 39;
                    ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);continue;
                }
                if(resolver_enabled && thread && event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT) {
                    restored=observe_resolver(child.hProcess,original,log,thread,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress,event.dwThreadId);
                    if(restored<0)return 38;
                    if(restored) { ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);continue; }
                }
                if(buffer_enabled && buffer_step && thread && event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_SINGLE_STEP) {
                    if(!rearm_buffer(child.hProcess,log,thread,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress,event.dwThreadId))return 36;
                    ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);continue;
                }
                if(buffer_enabled && thread && event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT) {
                    restored=observe_buffer(child.hProcess,original,log,thread,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress,event.dwThreadId);
                    if(restored<0)return 35;
                    if(restored) { ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);continue; }
                }
                if (thread && event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT &&
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress==0x2504e) {
                    if(!observe_symbol_allocation(child.hProcess,log,thread,event.dwThreadId)) return 29;
                    ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);continue;
                }
                if (thread && event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT &&
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress==0x199d7) {
                    if(!observe_type_page(child.hProcess,log,thread,event.dwThreadId)) return 32;
                    ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);continue;
                }
                if(type_page_step && thread && event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_SINGLE_STEP) {
                    if(!rearm_type_page(child.hProcess,log,thread,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress,event.dwThreadId)) return 33;
                    ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);continue;
                }
                if(symbol_step && thread && event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_SINGLE_STEP) {
                    if(!rearm_symbol_allocation(child.hProcess,log,thread,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress,event.dwThreadId)) return 30;
                    ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);continue;
                }
                if (read_step_site && thread && event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP) {
                    if (rearm_read(child.hProcess, thread, log, (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress) < 0) return 19;
                    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
                    continue;
                }
                if (read_site_count && thread && event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) {
                    restored = observe_read(child.hProcess, thread, log, (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress);
                    if (restored < 0) return 20;
                    if (restored) {
                        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
                        continue;
                    }
                }
                if (pdb_read_enabled && thread && event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT &&
                    ((DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress == 0x2af51 ||
                     (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress == 0x2b299)) {
                    if (!observe_pdb_header(child.hProcess,log,thread)) return 25;
                    ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);continue;
                }
                if (pdb_read_enabled && thread && event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT &&
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress==0x2b1a7) {
                    if (!observe_pdb_incremental(child.hProcess,log,thread)) return 27;
                    ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);continue;
                }
                if (pdb_read_step && thread && event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP) {
                    if (!rearm_pdb_header(child.hProcess,log,thread,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress)) return 26;
                    ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);continue;
                }
                if (sopen_return && thread && event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT &&
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress == 0x2df3c) {
                    if (begin_sopen_create(child.hProcess, log, thread) < 0) return 23;
                    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
                    continue;
                }
                if (sopen_create_step && thread && event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP) {
                    if (rearm_sopen_create(child.hProcess, log, thread,
                        (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress) < 0) return 24;
                    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
                    continue;
                }
                if (sopen_start && thread && event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP) {
                    if (rearm_sopen(child.hProcess, log, thread,
                        (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress) < 0) return 15;
                    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
                    continue;
                }
                if (sopen_return && thread && event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT &&
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress == sopen_return) {
                    if (finish_sopen(child.hProcess, log, thread) < 0) return 16;
                    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
                    continue;
                }
                if (sopen_start && thread && event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT &&
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress == sopen_start) {
                    if (begin_sopen(child.hProcess, original, log, thread) < 0) return 17;
                    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
                    continue;
                }
                if (thread && spawn_return &&
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress == spawn_return) {
                    if (!finish_spawn(child.hProcess, thread, log)) return 11;
                    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
                    continue;
                }
                if (thread && watched_spawn &&
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress == watched_spawn)
                    if (!watch_spawn(child.hProcess, thread, log)) return 12;
                restored = 0;
                if (thread && original != INVALID_HANDLE_VALUE &&
                    event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                    restored = restore_runtime(child.hProcess, original, log, thread,
                        (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress);
                if (restored < 0) return 9;
                if (restored > 0) {
                    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
                    continue;
                }
                sprintf(line, "exception %08lx va %08lx base %08lx first %lu\n",
                    event.u.Exception.ExceptionRecord.ExceptionCode,
                    (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress,
                    base, event.u.Exception.dwFirstChance);
                WriteFile(log, line, strlen(line), &written, 0);
                memset(&context, 0, sizeof(context));
                context.ContextFlags = CONTEXT_FULL;
                if (thread && GetThreadContext(thread, &context)) {
                    sprintf(line, "registers eax %08lx ebx %08lx ecx %08lx edx %08lx esi %08lx edi %08lx ebp %08lx esp %08lx eip %08lx\n",
                        context.Eax, context.Ebx, context.Ecx, context.Edx,
                        context.Esi, context.Edi, context.Ebp, context.Esp, context.Eip);
                    WriteFile(log, line, strlen(line), &written, 0);
                    if (ReadProcessMemory(child.hProcess, (void *)context.Esp,
                                          words, sizeof(words), &count)) {
                        for (i = 0; i < count / 4; ++i) {
                            sprintf(line, "stack %lu %08lx\n", i, words[i]);
                            WriteFile(log, line, strlen(line), &written, 0);
                        }
                    }
                }
                if(!dump_buffer(child.hProcess,log,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress))return 37;
                if(!dump_type_page(child.hProcess,log,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress)) return 34;
                if(!dump_symbol_allocation(child.hProcess,log,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress)) return 31;
                if (!dump_read_progress(child.hProcess,log,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress,&context,words)) return 28;
                if (!dump_output_buffers(child.hProcess, log, (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress, words, &context)) return 15;
                if (!dump_transient_children(child.hProcess, log, (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress, words)) return 21;
                dump_process_arguments(child.hProcess, log);
                dump_inspections(child.hProcess, log);
                if (!dump_walks(child.hProcess, log)) return 13;
                TerminateProcess(child.hProcess, 99);
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            sprintf(line, "exit %lu\n", event.u.ExitProcess.dwExitCode);
            WriteFile(log, line, strlen(line), &written, 0);
            ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
            break;
        }
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
    }
    CloseHandle(child.hThread);
    CloseHandle(child.hProcess);
    CloseHandle(output);
    CloseHandle(error);
    CloseHandle(log);
    if (original != INVALID_HANDLE_VALUE) CloseHandle(original);
    return 0;
}
