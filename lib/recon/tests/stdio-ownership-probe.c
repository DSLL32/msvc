#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <errno.h>
#include <recon.h>
extern unsigned char _osfile[];
extern long _osfhnd[];
extern int __cdecl _alloc_osfhnd(void);
extern int __cdecl _set_osfhnd(int,long);
extern int __cdecl _free_osfhnd(int);
int main(void)
{
    FILE *report,*result; HANDLE manual; DWORD written,last; int closed,error,fd,value;
    report=fopen("report.txt","wt");if(!report) return 3;
    fprintf(report,"before %d %u %d %d\n",stdout->_file,_osfile[1],stdout->_flag,GetStdHandle(STD_OUTPUT_HANDLE)==(HANDLE)_osfhnd[1]);
    errno=111;SetLastError(123);closed=fclose(stdout);error=errno;last=GetLastError();
    fprintf(report,"closed %d %d %lu %d %d %d %d %u\n",closed,error,last,stdout->_file,stdout->_flag,GetStdHandle(STD_OUTPUT_HANDLE)==NULL,_osfhnd[1]==-1,_osfile[1]);
    errno=111;SetLastError(123);fd=_alloc_osfhnd();error=errno;last=GetLastError();
    fprintf(report,"allocated %d %u %d %d %lu\n",fd,_osfile[fd],_osfhnd[fd]==-1,error,last);
    manual=CreateFileA("manual.bin",GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(manual==INVALID_HANDLE_VALUE) return 4;
    errno=111;SetLastError(123);value=_set_osfhnd(fd,(long)manual);error=errno;last=GetLastError();
    fprintf(report,"set %d %d %d %d %lu\n",value,_osfhnd[fd]==(long)manual,GetStdHandle(STD_OUTPUT_HANDLE)==manual,error,last);
    _osfile[fd]=0x81;errno=111;SetLastError(123);value=_free_osfhnd(fd);error=errno;last=GetLastError();
    fprintf(report,"freed %d %u %d %d %d %lu\n",value,_osfile[fd],_osfhnd[fd]==-1,GetStdHandle(STD_OUTPUT_HANDLE)==NULL,error,last);
    if(!WriteFile(manual,"live",4,&written,NULL) || written!=4) return 5;
    _osfile[fd]=0;errno=111;SetLastError(123);value=CloseHandle(manual);error=errno;last=GetLastError();
    fprintf(report,"winclose %d %d %lu\n",value,error,last);
    result=freopen("listing.cod","wt",stdout);
    fprintf(report,"reopened %d %d %u %d %d\n",result==stdout,stdout->_file,_osfile[1],stdout->_flag,GetStdHandle(STD_OUTPUT_HANDLE)==(HANDLE)_osfhnd[1]);
    if(!result || fputs("listing\n",stdout)==EOF) return 6;
    return fclose(report)?7:0;
}
