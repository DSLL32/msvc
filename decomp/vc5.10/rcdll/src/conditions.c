#include <string.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

int __cdecl rc_push_condition(void)
{
    ++rc_cpp_nesting_level;
    if(rc_cpp_nesting_level>=32){
        strcpy(rc_error_text,rc_message(1068));
        rc_input_fatal(1068);
    }
    return rc_cpp_nesting_level;
}

void __stdcall rc_skip_inactive_quoted(WCHAR delimiter)
{
    WCHAR character;
    unsigned short token;

    for(;;){
        character=*rc_source_cursor++;
        token=rc_character_token(character);
        if(token==3){
            if(rc_resume_input()==16)++rc_source_cursor;
        }else if(token==5){
            strcpy(rc_error_text,rc_message(4093));
            rc_input_warning(4093);
            --rc_source_cursor;
            return;
        }else if(token==7 || token==8){
            if(character==delimiter)return;
        }else if(token==42)rc_get_logical_character();
    }
}
