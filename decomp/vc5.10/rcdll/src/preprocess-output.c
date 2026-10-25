#include <string.h>
#include <wchar.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

void __cdecl rc_run_preprocessor(void)
{
    unsigned char token;
    const WCHAR *text;

    for(;;){
        token=rc_next_token();
        if(!token)return;
        if(token==1)continue;
        if(!rc_token_kind){
            strcpy(rc_error_text,rc_message(1011));
            rc_input_fatal(1011);
        }
        text=rc_token_info[rc_token_kind].description;
#ifdef RECON_NATIVE_TEST
        /* Unavailable descriptor rows remain unsupported, not empty strings. */
        if(!text){ __asm int 3 }
#endif
        rc_write_preprocessed(text,wcslen(text)*2,1,rc_cpp_output_file);
    }
}

void __stdcall rc_copy_quoted(WCHAR delimiter)
{
    WCHAR *cursor=rc_macro_name+1,character;
    unsigned short token;

    *rc_macro_name=delimiter;
    for(;;){
        character=*rc_source_cursor++;
        token=rc_character_token(character);
        if(token==1)continue;
        if(token==3){
            if(character!=L'\\'){
                rc_resume_input();
                continue;
            }
            *cursor++=character;
            character=rc_get_logical_character();
        }else if(token==5){
            --rc_source_cursor;
            break;
        }else if(token==6){
            *cursor++=character;
        }else if(token==7||token==8){
            if(character==delimiter){
                *cursor++=delimiter;
                break;
            }
        }
        *cursor++=character;
        if(cursor>=rc_macro_name+255){
            *cursor=0;
            rc_write_preprocessed(rc_macro_name,(cursor-rc_macro_name)*sizeof(WCHAR),
                                  1,rc_cpp_output_file);
            cursor=rc_macro_name;
        }
    }
    *cursor=0;
    rc_write_preprocessed(rc_macro_name,(cursor-rc_macro_name)*sizeof(WCHAR),
                          1,rc_cpp_output_file);
}
