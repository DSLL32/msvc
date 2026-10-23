#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <io.h>
#include <recon.h>
static int format_stdout(const char *format,...)
{
    int result;va_list args;
    va_start(args,format);result=vprintf(format,args);va_end(args);
    return result;
}
int main(void)
{
    FILE *s=stdout;
    const char *prefix="CVTRES : warning CVT4001: ";
    const char *body="machine type not specified; assumed IX86";
    int before;
    if(printf("%s",prefix)!=(int)strlen(prefix))return 1;
    before=s->_ptr-s->_base;
    if(before!=(int)strlen(prefix) || _filelength(1)!=0)return 2;
    if(memcmp(s->_base,prefix,before))return 3;
    if(format_stdout("machine type not specified; assumed %s","IX86")!=40)return 4;
    if(s->_ptr-s->_base!=before+40 || _filelength(1)!=0)return 5;
    if(memcmp(s->_base,prefix,before) || memcmp(s->_base+before,body,40))return 6;
    if(printf("\n")!=1 || fflush(s))return 7;
    if(format_stdout("%s %s %d","ARM","/WINDOWSCE",-7)!=17)return 8;
    if(fflush(s))return 9;
    return 0;
}
