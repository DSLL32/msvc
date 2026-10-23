/* Explicit original-only debugger. This is analysis tooling, not reconstruction. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static FILE *events;
static HANDLE process, thread;
static DWORD base, text_rva, text_size;
static unsigned char *mask;
typedef struct Entry { DWORD rva; unsigned byte, seen; char name[128]; } Entry;
static Entry entries[2048];
static unsigned entry_count;
static DWORD crt_base;
typedef struct StateRequest { char scope[16], name[128]; DWORD rva, size; } StateRequest;
static StateRequest requests[64];
static unsigned request_count;
static const char *output_directory;

static void quoted(const char *value)
{
    unsigned char c;
    fputc('"', events);
    while ((c = (unsigned char)*value++) != 0) {
        if (c == '"' || c == '\\') fputc('\\', events);
        if (c < 32 || c >= 127) fprintf(events, "\\u%04x", c);
        else fputc(c, events);
    }
    fputc('"', events);
}

static int remote_string(DWORD address, char *value, unsigned capacity)
{
    DWORD got;
    unsigned index;
    for (index = 0; index + 1 < capacity; ++index) {
        if (!ReadProcessMemory(process, (void *)(address + index), value + index, 1, &got) || got != 1) return 0;
        if (!value[index]) return 1;
    }
    return 0;
}

static int patch(DWORD rva, const void *bytes, DWORD size)
{
    DWORD old, restored, written;
    void *address = (void *)(base + rva);
    return VirtualProtectEx(process, address, size, PAGE_EXECUTE_READWRITE, &old) &&
        WriteProcessMemory(process, address, bytes, size, &written) && written == size &&
        FlushInstructionCache(process, address, size) &&
        VirtualProtectEx(process, address, size, old, &restored);
}

static int save_module(HANDLE file, DWORD address, const char *directory, unsigned index)
{
    HANDLE output;
    DWORD old, got, written, total = 0;
    unsigned char buffer[16384];
    char path[1024];
    if (!file || file == INVALID_HANDLE_VALUE) return 0;
    sprintf(path, "%s\\module-%03u.bin", directory, index);
    output = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    if (output == INVALID_HANDLE_VALUE) return 0;
    old = SetFilePointer(file, 0, NULL, FILE_CURRENT);
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    for (;;) {
        if (!ReadFile(file, buffer, sizeof(buffer), &got, NULL)) return 0;
        if (!got) break;
        if (!WriteFile(output, buffer, got, &written, NULL) || written != got) return 0;
        total += got;
    }
    SetFilePointer(file, old, NULL, FILE_BEGIN);
    CloseHandle(output);
    fprintf(events, "{\"event\":\"module\",\"index\":%u,\"base\":%lu,\"bytes\":%lu}\n",
            index, address, total);
    if (total == 277776) {
        if (crt_base) return 0;
        crt_base = address;
    }
    fflush(events);
    return 1;
}

static int snapshot(DWORD address)
{
    CONTEXT context;
    DWORD got, stack[32];
    unsigned index;
    unsigned char bytes[65536];
    DWORD address_base, written, argument_count, arguments, pointer;
    HANDLE output;
    char path[1024], value[4096];
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
    if (!GetThreadContext(thread, &context)) return 0;
    fprintf(events, "{\"event\":\"trap\",\"address\":%lu,\"rva\":%lu,\"eax\":%lu,\"ecx\":%lu,\"edx\":%lu,\"esi\":%lu,\"edi\":%lu,\"esp\":%lu,\"stack\":[",
            address, address - base, context.Eax, context.Ecx, context.Edx, context.Esi, context.Edi, context.Esp);
    if (!ReadProcessMemory(process, (void *)context.Esp, stack, sizeof(stack), &got)) return 0;
    for (index = 0; index < got / 4; ++index)
        fprintf(events, "%s%lu", index ? "," : "", stack[index]);
    fprintf(events, "],\"x87_control_word\":%lu,\"crt_base\":%lu,\"crt_argv\":[",
            context.FloatSave.ControlWord & 0xffff, crt_base);
    if (!crt_base || !ReadProcessMemory(process, (void *)(crt_base + 0x3fa84), &argument_count, 4, &got) || got != 4 ||
        argument_count > 1024 || !ReadProcessMemory(process, (void *)(crt_base + 0x3fa8c), &arguments, 4, &got) || got != 4) return 0;
    for (index = 0; index < argument_count; ++index) {
        if (!ReadProcessMemory(process, (void *)(arguments + 4 * index), &pointer, 4, &got) || got != 4 ||
            !remote_string(pointer, value, sizeof(value))) return 0;
        if (index) fputc(',', events);
        quoted(value);
    }
    fprintf(events, "]");
    if(address==base+0x15230) {
        if(!remote_string(stack[2],value,sizeof(value)))return 0;
        fprintf(events,",\"debug_options_pointer\":%lu,\"debug_environment\":",stack[1]);quoted(value);
    }
    if(address==base+0xb560) {
        if(!ReadProcessMemory(process,(void *)(base+0x7f074),&pointer,4,&got)||got!=4||!remote_string(pointer,value,sizeof(value)))return 0;
        fprintf(events,",\"program_name_pointer\":%lu,\"main_frame_pointer\":%lu,\"program_name\":",pointer,context.Ebp);quoted(value);
    }
    fprintf(events,"}\n");
    for (index = 0; index < request_count; ++index) {
        address_base = !strcmp(requests[index].scope, "target") ? base : crt_base;
        if (!address_base || !ReadProcessMemory(process, (void *)(address_base + requests[index].rva),
            bytes, requests[index].size, &got) || got != requests[index].size) return 0;
        sprintf(path, "%s\\%s", output_directory, requests[index].name);
        output = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (output == INVALID_HANDLE_VALUE || !WriteFile(output, bytes, requests[index].size, &written, NULL) ||
            written != requests[index].size) return 0;
        CloseHandle(output);
    }
    fflush(events);
    return 1;
}

int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION launched;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;
    CONTEXT context;
    HANDLE output, error;
    FILE *file;
    DWORD status, site, code, result = 999, done = 0;
    unsigned index, modules = 0;
    int failed = 0, loader_seen = 0;
    char command[1024], path[1024];
    if (argc != 6) return 2;
    /* target, output-directory, mask-or-dash, entries, working-directory */
    sprintf(path, "%s\\events.jsonl", argv[2]);
    events = fopen(path, "wt");
    if (!events) return 3;
    output_directory = argv[2];
    sprintf(path, "%s\\state-plan.txt", argv[2]);
    file = fopen(path, "rt");
    if (!file) return 3;
    while (request_count < 64 && fscanf(file, "%15s %lx %lu %127s", requests[request_count].scope,
           &requests[request_count].rva, &requests[request_count].size, requests[request_count].name) == 4) {
        if ((strcmp(requests[request_count].scope,"target") && strcmp(requests[request_count].scope,"crt")) ||
            requests[request_count].size > 65536) return 3;
        ++request_count;
    }
    if (!feof(file)) return 3;
    fclose(file);
    if (strcmp(argv[3], "-")) {
        file = fopen(argv[3], "rb");
        if (!file || fread(&text_rva, 4, 1, file) != 1 ||
            fread(&text_size, 4, 1, file) != 1 || text_size > 0x1000000) return 3;
        mask = malloc(text_size);
        if (!mask || fread(mask, 1, text_size, file) != text_size || fgetc(file) != EOF) return 3;
        fclose(file);
    }
    file = fopen(argv[4], "rt");
    if (!file) return 3;
    while (entry_count < 2048 && fscanf(file, "%lx %x %u %127s", &entries[entry_count].rva,
           &entries[entry_count].byte, &entries[entry_count].seen, entries[entry_count].name) == 4)
        ++entry_count;
    if (!feof(file)) return 3;
    fclose(file);
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security); security.bInheritHandle = TRUE;
    sprintf(path, "%s\\stdout.bin", argv[2]);
    output = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        &security, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    sprintf(path, "%s\\stderr.bin", argv[2]);
    error = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       &security, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (output == INVALID_HANDLE_VALUE || error == INVALID_HANDLE_VALUE) return 3;
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup); startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = output; startup.hStdError = error;
    sprintf(command, "\"%s\" /?", argv[1]);
    if (!CreateProcessA(argv[1], command, NULL, NULL, TRUE, DEBUG_ONLY_THIS_PROCESS,
                        NULL, argv[5], &startup, &launched)) return 4;
    CloseHandle(launched.hProcess); CloseHandle(launched.hThread);
    while (!done) {
        if (!WaitForDebugEvent(&event, 30000)) { failed = 5; break; }
        status = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            process = event.u.CreateProcessInfo.hProcess;
            thread = event.u.CreateProcessInfo.hThread;
            base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            if (base != 0x400000 || !save_module(event.u.CreateProcessInfo.hFile, base, argv[2], modules++)) { failed = 6; break; }
            CloseHandle(event.u.CreateProcessInfo.hFile);
            if (mask && !patch(text_rva, mask, text_size)) { failed = 6; break; }
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (!save_module(event.u.LoadDll.hFile, (DWORD)event.u.LoadDll.lpBaseOfDll, argv[2], modules++)) { failed = 6; break; }
            CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            /* No extra target thread is expected for this simple first case. */
            CloseHandle(event.u.CreateThread.hThread); failed = 7; break;
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = event.u.Exception.ExceptionRecord.ExceptionCode;
            site = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if (code == EXCEPTION_BREAKPOINT && !loader_seen &&
                (site < base || site >= base + 0x9c000)) loader_seen = 1;
            else if (code == EXCEPTION_BREAKPOINT) {
                for (index = 0; index < entry_count && entries[index].rva != site - base; ++index);
                if (index == entry_count || entries[index].seen) {
                    if (!snapshot(site)) { failed = 6; break; }
                    done = 1;
                    fprintf(events, "{\"event\":\"stopped\",\"code\":%lu}\n", code);
                } else {
                    unsigned char instruction = (unsigned char)entries[index].byte;
                    if (!patch(entries[index].rva, &instruction, 1)) { failed = 6; break; }
                    memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL;
                    if (!GetThreadContext(thread, &context)) { failed = 6; break; }
                    context.Eip = site;
                    if (!SetThreadContext(thread, &context)) { failed = 6; break; }
                    entries[index].seen = 1;
                    if (!strncmp(entries[index].name, "observe:initerm_", 16)) {
                        DWORD arguments[2], got, count, item, value;
                        if (!ReadProcessMemory(process,(void *)context.Esp,arguments,8,&got) || got!=8 ||
                            arguments[0]>arguments[1] || (arguments[1]-arguments[0])%4 ||
                            (count=(arguments[1]-arguments[0])/4)>64) {failed=6;break;}
                        fprintf(events,"{\"event\":\"initerm_call\",\"site\":%lu,\"begin\":%lu,\"end\":%lu,\"values\":[",site,arguments[0],arguments[1]);
                        for(item=0;item<count;++item) {
                            if(!ReadProcessMemory(process,(void *)(arguments[0]+item*4),&value,4,&got)||got!=4){failed=6;break;}
                            fprintf(events,"%s%lu",item?",":"",value);
                        }
                        fprintf(events,"]}\n");
                        if(failed)break;
                    } else if(!strncmp(entries[index].name,"observe:new_",12)) {
                        DWORD argument,got,new_handler,new_mode;
                        if(!ReadProcessMemory(process,(void *)context.Esp,&argument,4,&got)||got!=4){failed=6;break;}
                        if(!ReadProcessMemory(process,(void *)(crt_base+0x42034),&new_handler,4,&got)||got!=4 ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f198),&new_mode,4,&got)||got!=4){failed=6;break;}
                        fprintf(events,"{\"event\":\"operator_new_observation\",\"name\":\"%s\",\"site\":%lu,\"stack_top\":%lu,\"eax\":%lu,\"new_handler\":%lu,\"malloc_new_mode\":%lu}\n",entries[index].name,site,argument,context.Eax,new_handler,new_mode);
                    } else if(!strncmp(entries[index].name,"observe:atexit_",15)) {
                        DWORD argument,got;
                        if(!ReadProcessMemory(process,(void *)context.Esp,&argument,4,&got)||got!=4){failed=6;break;}
                        fprintf(events,"{\"event\":\"atexit_observation\",\"name\":\"%s\",\"site\":%lu,\"stack_top\":%lu,\"eax\":%lu}\n",entries[index].name,site,argument,context.Eax);
                    } else if(!strncmp(entries[index].name,"observe:console_",16)) {
                        DWORD arguments[2],got;
                        if(!ReadProcessMemory(process,(void *)context.Esp,arguments,8,&got)||got!=8){failed=6;break;}
                        fprintf(events,"{\"event\":\"console_observation\",\"name\":\"%s\",\"site\":%lu,\"arguments\":[%lu,%lu],\"eax\":%lu}\n",entries[index].name,site,arguments[0],arguments[1],context.Eax);
                    } else fprintf(events, "{\"event\":\"entry\",\"name\":\"%s\",\"rva\":%lu}\n",
                                   entries[index].name, entries[index].rva);
                }
            } else {
                snapshot(site); failed = 8; break;
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            result = event.u.ExitProcess.dwExitCode; done = 1;
            fprintf(events, "{\"event\":\"exit\",\"status\":%lu}\n", result);
        }
        fflush(events);
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status)) { failed = 6; break; }
    }
    if (result == 999 && process) TerminateProcess(process, 99);
    fprintf(events, "{\"event\":\"summary\",\"failed\":%d,\"status\":%lu}\n", failed, result);
    fclose(events); CloseHandle(output); CloseHandle(error);
    if (process) CloseHandle(process);
    if (thread) CloseHandle(thread);
    return failed;
}
