#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <recon.h>
static int checked(const char *format,const WCHAR *wide,unsigned tail,int expected)
{
    int before=stdout->_ptr?stdout->_ptr-stdout->_base:0;
    int result=printf(format,wide,tail);
    return result!=expected || stdout->_ptr-stdout->_base!=before+expected;
}
int main(void)
{
    const char *mode=recon_host_getenv("RECON_WIDE_PRINTF_NEGATIVE");
    WCHAR invalid[2]={128,0};
    if(mode){
        if(!strcmp(mode,"width"))printf("%5S",L"a");
        if(!strcmp(mode,"grammar"))printf("%S%u",L"a",1);
        if(!strcmp(mode,"nonascii"))printf("%S",invalid);
        if(!strcmp(mode,"locale"))recon_lc_handle[2]=1033;
        if(!strcmp(mode,"codepage"))recon_lc_codepage=1252;
        if(!strcmp(mode,"null"))printf("%S",(WCHAR *)NULL);
        if(!strcmp(mode,"char"))printf("%S%c",L"a",128);
    }
    if(checked("adding resource -- type:%S,%c",L"\"CUSTOMTYPE\"",32,38))return 1;
    if(checked("name:%S,%c",L"\"NAMEDDATA\"",32,18))return 2;
    if(checked("name:%S,%c",L"\"NAMEDKNOWN\"",32,19))return 3;
    if(checked("duplicate resource -- type:%S,%c",L"\"CUSTOMTYPE\"",32,41))return 4;
    if(checked("name:%S, language:0x%.4x\n",L"\"NAMEDDATA\"",1033,34))return 5;
    if(printf("%S|%c|%.4x|%%",L"Z",'!',0xbeef)!=10)return 6;
    if(fflush(stdout))return 7;
    return 0;
}
