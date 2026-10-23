/* Original-only debug controller. Never linked into the reconstruction. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef DWORD (WINAPI *PathFromHandle)(HANDLE, char *, DWORD, DWORD);
typedef struct RestoreRange { DWORD start, end, offset; } RestoreRange;
static RestoreRange ranges[256];
static unsigned range_count;
static DWORD crt_base;
static DWORD observations[16], observation_offsets[16], observation_count;
static unsigned char buffer[65536];

static int dump_handle(HANDLE source, const char *name)
{
    HANDLE out;
    DWORD count, written;
    SetFilePointer(source, 0, NULL, FILE_BEGIN);
    out = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    if (out == INVALID_HANDLE_VALUE) return 0;
    do {
        if (!ReadFile(source, buffer, sizeof(buffer), &count, NULL) ||
            !WriteFile(out, buffer, count, &written, NULL) || written != count) {
            CloseHandle(out); return 0;
        }
    } while (count);
    CloseHandle(out);
    return 1;
}

static int loaded_file(FILE *log, HANDLE file, DWORD base, const char *outdir)
{
    PathFromHandle getpath;
    char path[2048], output[2048], lower[2048];
    DWORD count, size;
    unsigned i;
    if (!file || file == INVALID_HANDLE_VALUE) {
        fprintf(log, "module %08lx no-file\n", base); return 1;
    }
    path[0] = 0;
    getpath = (PathFromHandle)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                           "GetFinalPathNameByHandleA");
    if (getpath) {
        count = getpath(file, path, sizeof(path), 0);
        if (!count || count >= sizeof(path)) return 0;
    }
    size = GetFileSize(file, NULL);
    for (i = 0; path[i] && i < sizeof(lower)-1; ++i)
        lower[i] = path[i] >= 'A' && path[i] <= 'Z' ? path[i] + 32 : path[i];
    lower[i] = 0;
    fprintf(log, "module %08lx size %lu path %s\n", base, size, path);
    if (strstr(lower, "msvcrt.dll") || size == 277776) {
        crt_base = base;
        sprintf(output, "%s\\loaded-msvcrt.dll", outdir);
        if (!dump_handle(file, output)) return 0;
        fprintf(log, "provider-dump %s\n", output);
    }
    return 1;
}

static int plant_observations(HANDLE child, DWORD start, DWORD end)
{
    DWORD i, count;
    unsigned char trap = 0xcc;
    for (i = 0; i < observation_count; ++i)
        if (observations[i] >= start && observations[i] < end &&
            (!WriteProcessMemory(child, (void *)observations[i], &trap, 1, &count) || count != 1)) return 0;
    return FlushInstructionCache(child, NULL, 0);
}

static void file_state(FILE *log, HANDLE child, DWORD pointer, unsigned index, const char *prefix)
{
    DWORD fields[8], got, i, valid, block, slot, io[2], position;
    HANDLE copy; PathFromHandle getpath; char path[2048];
    if (!pointer || !ReadProcessMemory(child, (void *)pointer, fields, sizeof(fields), &got) || got != sizeof(fields)) return;
    fprintf(log, "%sfile %u pointer %08lx raw ", prefix, index, pointer);
    for (i = 0; i < sizeof(fields); ++i) fprintf(log, "%02x", ((unsigned char *)fields)[i]);
    fputc('\n', log);
    valid = fields[2] ? fields[0]-fields[2] : 0;
    if ((fields[3]&1) && fields[1] < 65536) valid += fields[1];
    if (valid && valid <= fields[6] && valid <= sizeof(buffer) &&
        ReadProcessMemory(child, (void *)fields[2], buffer, valid, &got) && got == valid) {
        fprintf(log, "%sfile-buffer %u ", prefix, index);
        for (i = 0; i < valid; ++i) fprintf(log, "%02x", buffer[i]);
        fputc('\n', log);
    }
    /* Canonical e390 MSVCRT __pioinfo: _get_osfhandle proves 36-byte entries,
     * fd>>5 block and fd&31 slot, HANDLE at0 and osfile byte at4. */
    if (crt_base && fields[4] < 2048 &&
        ReadProcessMemory(child, (void *)(crt_base+0x3f460+4*(fields[4]>>5)), &block, 4, &got) && got == 4 && block) {
        slot = block+36*(fields[4]&31);
        if (ReadProcessMemory(child, (void *)slot, io, sizeof(io), &got) && got == sizeof(io))
        {
            fprintf(log, "%sfile-os %u fd %lu flags %02lx handle %08lx\n", prefix, index, fields[4], io[1]&255, io[0]);
            if (DuplicateHandle(child, (HANDLE)io[0], GetCurrentProcess(), &copy, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
                position = SetFilePointer(copy, 0, NULL, FILE_CURRENT);
                fprintf(log, "%sfile-position %u %lu\n", prefix, index, position);
                getpath = (PathFromHandle)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetFinalPathNameByHandleA");
                if (getpath && getpath(copy, path, sizeof(path), 0) && strlen(path) < sizeof(path)) {
                    fprintf(log, "%sfile-path %u ", prefix, index);
                    for (i = 0; path[i]; ++i) fprintf(log, "%02x", (unsigned char)path[i]);
                    fputc('\n', log);
                }
                CloseHandle(copy);
            }
        }
    }
}

static void trap_state(FILE *log, HANDLE child, HANDLE thread, DWORD address, const char *prefix)
{
    CONTEXT c;
    DWORD words[20], count, i;
    memset(&c, 0, sizeof(c)); c.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &c)) return;
    {
        char *request = getenv("CVTRES_CAPTURE_STATE");
        DWORD start, size;
        while (request && *request) {
            if (sscanf(request, "%lx:%lu", &start, &size) != 2 || !size || size > sizeof(buffer)) break;
            if (ReadProcessMemory(child, (void *)start, buffer, size, &count) && count == size) {
                fprintf(log, "%smemory %08lx ", prefix, start);
                for (i = 0; i < size; ++i) fprintf(log, "%02x", buffer[i]);
                fputc('\n', log);
            }
            request = strchr(request, ',');
            if (request) ++request;
        }
    }
    {
        char *request = getenv("CVTRES_POINTER_STRINGS");
        DWORD start, pointer, got; unsigned char ch;
        while (request && *request) {
            if (sscanf(request, "%lx", &start) != 1) break;
            if (ReadProcessMemory(child, (void *)start, &pointer, 4, &got) && got == 4) {
                fprintf(log, "%spointer-string %08lx ", prefix, start);
                for (i = 0; pointer && i < 4096; ++i) {
                    if (!ReadProcessMemory(child, (void *)(pointer+i), &ch, 1, &got) || got != 1 || !ch) break;
                    fprintf(log, "%02x", ch);
                }
                fputc('\n', log);
            }
            request = strchr(request, ','); if (request) ++request;
        }
    }
    {
        char *request = getenv("CVTRES_CAPTURE_CRT_STATE"), kind;
        DWORD rva, size, pointer, got;
        unsigned char ch;
        while (request && *request) {
            if (!crt_base || sscanf(request, "%lx:%lu:%c", &rva, &size, &kind) != 3 ||
                !size || size > sizeof(buffer)) break;
            if (ReadProcessMemory(child, (void *)(crt_base+rva), buffer, size, &count) && count == size) {
                fprintf(log, "%scrt-memory %08lx ", prefix, rva);
                for (i = 0; i < size; ++i) fprintf(log, "%02x", buffer[i]);
                fputc('\n', log);
                if (kind == 'f' && size == 32) {
                    DWORD fields[8], used;
                    memcpy(fields, buffer, 32);
                    used = fields[2] ? fields[0] - fields[2] : 0;
                    fprintf(log, "%scrt-buffer %08lx ", prefix, rva);
                    if (used && used <= fields[6] && used <= sizeof(buffer) &&
                        ReadProcessMemory(child, (void *)fields[2], buffer, used, &got) && got == used)
                        for (i = 0; i < used; ++i) fprintf(log, "%02x", buffer[i]);
                    fputc('\n', log);
                }
                if (kind == 'p' && size == 4) {
                    memcpy(&pointer, buffer, 4);
                    fprintf(log, "%scrt-string %08lx ", prefix, rva);
                    for (i = 0; i < 4096 && pointer; ++i) {
                        if (!ReadProcessMemory(child, (void *)(pointer+i), &ch, 1, &got) || got != 1 || !ch) break;
                        fprintf(log, "%02x", ch);
                    }
                    fputc('\n', log);
                }
            }
            request = strchr(request, ',');
            if (request) ++request;
        }
    }
    fprintf(log, "%strap %08lx eip %08lx esp %08lx eax %08lx ecx %08lx edx %08lx\n",
            prefix, address, c.Eip, c.Esp, c.Eax, c.Ecx, c.Edx);
    if (ReadProcessMemory(child, (void *)c.Esp, words, sizeof(words), &count)) {
        {
            char *request = getenv("CVTRES_FILE_ARGUMENTS");
            DWORD function, index;
            while (request && *request) {
                if (sscanf(request, "%lx:%lu", &function, &index) != 2 || index >= count/4-1) break;
                if (function == address) file_state(log, child, words[index+1], index, prefix);
                request = strchr(request, ','); if (request) ++request;
            }
        }
        fprintf(log, "%sstack", prefix);
        for (i = 0; i < count/4; ++i) fprintf(log, " %08lx", words[i]);
        fputc('\n', log);
        if (address == 0x010014a5 && words[1] < 64) {
            DWORD pointers[64], j, got;
            unsigned char ch;
            fprintf(log, "%sargc %lu\n", prefix, words[1]);
            if (ReadProcessMemory(child, (void *)words[2], pointers, words[1]*4, &got) && got == words[1]*4) {
                for (i = 0; i < words[1]; ++i) {
                    fprintf(log, "%sargv %lu ", prefix, i);
                    for (j = 0; j < 4096; ++j) {
                        if (!ReadProcessMemory(child, (void *)(pointers[i]+j), &ch, 1, &got) || got != 1) break;
                        if (!ch) break;
                        fprintf(log, "%02x", ch);
                    }
                    fputc('\n', log);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DEBUG_EVENT event;
    CONTEXT context;
    HANDLE out, err, original;
    FILE *log, *plan;
    DWORD disposition, count, i, length;
    char command[4096];
    int initial_break = 1, done = 0, result = 0;
    if (argc != 8 && argc != 9) return 90;
    {
        char *request = getenv("CVTRES_OBSERVATIONS");
        while (request && *request && observation_count < 16) {
            if (sscanf(request, "%lx:%lx", &observations[observation_count],
                       &observation_offsets[observation_count]) != 2) return 106;
            ++observation_count;
            request = strchr(request, ','); if (request) ++request;
        }
    }
    plan = fopen(argv[7], "rt");
    if (!plan) return 91;
    while (range_count < 256 && fscanf(plan, "%lx %lx %lx", &ranges[range_count].start,
             &ranges[range_count].end, &ranges[range_count].offset) == 3) {
        if (ranges[range_count].end <= ranges[range_count].start ||
            ranges[range_count].end-ranges[range_count].start > sizeof(buffer)) return 92;
        ++range_count;
    }
    fclose(plan);
    log = fopen(argv[4], "wt");
    if (!log) return 93;
    original = CreateFileA(argv[6], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (original == INVALID_HANDLE_VALUE) return 94;
    memset(&sa, 0, sizeof(sa)); sa.nLength = sizeof(sa); sa.bInheritHandle = TRUE;
    out = CreateFileA(argv[2], GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_NEW, 0, NULL);
    err = CreateFileA(argv[3], GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_NEW, 0, NULL);
    if (out == INVALID_HANDLE_VALUE || err == INVALID_HANDLE_VALUE) return 95;
    memset(&si, 0, sizeof(si)); si.cb = sizeof(si); si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = out; si.hStdError = err; si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    sprintf(command, "\"%s\" %s", argv[6], argc == 9 ? argv[8] : "/help");
    if (!CreateProcessA(argv[1], command, NULL, NULL, TRUE,
                        DEBUG_ONLY_THIS_PROCESS|CREATE_NO_WINDOW, NULL, argv[5], &si, &pi)) return 96;
    CloseHandle(out); CloseHandle(err);
    while (!done) {
        if (!WaitForDebugEvent(&event, 15000)) { result = 97; break; }
        disposition = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            fprintf(log, "image-base %08lx\n", (DWORD)event.u.CreateProcessInfo.lpBaseOfImage);
            if ((DWORD)event.u.CreateProcessInfo.lpBaseOfImage != 0x01000000) { result = 98; done = 1; }
            {
                char image_path[2048];
                sprintf(image_path, "%s\\loaded-image.exe", argv[5]);
                if (!event.u.CreateProcessInfo.hFile ||
                    !dump_handle(event.u.CreateProcessInfo.hFile, image_path)) { result = 105; done = 1; }
            }
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
            if (getenv("CVTRES_OBSERVE_UNMASKED") && !plant_observations(pi.hProcess, 0, 0xffffffffUL)) { result = 113; done = 1; }
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (!loaded_file(log, event.u.LoadDll.hFile, (DWORD)event.u.LoadDll.lpBaseOfDll, argv[5])) {
                result = 99; done = 1;
            }
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            DWORD code = event.u.Exception.ExceptionRecord.ExceptionCode;
            DWORD address = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if (code == EXCEPTION_BREAKPOINT && initial_break &&
                (address < 0x01001000 || address >= 0x010035de)) {
                initial_break = 0;
            } else if (code == EXCEPTION_BREAKPOINT) {
                for (i = 0; i < observation_count; ++i) if (observations[i] == address) break;
                if (i < observation_count) {
                    DWORD observed = i, j;
                    fprintf(log, "observation %08lx\n", address);
                    trap_state(log, pi.hProcess, pi.hThread, address, "observation-");
                    if (address == 0x0100196e || address == 0x010019f0) {
                        CONTEXT opened; memset(&opened, 0, sizeof(opened)); opened.ContextFlags = CONTEXT_FULL;
                        if (GetThreadContext(pi.hThread, &opened)) file_state(log, pi.hProcess, opened.Eax, 0, "opened-");
                    }
                    SetFilePointer(original, observation_offsets[observed], NULL, FILE_BEGIN);
                    if (!ReadFile(original, buffer, 1, &count, NULL) || count != 1 ||
                        !WriteProcessMemory(pi.hProcess, (void *)address, buffer, 1, &count) || count != 1) { result = 107; done = 1; }
                    memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL;
                    if (!GetThreadContext(pi.hThread, &context)) { result = 108; done = 1; }
                    context.Eip = address;
                    if (!SetThreadContext(pi.hThread, &context)) { result = 109; done = 1; }
                    for (j = 0; j < observation_count; ++j) if (j != observed)
                        if (!plant_observations(pi.hProcess, observations[j], observations[j]+1)) { result = 110; done = 1; }
                    FlushInstructionCache(pi.hProcess, (void *)address, 1);
                    fflush(log);
                    if (done) TerminateProcess(pi.hProcess, 0x77);
                    if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition)) { result = 111; break; }
                    continue;
                }
                for (i = 0; i < range_count; ++i) if (ranges[i].start == address) break;
                if (i == range_count) { trap_state(log, pi.hProcess, pi.hThread, address, ""); done = 1; }
                else {
                    length = ranges[i].end-ranges[i].start;
                    SetFilePointer(original, ranges[i].offset, NULL, FILE_BEGIN);
                    if (!ReadFile(original, buffer, length, &count, NULL) || count != length ||
                        !WriteProcessMemory(pi.hProcess, (void *)address, buffer, length, &count) || count != length) {
                        result = 100; done = 1;
                    } else {
                        if (!plant_observations(pi.hProcess, address, ranges[i].end)) { result = 112; done = 1; }
                        memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL;
                        if (!GetThreadContext(pi.hThread, &context)) { result = 101; done = 1; }
                        else {
                            fprintf(log, "entry %08lx\n", address);
                            if (address < 0x01003180)
                                trap_state(log, pi.hProcess, pi.hThread, address, "entry-");
                            context.Eip = address;
                            if (!SetThreadContext(pi.hThread, &context)) { result = 102; done = 1; }
                            FlushInstructionCache(pi.hProcess, (void *)address, length);
                        }
                    }
                }
            } else {
                fprintf(log, "exception %08lx at %08lx first %lu\n", code, address, event.u.Exception.dwFirstChance);
                disposition = DBG_EXCEPTION_NOT_HANDLED;
                if (!event.u.Exception.dwFirstChance) { result = 103; done = 1; }
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            fprintf(log, "exit %lu\n", event.u.ExitProcess.dwExitCode); done = 2;
        }
        fflush(log);
        if (done == 1) TerminateProcess(pi.hProcess, 0x77);
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition)) { result = 104; break; }
    }
    if (done != 2) { TerminateProcess(pi.hProcess, 0x77); WaitForSingleObject(pi.hProcess, 5000); }
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); CloseHandle(original); fclose(log);
    return result;
}
