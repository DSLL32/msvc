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
static DWORD crt_base, main_frame_base, configuration_context, parser_allocation_bytes, parser_allocation, virtual_arguments[4];
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

/* Explicit original-only typed heap capture, bounded to this help case. */
static int save_memory(DWORD address,unsigned size,const char *name)
{
    unsigned char bytes[65536];
    DWORD got,written;
    char path[1024];
    HANDLE file;
    if(size>sizeof(bytes) || !ReadProcessMemory(process,(void *)address,bytes,size,&got)||got!=size)return 0;
    sprintf(path,"%s\\%s",output_directory,name);
    file=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE||!WriteFile(file,bytes,size,&written,NULL)||written!=size)return 0;
    CloseHandle(file);
    return 1;
}
static int save_context_heap(DWORD context)
{
    DWORD pointer,got,table[10],hash[14],arena[4];
    unsigned index,kind;
    char name[128];
    for(index=0;index<2;++index) {
        if(!ReadProcessMemory(process,(void *)(context+0x1d0+index*4),&pointer,4,&got)||got!=4||!pointer||
           !ReadProcessMemory(process,(void *)pointer,table,sizeof(table),&got)||got!=sizeof(table))return 0;
        sprintf(name,"table-%u.bin",index);if(!save_memory(pointer,40,name))return 0;
        if(!ReadProcessMemory(process,(void *)table[0],hash,sizeof(hash),&got)||got!=sizeof(hash))return 0;
        sprintf(name,"hash-%u.bin",index);if(!save_memory(table[0],56,name))return 0;
        for(kind=0;kind<2;++kind) {
            pointer=hash[11+kind];
            if(!pointer||!ReadProcessMemory(process,(void *)pointer,arena,sizeof(arena),&got)||got!=sizeof(arena))return 0;
            sprintf(name,"arena-%u-%u.bin",index,kind);if(!save_memory(pointer,16,name))return 0;
            sprintf(name,"arena-data-%u-%u.bin",index,kind);if(!save_memory(arena[3],arena[1]+arena[2],name))return 0;
        }
    }
    if(!ReadProcessMemory(process,(void *)(context+0x260),&pointer,4,&got)||got!=4||!pointer)return 0;
    return save_memory(pointer,44,"context-auxiliary.bin");
}

static int snapshot(DWORD address)
{
    CONTEXT context;
    DWORD got, stack[32];
    unsigned index, argument_index;
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
    fprintf(events, "],\"crt_argv_pointer\":%lu,\"ebp\":%lu",arguments,context.Ebp);
    pointer=0;
    if(argument_count>1 && (!ReadProcessMemory(process,(void *)(arguments+4),&pointer,4,&got)||got!=4))return 0;
    fprintf(events,",\"crt_first_argument_pointer\":%lu",pointer);
    if(address==base+0x15230) {
        if(!remote_string(stack[2],value,sizeof(value)))return 0;
        fprintf(events,",\"debug_options_pointer\":%lu,\"debug_environment\":",stack[1]);quoted(value);
    }
    if(main_frame_base || address==base+0xb560 || address==base+0x152f0 || address==base+0x4fe70 || address==base+0xb660 || address==base+0x4f560 || address==base+0x48cb0 || address==base+0x44b10 || address==base+0x5caa0 || address==base+0x5cb20) {
        if(!ReadProcessMemory(process,(void *)(base+0x7f074),&pointer,4,&got)||got!=4||!remote_string(pointer,value,sizeof(value)))return 0;
        fprintf(events,",\"program_name_pointer\":%lu,\"main_frame_pointer\":%lu,\"program_name\":",pointer,main_frame_base?main_frame_base:address==base+0x44b10?stack[2]:address==base+0x5caa0?stack[6]:address==base+0x5cb20?stack[13]:context.Ebp);quoted(value);
    }
    if(main_frame_base || address==base+0x48cb0 || address==base+0x44b10 || address==base+0x5caa0 || address==base+0x5cb20) {
        if(!ReadProcessMemory(process,(void *)(base+0x7f060),&pointer,4,&got)||got!=4||!remote_string(pointer,value,sizeof(value)))return 0;
        fprintf(events,",\"selected_tool_name_pointer\":%lu,\"selected_tool_name\":",pointer);quoted(value);
    }
    /* Parameter roles are keyed to the exact known function ABI. */
    argument_index=0;
    switch(address-base) {
    case 0x5c780: /* parse_argument(text, source) */
    case 0x5c750: /* is_libpath(text) */
    case 0x4fed0: /* duplicate_argument(text) */
    case 0x44f20: /* has_switch(name) */
        argument_index=1; break;
    case 0xd0c0: /* parse_switch(file, text) */
    case 0x5cca0: /* append_argument(list, text) */
    case 0x5cc00: /* append_argument_record(list, text, original_name) */
        argument_index=2; break;
    }
    if(argument_index) {
        pointer=stack[argument_index];
        if(!remote_string(pointer,value,sizeof(value)))return 0;
        fprintf(events,",\"argument_pointer\":%lu,\"argument_text\":",pointer);quoted(value);
    }
    if(configuration_context)fprintf(events,",\"configuration_context\":%lu",configuration_context);
    fprintf(events,"}\n");
    for (index = 0; index < request_count; ++index) {
        address_base = strcmp(requests[index].scope, "crt") ? base : crt_base;
        pointer=address_base+requests[index].rva;
        if(!strcmp(requests[index].scope,"targetptr")) {
            if(!ReadProcessMemory(process,(void *)pointer,&pointer,4,&got)||got!=4||!pointer)return 0;
        }
        if (!address_base || !ReadProcessMemory(process, (void *)pointer,
            bytes, requests[index].size, &got) || got != requests[index].size) return 0;
        sprintf(path, "%s\\%s", output_directory, requests[index].name);
        output = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (output == INVALID_HANDLE_VALUE || !WriteFile(output, bytes, requests[index].size, &written, NULL) ||
            written != requests[index].size) return 0;
        CloseHandle(output);
    }
    if(main_frame_base || address==base+0x4f560 || address==base+0x48cb0 || address==base+0x44b10 || address==base+0x5caa0 || address==base+0x5cb20) {
        DWORD slots[16],array;
        if(!ReadProcessMemory(process,(void *)(base+0x67538),&array,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)array,slots,sizeof(slots),&got)||got!=sizeof(slots))return 0;
        for(index=0;index<16;++index) {
            if(!ReadProcessMemory(process,(void *)slots[index],bytes,40,&got)||got!=40)return 0;
            sprintf(path,"%s\\file-record-%02u.bin",output_directory,index);
            output=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
            if(output==INVALID_HANDLE_VALUE||!WriteFile(output,bytes,40,&written,NULL)||written!=40)return 0;
            CloseHandle(output);
        }
    }
    if(address==base+0x337d0 || address==base+0x62220 || address==base+0x30c70 || address==base+0x30c30 || address==base+0x4f5e0 || address==base+0x4f640 || address==base+0x4fe50) {
        pointer=address==base+0x337d0?context.Ecx:context.Ebp;
        if(!ReadProcessMemory(process,(void *)pointer,bytes,664,&got)||got!=664)return 0;
        sprintf(path,"%s\\link-context.bin",output_directory);
        output=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        if(output==INVALID_HANDLE_VALUE||!WriteFile(output,bytes,664,&written,NULL)||written!=664)return 0;
        CloseHandle(output);
        if(address==base+0x30c70 || address==base+0x30c30 || address==base+0x4f5e0 || address==base+0x4f640 || address==base+0x4fe50) {
            if(!ReadProcessMemory(process,(void *)(pointer+0x1d0),&pointer,4,&got)||got!=4||!pointer||
               !ReadProcessMemory(process,(void *)pointer,bytes,40,&got)||got!=40)return 0;
            sprintf(path,"%s\\string-table.bin",output_directory);
            output=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
            if(output==INVALID_HANDLE_VALUE||!WriteFile(output,bytes,40,&written,NULL)||written!=40)return 0;
            CloseHandle(output);
            if(address==base+0x30c30 || address==base+0x4f5e0 || address==base+0x4f640 || address==base+0x4fe50) {
                if(!ReadProcessMemory(process,(void *)pointer,&pointer,4,&got)||got!=4||!pointer||
                   !ReadProcessMemory(process,(void *)pointer,bytes,56,&got)||got!=56)return 0;
                sprintf(path,"%s\\hash-state.bin",output_directory);
                output=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
                if(output==INVALID_HANDLE_VALUE||!WriteFile(output,bytes,56,&written,NULL)||written!=56)return 0;
                CloseHandle(output);
            }
        }
    }
    if(address==base+0x45130 || ((address==base+0xd0c0 || address==base+0x4fed0) && configuration_context)) {
        pointer=address==base+0x45130?stack[1]:configuration_context;
        if(!save_memory(pointer,664,"link-context.bin") || !save_context_heap(pointer))return 0;
    }
    if(address==base+0x4fed0) {
        if(!parser_allocation || context.Ebp!=parser_allocation || !save_memory(parser_allocation,parser_allocation_bytes,"parsed-switch.bin"))return 0;
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
        if ((strcmp(requests[request_count].scope,"target") && strcmp(requests[request_count].scope,"crt") && strcmp(requests[request_count].scope,"targetptr")) ||
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
                    if (!strcmp(entries[index].name,"observe:main_frame")) {
                        main_frame_base=context.Ebp;
                        fprintf(events,"{\"event\":\"main_frame\",\"site\":%lu,\"ebp\":%lu}\n",site,main_frame_base);
                    } else if (!strncmp(entries[index].name, "observe:initerm_", 16)) {
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
                    } else if(!strncmp(entries[index].name,"observe:module_",15) || !strncmp(entries[index].name,"observe:proc_",13)) {
                        DWORD arguments[2],got;
                        char requested[512];
                        if(!ReadProcessMemory(process,(void *)context.Esp,arguments,8,&got)||got!=8){failed=6;break;}
                        fprintf(events,"{\"event\":\"module_probe\",\"name\":\"%s\",\"site\":%lu,\"arguments\":[%lu,%lu],\"eax\":%lu",entries[index].name,site,arguments[0],arguments[1],context.Eax);
                        if(!strcmp(entries[index].name,"observe:module_call") || !strcmp(entries[index].name,"observe:proc_call")) {
                            if(!remote_string(arguments[!strcmp(entries[index].name,"observe:proc_call")],requested,sizeof(requested))){failed=6;break;}
                            fprintf(events,",\"requested\":");quoted(requested);
                        }
                        fprintf(events,"}\n");
                    } else if(!strncmp(entries[index].name,"observe:parser_alloc_",21)) {
                        DWORD got;
                        if(!strcmp(entries[index].name,"observe:parser_alloc_call")) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,&parser_allocation_bytes,4,&got)||got!=4){failed=6;break;}
                        } else parser_allocation=context.Eax;
                        fprintf(events,"{\"event\":\"parser_allocation\",\"name\":\"%s\",\"site\":%lu,\"bytes\":%lu,\"result\":%lu}\n",entries[index].name,site,parser_allocation_bytes,parser_allocation);
                    } else if(!strncmp(entries[index].name,"observe:virtual_",16)) {
                        DWORD got;
                        if(!strcmp(entries[index].name,"observe:virtual_call")) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,virtual_arguments,sizeof(virtual_arguments),&got)||got!=sizeof(virtual_arguments)){failed=6;break;}
                        } else if(!context.Eax || !save_memory(context.Eax,virtual_arguments[1],"virtual-first-zero.bin")){failed=6;break;}
                        fprintf(events,"{\"event\":\"virtual_observation\",\"name\":\"%s\",\"site\":%lu,\"arguments\":[%lu,%lu,%lu,%lu],\"eax\":%lu}\n",entries[index].name,site,virtual_arguments[0],virtual_arguments[1],virtual_arguments[2],virtual_arguments[3],context.Eax);
                    } else if(!strncmp(entries[index].name,"observe:mbs_",12)) {
                        DWORD arguments[3],got,codepage,locale_handle,mb_cur_max,locale_codepage;
                        unsigned char table[257];
                        char left[1024],right[1024];
                        if(!ReadProcessMemory(process,(void *)context.Esp,arguments,12,&got)||got!=12 ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f1c0),&codepage,4,&got)||got!=4 ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f1c8),table,sizeof(table),&got)||got!=sizeof(table) ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f1b0),&locale_handle,4,&got)||got!=4 ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3fba0),&mb_cur_max,4,&got)||got!=4 ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f8f8),&locale_codepage,4,&got)||got!=4) {failed=6;break;}
                        if(!remote_string(arguments[0],left,sizeof(left))||!remote_string(arguments[1],right,sizeof(right))){failed=6;break;}
                        fprintf(events,"{\"event\":\"mbs_observation\",\"name\":\"%s\",\"site\":%lu,\"codepage\":%lu,\"eax\":%lu,\"arguments\":[%lu,%lu,%lu],\"left\":",entries[index].name,site,codepage,context.Eax,arguments[0],arguments[1],arguments[2]);quoted(left);
                        fprintf(events,",\"lc_ctype_handle\":%lu,\"mb_cur_max\":%lu,\"locale_codepage\":%lu",locale_handle,mb_cur_max,locale_codepage);
                        fprintf(events,",\"right\":");quoted(right);fprintf(events,",\"mbctype_hex\":\"");
                        for(got=0;got<sizeof(table);++got)fprintf(events,"%02x",table[got]);
                        fprintf(events,"\"}\n");
                    } else if(!strncmp(entries[index].name,"observe:system_",15) || !strncmp(entries[index].name,"observe:memory_",15)) {
                        DWORD pointer,size,got,written;
                        unsigned char observed[36];
                        HANDLE saved;
                        char path[1024];
                        int system=entries[index].name[8]=='s';
                        int before=!strcmp(entries[index].name+15,"before");
                        size=system?36:32;
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,&pointer,4,&got)||got!=4){failed=6;break;}
                        } else pointer=context.Esp+(system?0x20:0);
                        if(!ReadProcessMemory(process,(void *)pointer,observed,size,&got)||got!=size){failed=6;break;}
                        sprintf(path,"%s\\%s-%s.bin",output_directory,system?"system":"memory",before?"before":"after");
                        saved=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
                        if(saved==INVALID_HANDLE_VALUE||!WriteFile(saved,observed,size,&written,NULL)||written!=size){failed=6;break;}
                        CloseHandle(saved);
                        fprintf(events,"{\"event\":\"system_observation\",\"name\":\"%s\",\"site\":%lu,\"buffer\":%lu,\"bytes\":%lu,\"eax\":%lu}\n",entries[index].name,site,pointer,size,context.Eax);
                    } else {
                        if(!strcmp(entries[index].name,"_configure_link_options@12")) {
                            DWORD arguments[3],got;
                            if(!ReadProcessMemory(process,(void *)(context.Esp+4),arguments,12,&got)||got!=12||
                               !arguments[0]||arguments[1]||arguments[2]){failed=6;break;}
                            configuration_context=arguments[0];
                            fprintf(events,"{\"event\":\"configuration_arguments\",\"context\":%lu,\"module\":%lu,\"file\":%lu}\n",arguments[0],arguments[1],arguments[2]);
                        }
                        fprintf(events, "{\"event\":\"entry\",\"name\":\"%s\",\"rva\":%lu}\n",
                                   entries[index].name, entries[index].rva);
                    }
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
