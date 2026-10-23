#include <stdio.h>
#include <stdlib.h>
#include <mbstring.h>
#include <windows.h>
#include <errno.h>
#include <recon.h>
int main(int argc,char **argv)
{
    unsigned char buffer[5]={0,'1','2','3',0},*result; int n;
#ifdef RECON_NATIVE_TEST
    if(argc>1) {
        if(!strcmp(argv[1],"codepage")) recon_mb_codepage=932;
        if(!strcmp(argv[1],"locale")) recon_lc_codepage=1252;
        if(!strcmp(argv[1],"nonascii")) buffer[2]=128;
    }
#else
    (void)argc; (void)argv;
#endif
    for(n=0;n<5;++n) {
        errno=111;SetLastError(0x12345678); result=_mbsdec(buffer,buffer+n);
        printf("%d %ld %d %lu\n",n,result?(long)(result-buffer):-1L,errno,GetLastError());
    }
    errno=111;SetLastError(0x12345678);result=_mbsdec(buffer+2,buffer+1);
    printf("before %d %d %lu\n",result==NULL,errno,GetLastError());
    return 0;
}
