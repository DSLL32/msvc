#include <stdio.h>
#include <string.h>
#include <mbstring.h>
#include <windows.h>
#include <errno.h>
#include <recon.h>
int main(int argc,char **argv)
{
    static const char *left[]={"c","C","c","cpp","z","[","","a","c\0x"};
    static const char *right[]={"c","c","cpp","C","a","a","A","","C\0y"};
    unsigned n;int value;
#ifdef RECON_NATIVE_TEST
    if(argc>1) {
        if(!strcmp(argv[1],"codepage"))recon_mb_codepage=932;
        if(!strcmp(argv[1],"locale"))recon_lc_handle[2]=1033;
        if(!strcmp(argv[1],"nonascii"))_mbsicmp((const unsigned char*)"\200",(const unsigned char*)"a");
    }
#else
    (void)argc;(void)argv;
#endif
    for(n=0;n<9;++n) {
        errno=111;SetLastError(123);
        value=_mbsicmp((const unsigned char*)left[n],(const unsigned char*)right[n]);
        printf("%u %d %d %lu\n",n,value,errno,GetLastError());
    }
    return 0;
}
