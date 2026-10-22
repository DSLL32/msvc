/* Small controlled workload for debugger failure-path checks. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
static volatile int value;
void phase_a(void) { value=3; }
void phase_b(void) { value+=4; }
static DWORD WINAPI thread_body(void *unused) { return 0; }
int main(int argc,char **argv)
{
    char mode[32],path[MAX_PATH],command[MAX_PATH+16];DWORD id;
    STARTUPINFOA startup;PROCESS_INFORMATION child;
    if(argc>1) return 0;
    GetEnvironmentVariableA("REPLAY_TEST_MODE",mode,sizeof(mode));
    if(!strcmp(mode,"early"))ExitProcess(7);
    if(!strcmp(mode,"fault"))*(volatile DWORD *)0=1;
    if(!strcmp(mode,"wait"))Sleep(10000);
    if(!strcmp(mode,"thread")) {
        CreateThread(0,0,thread_body,0,0,&id);Sleep(1000);
    }
    if(!strcmp(mode,"child")) {
        GetModuleFileNameA(0,path,sizeof(path));
        sprintf(command,"\"%s\" leaf",path);
        memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);
        if(!CreateProcessA(path,command,0,0,FALSE,0,0,0,&startup,&child))return 9;
        WaitForSingleObject(child.hProcess,1000);
    }
    phase_a();phase_b();
    puts(!strcmp(mode,"bad-output")?"bad":"ok");
    return value==7?0:1;
}
