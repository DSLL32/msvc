#include <string.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

/* Byte offsets0..4086 may receive WCHARs, then the terminator at4088. */
WCHAR rc_string_buffer[2045];

void __cdecl rc_read_string(void)
{
    WCHAR *cursor=rc_string_buffer;
    WCHAR character;
    int warn=1;
    rc_token_value=rc_string_buffer;
    for(;;){
        character=*rc_source_cursor++;
        switch(rc_character_token(character)){
        case 3:
            if(rc_resume_input()!=16)continue;
            if(!rc_header_mode)
                character=(WCHAR)rc_escape_character(rc_get_logical_character());
            break;
        case 5:
            --rc_source_cursor;
            strcpy(rc_error_text,rc_message(2001));
            rc_input_error(2001);
            goto done;
        case 8:
            goto done;
        }
        if((char *)cursor-(char *)rc_string_buffer<0xff7)
            *cursor++=character;
        else if(warn){
            strcpy(rc_error_text,rc_message(4009));
            rc_input_warning(4009);
            warn=0;
        }
    }
done:
    *cursor=0;
    rc_string_length=(unsigned short)(cursor-(WCHAR *)rc_token_value+1);
}
