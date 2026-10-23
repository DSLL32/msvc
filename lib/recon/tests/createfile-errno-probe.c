/* Explicit original-only CreateFileA/independent CRT errno probe. */
#include <windows.h>
#include <stdio.h>
#include <errno.h>
#include <recon.h>
int main(void)
{
    DWORD errors[2]={0,0x12345678},after;int values[2]={0,111},saved;
    unsigned i,j,kind;char name[64];HANDLE file;
    for(i=0;i<2;++i)for(j=0;j<2;++j)for(kind=0;kind<3;++kind){
        if(kind==0)sprintf(name,"PrEsEnT.BiN");
        else if(kind==1)sprintf(name,"new-%u-%u.bin",i,j);
        else sprintf(name,"missing.bin");
        errno=values[j];SetLastError(errors[i]);
        file=CreateFileA(name,kind==1?GENERIC_WRITE:GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,kind==1?CREATE_ALWAYS:OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        saved=errno;after=GetLastError();
        if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);
        printf("kind=%u errno=%d->%d error=%lu->%lu success=%d\n",kind,values[j],saved,errors[i],after,file!=INVALID_HANDLE_VALUE);
    }
    return 0;
}
