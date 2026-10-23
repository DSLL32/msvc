#include <stdio.h>
#include <errno.h>
#include <mbstring.h>
#include <recon.h>
int main(int argc,char **argv)
{
    static const char *text[]={"R:\\toolchains\\5.10\\include"," \t abc","abcabcZ","","abc","abc"};
    static const char *set[]={" \t"," \t","cba","abc","","abcabc"};
    static const size_t expected[]={0,3,6,0,0,3};
    unsigned n;
    if(argc>1) {
        if(!strcmp(argv[1],"text"))_mbsspn((const unsigned char*)"\200",(const unsigned char*)"a");
        if(!strcmp(argv[1],"set"))_mbsspn((const unsigned char*)"a",(const unsigned char*)"\200");
        if(!strcmp(argv[1],"null"))_mbsspn(NULL,(const unsigned char*)"a");
        if(!strcmp(argv[1],"codepage"))recon_mb_codepage=932;
        if(!strcmp(argv[1],"locale"))recon_lc_handle[2]=1033;
    }
    for(n=0;n<6;++n) {
        errno=111;SetLastError(0x12345678);
        if(_mbsspn((const unsigned char*)text[n],(const unsigned char*)set[n])!=expected[n])return 1;
        if(errno!=111 || GetLastError()!=0x12345678)return 2;
    }
    return 0;
}
