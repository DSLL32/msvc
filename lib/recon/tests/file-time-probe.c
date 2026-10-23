#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <recon.h>
int main(int argc,char **argv)
{
    SYSTEMTIME time; FILETIME stamp; unsigned n; BOOL result; HANDLE file;
    memset(&time,0,sizeof(time)); time.wYear=2026;time.wMonth=10;time.wDay=23;time.wHour=13;time.wMinute=8;time.wSecond=44;time.wMilliseconds=127;
    file=CreateFileA("clock.dat",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE) return 3;
    for(n=0;n<4;++n) {
        errno=(n&2)?111:0;SetLastError((n&1)?0x12345678:0);
#ifdef RECON_NATIVE_TEST
        if(argc>1 && !strcmp(argv[1],"invalid")) time.wMonth=13;
#endif
        result=SystemTimeToFileTime(&time,&stamp);
        printf("convert %u %d %lu %lu %d %lu\n",n,result,stamp.dwLowDateTime,stamp.dwHighDateTime,errno,GetLastError());
        errno=(n&2)?111:0;SetLastError((n&1)?0x12345678:0);
        result=SetFileTime(file,argc>1 && !strcmp(argv[1],"create")?&stamp:NULL,NULL,&stamp);
        printf("set %u %d %d %lu\n",n,result,errno,GetLastError());
#ifndef RECON_NATIVE_TEST
        { FILETIME actual; if(!GetFileTime(file,NULL,NULL,&actual) || memcmp(&actual,&stamp,sizeof(actual))) return 4; }
#endif
    }
    CloseHandle(file);
    memset(&time,0,sizeof(time));time.wMonth=time.wDay=1;
    time.wYear=1601;SystemTimeToFileTime(&time,&stamp);printf("epoch %lu %lu\n",stamp.dwLowDateTime,stamp.dwHighDateTime);
    time.wYear=1970;SystemTimeToFileTime(&time,&stamp);printf("unix %lu %lu\n",stamp.dwLowDateTime,stamp.dwHighDateTime);
    time.wYear=2000;time.wMonth=2;time.wDay=29;SystemTimeToFileTime(&time,&stamp);printf("leap %lu %lu\n",stamp.dwLowDateTime,stamp.dwHighDateTime);
    return 0;
}
