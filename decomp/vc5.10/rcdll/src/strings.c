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

/* Return the copied terminator, rather than the initial destination. */
WCHAR *__stdcall rc_copy_wide_end(WCHAR *destination,const WCHAR *source)
{
    while((*destination=*source)!=0){
        ++destination;
        ++source;
    }
    return destination;
}

unsigned char __cdecl rc_read_character_constant(void)
{
    extern int rc_cpp_J;
    unsigned char token=rc_cpp_J?9:7;
    WCHAR character,closing;
    unsigned value[2];
    for(;;){
        character=*rc_source_cursor++;
        switch(rc_character_token(character)){
        case 3:
            if(rc_resume_input()!=16)continue;
            value[0]=rc_escape_character(rc_get_logical_character());
            if(token==9)value[0]&=255;
            goto close_quote;
        case 5:
            strcpy(rc_error_text,rc_message(2001));
            rc_input_error(2001);
            --rc_source_cursor;
            break;
        case 6:
            /* Original leaves the local value uninitialized for this synthetic
             * token. It is outside the supported character-constant input. */
            __asm int 3
            goto close_quote;
        case 7:
            strcpy(rc_error_text,rc_message(2137));
            rc_input_error(2137);
            value[0]=0;
            --rc_source_cursor;
            goto close_quote;
        }
        value[0]=character;
        break;
    }
close_quote:
    closing=rc_get_logical_character();
    if(closing!=L'\''){
        strcpy(rc_error_text,rc_message(2015));
        rc_input_error(2015);
        while(closing!=L'\''&&closing!=L'\n')closing=rc_get_logical_character();
        if(closing==L'\n'){
            strcpy(rc_error_text,rc_message(2016));
            rc_input_error(2016);
        }
    }
    rc_token_value=rc_store_integer(token,value);
    return token;
}
