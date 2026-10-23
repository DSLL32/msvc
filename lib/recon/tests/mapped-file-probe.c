#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <recon.h>
int main(int argc,char **argv)
{
    HANDLE file,mapping; unsigned char *bytes; unsigned n; DWORD size,high; BOOL result;
    for(n=0;n<4;++n) {
        file=CreateFileA("mapped.dat",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file==INVALID_HANDLE_VALUE) return 3;
        if(SetFilePointer(file,2097152,NULL,FILE_BEGIN)!=2097152 || !SetEndOfFile(file)) return 4;
        errno=(n&2)?111:0; SetLastError((n&1)?0x12345678:0);
        size=GetFileSize(file,argc>1 && !strcmp(argv[1],"high")?&high:NULL);
        printf("size %u %lu %d %lu\n",n,size,errno,GetLastError());
        mapping=CreateFileMappingA(file,NULL,PAGE_READWRITE,0,0,NULL);
        bytes=(unsigned char *)MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,0); if(!bytes) return 5;
        bytes[1]=(unsigned char)(n+1); bytes[2097151]=(unsigned char)(n+10);
        errno=(n&2)?111:0; SetLastError((n&1)?0x12345678:0);
        result=FlushViewOfFile(argc>1 && !strcmp(argv[1],"address")?bytes+1:bytes,argc>1 && !strcmp(argv[1],"length")?1:0);
        printf("flush %u %d %d %lu\n",n,result,errno,GetLastError());
        if(!result || !UnmapViewOfFile(bytes) || !CloseHandle(mapping) || !CloseHandle(file)) return 6;
        file=CreateFileA("mapped.dat",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        { unsigned char byte; DWORD got; SetFilePointer(file,1,NULL,FILE_BEGIN); if(!ReadFile(file,&byte,1,&got,NULL) || got!=1 || byte!=n+1) return 7;
          SetFilePointer(file,2097151,NULL,FILE_BEGIN); if(!ReadFile(file,&byte,1,&got,NULL) || got!=1 || byte!=n+10) return 8; }
        CloseHandle(file);
    }
    return 0;
}
