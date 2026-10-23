/* Explicit original-only _access independent errno/DOS/Win32 state probe. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <errno.h>
int main(void)
{
    const char *names[3]={"PrEsEnT.BiN","missing.bin","absent-parent\\child.bin"};
    DWORD errors[2]={0,0x12345678},after,dos;int values[2]={0,111},saved,result;
    unsigned i,j,mode,kind;
    for(i=0;i<2;++i)for(j=0;j<2;++j)for(mode=0;mode<=4;mode+=4)for(kind=0;kind<3;++kind){
        errno=values[j];_doserrno=77;SetLastError(errors[i]);
        result=_access(names[kind],mode);after=GetLastError();saved=errno;dos=_doserrno;
        printf("mode=%u kind=%u result=%d errno=%d->%d dos=77->%lu error=%lu->%lu\n",mode,kind,result,values[j],saved,dos,errors[i],after);
    }
    return 0;
}
