/* Explicit original-only provenance check. No target writes or breakpoints.
 * Run with the original VC5 toolchain; never used by native fixtures. */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static HANDLE child;
static FILE *events;
static int verified;
static void fail(const char *reason)
{
    if(events){fprintf(events,"failure %s\n",reason);fflush(events);}
    if(child)TerminateProcess(child,125);
    ExitProcess(125);
}
static void read_memory(DWORD address,void *data,DWORD size)
{
    DWORD got;
    if(!ReadProcessMemory(child,(void*)address,data,size,&got)||got!=size)fail("read-memory");
}
static void verify_file(HANDLE actual,const char *expected,const char *output)
{
    BYTE left[16384],right[16384];DWORD a,b,written,total=0;
    HANDLE reference,saved;
    if(!actual||actual==INVALID_HANDLE_VALUE)fail("missing-hfile");
    reference=CreateFileA(expected,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    saved=CreateFileA(output,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
    if(reference==INVALID_HANDLE_VALUE||saved==INVALID_HANDLE_VALUE)fail("open-proof");
    if(SetFilePointer(actual,0,NULL,FILE_BEGIN)==0xffffffff)fail("seek-hfile");
    for(;;){
        if(!ReadFile(actual,left,sizeof(left),&a,NULL)||!ReadFile(reference,right,sizeof(right),&b,NULL))fail("read-hfile");
        if(a!=b||memcmp(left,right,a))fail("actual-hfile-differs");
        if(!a)break;
        if(!WriteFile(saved,left,a,&written,NULL)||written!=a)fail("save-hfile");
        total+=a;
    }
    CloseHandle(reference);CloseHandle(saved);
    fprintf(events,"verified %s %lu\n",output,total);fflush(events);
}
static void loaded(DWORD address,HANDLE file,const char *expected)
{
    IMAGE_DOS_HEADER dos;IMAGE_NT_HEADERS nt;IMAGE_EXPORT_DIRECTORY table;
    char name[80];unsigned n;
    read_memory(address,&dos,sizeof(dos));
    if(dos.e_magic!=IMAGE_DOS_SIGNATURE)fail("dos-signature");
    read_memory(address+dos.e_lfanew,&nt,sizeof(nt));
    if(nt.Signature!=IMAGE_NT_SIGNATURE)fail("pe-signature");
    if(!nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress)return;
    read_memory(address+nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,&table,sizeof(table));
    if(table.Name>=nt.OptionalHeader.SizeOfImage)fail("export-name-range");
    for(n=0;n<sizeof(name)-1;++n){read_memory(address+table.Name+n,name+n,1);if(!name[n])break;}
    name[sizeof(name)-1]=0;
    if(!_stricmp(name,"MSVCRT.dll")){
        if(verified)fail("duplicate-provider");
        verify_file(file,expected,"loaded-crt.bin");verified=1;
    }
}
int main(int argc,char **argv)
{
    STARTUPINFOA startup;PROCESS_INFORMATION process;DEBUG_EVENT event;
    char command[2048];DWORD status;
    if(argc!=4||strlen(argv[1])+strlen(argv[2])+4>=sizeof(command))return 125;
    events=fopen("verification.txt","wb");if(!events)return 125;
    sprintf(command,"\"%s\" %s",argv[1],argv[2]);
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);
    if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,NULL,&startup,&process))fail("create-process");
    for(;;){
        if(!WaitForDebugEvent(&event,30000))fail("debug-wait");status=DBG_CONTINUE;
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){
            child=event.u.CreateProcessInfo.hProcess;
            verify_file(event.u.CreateProcessInfo.hFile,argv[1],"loaded-probe.bin");
            CloseHandle(event.u.CreateProcessInfo.hFile);
        }else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT){
            loaded((DWORD)event.u.LoadDll.lpBaseOfDll,event.u.LoadDll.hFile,argv[3]);
            if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);
        }else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT){
            if(event.u.Exception.ExceptionRecord.ExceptionCode!=EXCEPTION_BREAKPOINT)status=DBG_EXCEPTION_NOT_HANDLED;
        }else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){
            if(!verified)fail("provider-not-observed");
            fprintf(events,"exit %lu\n",event.u.ExitProcess.dwExitCode);fclose(events);
            ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
            return event.u.ExitProcess.dwExitCode;
        }
        if(!ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status))fail("continue-debug");
    }
}
