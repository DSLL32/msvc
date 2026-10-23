/* Explicit original-only probe for the input detector's encountered API. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <recon.h>

int main(void)
{
    DWORD errors[2]={0,0x12345678},error;
    int values[2]={0,111},saved,result;
    unsigned i,j,k;
    CPINFO info;
    GetACP();
    for(i=0;i<2;++i)for(j=0;j<2;++j){
        memset(&info,0x5a,sizeof(info));
#ifdef RECON_NATIVE_TEST
        if(recon_host_getenv("RECON_CPINFO_PADDING")){((BYTE*)&info)[18]=0xa7;((BYTE*)&info)[19]=0xc9;}
#endif
        errno=values[j];SetLastError(errors[i]);
        result=GetCPInfo(0,&info);error=GetLastError();saved=errno;
        printf("cpinfo page=0 bytes=%u before-error=%lu before-errno=%d result=%d after-error=%lu after-errno=%d raw=",sizeof(info),errors[i],values[j],result,error,saved);
        for(k=0;k<sizeof(info);++k)printf("%02x",((BYTE *)&info)[k]);
        printf("\n");
    }
    return 0;
}
