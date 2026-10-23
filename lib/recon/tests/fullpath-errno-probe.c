/* Explicit original-only successful _fullpath state/tail probe. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <recon.h>
int main(void)
{
    DWORD errors[2]={0,0x12345678},after;int values[2]={0,111},saved;
    const char *names[2]={"resources.res","."};
    unsigned i,j,kind;char buffer[260],filename[64],*result;FILE *file;
    for(i=0;i<2;++i)for(j=0;j<2;++j)for(kind=0;kind<2;++kind){
        memset(buffer,0xa5,sizeof(buffer));errno=values[j];SetLastError(errors[i]);
        result=_fullpath(buffer,names[kind],sizeof(buffer));saved=errno;after=GetLastError();
        printf("kind=%u errno=%d->%d error=%lu->%lu alias=%d length=%u\n",kind,values[j],saved,errors[i],after,result==buffer,result?(unsigned)strlen(result):0);
        sprintf(filename,"buffer-%u-%u-%u.bin",i,j,kind);file=fopen(filename,"wb");if(!file)return 125;
        if(fwrite(buffer,1,sizeof(buffer),file)!=sizeof(buffer))return 125;fclose(file);
    }
    return 0;
}
