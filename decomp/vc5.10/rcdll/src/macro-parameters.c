#include <string.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

/* Native checks cover the encountered packed entries in the existing WCHAR
 * workspace. They do not add an overflow/error behavior to the Win32 target. */
#ifdef RECON_NATIVE_TEST
#define PARAMETER_SPACE(cursor,bytes) \
    if((DWORD)(cursor)<(DWORD)rc_string_buffer || \
       (((DWORD)(cursor)-(DWORD)rc_string_buffer)&1) || \
       (DWORD)(cursor)-(DWORD)rc_string_buffer>sizeof(rc_string_buffer)-(bytes)){ __asm int 3 }
#else
#define PARAMETER_SPACE(cursor,bytes)
#endif

void __cdecl rc_parse_macro_parameters(void)
{
    WCHAR *start,*out;
    WCHAR character;
    unsigned short token;

    PARAMETER_SPACE(rc_macro_replacement,4);
    start=out=rc_macro_replacement+1;
    for(;;){
        character=rc_read_nonspace();
        token=rc_character_token(character);
        if(token==40){
            if(out!=start){
                rc_error_template=rc_message(2010);
                rc_format(rc_error_text,2048,rc_error_template,character);
                rc_input_error(2010);
            }
            PARAMETER_SPACE(out,2);
            *out++=character;
            for(;;){
                character=*rc_source_cursor++;
                if(rc_character_class(character)&16){
                    PARAMETER_SPACE(out,2);
                    *out++=character;
                    continue;
                }
                if(!character){
                    if(rc_refill_source()){
                        strcpy(rc_error_text,rc_message(1004));
                        rc_input_fatal(1004);
                    }
                    continue;
                }
                if(character==L'\\' && rc_splice_line())continue;
                --rc_source_cursor;
                PARAMETER_SPACE(out,2);
                *out++=0;
                break;
            }
            continue;
        }
        if(token==27 || token==28){
            if(out>start){
                if((unsigned)((out-start)*sizeof(WCHAR))>=64){
                    PARAMETER_SPACE(start,64);
                    start[31]=0;
                    strcpy(rc_error_text,rc_message(4111));
                    rc_input_warning(4011);
                    out=start+32;
                }
                if(rc_find_macro_parameter(start)>=1){
                    rc_error_template=rc_message(2009);
                    rc_format(rc_error_text,2048,rc_error_template,start);
                    rc_input_error(2009);
                }
                PARAMETER_SPACE(rc_macro_replacement,2);
                *rc_macro_replacement=(WCHAR)((out-rc_macro_replacement)*sizeof(WCHAR));
                ++rc_macro_parameter_count;
                rc_macro_replacement=out;
            }else if(rc_character_token(character)==27 || rc_macro_parameter_count>0){
                rc_error_template=rc_message(2010);
                rc_format(rc_error_text,2048,rc_error_template,character);
                rc_input_error(2010);
            }
            if(rc_character_token(character)==28)return;
            PARAMETER_SPACE(rc_macro_replacement,4);
            start=out=rc_macro_replacement+1;
            continue;
        }
        rc_error_template=rc_message(2010);
        rc_format(rc_error_text,2048,rc_error_template,character);
        rc_input_error(2010);
    }
}

#undef PARAMETER_SPACE
