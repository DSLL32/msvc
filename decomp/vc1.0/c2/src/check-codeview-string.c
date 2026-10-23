/* Standalone native test driver; not linked into the reconstruction. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
/* The isolated driver reports through the host CRT; the helper translation
   unit is compiled with the reconstruction remappings independently. */
#undef RECON_NATIVE_TEST
#include <recon.h>
static unsigned warning_count, warning_limit;
void __cdecl compiler_warning(int level,int code,...)
{
    va_list arguments;
    if(level!=1||code!=0x312)abort();
    va_start(arguments,code);warning_limit=va_arg(arguments,unsigned);va_end(arguments);
    ++warning_count;
}
unsigned __fastcall copy_codeview_string(unsigned char *,char *);
int main(int argc,char **argv)
{
    unsigned char buffer[512];char source[1024];unsigned length,i,result;
    if(argc!=2)return 2;
    length=(unsigned)strtoul(argv[1],0,10);
    if(length>1000)return 3;
    for(i=0;i<length;++i)source[i]=(char)('A'+i%26);source[length]=0;
    memset(buffer,0xcc,sizeof(buffer));
    result=copy_codeview_string(buffer,length==1000?NULL:source);
    printf("%u %u %u\n",result,warning_count,warning_limit);
    for(i=0;i<sizeof(buffer);++i)printf("%02x",buffer[i]);puts("");
    return 0;
}
