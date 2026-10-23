#include <stdio.h>
#include <stdlib.h>
#include <mbstring.h>
#include <windows.h>
#include <errno.h>
#include <recon.h>
int main(int argc,char **argv)
{
    unsigned char text[]="_MSC_VER=1100;A=A"; unsigned values[]={'#','=',';',0,'_'}; unsigned n;unsigned char *result;
#ifdef RECON_NATIVE_TEST
    if(argc>1) {
        if(!strcmp(argv[1],"codepage")) recon_mb_codepage=932;
        if(!strcmp(argv[1],"locale")) recon_lc_codepage=1252;
        if(!strcmp(argv[1],"nonascii")) text[0]=128;
        if(!strcmp(argv[1],"value")) values[0]=128;
    }
#else
    (void)argc;(void)argv;
#endif
    for(n=0;n<5;++n) {
        errno=111;SetLastError(0x12345678);result=_mbschr(text,values[n]);
        printf("%u %ld %d %lu\n",values[n],result?(long)(result-text):-1L,errno,GetLastError());
    }
    result=_mbschr((unsigned char *)"",0);printf("empty %d\n",result!=NULL);return 0;
}
