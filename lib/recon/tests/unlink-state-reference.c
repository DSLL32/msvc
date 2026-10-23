/* Explicit original-only unlink error/ownership/file-effect probe. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <errno.h>
int main(void)
{
    const char *names[4]={"PrEsEnT.BiN","missing.bin","absent-parent\\child.bin","blocked.bin\\child.bin"};
    int values[2]={0,111},result,saved;DWORD errors[2]={0,0x12345678},after,dos;
    unsigned i,j,k,kind;char name[256],copy[256];FILE *file;
    file=fopen("blocked.bin","wb");if(!file)return 83;fwrite("guard",1,5,file);fclose(file);
    for(i=0;i<2;++i)for(j=0;j<2;++j)for(k=0;k<2;++k)for(kind=0;kind<4;++kind){
        if(!kind){file=fopen("present.bin","wb");if(!file)return 81;fwrite("payload",1,7,file);fclose(file);}
        memset(name,0x5a,sizeof(name));strcpy(name,names[kind]);memcpy(copy,name,sizeof(name));
        errno=values[j];_doserrno=k?77:0;SetLastError(errors[i]);
        result=_unlink(name);after=GetLastError();saved=errno;dos=_doserrno;
        if(memcmp(name,copy,sizeof(name)))return 82;
        printf("kind=%u result=%d errno=%d->%d dos=%u->%lu error=%lu->%lu\n",kind,result,values[j],saved,k?77:0,dos,errors[i],after);
    }
    return 0;
}
