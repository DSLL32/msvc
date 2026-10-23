/* Explicit original-only libc100 tempnam TMP/counter/errors/ownership probe. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
int main(int argc,char **argv)
{
    char assignment[520],*names[4];DWORD errors[2]={0,12345678},after,dos;
    int values[2]={0,111},saved;unsigned i,j,n=0,k;
    if(argc!=2)return 125;
    sprintf(assignment,"TMP=%s",argv[1]);if(_putenv(assignment))return 125;
    printf("TMP=%s\n",getenv("TMP"));
    for(i=0;i<2;++i)for(j=0;j<2;++j){
        errno=values[j];_doserrno=77;SetLastError(errors[i]);
        names[n]=_tempnam("\\","tmp");after=GetLastError();saved=errno;dos=_doserrno;
        if(!names[n])return 125;
        for(k=0;k<n;++k)if(names[n]==names[k])return 125;
        printf("call=%u errno=%d->%d dos=77->%lu error=%lu->%lu name=%s\n",n+1,values[j],saved,dos,errors[i],after,names[n]);
        ++n;
    }
    for(k=0;k<n;++k)free(names[k]);return 0;
}
