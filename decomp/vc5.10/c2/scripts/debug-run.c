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
                memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL;
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
