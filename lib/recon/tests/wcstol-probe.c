/* Explicit original-only canonical CRT probe for numeric preprocessor options. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>
#include <recon.h>
int main(void)
{
    const WCHAR *texts[]={L"0",L"1",L"1252",L"65001",L"65534",L"65535",
        L"2147483647",L"2147483648",L"4294967295",L"123x",L" 1252",
        L"+1252",L"-1252"};
    DWORD errors[2]={0,0x12345678},error;
    int values[2]={0,111},saved;
    unsigned i,j,k;
    WCHAR buffer[64];
    long result;
    FILE *file=fopen("wcstol-inputs.bin","wb");
    if(!file) return 125;
#ifdef RECON_NATIVE_TEST
    {
        const char *mode=recon_host_getenv("RECON_WCSTOL_NEGATIVE");
        WCHAR invalid[2]={128,0},*end;
        if(mode) {
            if(!strcmp(mode,"nonascii"))wcstol(invalid,NULL,10);
            if(!strcmp(mode,"end"))wcstol(texts[0],&end,10);
            if(!strcmp(mode,"base"))wcstol(texts[0],NULL,16);
            if(!strcmp(mode,"locale"))recon_lc_handle[2]=1033;
            if(!strcmp(mode,"codepage"))recon_mb_codepage=932;
        }
    }
#endif
    for(i=0;i<2;++i)for(j=0;j<2;++j)for(k=0;k<13;++k){
        memset(buffer,0x5a,sizeof(buffer));wcscpy(buffer,texts[k]);
        if(fwrite(buffer,1,sizeof(buffer),file)!=sizeof(buffer)) return 125;
        errno=values[j];SetLastError(errors[i]);
        result=wcstol(buffer,NULL,10);error=GetLastError();saved=errno;
        if(fwrite(buffer,1,sizeof(buffer),file)!=sizeof(buffer)) return 125;
        printf("wcstol case=%u before-error=%lu before-errno=%d result=%ld after-error=%lu after-errno=%d\n",k,errors[i],values[j],result,error,saved);
    }
    return fclose(file)?125:0;
}
