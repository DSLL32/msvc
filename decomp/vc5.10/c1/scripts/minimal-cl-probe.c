/* Original-only debugger analysis harness; no reconstructed target functions. */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static HANDLE child;
static FILE *logfile;
static DWORD points[5] = { 0x4056c9, 0x40572e, 0, 0x4055d2, 0x4056cc };
static DWORD invoke_esp;
static BYTE saved[5];
static DWORD thread_ids[64], thread_count;
static HANDLE threads[64];
static DWORD event_number;
static char compiler_environment[4096];

static void readmem(DWORD address, void *out, DWORD size)
{
    DWORD got;
    if (!ReadProcessMemory(child, (void *)address, out, size, &got) || got != size) {
        fprintf(logfile, "read-failed %08lx %lu\n", address, size);
        fflush(logfile); ExitProcess(2);
    }
}
static void string_at(DWORD address, char *buffer, DWORD capacity)
{
    DWORD i;
    for (i = 0; i + 1 < capacity; ++i) {
        readmem(address + i, buffer + i, 1);
        if (!buffer[i]) return;
    }
    buffer[capacity - 1] = 0;
}
static void patch(DWORD address, BYTE value)
{
    DWORD wrote;
    if (!WriteProcessMemory(child, (void *)address, &value, 1, &wrote) || wrote != 1)
        ExitProcess(6);
    FlushInstructionCache(child, (void *)address, 1);
}
static void copy_ir(const char *prefix)
{
    WIN32_FIND_DATAA found;
    HANDLE search;
    char pattern[MAX_PATH], directory[MAX_PATH], source[MAX_PATH], destination[MAX_PATH];
    char *slash;
    sprintf(pattern,"%s*",prefix);
    strcpy(directory,prefix);
    slash=strrchr(directory,'\\');
    if (!slash) slash=strrchr(directory,'/');
    if (slash) slash[1]=0; else directory[0]=0;
    CreateDirectoryA("captured-ir",NULL);
    search=FindFirstFileA(pattern,&found);
    if (search == INVALID_HANDLE_VALUE) {
        fprintf(logfile,"ir-missing %s error=%lu\n",pattern,GetLastError()); return;
    }
    do {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            sprintf(source,"%s%s",directory,found.cFileName);
            sprintf(destination,"captured-ir\\%s",found.cFileName);
            if (!CopyFileA(source,destination,TRUE)) ExitProcess(7);
            fprintf(logfile,"ir-copy %s -> %s bytes=%lu:%lu\n",source,destination,
                found.nFileSizeHigh,found.nFileSizeLow);
        }
    } while (FindNextFileA(search,&found));
    FindClose(search);
}
static void vector_at(DWORD address, DWORD count, int capture)
{
    DWORD i, pointer;
    int next_is_ir=0;
    char text[4096];
    for (i = 0; i < count && i < 256; ++i) {
        readmem(address + i * 4, &pointer, 4);
        if (!pointer) { fprintf(logfile,"argv-end %lu\n",i); return; }
        string_at(pointer,text,sizeof(text));
        fprintf(logfile,"argv %lu %08lx %s\n",i,pointer,text);
        if (capture && next_is_ir) copy_ir(text);
        next_is_ir=!strcmp(text,"-il");
    }
}
static void captured_environment(DWORD address)
{
    FILE *file;
    string_at(address,compiler_environment,sizeof(compiler_environment));
    fprintf(logfile,"putenv event=%lu %s\n",event_number,compiler_environment);
    if (strncmp(compiler_environment,"MSC_CMD_FLAGS=",14)) ExitProcess(13);
    file=fopen("msc-cmd-flags.bin","wb");
    if (!file) ExitProcess(14);
    fwrite(compiler_environment,1,strlen(compiler_environment)+1,file);
    fclose(file);
}
static void copy_environment_ir(void)
{
    const char *at;
    char prefix[MAX_PATH];
    DWORD length=0;
    at=strstr(compiler_environment,"-il ");
    if (!at) ExitProcess(15);
    at+=4;
    while (*at && *at!=' ') {
        if (length+1>=sizeof(prefix) || *at=='"') ExitProcess(16);
        prefix[length++]=*at++;
    }
    prefix[length]=0;
    copy_ir(prefix);
}
static void save_loaded_file(HANDLE file, const char *name)
{
    HANDLE output;
    BYTE buffer[16384];
    DWORD got,wrote,total=0;
    if (!file || file==INVALID_HANDLE_VALUE) ExitProcess(17);
    if (SetFilePointer(file,0,NULL,FILE_BEGIN)==0xffffffffUL) ExitProcess(18);
    output=CreateFileA(name,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
    if (output==INVALID_HANDLE_VALUE) ExitProcess(19);
    for (;;) {
        if (!ReadFile(file,buffer,sizeof(buffer),&got,NULL)) ExitProcess(20);
        if (!got) break;
        if (!WriteFile(output,buffer,got,&wrote,NULL) || wrote!=got) ExitProcess(21);
        total+=got;
    }
    CloseHandle(output);
    fprintf(logfile,"loaded-file bytes=%lu saved=%s\n",total,name);
}
static void loaded_module(DWORD base,HANDLE file)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_EXPORT_DIRECTORY exports;
    char name[MAX_PATH];
    readmem(base,&dos,sizeof(dos));
    readmem(base+dos.e_lfanew,&nt,sizeof(nt));
    strcpy(name,"<no-exports>");
    if (nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress) {
        readmem(base+nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
            &exports,sizeof(exports));
        string_at(base+exports.Name,name,sizeof(name));
    }
    fprintf(logfile,"load event=%lu name=%s actual=%08lx preferred=%08lx entry=%08lx timestamp=%08lx size=%08lx\n",
        event_number,name,base,nt.OptionalHeader.ImageBase,
        base+nt.OptionalHeader.AddressOfEntryPoint,nt.FileHeader.TimeDateStamp,
        nt.OptionalHeader.SizeOfImage);
    if (nt.FileHeader.TimeDateStamp == 861912360UL &&
        nt.OptionalHeader.SizeOfImage == 634880UL &&
        nt.OptionalHeader.ImageBase == 0x10600000UL &&
        nt.OptionalHeader.AddressOfEntryPoint == 0x1202aUL) {
        save_loaded_file(file,"loaded-c1.bin");
        points[2]=base+nt.OptionalHeader.AddressOfEntryPoint;
        readmem(points[2],saved+2,1);
        fprintf(logfile,"entry-before-patch event=%lu address=%08lx byte=%02x\n",
            event_number,points[2],(unsigned)saved[2]);
        patch(points[2],0xcc);
    }
    if (!_stricmp(name,"MSVCRT.dll")) save_loaded_file(file,"loaded-msvcrt.bin");
    fflush(logfile);
}
int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    DEBUG_EVENT event;
    CONTEXT context;
    DWORD i, status, stack[3];
    int pending=-1;
    char command[8192], text[4096];
    HANDLE thread;
    if (argc != 4) return 2;
    logfile=fopen(argv[3],"wb");
    if (!logfile) return 2;
    sprintf(command,"\"%s\" %s",argv[1],argv[2]);
    memset(&startup,0,sizeof(startup)); startup.cb=sizeof(startup);
    SetEnvironmentVariableA("TMP","."); SetEnvironmentVariableA("TEMP",".");
    if (!CreateProcessA(argv[1],command,NULL,NULL,TRUE,
        DEBUG_ONLY_THIS_PROCESS,NULL,NULL,&startup,&process)) return 3;
    for (;;) {
        if (!WaitForDebugEvent(&event,30000)) return 4;
        ++event_number;
        status=DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            child=event.u.CreateProcessInfo.hProcess;
            thread_ids[thread_count]=event.dwThreadId;
            threads[thread_count++]=event.u.CreateProcessInfo.hThread;
            fprintf(logfile,"create event=%lu base=%08lx start=%08lx\n",event_number,
                (DWORD)event.u.CreateProcessInfo.lpBaseOfImage,
                (DWORD)event.u.CreateProcessInfo.lpStartAddress);
            for (i=0;i<5;++i) if (points[i]) {
                readmem(points[i],saved+i,1); patch(points[i],0xcc);
            }
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            if (thread_count == 64) return 9;
            thread_ids[thread_count]=event.dwThreadId;
            threads[thread_count++]=event.u.CreateThread.hThread;
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            loaded_module((DWORD)event.u.LoadDll.lpBaseOfDll,event.u.LoadDll.hFile);
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            DWORD code=event.u.Exception.ExceptionRecord.ExceptionCode;
            DWORD address=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            thread=NULL;
            for (i=0;i<thread_count;++i)
                if (thread_ids[i] == event.dwThreadId) thread=threads[i];
            if (!thread) return 10;
            memset(&context,0,sizeof(context)); context.ContextFlags=CONTEXT_FULL;
            if (!GetThreadContext(thread,&context)) return 11;
            if (code == EXCEPTION_SINGLE_STEP && pending >= 0) {
                patch(points[pending],0xcc);
                context.EFlags &= ~0x100;
                if (!SetThreadContext(thread,&context)) return 12;
                pending=-1;
            } else if (code == EXCEPTION_BREAKPOINT) {
                for (i=0;i<5;++i) if (points[i] && address == points[i]) break;
                if (i<5) {
                    readmem(context.Esp+(i==2 ? 4:0),stack,sizeof(stack));
                    fprintf(logfile,"boundary event=%lu address=%08lx esp=%08lx raw=%08lx,%08lx,%08lx\n",
                        event_number,address,context.Esp,stack[0],stack[1],stack[2]);
                    if (i==4) {
                        fprintf(logfile,"invoke-return status=%lu esp=%08lx expected=%08lx\n",context.Eax,context.Esp,invoke_esp+12);
                        if (context.Esp!=invoke_esp+12) ExitProcess(22);
                    } else if (i==0) {
                        invoke_esp=context.Esp;
                        fprintf(logfile,"invoke argc=%lu flag=%lu\n",stack[0],stack[2]);
                        vector_at(stack[1],stack[0]+1,0);
                    } else if (i==1) {
                        string_at(stack[1],text,sizeof(text));
                        fprintf(logfile,"spawn mode=%lu path=%s\n",stack[0],text);
                        vector_at(stack[2],256,0);
                        copy_environment_ir();
                    } else if (i==3) {
                        captured_environment(stack[0]);
                    } else {
                        fprintf(logfile,"dll-entry module=%08lx reason=%lu reserved=%08lx\n",stack[0],stack[1],stack[2]);
                    }
                    fflush(logfile);
                    patch(address,saved[i]);
                    context.Eip=address; context.EFlags|=0x100;
                    if (!SetThreadContext(thread,&context)) return 12;
                    pending=i;
                } else fprintf(logfile,"initial-break event=%lu address=%08lx\n",event_number,address);
            } else {
                fprintf(logfile,"exception %08lx at %08lx first=%lu\n",code,address,event.u.Exception.dwFirstChance);
                fflush(logfile); status=DBG_EXCEPTION_NOT_HANDLED;
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            fprintf(logfile,"exit event=%lu status=%lu\n",event_number,event.u.ExitProcess.dwExitCode);
            fclose(logfile);
            ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
            return event.u.ExitProcess.dwExitCode;
        }
        ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
    }
}
