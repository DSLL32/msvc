#include <string.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

unsigned __cdecl rc_read_macro_replacement(void)
{
    WCHAR *out=rc_macro_replacement,*start;
    WCHAR character,next;
    unsigned short token;
    int stringify=0,charize=0,parameter;

    character=rc_read_nonspace();
scan:
    rc_check_macro_space(out);
    token=rc_character_token(character);
    switch(token){
    case 0:
    case 1:
whitespace:
        character=rc_read_nonspace();
        if(rc_character_token(character)!=5)*out++=L' ';
        goto scan;
    case 2:
        if(rc_scan_comment())goto whitespace;
        goto copy_character;
    case 3:
        if(rc_resume_input()==16){
            *out++=character;
            character=rc_get_logical_character();
            goto copy_character;
        }
        goto next_character;
    case 5:
        --rc_source_cursor;
        if(out==rc_macro_replacement)return 0;
        rc_check_macro_space(out);
        out[0]=0;
        out[1]=L'D';
        return out-rc_macro_replacement+2;
    case 7:
    case 8:
        out=rc_copy_replacement_quoted(out,character);
        goto next_character;
    case 11:
        for(;;){
            next=*rc_source_cursor++;
            token=rc_character_token(next);
            if(token!=3)break;
            if(rc_resume_input()==16)goto stringize;
        }
        if(token==11){
            if(out==rc_macro_replacement){
                strcpy(rc_error_text,rc_message(2160));
                rc_input_error(2160);
                goto scan;
            }
            if(out[-1]==L' ')--out;
            character=rc_read_nonspace();
            if(rc_character_token(character)==5){
                --rc_source_cursor;
                strcpy(rc_error_text,rc_message(2161));
                rc_input_error(2161);
            }
            goto scan;
        }
        if(token==41)charize=1;
        else{
stringize:
            --rc_source_cursor;
            stringify=1;
        }
        character=rc_read_nonspace();
        if(rc_character_token(character)==40)goto identifier;
        strcpy(rc_error_text,rc_message(2162));
        rc_input_error(2162);
        goto scan;
    case 23:
        rc_error_template=rc_message(2018);
        rc_format(rc_error_text,2048,rc_error_template,character);
        rc_input_error(2018);
        goto next_character;
    case 40:
identifier:
        start=out;
        for(;;){
            rc_check_macro_space(out);
            *out++=character;
read_identifier_character:
            character=*rc_source_cursor++;
            if(rc_character_class(character)&16)continue;
            if(rc_character_token(character)==3 && rc_resume_input()!=16)
                goto read_identifier_character;
            break;
        }
        *out=0;
        parameter=rc_find_macro_parameter(start);
        if(parameter>=1){
            *start=(WCHAR)(stringify ? 2 : charize ? 3 : 1);
            start[1]=(WCHAR)parameter;
            out=start+2;
        }else if(stringify || charize){
            strcpy(rc_error_text,rc_message(2162));
            rc_input_error(2162);
        }
        stringify=charize=0;
        goto scan;
    }
copy_character:
    *out++=character;
next_character:
    character=*rc_source_cursor++;
    goto scan;
}

void __stdcall rc_check_macro_space(WCHAR *cursor)
{
    if(cursor>=rc_string_buffer+2043){
        rc_error_template=rc_message(1011);
        rc_format(rc_error_text,2048,rc_error_template,rc_macro_name);
        rc_input_fatal(1011);
    }
}
