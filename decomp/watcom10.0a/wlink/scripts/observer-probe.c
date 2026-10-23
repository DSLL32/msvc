/* Authored Win32 debugger child only. _KERNEL32_ suppresses dllimport so the
 * linker emits the same real FF25 import-transfer form used by WLINK. */
#define _KERNEL32_
#include <windows.h>
#include <string.h>
#include <recon.h>
typedef DWORD (__cdecl *ProbeCallback)(DWORD);
extern DWORD __stdcall ProbeValue(DWORD, ProbeCallback);
extern DWORD __stdcall ProbeBadReturn(DWORD, DWORD);
extern __declspec(dllimport) volatile DWORD fixture_dll_entries;
extern __declspec(dllimport) volatile DWORD fixture_bad_return_entries;
__declspec(dllexport) volatile DWORD fixture_stage;
__declspec(dllexport) volatile DWORD fixture_callback_entries;
__declspec(dllexport) volatile DWORD fixture_thread_entries;
static unsigned recursion;
static int overlap_reentry;
static char *saved_marker;
int __cdecl main(int, char **);

static DWORD __cdecl invoke_probe(DWORD value);
static DWORD __cdecl recursive_callback(DWORD value)
{
    DWORD result;
    ++fixture_callback_entries; fixture_stage=31; recursion=1;
    result=invoke_probe(value+1);
    recursion=0;
    return result;
}
static DWORD __cdecl invoke_probe(DWORD value)
{
    return ProbeValue(value,recursion ? NULL : recursive_callback);
}
static DWORD WINAPI extra_thread(void *unused)
{
    unused=unused;
    ++fixture_thread_entries;
    return 0;
}

static __declspec(naked) void authored_trap(void)
{
    __asm { int 3 }
    __asm { ret }
}
static int trap_filter(EXCEPTION_POINTERS *info)
{
    return info->ExceptionRecord->ExceptionCode==EXCEPTION_BREAKPOINT &&
        info->ExceptionRecord->ExceptionAddress==(void *)authored_trap ?
        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

static void complete(const char *path)
{
    HANDLE marker;
    DWORD count,values[5];
    values[0]=fixture_stage; values[1]=fixture_callback_entries;
    values[2]=fixture_dll_entries; values[3]=fixture_bad_return_entries;
    values[4]=fixture_thread_entries;
    marker=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
    if(marker==INVALID_HANDLE_VALUE || !WriteFile(marker,"DONE",4,&count,NULL) || count!=4 ||
       !WriteFile(marker,values,sizeof(values),&count,NULL) || count!=sizeof(values) || !CloseHandle(marker)) ExitProcess(121);
    ExitProcess(0);
}

/* Actual stdcall input frame with a caller return equal to the observed main
 * entry, an existing breakpoint role. It must be rejected before ProbeValue
 * executes. Assembly defines this intentionally unsupported authored frame. */
static __declspec(naked) void return_overlap(void)
{
    __asm {
        push 0
        push 1
        push offset main
        jmp ProbeValue
    }
}

int __cdecl main(int argc, char **argv)
{
    HANDLE output,input,thread;
    DWORD count,mode=0x12345678UL,pid,start;
    char buffer[16];
    if(overlap_reentry) { fixture_stage=51; complete(saved_marker); }
    fixture_stage=1;
    if(argc!=3) return 110;
    saved_marker=argv[2];
    pid=GetCurrentProcessId();
    if(!pid || !GetCommandLineA()) return 111;
    output=GetStdHandle(STD_OUTPUT_HANDLE);
    if(GetFileType(output)!=FILE_TYPE_DISK) return 112;
    GetConsoleMode(output,&mode);
    SetConsoleMode(output,mode);
    input=CreateFileA("input.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if(input==INVALID_HANDLE_VALUE) return 113;
    count=99;
    if(!ReadFile(input,buffer,sizeof(buffer),&count,NULL) || count!=3 || memcmp(buffer,"abc",3)) return 114;
    count=99;
    if(!ReadFile(input,buffer,sizeof(buffer),&count,NULL) || count!=0 || !CloseHandle(input)) return 115;
    if(!WriteFile(output,"probe\n",6,&count,NULL) || count!=6) return 116;
    if(!WriteFile(output,"probe\n",6,&count,NULL) || count!=6) return 117;
    fixture_stage=20;
    if(!strcmp(argv[1],"thread")) {
        fixture_stage=70;
        thread=CreateThread(NULL,0,extra_thread,NULL,0,NULL);
        if(!thread) return 118;
        WaitForSingleObject(thread,5000); CloseHandle(thread);
    } else if(!strcmp(argv[1],"trap")) {
        fixture_stage=60;
        __try { authored_trap(); }
        __except(trap_filter(GetExceptionInformation())) { fixture_stage=61; }
    } else if(!strcmp(argv[1],"timeout")) {
        fixture_stage=80; start=GetTickCount();
        while(GetTickCount()-start<5000) { }
        fixture_stage=81;
    } else if(!strcmp(argv[1],"recursive")) {
        fixture_stage=30;
        invoke_probe(1);
        fixture_stage=32;
    } else if(!strcmp(argv[1],"return-overlap")) {
        fixture_stage=50; overlap_reentry=1;
        return_overlap();
    } else if(!strcmp(argv[1],"bad-return")) {
        fixture_stage=40;
        ProbeBadReturn(2,3);
        fixture_stage=41;
    } else if(strcmp(argv[1],"positive")) return 119;
    fixture_stage=90;
    if(ProbeValue(1,NULL)!=8 || ProbeValue(2,NULL)!=9) return 120;
    fixture_stage=99; complete(argv[2]);
    return 122;
}
