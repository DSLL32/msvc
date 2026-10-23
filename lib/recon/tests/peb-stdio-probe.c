/* Explicit original-only Win32 observation; never part of native fixtures. */
#include <windows.h>
#include <stdio.h>
#include <recon.h>
typedef LONG (WINAPI *QueryProcess)(HANDLE,ULONG,void *,ULONG,ULONG *);
typedef struct BasicInfo { LONG status; void *peb; ULONG affinity; LONG priority; ULONG pid,parent; } BasicInfo;
int main(void)
{
    QueryProcess query; BasicInfo basic; ULONG length; void *parameters;
    HANDLE current,sink,old; unsigned n;
    query=(QueryProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"),"NtQueryInformationProcess");
    if(!query || query(GetCurrentProcess(),0,&basic,sizeof(basic),&length) || length!=24) return 3;
    parameters=*(void **)((char *)basic.peb+0x10);
    for(n=0;n<3;++n) {
        current=GetStdHandle((DWORD)(-10-(int)n));
        printf("initial %u %lu %lu\n",n,(DWORD)current,(DWORD)*(HANDLE *)((char *)parameters+0x18+n*4));
        if(current!=*(HANDLE *)((char *)parameters+0x18+n*4)) return 4;
    }
    old=GetStdHandle(STD_OUTPUT_HANDLE);
    sink=CreateFileA("sink.bin",GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(sink==INVALID_HANDLE_VALUE || !SetStdHandle(STD_OUTPUT_HANDLE,sink)) return 5;
    printf("changed %lu %lu\n",(DWORD)GetStdHandle(STD_OUTPUT_HANDLE),(DWORD)*(HANDLE *)((char *)parameters+0x1c));
    if(GetStdHandle(STD_OUTPUT_HANDLE)!=sink || *(HANDLE *)((char *)parameters+0x1c)!=sink) return 6;
    if(!SetStdHandle(STD_OUTPUT_HANDLE,old) || !CloseHandle(sink)) return 7;
    printf("restored %lu %lu\n",(DWORD)GetStdHandle(STD_OUTPUT_HANDLE),(DWORD)*(HANDLE *)((char *)parameters+0x1c));
    return GetStdHandle(STD_OUTPUT_HANDLE)==old && *(HANDLE *)((char *)parameters+0x1c)==old?0:8;
}
