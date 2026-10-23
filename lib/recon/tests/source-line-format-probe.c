/* Explicit original-only legacy swprintf ABI and encountered format probe. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>
#include <limits.h>
#include <recon.h>
int main(void)
{
    DWORD errors[2]={0,0x12345678},error;
    int values[2]={0,111},numbers[6]={INT_MIN,-1,0,1,63,INT_MAX};
    int saved,result;
    unsigned i,j,k;
    WCHAR buffer[4098];
    FILE *file;char name[80];
#ifdef RECON_NATIVE_TEST
    {
        const char *mode=recon_host_getenv("RECON_SOURCE_LINE_NEGATIVE");
        if(mode && !strcmp(mode,"format"))swprintf(buffer,L"#line %u ",1);
        if(mode && !strcmp(mode,"suffix"))swprintf(buffer,L"#line %d\n",1);
        if(mode && !strcmp(mode,"extra"))swprintf(buffer,L"#line %d  ",1);
        if(mode && !strcmp(mode,"locale"))recon_lc_handle[2]=1033;
        if(mode && !strcmp(mode,"codepage"))recon_lc_codepage=1252;
        if(mode && !strcmp(mode,"null"))swprintf(NULL,L"#line %d ",1);
    }
#endif
    memset(buffer,0x5a,sizeof(buffer));
    file=fopen("before.bin","wb");if(!file)return 126;
    if(fwrite(buffer,1,sizeof(buffer),file)!=sizeof(buffer))return 127;
    fclose(file);
    for(i=0;i<2;++i)for(j=0;j<2;++j)for(k=0;k<6;++k){
        memset(buffer,0x5a,sizeof(buffer));
        errno=values[j];SetLastError(errors[i]);
        result=swprintf(buffer,L"#line %d ",numbers[k]);error=GetLastError();saved=errno;
        printf("source-line-format value=%d before-error=%lu before-errno=%d result=%d after-error=%lu after-errno=%d\n",numbers[k],errors[i],values[j],result,error,saved);
        sprintf(name,"after-%u-%u-%u.bin",i,j,k);file=fopen(name,"wb");if(!file)return 126;
        if(fwrite(buffer,1,sizeof(buffer),file)!=sizeof(buffer))return 127;
    fclose(file);
    }
    return 0;
}
