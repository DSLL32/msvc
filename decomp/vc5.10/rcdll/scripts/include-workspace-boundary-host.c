/* Native-only guard exercise; the production include-search object is linked. */
#define RECON_SHIM_IMPL
#include <stdlib.h>
#include "command.h"
#include "directives.h"
#include <recon.h>
static int observed_open;
int __stdcall __wrap_rc_open_source(const WCHAR *path,int first)
{
    const WCHAR *expected=L"include/winresrc.h";
    unsigned index;
    if(first || path!=rc_number_buffer)exit(7);
    for(index=0;expected[index];++index)
        if(path[index]!=expected[index])exit(8);
    if(path[index])exit(9);
    observed_open=1;
    return 1;
}
int main(int count,char **arguments)
{
    unsigned offset;
    if(count!=2)return 2;
    offset=(unsigned)strtoul(arguments[1],NULL,0);
    rc_number_buffer=(WCHAR *)((char *)rc_string_buffer+offset);
    rc_token_value=L"winresrc.h";
    rc_cpp_path_characters=L"\\:/";
    rc_cpp_includes.remaining=99;
    rc_cpp_includes.items[99]=L"include";
    rc_search_include_directories();
    return observed_open?0:3;
}
