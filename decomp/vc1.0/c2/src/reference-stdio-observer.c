/* Small debugger used only by the differential harness. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <recon.h>

/* Observation-only breakpoints in otherwise untouched original CRT code. */
typedef struct ObservedCall {
    DWORD rva, nargs, return_address, args[5];
    unsigned char first, return_byte;
    int entered, returned;
    char name[80];
} ObservedCall;
static ObservedCall observed_calls[32];
static unsigned observed_count;
static HANDLE observation_log;
static DWORD image_base;
static DWORD read_word(HANDLE process,DWORD address)
{
    DWORD value,got;
    if(!ReadProcessMemory(process,(void*)address,&value,4,&got)||got!=4)exit(241);
    return value;
}
static void write_byte(HANDLE process,DWORD address,unsigned char value)
{
    DWORD wrote;
    if(!WriteProcessMemory(process,(void*)address,&value,1,&wrote)||wrote!=1)exit(242);
    FlushInstructionCache(process,(void*)address,1);
}
static void log_line(char *line)
{
    DWORD wrote;
    if(!WriteFile(observation_log,line,strlen(line),&wrote,0)||wrote!=strlen(line))exit(243);
}
static void snapshot(HANDLE process,ObservedCall *call,char *stage,DWORD eax)
{
    DWORD fields[8],got,i,used,position,high=0,handle,counter=0,cursor=0,value=0;
    HANDLE copy;char line[10000],hex[8193];unsigned char bytes[4096];
    if(!ReadProcessMemory(process,(void*)(image_base+0x761b8),fields,sizeof(fields),&got)||got!=sizeof(fields))exit(244);
    used=fields[0]&&fields[2]?fields[0]-fields[2]:0;
    if(used>4096)exit(245);
    if(used&&(!ReadProcessMemory(process,(void*)fields[2],bytes,used,&got)||got!=used))exit(246);
    for(i=0;i<used;++i)sprintf(hex+2*i,"%02x",bytes[i]);hex[2*used]=0;
    handle=read_word(process,image_base+0x767b8+4);
    if(!DuplicateHandle(process,(HANDLE)handle,GetCurrentProcess(),&copy,0,FALSE,DUPLICATE_SAME_ACCESS))exit(247);
    position=SetFilePointer(copy,0,(LONG*)&high,FILE_CURRENT);CloseHandle(copy);
    if(high||position==0xffffffff)exit(248);
    if(!strcmp(call->name,"_write_char")||!strcmp(call->name,"_write_multi_char")||!strcmp(call->name,"_write_string"))
        counter=read_word(process,call->args[call->nargs-1]);
    if(!strcmp(call->name,"_get_int_arg")||!strcmp(call->name,"_get_short_arg")) {
        cursor=read_word(process,call->args[0]);value=read_word(process,cursor);
    }
    sprintf(line,"%s %s eax %08lx args",stage,call->name,eax);log_line(line);
    for(i=0;i<call->nargs;++i){sprintf(line," %08lx",call->args[i]);log_line(line);}
    sprintf(line,"\nfile flags %lu count %ld fd %lu size %lu null %lu used %lu position %lu counter %lu cursor %08lx value %08lx\nbuffer_hex %s\n",
        fields[3],(long)fields[1],fields[4],fields[6],!fields[0]&&!fields[2],used,position,counter,cursor,value,hex);log_line(line);
}
static void install_plan(HANDLE process,DWORD base)
{
    char path[1024];FILE *plan;unsigned long rva,first,nargs;
    image_base=base;
    if(!GetEnvironmentVariableA("C2_CRT_CAPTURE",path,sizeof(path)))exit(249);
    observation_log=CreateFileA(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    if(!GetEnvironmentVariableA("C2_CRT_PLAN",path,sizeof(path)))exit(250);
    plan=fopen(path,"r");if(!plan)exit(251);
    while(observed_count<32&&fscanf(plan,"%lx %lx %lu %79s",&rva,&first,&nargs,observed_calls[observed_count].name)==4){
        ObservedCall *call=&observed_calls[observed_count++];
        call->rva=rva;call->first=(unsigned char)first;call->nargs=nargs;
        if(nargs>5||(read_word(process,base+rva)&255)!=first)exit(252);
        write_byte(process,base+rva,0xcc);
    }
    fclose(plan);if(!observed_count)exit(253);
}
static void capture_crt(HANDLE process,HANDLE thread,DWORD base,DWORD address)
{
    unsigned i;ObservedCall *call=0;DWORD got;CONTEXT c;
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(!GetThreadContext(thread,&c))exit(254);
    for(i=0;i<observed_count;++i)if(observed_calls[i].entered&&!observed_calls[i].returned&&observed_calls[i].return_address==address){call=&observed_calls[i];break;}
    if(call){
        snapshot(process,call,"return",c.Eax);call->returned=1;
        if(!strcmp(call->name,"_printf")){CloseHandle(observation_log);TerminateProcess(process,99);return;}
        write_byte(process,address,call->return_byte);
    }else{
        for(i=0;i<observed_count;++i)if(!observed_calls[i].entered&&base+observed_calls[i].rva==address){call=&observed_calls[i];break;}
        if(!call)exit(255);
        call->entered=1;call->return_address=read_word(process,c.Esp);
        if(!ReadProcessMemory(process,(void*)(c.Esp+4),call->args,call->nargs*4,&got)||got!=call->nargs*4)exit(256);
        call->return_byte=(unsigned char)read_word(process,call->return_address);
        snapshot(process,call,"enter",c.Eax);
        write_byte(process,address,call->first);write_byte(process,call->return_address,0xcc);
    }
    c.Eip=address;if(!SetThreadContext(thread,&c))exit(257);
}

int main(int argc, char **argv)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DEBUG_EVENT ev;
    HANDLE out, err, log;
    DWORD base = 0, written, status, code;
    char command[2048], line[256], environment[4096];
    DWORD environment_length;
    int loader_break = 0;

    if (argc != 6 && argc != 7) return 2;
    memset(&si, 0, sizeof(si));
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    out = CreateFileA(argv[3], GENERIC_WRITE, FILE_SHARE_READ, &sa,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    err = CreateFileA(argv[4], GENERIC_WRITE, FILE_SHARE_READ, &sa,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    log = CreateFileA(argv[5], GENERIC_WRITE, FILE_SHARE_READ, 0,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = out;
    si.hStdError = err;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    sprintf(command, "\"%s\" %s", argv[1], argv[2]);
    strcpy(environment, "MSC_CMD_FLAGS=");
    environment_length = GetEnvironmentVariableA("MSC_CMD_FLAGS", environment + 14, sizeof(environment) - 15);
    if (!environment_length || environment_length >= sizeof(environment)-15) return 5;
    environment[14 + environment_length + 1] = 0;
    if (!CreateProcessA(argv[1], command, 0, 0, TRUE,
                        DEBUG_ONLY_THIS_PROCESS, environment, argc == 7 ? argv[6] : 0, &si, &pi)) {
        sprintf(line, "create-error %lu\n", GetLastError());
        WriteFile(log, line, strlen(line), &written, 0);
        return 3;
    }
    for (;;) {
        if (!WaitForDebugEvent(&ev, 30000)) return 4;
        status = DBG_CONTINUE;
        if (ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)ev.u.CreateProcessInfo.lpBaseOfImage;
            install_plan(pi.hProcess,base);
            if (ev.u.CreateProcessInfo.hFile)
                CloseHandle(ev.u.CreateProcessInfo.hFile);
        } else if (ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (ev.u.LoadDll.hFile) CloseHandle(ev.u.LoadDll.hFile);
        } else if (ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = ev.u.Exception.ExceptionRecord.ExceptionCode;
            if (code == EXCEPTION_BREAKPOINT && !loader_break) {
                loader_break = 1;
            } else {
                sprintf(line, "exception %08lx rva %08lx base %08lx first %lu\n",
                        code, (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress - base,
                        base, ev.u.Exception.dwFirstChance);
                WriteFile(log, line, strlen(line), &written, 0);
                if(code!=EXCEPTION_BREAKPOINT) return 224;
                capture_crt(pi.hProcess,pi.hThread,base,(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress);
            }
        } else if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            sprintf(line, "exit %lu\n", ev.u.ExitProcess.dwExitCode);
            WriteFile(log, line, strlen(line), &written, 0);
            ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, status);
            break;
        }
        ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, status);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(out);
    CloseHandle(err);
    CloseHandle(log);
    return 0;
}
