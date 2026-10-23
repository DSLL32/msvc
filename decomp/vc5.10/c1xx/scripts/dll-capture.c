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
static BYTE return_saved;
static int return_active;
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

static void prepare_mask(void)
{
    BYTE *masked;
    HANDLE saved;
    DWORD wrote;
    unsigned i;
    DWORD resolved, adjust_value;
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

static void save_state(void)
{
    BYTE globals[0xa4];
    DWORD begin, first = 0;
    char filename[80];
    FILE *state;
    readmem(base + 0xcb000, globals, sizeof(globals));
    memcpy(&begin, globals + 0x1c, 4);
    if (begin) readmem(begin, &first, 4);
    fprintf(events, "state seq=%u heap-begin=%08lx heap-first=%08lx\n", sequence, begin, first);
    sprintf(filename, "state-%03u.bin", sequence++);
    state = fopen(filename, "wb");
    if (!state || fwrite(globals, 1, sizeof(globals), state) != sizeof(globals)) fail("save state");
    fclose(state); fflush(events);
}

static void boundary(CONTEXT *context, DWORD address, HANDLE thread)
{
    DWORD stack[5];
    readmem(context->Esp, stack, sizeof(stack));
    fprintf(events, "trap seq=%u rva=%08lx esp=%08lx eax=%08lx stack=%08lx,%08lx,%08lx,%08lx,%08lx\n",
            sequence, address - base, context->Esp, context->Eax,
            stack[0], stack[1], stack[2], stack[3], stack[4]);
    fprintf(events, "last-error=%lu\n", thread_error(context, thread));
    fprintf(events, "control-word=%04lx\n", context->FloatSave.ControlWord);
    save_state();
    if (address == base + entry_rva) {
        if (return_active) fail("nested target DLL entry unsupported");
        return_address = stack[0]; return_reason = stack[2];
        readmem(return_address, &return_saved, 1);
        patch(return_address, 0xcc); return_active = 1;
    }
}

static int host(const char *path)
{
    HMODULE module;
    FARPROC invoke, abort;
    module = LoadLibraryA(path);
    if (!module) return 3;
    puts("attach=1");
    invoke = GetProcAddress(module, "_InvokeCompilerPass@12");
    abort = GetProcAddress(module, "_AbortCompilerPass@4");
    if (!invoke || !abort || invoke != GetProcAddress(module, (LPCSTR)2) ||
            abort != GetProcAddress(module, (LPCSTR)1)) return 4;
    puts("exports=1");
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
            } else if (code == EXCEPTION_BREAKPOINT && return_active && address == return_address) {
                fprintf(events, "crt-return seq=%u reason=%lu status=%lu pc=%08lx\n",
                        sequence, return_reason, context.Eax, address);
                fprintf(events, "last-error=%lu\n", thread_error(&context, thread));
                fprintf(events, "control-word=%04lx\n", context.FloatSave.ControlWord);
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
                boundary(&context, address, thread);
                for (i = 0; i < count; ++i)
                    if (ranges[i].entry && address == base + ranges[i].first) break;
                if (i < count) {
                    patch(address, ranges[i].saved); pending = i;
                    context.EFlags |= 0x100;
                } else if (baseline && address == base + entry_rva) {
                    patch(address, 0x53);
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
