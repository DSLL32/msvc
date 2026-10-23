/* Explicit original-only bounded APIs encountered by resource input preparation. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>

int main(void)
{
    const char name[]="resources.rc";
    const WCHAR initial[]=L"#line 1\"resources.rc";
    DWORD errors[2]={0,0x12345678},error;
    int values[2]={0,111},saved,result;
    unsigned i,j,k;
    WCHAR buffer[4098],warm[32],*returned;
    FILE *file;char output[80];
    if(sizeof(WCHAR)!=2)return 125;
    MultiByteToWideChar(1252,1,name,-1,warm,32);
    for(i=0;i<2;++i)for(j=0;j<2;++j){
        for(k=0;k<sizeof(name);++k){
            errno=values[j];SetLastError(errors[i]);
            result=IsDBCSLeadByteEx(1252,(BYTE)name[k]);error=GetLastError();saved=errno;
            printf("dbcs byte=%u before-error=%lu before-errno=%d result=%d after-error=%lu after-errno=%d\n",(BYTE)name[k],errors[i],values[j],result,error,saved);
        }
        memset(buffer,0x5a,sizeof(buffer));memcpy(buffer,initial,sizeof(initial));
        sprintf(output,"before-%u-%u.bin",i,j);file=fopen(output,"wb");if(!file)return 126;
        if(fwrite(buffer,1,sizeof(buffer),file)!=sizeof(buffer))return 127;fclose(file);
        errno=values[j];SetLastError(errors[i]);
        returned=wcscat(buffer,L"\"\r\n");error=GetLastError();saved=errno;
        printf("wcscat before-error=%lu before-errno=%d same-pointer=%d after-error=%lu after-errno=%d\n",errors[i],values[j],returned==buffer,error,saved);
        sprintf(output,"after-%u-%u.bin",i,j);file=fopen(output,"wb");if(!file)return 126;
        if(fwrite(buffer,1,sizeof(buffer),file)!=sizeof(buffer))return 127;fclose(file);
    }
    return 0;
}
