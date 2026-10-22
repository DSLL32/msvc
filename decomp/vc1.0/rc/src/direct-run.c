/* Direct helper tests only, never linked into RC. The debugger supplies a
 * stdcall frame and observes the real return into an entry-point INT3 barrier.
 * RC startup and its CLI are deliberately not run in these labelled cases. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <recon.h>

static HANDLE proof;
static void note(char *format, ...)
{
    char line[512]; DWORD done; va_list ap;
    va_start(ap,format);vsprintf(line,format,ap);va_end(ap);
    WriteFile(proof,line,strlen(line),&done,0);FlushFileBuffers(proof);
}
static void require(BOOL condition, char *operation)
{
    if(!condition) {note("failure=%s error=%lu\n",operation,GetLastError());ExitProcess(20);}
}
static void write_memory(HANDLE process, DWORD address, void *data, DWORD bytes)
{
    DWORD done;
    require(WriteProcessMemory(process,(void *)address,data,bytes,&done) && done==bytes,"write test frame/data");
}
int main(int argc, char **argv)
{
    static struct { char *path, *name; } examples[] = {
        {"", ""}, {"plain.txt", "plain.txt"}, {"one/two\\three.rc", "three.rc"},
        {"C:leaf", "leaf"}, {"C:\\dir\\name.bin", "name.bin"}, {"trailing/", ""}
    };
    STARTUPINFOA startup;PROCESS_INFORMATION target;SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;CONTEXT context;
    HANDLE output,error,results,events;
    DWORD base=0,entry=0,function=0,stack=0,input=0,destination=0,status,code,address,done;
    DWORD frame[3],number=0,limit,argument_bytes,expected,value;
    BYTE trap=0xcc,buffer[96],wanted[96];
    int loader_break=0,entered=0,finished=0,whitespace;
    char command[1024],line[256],proof_path[1024];
    if(argc!=9)return 2;
    whitespace=!strcmp(argv[2],"whitespace");
    limit=whitespace ? 256 : sizeof(examples)/sizeof(examples[0]);
    argument_bytes=whitespace ? 4 : 8;
    sprintf(proof_path,"%s.direct",argv[6]);
    proof=CreateFileA(proof_path,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);
    require(proof!=INVALID_HANDLE_VALUE,"proof");
    note("case=direct-%s entry=debugger supplied ABI frame; no RC CLI\n",argv[2]);
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    output=CreateFileA(argv[7],GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_ALWAYS,0,0);
    error=CreateFileA(argv[8],GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_ALWAYS,0,0);
    results=CreateFileA(argv[5],GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);
    events=CreateFileA(argv[6],GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);
    require(output!=INVALID_HANDLE_VALUE && error!=INVALID_HANDLE_VALUE &&
            results!=INVALID_HANDLE_VALUE && events!=INVALID_HANDLE_VALUE,"output files");
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    sprintf(command,"\"%s\"",argv[1]);
    require(CreateProcessA(argv[1],command,0,0,TRUE,DEBUG_ONLY_THIS_PROCESS,0,0,&startup,&target),"direct target");
    for(;;) {
        require(WaitForDebugEvent(&event,30000),"debug event");status=DBG_CONTINUE;
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            function=base+strtoul(argv[3],0,16);entry=base+strtoul(argv[4],0,16);
            /* Entry remains a test return barrier. No original instruction is
             * replaced by implementation code; the helper body remains masked
             * or accepted exactly as recorded by the ordinary paired harness. */
            write_memory(target.hProcess,entry,&trap,1);FlushInstructionCache(target.hProcess,(void *)entry,1);
            if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);
        } else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            code=event.u.Exception.ExceptionRecord.ExceptionCode;
            address=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if(code==EXCEPTION_BREAKPOINT && !loader_break)loader_break=1;
            else if(code==EXCEPTION_BREAKPOINT && address==entry && !finished) {
                require(event.dwThreadId==target.dwThreadId,"primary helper thread");
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_CONTROL|CONTEXT_INTEGER;
                require(GetThreadContext(target.hThread,&context),"direct context");
                if(!entered) {
                    stack=context.Esp-0x80;input=context.Esp-0x400;destination=context.Esp-0x300;
                    entered=1;
                    note("function_rva=%08lx argument_bytes=%lu\n",function-base,argument_bytes);
                } else {
                    expected=whitespace ? (number==255 || number==9 || number==10 || number==13 || number==32 ? 0xffffffffUL : 0) : strlen(examples[number].name)+1;
                    require(context.Eax==expected,"native return value");
                    if(number==0) {
                        require(context.Esp==stack+4+argument_bytes,"stdcall callee cleanup");
                        require(context.Ebx==0x13579bdf && context.Esi==0x2468ace0 &&
                                context.Edi==0x11223344 && context.Ebp==0x55667788,"nonvolatile registers");
                        note("ABI cleanup=%lu nonvolatile=EBX,ESI,EDI,EBP preserved\n",argument_bytes);
                    }
                    value=context.Eax;require(WriteFile(results,&value,4,&done,0)&&done==4,"write return result");
                    if(!whitespace) {
                        require(ReadProcessMemory(target.hProcess,(void *)destination,buffer,sizeof(buffer),&done)&&done==sizeof(buffer),"read guarded destination");
                        memset(wanted,0xa5,sizeof(wanted));memcpy(wanted+8,examples[number].name,expected);
                        require(!memcmp(buffer,wanted,sizeof(buffer)),"basename output and canaries");
                        require(WriteFile(results,buffer,sizeof(buffer),&done,0)&&done==sizeof(buffer),"write guarded result");
                    }
                    if(++number==limit) {
                        note("passed=%lu output_canaries=%s\n",number,whitespace ? "not applicable" : "preserved");
                        finished=1;require(TerminateProcess(target.hProcess,0),"finish direct test");
                        ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);continue;
                    }
                }
                frame[0]=entry;
                /* Nonzero high argument bytes prove the signed low byte is
                 * the whole input domain, including ff representing EOF. */
                frame[1]=whitespace ? 0xa55a0000UL|number : input;
                frame[2]=destination+8;
                if(!whitespace) {
                    write_memory(target.hProcess,input,examples[number].path,strlen(examples[number].path)+1);
                    memset(buffer,0xa5,sizeof(buffer));write_memory(target.hProcess,destination,buffer,sizeof(buffer));
                }
                write_memory(target.hProcess,stack,frame,4+argument_bytes);
                context.Eip=function;context.Esp=stack;context.EFlags&=~0x400UL;
                context.Ebx=0x13579bdf;context.Esi=0x2468ace0;context.Edi=0x11223344;context.Ebp=0x55667788;
                require(SetThreadContext(target.hThread,&context),"supply direct stdcall frame");
            } else {
                sprintf(line,"exception %08lx rva %08lx base %08lx first %lu\n",code,address-base,base,event.u.Exception.dwFirstChance);
                WriteFile(events,line,strlen(line),&done,0);TerminateProcess(target.hProcess,99);
            }
        } else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            sprintf(line,"exit %lu\n",event.u.ExitProcess.dwExitCode);WriteFile(events,line,strlen(line),&done,0);
            ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);break;
        }
        ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
    }
    CloseHandle(target.hThread);CloseHandle(target.hProcess);CloseHandle(results);CloseHandle(events);
    CloseHandle(output);CloseHandle(error);CloseHandle(proof);return 0;
}
