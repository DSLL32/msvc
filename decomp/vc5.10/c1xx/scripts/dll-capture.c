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
static BYTE return_saved;
static int return_active, restore_observer;
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
    write_mem(allocated, bytes, sizeof(bytes));
    write_mem(base + 0xc4d50, &allocated, 4);
    value = first_copy ? 17 : 0; write_mem(base + 0xcd7e0, &value, 4);
    value = 32; write_mem(base + 0xc4d54, &value, 4);
    for (i = 0; i < 32; ++i) callbacks[i] = base + 0x38610;
    write_mem(base + 0xc4d58, callbacks, sizeof(callbacks));
    for (i = 0; i < 3; ++i) write_mem(base + 0xba900 + 4 * i, &allocated, 4);
    for (i = 0; i < 3; ++i) write_mem(base + 0xba8f0 + 4 * i, &allocated, 4);
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

static void invocation_arguments(CONTEXT *context);

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
        pass_argc = read_invoke_arguments(argument_file, &pass_argv);
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
            } else if (code == EXCEPTION_BREAKPOINT && target_seen &&
                    address >= base + code_rva && address < base + code_rva + code_size) {
                if (!initialized) {
                    if (address != base + entry_rva || !crt_seen) fail("unguarded entry/provider");
                    prepare_mask();
                }
                if (address == base + 0x384d4) seed_frontend_restore();
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
