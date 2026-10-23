#include <stdio.h>
#include <errno.h>
#include <mbstring.h>
#include <recon.h>
int main(int argc,char **argv)
{
    static const char *texts[]={"hello.obj","hello?.obj","*hello.obj","abc?def*","","abc","abc"};
    static const char *sets[]={"?*","?*","?*","*?","abc","","cbaabc"};
    static const int offsets[]={-1,5,0,3,-1,-1,0};
    unsigned n;unsigned char *result;
    if(argc>1){
        if(!strcmp(argv[1],"text"))_mbspbrk((const unsigned char*)"\200",(const unsigned char*)"a");
        if(!strcmp(argv[1],"set"))_mbspbrk((const unsigned char*)"a",(const unsigned char*)"\200");
        if(!strcmp(argv[1],"null"))_mbspbrk(NULL,(const unsigned char*)"a");
        if(!strcmp(argv[1],"codepage"))recon_mb_codepage=932;
        if(!strcmp(argv[1],"locale"))recon_lc_handle[2]=1033;
    }
    for(n=0;n<7;++n){
        errno=111;SetLastError(0x12345678);
        result=_mbspbrk((const unsigned char*)texts[n],(const unsigned char*)sets[n]);
        if(result!=(offsets[n]<0?NULL:(const unsigned char*)texts[n]+offsets[n]))return 1;
        if(errno!=111 || GetLastError()!=0x12345678)return 2;
    }
    return 0;
}
