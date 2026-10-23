#include "command.h"
#include "preprocess.h"
#include "directives.h"
#include <recon.h>

WCHAR __cdecl rc_read_nonspace(void)
{
    WCHAR character;
    unsigned short token;

    for(;;){
        character=*rc_source_cursor++;
        if(character<=L'/'){
            if(character==L'/'){
                if(!rc_scan_comment()) return L'/';
            }else{
                if(character>L' ') return character;
                token=rc_character_token(character);
                if(token!=0 && token!=1){
                    if(token!=3) return character;
                    rc_resume_input();
                }
            }
        }else{
            if(character!=L'\\') return character;
            if(!rc_splice_line()) return L'\\';
        }
    }
}
