/* Authored channel probe only: no target creation or application reconstruction. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAP_BYTES=16384, MAX_PAYLOAD=4096, PAYLOAD=256, MAX_SEQUENCE=1000000 };
static BYTE *area;
static HANDLE file_handle, mapping_handle;
static DWORD load(unsigned offset)
{
    return *(volatile DWORD *)(area+offset);
}
static void publish(unsigned offset,DWORD value)
{
    InterlockedExchange((LONG *)(area+offset),(LONG)value);
}
static int finish(int status,const char *reason,DWORD seen)
{
    printf("status=%d reason=%s processed=%lu pointer_bits=%u\n",status,reason,
           seen,(unsigned)(sizeof(void *)*8));
    if(area) UnmapViewOfFile(area);
    if(mapping_handle) CloseHandle(mapping_handle);
    if(file_handle!=INVALID_HANDLE_VALUE) CloseHandle(file_handle);
    return status;
}
int main(int argc,char **argv)
{
    DWORD seen=0,lease,command,length,operation,i,high=0;
    DWORD start,last_lease,deadline_ms,lease_ms,current,next_lease;
    const char *mode;
    BYTE copy[MAX_PAYLOAD];
    file_handle=INVALID_HANDLE_VALUE;
    if(argc!=5) return 90;
    mode=argv[2];deadline_ms=strtoul(argv[3],NULL,10);lease_ms=strtoul(argv[4],NULL,10);
    if(deadline_ms<1 || deadline_ms>300000 || lease_ms<1 || lease_ms>10000) return 91;
    file_handle=CreateFileA(argv[1],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
                            NULL,OPEN_EXISTING,0,NULL);
    if(file_handle==INVALID_HANDLE_VALUE) return finish(92,"open_map_file",seen);
    if(GetFileSize(file_handle,&high)!=MAP_BYTES || high) return finish(92,"map_size",seen);
    mapping_handle=CreateFileMappingA(file_handle,NULL,PAGE_READWRITE,0,MAP_BYTES,NULL);
    if(!mapping_handle) return finish(93,"create_mapping",seen);
    area=MapViewOfFile(mapping_handle,FILE_MAP_READ|FILE_MAP_WRITE,0,0,MAP_BYTES);
    if(!area || ((DWORD)area&3)) return finish(93,"map_alignment",seen);
    if(load(0)!=0x50434757UL || load(4)!=1 || load(8)!=MAP_BYTES)
        return finish(94,"header",seen);
    lease=load(20);
    if(!lease || lease>MAX_SEQUENCE) return finish(75,"initial_lease",seen);
    start=last_lease=GetTickCount();
    while(1) {
        current=GetTickCount();next_lease=load(20);
        if(next_lease!=lease) {
            if(next_lease<=lease || next_lease>MAX_SEQUENCE)
                return finish(75,"lease_sequence",seen);
            lease=next_lease;last_lease=current;
        }
        if(current-start>=deadline_ms) return finish(73,"global_deadline",seen);
        if(current-last_lease>=lease_ms) return finish(74,"owner_lease",seen);
        if(load(0)!=0x50434757UL || load(4)!=1 || load(8)!=MAP_BYTES)
            return finish(94,"header",seen);
        command=load(12);
        if(command==seen) {Sleep(1);continue;}
        if(command!=seen+1 || command>MAX_SEQUENCE || load(16)!=seen)
            return finish(76,"command_sequence",seen);
        operation=load(24);length=load(28);
        if(length>MAX_PAYLOAD) return finish(77,"command_length",seen);
        memcpy(copy,area+PAYLOAD,length);
        if(load(12)!=command || load(24)!=operation || load(28)!=length)
            return finish(78,"command_changed",seen);
        if(operation!=1 && operation!=2) {
            *(DWORD *)(area+32)=79;*(DWORD *)(area+36)=0;
            publish(16,command);return finish(79,"unknown_operation",seen);
        }
        if(operation==2) {
            *(DWORD *)(area+32)=0;*(DWORD *)(area+36)=0;
            publish(16,command);return finish(0,"requested_exit",command);
        }
        /* Deliberate faults belong to this disposable IPC probe only. */
        if(!strcmp(mode,"late")) Sleep(250);
        if(!strcmp(mode,"partial")) {
            memset(area+PAYLOAD,0x5a,length);
            *(DWORD *)(area+32)=0;*(DWORD *)(area+36)=length;Sleep(50);
        }
        for(i=0;i<length;i++) area[PAYLOAD+i]=copy[i]^0xa5;
        *(DWORD *)(area+32)=0;
        *(DWORD *)(area+36)=!strcmp(mode,"oversized")?MAX_PAYLOAD+1:length;
        if(!strcmp(mode,"header")) *(DWORD *)area=0;
        publish(16,!strcmp(mode,"skip")?command+1:command);
        seen=command;
    }
}
