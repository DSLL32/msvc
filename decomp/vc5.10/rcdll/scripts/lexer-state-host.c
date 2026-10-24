/* Auxiliary host for actual production lexer branch selection only.
 * Debugger stops at the selected dependency, before it consumes synthetic input. */
#define RECON_SHIM_IMPL
#include <stdlib.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

int main(int count,char **arguments)
{
    WCHAR source[2];
    if(count!=4)return 2;
    source[0]=atoi(arguments[1]) ? L'x' : 0;
    source[1]=0;
    rc_source_cursor=source;
    rc_expanding_macro=atoi(arguments[2]);
    rc_source_depth=atoi(arguments[3]);
    return rc_next_token();
}
