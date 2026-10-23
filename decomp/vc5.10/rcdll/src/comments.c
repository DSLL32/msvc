#include "directives.h"
#include <recon.h>

int __cdecl rc_scan_comment(void)
{
    if(rc_match_logical_character(L'*')) rc_skip_block_comment();
    else{
        if(!rc_match_logical_character(L'/'))return 0;
        rc_skip_line_comment();
    }
    return 1;
}
