#include <stdio.h>
#include <errno.h>
#include <mbstring.h>
#include <recon.h>
int main(int argc,char **argv)
{
    unsigned n;unsigned char byte;size_t result;
    if(argc>1) {
        if(!strcmp(argv[1],"nonascii")) {byte=128;_mbclen(&byte);}
        if(!strcmp(argv[1],"null"))_mbclen(NULL);
        if(!strcmp(argv[1],"codepage"))recon_mb_codepage=932;
        if(!strcmp(argv[1],"max"))recon_mb_cur_max=2;
        if(!strcmp(argv[1],"locale"))recon_lc_handle[2]=1033;
    }
    for(n=0;n<128;++n) {
        byte=(unsigned char)n;errno=111;SetLastError(0x12345678);
        result=_mbclen(&byte);
        if(result!=1 || errno!=111 || GetLastError()!=0x12345678)return 1;
    }
    return 0;
}
