#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#include <recon.h>
extern unsigned long _doserrno;
static void show(const char *name,int fd)
{
    struct _stat info;unsigned n;int result,error;DWORD last;unsigned long dos;
    memset(&info,0xa5,sizeof(info));errno=111;_doserrno=77;SetLastError(123);
    result=_fstat(fd,&info);error=errno;dos=_doserrno;last=GetLastError();
    printf("%s %d %d %lu %lu ",name,result,error,dos,last);
    for(n=0;n<sizeof(info);++n)printf("%02x",((unsigned char*)&info)[n]);printf("\n");
}
int main(void)
{
    int fd=_open("input.bin",_O_RDONLY|_O_BINARY);if(fd<0)return 3;
    show("disk",fd);show("stdin",0);show("stdout",1);show("stderr",2);
    show("negative",-1);show("bound",10000);_close(fd);show("closed",fd);return 0;
}
