#include <string.h>
#include "command.h"
#include "macro-expansion.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(memcmp)
#endif

/* The two initial pointers are relocations in the original .data section. */
WCHAR *rc_macro_output_cursor=rc_string_buffer;
WCHAR *rc_saved_macro_number=&rc_string_buffer[1024];
WCHAR *rc_saved_macro_output;
int rc_expansion_line,rc_expanded_argument_count;
RcMacroFrame rc_macro_frames[64];

int __stdcall rc_expand_macro(RcMacro *macro)
{
    int result=0,parameters,saved_output;
    WCHAR character,*number;
    unsigned short token;

    rc_number_buffer_active=0;
    rc_saved_macro_number=rc_number_buffer;
    rc_expansion_line=rc_preparing_line;
expand:
    rc_expanded_argument_count=0;
    rc_saved_macro_output=rc_macro_output_cursor;
    parameters=macro->parameters;
    if(parameters<-2){
        rc_push_macro(macro);
#ifdef RECON_NATIVE_TEST
        if(rc_expanding_macro<0||rc_expanding_macro>=64){ __asm int 3 }
#endif
        ++rc_macro_frames[rc_expanding_macro].macro->flags;
        if(rc_finish_macro_frame())return 1;
    }else if(parameters==0){
        result=1;
        if(macro->text){
            rc_push_macro(macro);
            rc_select_macro_text();
        }else rc_number_buffer=rc_saved_macro_number;
    }else{
        if(parameters==-1)parameters=0;
        for(;;){
            if(!rc_skip_expression_space())goto continue_expansion;
            if(rc_character_token(*rc_source_cursor)!=2)break;
            ++rc_source_cursor;
            if(!rc_scan_comment()){
                --rc_source_cursor;
                break;
            }
        }
        if(rc_character_token(*rc_source_cursor)==32){
            ++rc_source_cursor;
            result=1;
            rc_expand_macro_arguments(macro,parameters);
        }else{
            number=rc_number_buffer;
            rc_push_macro(macro);
#ifdef RECON_NATIVE_TEST
            if(rc_expanding_macro<0||rc_expanding_macro>=64){ __asm int 3 }
#endif
            ++rc_macro_frames[rc_expanding_macro].macro->flags;
            rc_number_buffer=number;
            if(rc_finish_macro_frame())return 0;
        }
    }
continue_expansion:
    while(rc_number_buffer_active){
        if(rc_number_buffer>=&rc_string_buffer[2043])
            rc_macro_buffer_fatal(10056);
        character=*rc_source_cursor++;
        token=rc_character_token(character);
        switch(token){
        case 0:
            do character=*rc_source_cursor++;
            while(rc_character_class(character)&0x40);
            --rc_source_cursor;
            character=L' ';
            break;
        case 3:
            if(rc_resume_input()!=16)continue;
            *rc_number_buffer++=character;
            character=*rc_source_cursor++;
            break;
        case 7:
        case 8:
            rc_number_buffer=rc_copy_replacement_quoted(rc_number_buffer,character);
            continue;
        case 9:
            *rc_number_buffer++=L'.';
            for(;;){
                character=*rc_source_cursor++;
                token=rc_character_token(character);
                if(token==3){
                    if(rc_resume_input()==16){--rc_source_cursor;break;}
                    if(rc_number_buffer_active>0)continue;
                    break;
                }
                if(token==9){
                    *rc_number_buffer++=L'.';
                    if(rc_match_logical_character(L'.'))
                        *rc_number_buffer++=L'.';
                    else --rc_source_cursor;
                    break;
                }
                if(token==29){
                    *rc_number_buffer++=character;
                    rc_read_fraction(rc_number_buffer);
                    break;
                }
                --rc_source_cursor;
                break;
            }
            continue;
        case 29:
            saved_output=rc_cpp_active;
            rc_cpp_active=1;
            rc_read_number(character);
            rc_cpp_active=saved_output;
            continue;
        case 36:
        case 40:
            rc_saved_macro_number=rc_number_buffer;
            if(rc_collect_macro_identifier(character)){
                macro=rc_find_macro();
                if(macro){
                    if(!macro->flags)goto expand;
                    *rc_number_buffer++=4;
                    *rc_number_buffer++=(WCHAR)(rc_macro_name_length-1);
                }
            }
            if(rc_in_condition_expression&&
               !memcmp(rc_macro_name,L"defined",16))
                rc_evaluate_defined(rc_macro_name);
            continue;
        case 37:
            rc_copy_expanded_quoted(L'"',rc_macro_is_wide());
            continue;
        case 38:
            rc_copy_expanded_quoted(L'\'',rc_macro_is_wide());
            continue;
        }
        *rc_number_buffer++=character;
    }
    return result;
}

void __stdcall rc_push_macro(RcMacro *macro)
{
    ++rc_expanding_macro;
    if(rc_expanding_macro>=64){
        rc_error_template=rc_message(1009);
        rc_format(rc_error_text,2048,rc_error_template,rc_macro_name);
        rc_input_fatal(1009);
    }
#ifdef RECON_NATIVE_TEST
    if(rc_expanding_macro<0||rc_expanding_macro>=64){ __asm int 3 }
#endif
    ++rc_number_buffer_active;
    rc_macro_frames[rc_expanding_macro].macro=macro;
    rc_macro_frames[rc_expanding_macro].output=rc_saved_macro_output;
    rc_macro_frames[rc_expanding_macro].arguments=(unsigned char)rc_expanded_argument_count;
    rc_macro_frames[rc_expanding_macro].expanded=0;
    rc_macro_frames[rc_expanding_macro].source=rc_source_cursor;
    rc_number_buffer=rc_saved_macro_number;
    rc_macro_frames[rc_expanding_macro].number=rc_saved_macro_number;
}
