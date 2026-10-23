#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#include <mbstring.h>
#include <mbctype.h>
#include <recon.h>
int main(int argc,char **argv)
{
    unsigned char text[]="X",empty[]={0},output[3]; int result;
#ifdef RECON_NATIVE_TEST
    unsigned char unknown[]={0x81,0};
    if(argc>1 && !strcmp(argv[1],"codepage")) recon_mb_codepage=932;
    if(argc>1 && !strcmp(argv[1],"locale")) recon_lc_handle[2]=1033;
    if(argc>1 && !strcmp(argv[1],"nonascii-inc")) _mbsinc(unknown);
    if(argc>1 && !strcmp(argv[1],"nonascii-copy")) _mbccpy(output,unknown);
    if(argc>1 && !strcmp(argv[1],"nonascii-compare")) _mbsncmp(unknown,text,1);
    printf("state %d %d\n",recon_mb_codepage,MB_CUR_MAX);
#else
    (void)argc; (void)argv;
    printf("state %d %d\n",_getmbcp(),MB_CUR_MAX);
#endif
    errno=123; SetLastError(456);
    result=_mbsncmp((unsigned char *)"a",(unsigned char *)"z",1); printf("compare %d\n",result);
    result=_mbsncmp((unsigned char *)"z",(unsigned char *)"a",1); printf("compare %d\n",result);
    result=_mbsncmp((unsigned char *)"a",empty,1); printf("compare %d\n",result);
    result=_mbsncmp(empty,(unsigned char *)"a",1); printf("compare %d\n",result);
    result=_mbsncmp((unsigned char *)"ab",(unsigned char *)"ac",1); printf("compare %d\n",result);
    result=_mbsncmp(text,text,10); printf("compare %d\n",result);
    result=_mbsncmp(NULL,NULL,0); printf("compare %d\n",result);
    printf("advance %ld %ld\n",(long)(_mbsinc(text)-text),(long)(_mbsinc(empty)-empty));
    memset(output,0xa5,sizeof(output)); _mbccpy(output,text);
    printf("copy %02x%02x%02x\n",output[0],output[1],output[2]);
    _mbccpy(output,empty); printf("copy %02x%02x%02x\n",output[0],output[1],output[2]);
    printf("errors %d %lu\n",errno,GetLastError());
    return 0;
}
