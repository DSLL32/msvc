/* Native-only length guard probe; target logic remains the linked object. */
#define RECON_SHIM_IMPL
#include <stdlib.h>
#include "command.h"
#include "directives.h"
#include <recon.h>
int main(int count,char **arguments)
{
    WCHAR source[300];
    const char *prefix="include \"";
    unsigned length,index=0;
    if(count!=2)return 2;
    length=(unsigned)strtoul(arguments[1],NULL,0);
    if(length>270)return 3;
    while(*prefix)source[index++]=(unsigned char)*prefix++;
    while(length--)source[index++]=L'a';
    source[index++]=L'"';source[index++]=L'\n';source[index]=0;
    rc_source_cursor=source;
    rc_cpp_active=1;
    rc_handle_directive();
    return 4;
}
