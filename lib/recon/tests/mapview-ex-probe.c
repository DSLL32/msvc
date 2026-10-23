#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <recon.h>
int main(int argc,char **argv)
{
    HANDLE file,mapping;unsigned char *bytes;DWORD written;unsigned n;int error;DWORD last;
    file=CreateFileA("mapped.dat",GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE || !WriteFile(file,"seed",4,&written,NULL))return 3;
    mapping=CreateFileMappingA(file,NULL,PAGE_READWRITE,0,2097152,NULL);if(!mapping)return 4;
    errno=111;SetLastError(123);
    bytes=MapViewOfFileEx(mapping,FILE_MAP_ALL_ACCESS,0,0,0,argc>1?(void*)0x30000000:NULL);error=errno;last=GetLastError();
    printf("mapped=%d errno=%d last=%lu\n",bytes!=NULL,error,last);
    if(!bytes || memcmp(bytes,"seed",4))return 5;
    for(n=4;n<2097152;++n)if(bytes[n])return 6;
    if(!CloseHandle(mapping))return 7;
    bytes[0]='S';bytes[2097151]=0x79;
    if(!FlushViewOfFile(bytes,0) || !UnmapViewOfFile(bytes) || !CloseHandle(file))return 8;
    printf("zero-extension/view-survives-close/persistence=1\n");return 0;
}
