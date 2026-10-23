/* Original-only debugger capture. Never linked into a reconstruction or fixture. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static HANDLE child;
static FILE *events;
static DWORD ids[128];
static HANDLE handles[128];
static unsigned count;
static DWORD entry[2048];
static BYTE entry_byte[2048];
static unsigned entries;
static DWORD mainargs_slots[5];
static DWORD message_slots[4], message_number;

static int read_memory(DWORD address, void *buffer, DWORD size)
{
    DWORD got;
    return ReadProcessMemory(child, (void *)address, buffer, size, &got) && got == size;
}
static void save_file(HANDLE source, const char *name)
{
    HANDLE destination;
    BYTE bytes[16384];
    DWORD got, wrote, previous;
    previous = SetFilePointer(source, 0, NULL, FILE_CURRENT);
    if (previous == 0xffffffffUL || SetFilePointer(source, 0, NULL, FILE_BEGIN) == 0xffffffffUL)
        ExitProcess(124);
    destination = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    if (destination == INVALID_HANDLE_VALUE) ExitProcess(125);
    for (;;) {
        if (!ReadFile(source, bytes, sizeof(bytes), &got, NULL)) ExitProcess(126);
        if (!got) break;
        if (!WriteFile(destination, bytes, got, &wrote, NULL) || wrote != got) ExitProcess(127);
    }
    CloseHandle(destination);
    SetFilePointer(source, previous, NULL, FILE_BEGIN);
}
static void loaded(DWORD base, HANDLE file)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_EXPORT_DIRECTORY exports;
    char name[256];
    if (!read_memory(base, &dos, sizeof(dos)) ||
        !read_memory(base+dos.e_lfanew, &nt, sizeof(nt))) ExitProcess(128);
    strcpy(name, "<no-exports>");
    if (nt.OptionalHeader.DataDirectory[0].VirtualAddress) {
        if (!read_memory(base+nt.OptionalHeader.DataDirectory[0].VirtualAddress,
                         &exports, sizeof(exports)) ||
            !read_memory(base+exports.Name, name, sizeof(name))) ExitProcess(129);
        name[sizeof(name)-1] = 0;
    }
    fprintf(events, "module %08lx %08lx %s\n", base, nt.OptionalHeader.ImageBase, name);
    if (!_stricmp(name, "msvcrt.dll")) {
        save_file(file, "loaded-msvcrt.bin");
        fprintf(events, "provider-file loaded-msvcrt.bin\n");
    }
}
static void save_vector(DWORD address, const char *name)
{
    FILE *file = fopen(name, "wb");
    DWORD item, count = 0;
    char value;
    if (!file) ExitProcess(135);
    for (;;) {
        if (++count > 4096 || !read_memory(address, &item, 4)) ExitProcess(136);
        address += 4;
        if (!item) break;
        do {
            if (!read_memory(item++, &value, 1)) ExitProcess(137);
            fputc(value, file);
        } while (value);
    }
    fputc(0, file); fclose(file);
}
static void save_string(DWORD address, const char *name)
{
    FILE *file=fopen(name,"wb");
    DWORD i; char value;
    if (!file) ExitProcess(156);
    for (i=0;i<4096;++i) {
        if (!read_memory(address+i,&value,1)) ExitProcess(157);
        fputc(value,file); if (!value) break;
    }
    fclose(file); if (i==4096) ExitProcess(158);
}
static void save_contextual_data(void)
{
    FILE *plan=fopen("data-plan.txt","r"), *output;
    DWORD address,size; BYTE bytes[4096];
    if (!plan) ExitProcess(159);
    output=fopen("data-state.bin","wb");if (!output) ExitProcess(160);
    while (fscanf(plan,"%lx %lu",&address,&size)==2) {
        if (!size || size>sizeof(bytes) || !read_memory(address,bytes,size)) ExitProcess(161);
        if (fwrite(bytes,1,size,output)!=size) ExitProcess(162);
    }
    fclose(plan);fclose(output);
}
static void observe(CONTEXT *context, DWORD address)
{
    DWORD stack[16], value, i, pointer;
    static DWORD globals[] = {0x499708,0x4996d4,0x49975c,0x4980d4,0x499718,0x499694,0x4980d0,0x4980cc,0x48d064,0x7803fa90,0x7803fa88,0x7803fa94,0x7803f198,0x7803f928,0x7803fde8};
    fprintf(events,"observe %08lx eax=%08lx ecx=%08lx edx=%08lx esp=%08lx\n",
        address,context->Eax,context->Ecx,context->Edx,context->Esp);
    if (read_memory(context->Esp,stack,sizeof(stack))) {
        fprintf(events,"observe-stack");
        for (i=0;i<16;++i) fprintf(events," %08lx",stack[i]);
        fprintf(events,"\n");
    }
    if (read_memory(context->Eax,&value,4)) fprintf(events,"eax-word %08lx %08lx\n",context->Eax,value);
    for (i=0;i<sizeof(globals)/sizeof(globals[0]);++i)
        if (read_memory(globals[i],&value,4)) fprintf(events,"global %08lx %08lx\n",globals[i],value);
    fprintf(events,"x87-control %08lx\n", context->FloatSave.ControlWord);
    if (address == 0x48b78c || address == 0x48b7ba || address == 0x48b3d0 || address == 0x48b6ce || address == 0x48b6ae) {
        DWORD initialized, cache[14];
        unsigned char buffer[384];
        FILE *output;
        if (!read_memory(context->Esp,stack,sizeof(stack)) || !read_memory(0x49e650,&initialized,4) || !read_memory(0x49e1d0,cache,56) || !read_memory(0x49e4d0,buffer,384)) ExitProcess(168);
        if (address == 0x48b78c)
            fprintf(events,"message-load id=%lu buffer=%08lx capacity=%lu allow=%lu initialized=%lu\n",context->Ecx,context->Edx,stack[1],stack[2],initialized);
        else if (address == 0x48b7ba)
            fprintf(events,"message-unescape buffer=%08lx initialized=%lu\n",context->Ecx,initialized);
        if (address == 0x48b6ce || address == 0x48b6ae) fprintf(events,"cached-message id=%lu\n",context->Ecx);
        if (address == 0x48b6ce || address == 0x48b6ae) {
            char name[64];
            for(i=0;i<6;++i) {
                sprintf(name,"cache-message-%lu.bin",cache[2*i]);
                if (!cache[2*i+1]) ExitProcess(172);
                save_string(cache[2*i+1],name);
            }
        }
        if (address == 0x48b3d0) {
            DWORD arguments[2], unexpected;
            char name[64];
            if (!read_memory(stack[4],arguments,8) || !read_memory(0x49e284,&unexpected,4)) ExitProcess(170);
            fprintf(events,"diagnostic-emit category=%lu message=%lu format=%08lx arguments=%08lx argument=%08lx unexpected=%08lx\n",stack[1],stack[2],stack[3],stack[4],arguments[0],unexpected);
            save_string(stack[3],"diagnostic-format.bin");save_string(arguments[0],"fatal-argument.bin");save_string(arguments[1],"fatal-phase.bin");
            for(i=0;i<6;++i) {
                sprintf(name,"cache-message-%lu.bin",cache[2*i]);
                if (!cache[2*i+1]) ExitProcess(171);
                save_string(cache[2*i+1],name);
            }
        }
        output=fopen("message-cache.bin","wb");if(!output)ExitProcess(169);
        fwrite(&initialized,4,1,output);fwrite(cache,56,1,output);fwrite(buffer,384,1,output);fclose(output);
    }
    if (address == 0x48b708) {
        DWORD current, unexpected;
        if (!read_memory(0x49968c,&current,4) || !read_memory(0x49e284,&unexpected,4)) ExitProcess(167);
        fprintf(events,"message-initialize current=%08lx unexpected=%08lx\n",current,unexpected);
        save_string(current,"current-option.bin");
        save_contextual_data();
    }
    if (address == 0x48b6e8) {
        DWORD current, unexpected;
        if (!read_memory(0x49968c,&current,4) || !read_memory(0x49e284,&unexpected,4)) ExitProcess(166);
        fprintf(events,"message-lookup id=%lu allow=%lu current=%08lx unexpected=%08lx\n",context->Ecx,context->Edx,current,unexpected);
        save_string(current,"current-option.bin");
        save_contextual_data();
    }
    if (address == 0x48b566) {
        if (!read_memory(context->Esp,stack,sizeof(stack))) ExitProcess(173);
        fprintf(events,"format-prefix buffer=%08lx offset=%lu format=%08lx character=%lu number=%lu\n",stack[1],stack[1]-context->Esp,stack[2],stack[3],stack[4]);
        save_string(stack[2],"format-pattern.bin");save_string(stack[1]-11,"format-prefix.bin");
    }
    if (address == 0x48b367) {
        DWORD current, unexpected;
        if (!read_memory(context->Esp,stack,sizeof(stack)) || !read_memory(0x49968c,&current,4) || !read_memory(0x49e284,&unexpected,4)) ExitProcess(165);
        fprintf(events,"fatal-option message=%lu argument=%08lx phase=%08lx current=%08lx unexpected=%08lx\n",stack[1],stack[2],stack[3],current,unexpected);
        save_string(stack[2],"fatal-argument.bin");
        save_string(stack[3],"fatal-phase.bin");
        save_contextual_data();
    }
    if (address == 0x452cfe) {
        DWORD current;
        if (!read_memory(context->Esp,stack,sizeof(stack)) || !read_memory(0x49968c,&current,4)) ExitProcess(164);
        fprintf(events,"apply-state option=%08lx value=%08lx mode=%lu current=%08lx\n",context->Ecx,context->Edx,stack[1],current);
        save_string(context->Edx,"option-value.bin");
        save_contextual_data();
    }
    if (address == 0x48b7f8) {
        DWORD current;
        if (!read_memory(context->Esp,stack,sizeof(stack)) || !read_memory(0x49968c,&current,4)) ExitProcess(163);
        fprintf(events,"wildcard-state pattern=%08lx input=%08lx kind=%lu current=%08lx\n",context->Ecx,context->Edx,stack[1],current);
        save_string(context->Ecx,"wildcard-pattern.bin");
        save_string(context->Edx,"wildcard-input.bin");
        save_contextual_data();
    }
    if (address == 0x452bd9) {
        char value;
        FILE *file;
        if (!read_memory(context->Esp,stack,sizeof(stack))) ExitProcess(152);
        fprintf(events,"matcher-state table=%08lx input=%08lx callback=%08lx mode=%lu\n",context->Ecx,context->Edx,stack[1],stack[2]);
        file=fopen("matcher-input.bin","wb");if (!file) ExitProcess(153);
        for(i=0;i<4096;++i) {
            if (!read_memory(context->Edx+i,&value,1)) ExitProcess(154);
            fputc(value,file);if (!value)break;
        }
        fclose(file);if(i==4096)ExitProcess(155);
    }
    if (address == 0x452a22 || address == 0x452acb || address == 0x452bb7 || address == 0x452bd9 || address == 0x48b7f8 || address == 0x452cfe || address == 0x48b367 || address == 0x48b6e8 || address == 0x48b708) {
        DWORD argc, argv;
        if (!read_memory(0x49972c,&argc,4) || !read_memory(0x499728,&argv,4)) ExitProcess(151);
        fprintf(events,"command-state argc=%lu argv=%08lx input=%08lx\n",argc,argv,context->Ecx);
        save_vector(argv,"command-argv.bin");
    }
    if (address == 0x452a18) {
        if (!read_memory(context->Esp,&value,4)) ExitProcess(150);
        fprintf(events,"onexit-input callback=%08lx\n",value);
    }
    if (address == 0x452a1e)
        fprintf(events,"onexit-output result=%08lx\n",context->Eax);
    if ((address == 0x4529f1 || address == 0x452a06)) {
        if (!read_memory(context->Esp+4,&value,4)) ExitProcess(149);
        fprintf(events,"exit-callback %08lx\n",value);
    }
    if (address == 0x4528b0) {
        if (!read_memory(context->Esp,stack,sizeof(stack))) ExitProcess(147);
        fprintf(events,"main-input argc=%lu argv=%08lx environment=%08lx\n",stack[1],stack[2],stack[3]);
        save_vector(stack[2],"main-argv.bin");
        save_vector(stack[3],"main-environment.bin");
        if (!read_memory(0x7803fde8,&value,4)) ExitProcess(148);
        fprintf(events,"main-initenv-alias %lu\n",value==stack[3]);
    }
    if (address == 0x48b79f) {
        DWORD wrote;
        BYTE trap = 0xcc;
        if (!read_memory(context->Esp,message_slots,sizeof(message_slots))) ExitProcess(142);
        fprintf(events,"loadstring-input module=%08lx id=%lu buffer=%08lx capacity=%lu\n",message_slots[0],message_slots[1],message_slots[2],message_slots[3]);
        if (!WriteProcessMemory(child,(void *)0x48b7a5,&trap,1,&wrote) || wrote != 1) ExitProcess(143);
        FlushInstructionCache(child,(void *)0x48b7a5,1);
    }
    if (address == 0x48b7a5) {
        DWORD wrote;
        BYTE trap=0xcc, bytes[384];
        char name[80];
        FILE *file;
        if (message_slots[3] != sizeof(bytes) || !read_memory(message_slots[2],bytes,sizeof(bytes))) ExitProcess(144);
        sprintf(name,"message-%02lu-%lu.bin",message_number++,message_slots[1]);
        file=fopen(name,"wb"); if (!file) ExitProcess(145);
        fwrite(bytes,1,sizeof(bytes),file);fclose(file);
        fprintf(events,"loadstring-output id=%lu result=%ld bytes=%s\n",message_slots[1],(long)context->Eax,name);
        if (!WriteProcessMemory(child,(void *)0x48b79f,&trap,1,&wrote) || wrote!=1) ExitProcess(146);
        FlushInstructionCache(child,(void *)0x48b79f,1);
    }
    if (address == 0x452855 && !read_memory(context->Esp,mainargs_slots,sizeof(mainargs_slots))) ExitProcess(138);
    if (address == 0x45285b) {
        for (i=0;i<5;++i) {
            if (i == 3) value=mainargs_slots[i];
            else if (!read_memory(mainargs_slots[i],&value,4)) ExitProcess(139);
            fprintf(events,"mainargs %lu %08lx %08lx\n",i,mainargs_slots[i],value);
        }
        if (!read_memory(mainargs_slots[1],&pointer,4)) ExitProcess(140);
        save_vector(pointer,"argv.bin");
        if (!read_memory(mainargs_slots[2],&pointer,4)) ExitProcess(141);
        save_vector(pointer,"environment.bin");
    }
}
int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;
    CONTEXT context;
    HANDLE out, err, input;
    DWORD disposition, address, code, wrote, base = 0;
    BYTE trap = 0xcc;
    char command[4096];
    unsigned i;
    int loader_break = 0, completed = 0;
    FILE *plan;
    char child_environment[] = "PATH=R:\\toolchains\\5.10\\bin\0SystemRoot=C:\\windows\0TEMP=.\0TMP=.\0WINEDLLOVERRIDES=msvcrt=n\0WINEPATH=R:\\toolchains\\5.10\\bin\0windir=C:\\windows\0";
    if (argc != 5) return 120;
    events = fopen("events.txt", "wb");
    plan = fopen(argv[4], "r");
    if (!events || !plan) return 121;
    while (entries < 2048 && fscanf(plan, "%lx %x", &entry[entries], &i) == 2)
        entry_byte[entries++] = (BYTE)i;
    fclose(plan);
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security); security.bInheritHandle = TRUE;
    out = CreateFileA("stdout.bin", GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                      &security, CREATE_NEW, 0, NULL);
    err = CreateFileA("stderr.bin", GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                      &security, CREATE_NEW, 0, NULL);
    input = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                        &security, OPEN_EXISTING, 0, NULL);
    memset(&startup, 0, sizeof(startup)); startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input; startup.hStdOutput = out; startup.hStdError = err;
    sprintf(command, "\"%s\" %s", argv[2], argv[3]);
    {
        FILE *input_file = fopen("child-environment-input.bin", "wb");
        if (!input_file) return 147;
        fwrite(child_environment, 1, sizeof(child_environment), input_file); fclose(input_file);
        input_file = fopen("child-command-input.bin", "wb");
        if (!input_file) return 148;
        fwrite(command, 1, strlen(command)+1, input_file); fclose(input_file);
    }
    if (!CreateProcessA(argv[1], command, NULL, NULL, TRUE, DEBUG_ONLY_THIS_PROCESS,
                        child_environment, "S:\\vc5.10\\c2\\original", &startup, &process)) return 122;
    child = process.hProcess;
    while (!completed && WaitForDebugEvent(&event, 30000)) {
        disposition = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            fprintf(events, "image-base %08lx\n", base);
            ids[count] = event.dwThreadId; handles[count++] = event.u.CreateProcessInfo.hThread;
            CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            if (count == 128) return 123;
            ids[count] = event.dwThreadId; handles[count++] = event.u.CreateThread.hThread;
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            loaded((DWORD)event.u.LoadDll.lpBaseOfDll, event.u.LoadDll.hFile);
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = event.u.Exception.ExceptionRecord.ExceptionCode;
            address = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if (code == EXCEPTION_BREAKPOINT && !loader_break && (address < base || address >= base+0xa3000)) {
                loader_break = 1;
            } else {
                for (i=0; i<count && ids[i]!=event.dwThreadId; ++i) {}
                if (i == count) return 130;
                memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
                if (!GetThreadContext(handles[i], &context)) return 131;
                for (i=0; i<entries && entry[i]!=address; ++i) {}
                if (code == EXCEPTION_BREAKPOINT && i < entries) {
                    fprintf(events, "entry %08lx\n", address);
                    observe(&context,address);
                    if (!WriteProcessMemory(child, (void *)address, entry_byte+i, 1, &wrote) || wrote!=1) return 132;
                    FlushInstructionCache(child, (void *)address, 1);
                    context.Eip = address;
                    for (i=0; i<count && ids[i]!=event.dwThreadId; ++i) {}
                    if (!SetThreadContext(handles[i], &context)) return 133;
                } else {
                    DWORD stack[16];
                    fprintf(events, "trap %08lx %08lx eip=%08lx esp=%08lx ecx=%08lx edx=%08lx\n",
                        code, address, context.Eip, context.Esp, context.Ecx, context.Edx);
                    if (read_memory(context.Esp, stack, sizeof(stack))) {
                        fprintf(events,"stack");
                        for (i=0; i<16; ++i) fprintf(events," %08lx",stack[i]);
                        fprintf(events,"\n");
                    }
                    observe(&context,address);
                    TerminateProcess(child, 237); completed = 1;
                }
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            fprintf(events, "exit %08lx\n", event.u.ExitProcess.dwExitCode); completed = 1;
        }
        fflush(events);
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
    }
    if (!completed) { fprintf(events, "timeout\n"); TerminateProcess(child, 238); }
    fclose(events); CloseHandle(out); CloseHandle(err); CloseHandle(input);
    return completed ? 0 : 134;
}
