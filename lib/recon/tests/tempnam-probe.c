/* Explicit original-only libc100 tempnam TMP/counter/errors/ownership probe. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <recon.h>
int main(int argc,char **argv)
{
    char assignment[520],*names[4];DWORD errors[2]={0,12345678},after,dos;
    int values[2]={0,111},saved;unsigned i,j,n=0,k;
    if(argc<2)return 125;
    sprintf(assignment,"TMP=%s",argv[1]);if(setenv("TMP",argv[1],1))return 125;
    if(argc==3){
        if(!strcmp(argv[2],"missing"))unsetenv("TMP");
        if(!strcmp(argv[2],"counter")){recon_tempnam_prefix_length=3;recon_tempnam_counter=~0U;}
        if(!strcmp(argv[2],"basename")){recon_tempnam_prefix_length=3;recon_tempnam_counter=999999;}
        if(!strcmp(argv[2],"locale"))recon_mb_codepage=1252;
        _tempnam(!strcmp(argv[2],"dir")?NULL:"\\",!strcmp(argv[2],"prefix")?"abcdef":"tmp");return 42;
    }
    printf("TMP=%s\n",getenv("TMP"));
    for(i=0;i<2;++i)for(j=0;j<2;++j){
        errno=values[j];_doserrno=77;SetLastError(errors[i]);
        names[n]=_tempnam("\\","tmp");after=GetLastError();saved=errno;dos=_doserrno;
        if(!names[n])return 125;
        for(k=0;k<n;++k)if(names[n]==names[k])return 125;
        printf("call=%u errno=%d->%d dos=77->%lu error=%lu->%lu name=%s\n",n+1,values[j],saved,dos,errors[i],after,names[n]);
        ++n;
    }
    if(recon_tempnam_counter!=5 || recon_tempnam_prefix_length!=3)return 124;
    for(k=0;k<n;++k)free(names[k]);
    return 0;
}
