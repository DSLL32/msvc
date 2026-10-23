/* Explicit original-only single-directory _searchenv state and tail probe. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
int main(void)
{
    DWORD errors[2]={0,0x12345678},after,dos;int values[2]={0,111},saved;
    unsigned i,j;char output[260],name[64];FILE *file;
    printf("LIB=%s\n",getenv("LIB"));
    for(i=0;i<2;++i)for(j=0;j<2;++j){
        memset(output,0xa5,sizeof(output));errno=values[j];_doserrno=77;SetLastError(errors[i]);
        _searchenv("kernel32.lib","LIB",output);after=GetLastError();saved=errno;dos=_doserrno;
        printf("errno=%d->%d dos=77->%lu error=%lu->%lu output=%s\n",values[j],saved,dos,errors[i],after,output);
        sprintf(name,"output-%u-%u.bin",i,j);file=fopen(name,"wb");if(!file)return 125;
        if(fwrite(output,1,sizeof(output),file)!=sizeof(output))return 125;fclose(file);
    }
    return 0;
}
