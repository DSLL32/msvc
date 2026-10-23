#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include <windows.h>
#include <errno.h>
#include <recon.h>
int main(int argc,char **argv)
{
    static const char *left[]={"?","LIBPATH","libpathX","libpathX","z","a","[","","A",NULL,"a\0X"};
    static const char *right[]={"libpath","libpath","LIBPATHY","libpathy","a","Z","A","A","",NULL,"A\0Y"};
    static const unsigned count[]={7,7,7,8,1,1,1,7,7,0,9}; unsigned n;int result;
#ifdef RECON_NATIVE_TEST
    if(argc>1) {
        if(!strcmp(argv[1],"codepage")) recon_mb_codepage=932;
        if(!strcmp(argv[1],"locale")) recon_lc_handle[2]=1033;
        if(!strcmp(argv[1],"nonascii")) _mbsnbicmp((const unsigned char *)"\200",(const unsigned char *)"a",1);
    }
#else
    (void)argc;(void)argv;
#endif
    for(n=0;n<11;++n) {
        errno=111;SetLastError(0x12345678);
        result=_mbsnbicmp((const unsigned char *)left[n],(const unsigned char *)right[n],count[n]);
        printf("%u %d %d %lu\n",n,result,errno,GetLastError());
    }
    return 0;
}
