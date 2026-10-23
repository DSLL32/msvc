#include "directives.h"
#include <recon.h>

WCHAR __cdecl rc_get_logical_character(void)
{
    WCHAR character;
    for(;;){
        character=*rc_source_cursor++;
        if(character<L'\x000e'){
            if(character==L'\r')continue;
            if(!character&&rc_number_buffer_active<1){
                rc_resume_input();
                continue;
            }
        }
        if(character!=L'\\')return character;
        if(!rc_splice_line())return L'\\';
    }
}

int __stdcall rc_match_logical_character(unsigned character)
{
    if(character==rc_get_logical_character())return 1;
    --rc_source_cursor;
    return 0;
}
