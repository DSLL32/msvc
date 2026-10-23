#include <stdio.h>
#include <stdlib.h>
#include <mbctype.h>
#include <windows.h>
#include <errno.h>
#include <recon.h>
int main(int argc,char **argv)
{
    unsigned n; int result;
#ifdef RECON_NATIVE_TEST
    if(argc>1) {
        if(!strcmp(argv[1],"codepage")) recon_mb_codepage=932;
        if(!strcmp(argv[1],"locale")) recon_lc_codepage=1252;
        if(!strcmp(argv[1],"nonascii")) _ismbcspace(128);
    }
#else
    (void)argc; (void)argv;
#endif
    for(n=0;n<128;++n) {
        errno=111; SetLastError(0x12345678); result=_ismbcspace(n);
        printf("%u %d %d %lu\n",n,result,errno,GetLastError());
    }
    return 0;
}
