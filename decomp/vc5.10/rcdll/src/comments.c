#include "command.h"
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

void __cdecl rc_skip_block_comment(void)
{
    unsigned character;
    for(;;){
        character=*rc_source_cursor++;
        if(character==L'*'){
read_after_star:
            character=*rc_source_cursor++;
            if(character==L'/')return;
            if(character==L'*')goto read_after_star;
            if(!character){
                rc_resume_input();
                goto read_after_star;
            }
        }
        if(character<=L'\n'){
            if(character==L'\n'){
                ++rc_preparing_line;
                if(rc_cpp_active)
                    rc_write_preprocessed(L"\r\n",4,1,rc_cpp_output_file);
            }else if(!character)rc_resume_input();
        }
    }
}

void __cdecl rc_skip_line_comment(void)
{
    WCHAR character;
    unsigned short token;

    for(;;){
        character=*rc_source_cursor++;
        token=rc_character_token(character);
        if(token==3)rc_resume_input();
        else if(token==5){
            --rc_source_cursor;
            return;
        }
    }
}
