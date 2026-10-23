/* Explicit original-only legacy swprintf ABI and encountered format probe. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>
#include <limits.h>
int main(void)
{
    DWORD errors[2]={0,0x12345678},error;
    int values[2]={0,111},numbers[6]={INT_MIN,-1,0,1,63,INT_MAX};
    int saved,result;
    unsigned i,j,k;
    WCHAR buffer[4098];
    FILE *file;char name[80];
    memset(buffer,0x5a,sizeof(buffer));
    file=fopen("before.bin","wb");if(!file)return 126;
    if(fwrite(buffer,1,sizeof(buffer),file)!=sizeof(buffer))return 127;fclose(file);
    for(i=0;i<2;++i)for(j=0;j<2;++j)for(k=0;k<6;++k){
        memset(buffer,0x5a,sizeof(buffer));
        errno=values[j];SetLastError(errors[i]);
        result=swprintf(buffer,L"#line %d\r\n",numbers[k]);error=GetLastError();saved=errno;
        printf("line-format value=%d before-error=%lu before-errno=%d result=%d after-error=%lu after-errno=%d\n",numbers[k],errors[i],values[j],result,error,saved);
        sprintf(name,"after-%u-%u-%u.bin",i,j,k);file=fopen(name,"wb");if(!file)return 126;
        if(fwrite(buffer,1,sizeof(buffer),file)!=sizeof(buffer))return 127;fclose(file);
    }
    return 0;
}
