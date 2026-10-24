/* Native-only production-object guard check; outside the target graph. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <string.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

int main(int count,char **arguments)
{
    static WCHAR input[]=L"quote)\n";
    static const WCHAR expected[]={14,L'q',L'u',L'o',L't',L'e',0};
    unsigned i;
    if(count!=2)return 2;
    memset(rc_string_buffer,0xa7,sizeof(rc_string_buffer));
    rc_macro_parameter_count=0;
    rc_macro_replacement=rc_string_buffer;
    rc_source_cursor=input;
    if(!strcmp(arguments[1],"before"))rc_macro_replacement=(WCHAR *)((DWORD)rc_string_buffer-2);
    else if(!strcmp(arguments[1],"unaligned"))rc_macro_replacement=(WCHAR *)((DWORD)rc_string_buffer+1);
    else if(!strcmp(arguments[1],"end"))rc_macro_replacement=rc_string_buffer+2045;
    else if(!strcmp(arguments[1],"short-header"))rc_macro_replacement=rc_string_buffer+2044;
    else if(!strcmp(arguments[1],"overflow"))rc_macro_replacement=rc_string_buffer+2043;
    else if(strcmp(arguments[1],"valid"))return 2;
    rc_parse_macro_parameters();
    if(strcmp(arguments[1],"valid"))return 3;
    if(sizeof(WCHAR)!=2||rc_macro_parameter_count!=1||rc_macro_replacement!=rc_string_buffer+7||rc_source_cursor!=input+6)return 4;
    if(memcmp(rc_string_buffer,expected,sizeof(expected)))return 5;
    for(i=sizeof(expected);i<sizeof(rc_string_buffer);++i)
        if(((unsigned char *)rc_string_buffer)[i]!=0xa7)return 6;
    puts("one packed parameter; 14-byte initialized prefix, 4076-byte untouched tail");
    return 0;
}
