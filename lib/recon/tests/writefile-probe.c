#include <stdio.h>
#include <errno.h>
#include <windows.h>
#include <recon.h>
int main(void)
{
    HANDLE file;DWORD n,last;int error,ok;unsigned row;
    file=CreateFileA("written.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 3;
    for(row=0;row<4;++row) {
        errno=(row&1)?111:0;SetLastError((row&2)?123:0);
        ok=WriteFile(file,"abc",3,&n,NULL);error=errno;last=GetLastError();
        printf("%d %lu %d %lu\n",ok,n,error,last);
    }
    CloseHandle(file);return 0;
}
