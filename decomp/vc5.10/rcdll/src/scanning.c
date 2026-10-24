#include <string.h>
#include "command.h"
#include "preprocess.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

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

void __stdcall rc_read_identifier(WCHAR character)
{
    WCHAR *cursor=rc_macro_name+1;
    unsigned hash=character&0x5f;

    *rc_macro_name=character;
    for(;;){
        *cursor=*rc_source_cursor++;
        if(rc_character_class(*cursor)&0x10){
            hash+=*cursor++&0x5f;
            continue;
        }
        if(!*cursor){
            if(rc_refill_source()){
                strcpy(rc_error_text,rc_message(1004));
                rc_input_fatal(1004);
            }
            continue;
        }
        if(*cursor==L'\\' && rc_splice_line()) continue;
        break;
    }
    --rc_source_cursor;
    if(cursor>=rc_macro_name+512){
        strcpy(rc_error_text,rc_message(1067));
        rc_input_fatal(1067);
    }
    if(cursor-rc_macro_name<=31 || rc_cpp_active)
        *cursor++=0;
    else{
        rc_macro_name[31]=0;
        cursor=rc_macro_name+32;
        hash=rc_hash_macro_name(rc_macro_name);
        rc_error_template=rc_message(4011);
        rc_format(rc_error_text,2048,rc_error_template,rc_macro_name);
        rc_input_warning(4011);
    }
    rc_macro_name_length=cursor-rc_macro_name;
    rc_macro_hash=(unsigned char)hash;
}

int __cdecl rc_splice_line(void)
{
    WCHAR character;

    for(;;){
        character=*rc_source_cursor++;
        if(character==L'\r')continue;
        if(character==0){
            rc_resume_input();
            rc_source_cursor[-1]=L'\\';
            continue;
        }
        if(character!=L'\n'){
            --rc_source_cursor;
            return 0;
        }
        ++rc_preparing_line;
        if(rc_cpp_active)
            rc_write_preprocessed(L"\r\n",4,1,rc_cpp_output_file);
        return 1;
    }
}
