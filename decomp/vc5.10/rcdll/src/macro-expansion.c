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

void __cdecl rc_select_macro_text(void)
{
#ifdef RECON_NATIVE_TEST
    if(rc_expanding_macro<0||rc_expanding_macro>=64){ __asm int 3 }
#endif
    rc_source_cursor=rc_macro_frames[rc_expanding_macro].macro->text;
    ++rc_macro_frames[rc_expanding_macro].macro->flags;
}

/* Native-only ownership holds preserve the Win32 buffer logic unchanged. */
#ifdef RECON_NATIVE_TEST
#define ARGUMENT_WRITE_SPACE() \
    if(((DWORD)rc_number_buffer&1) || \
       (DWORD)rc_number_buffer<(DWORD)&rc_string_buffer[1024] || \
       (DWORD)rc_number_buffer>(DWORD)&rc_string_buffer[2044]){ __asm int 3 }
#define ARGUMENT_PREVIOUS_SPACE() \
    if(((DWORD)rc_number_buffer&1) || \
       (DWORD)rc_number_buffer<(DWORD)&rc_string_buffer[1025] || \
       (DWORD)rc_number_buffer>(DWORD)&rc_string_buffer[2045]){ __asm int 3 }
#else
#define ARGUMENT_WRITE_SPACE()
#define ARGUMENT_PREVIOUS_SPACE()
#endif

void __stdcall rc_expand_macro_arguments(RcMacro *macro,int count)
{
    WCHAR character,*argument,*saved_number,*saved_output;
    WCHAR *output_before=rc_macro_output_cursor;
    unsigned short token;
    int parentheses=0,expanded,resumed,message;

    ARGUMENT_WRITE_SPACE();
    *rc_number_buffer++=rc_source_cursor[-1];
    do{
        if(rc_number_buffer>=&rc_string_buffer[2043])
            rc_macro_buffer_fatal(10056);
        for(;;){
            if(!rc_skip_expression_space())return;
            if(rc_character_token(*rc_source_cursor)!=2)break;
            ++rc_source_cursor;
            if(rc_scan_comment())continue;
            argument=rc_number_buffer;
            ARGUMENT_WRITE_SPACE();
            *rc_number_buffer++=L'/';
            goto collect_argument;
        }
        argument=rc_number_buffer;
collect_argument:
        expanded=0;
        for(;;){
            character=*rc_source_cursor++;
            token=rc_character_token(character);
            switch(token){
            case 0:
            case 1:
            case 5:
                --rc_source_cursor;
                if(!rc_skip_expression_space())return;
                continue;
            case 2:
                if(rc_scan_comment()){
                    ARGUMENT_PREVIOUS_SPACE();
                    if(rc_number_buffer[-1]==L' ')continue;
                    character=L' ';
                }
                break;
            case 3:
                saved_number=rc_number_buffer;
                saved_output=rc_macro_output_cursor;
                resumed=rc_resume_input();
                if(resumed&10)return;
                rc_macro_output_cursor=saved_output;
                rc_number_buffer=saved_number;
                if(resumed!=16)continue;
                ARGUMENT_WRITE_SPACE();
                *rc_number_buffer++=character;
                character=rc_get_logical_character();
                break;
            case 7:
            case 8:
                rc_number_buffer=rc_copy_replacement_quoted(rc_number_buffer,character);
                continue;
            case 27:
                if(!parentheses)goto finish_argument;
                break;
            case 28:
                if(--parentheses<0)goto finish_argument;
                break;
            case 32:
                ++parentheses;
                break;
            case 40:
                for(;;){
                    ARGUMENT_WRITE_SPACE();
                    *rc_number_buffer++=character;
                    character=*rc_source_cursor++;
                    if(!(rc_character_class(character)&16))break;
                    if(rc_number_buffer>=&rc_string_buffer[2043])
                        rc_macro_buffer_fatal(10056);
                }
                if(rc_character_token(character)!=36){
                    --rc_source_cursor;
                    continue;
                }
                expanded=1;
                /* Continue with the encoded argument-reference case. */
            case 36:
                rc_append_macro_text(rc_substitute_macro_argument(&expanded));
                continue;
            case 37:
                rc_copy_expanded_quoted(L'"',rc_macro_is_wide());
                continue;
            case 38:
                rc_copy_expanded_quoted(L'\'',rc_macro_is_wide());
                continue;
            }
            ARGUMENT_WRITE_SPACE();
            *rc_number_buffer++=character;
        }
finish_argument:
        ARGUMENT_PREVIOUS_SPACE();
        if(!rc_character_token(rc_number_buffer[-1]))--rc_number_buffer;
        if(rc_number_buffer>argument){
            ++rc_expanded_argument_count;
            rc_pack_macro_argument(argument,rc_number_buffer);
        }
        ARGUMENT_WRITE_SPACE();
        *rc_number_buffer++=character;
    }while(parentheses>=0);
    rc_saved_macro_output=output_before;
    if(count!=rc_expanded_argument_count){
        message=count<rc_expanded_argument_count?4002:4003;
        rc_error_template=rc_message(message);
        rc_format(rc_error_text,2048,rc_error_template,rc_macro_name);
        rc_input_warning(message);
    }
    if(macro->text){
        rc_push_macro(macro);
        rc_advance_macro_text();
    }else{
        rc_macro_output_cursor=rc_saved_macro_output;
        rc_number_buffer=rc_saved_macro_number;
    }
}

#undef ARGUMENT_WRITE_SPACE
#undef ARGUMENT_PREVIOUS_SPACE
