#include <string.h>
#include <wctype.h>
#include "command.h"
#include "directives.h"
#ifdef RECON_NATIVE_TEST
#include "macro-expansion.h"
#endif
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

int rc_number_buffer_active;
/* Alternate numeric spelling starts inside the encountered string work area.
 * Native reads require live macro text or the current packed argument record. */
WCHAR *rc_number_buffer=&rc_number_spelling_address_identity;

unsigned char __stdcall rc_read_number(WCHAR first)
{
    WCHAR *begin,*cursor,character;
    unsigned radix,value[2];
    unsigned char token=7;

#ifdef RECON_NATIVE_TEST
    if(rc_number_buffer_active){
        RcMacro *macro;
        if((rc_number_buffer_active!=1 &&
            !(rc_number_buffer_active==2&&rc_expanding_macro==2))||
           rc_expanding_macro<1||rc_expanding_macro>=64||
           ((DWORD)rc_number_buffer&1)||
           (DWORD)rc_number_buffer<(DWORD)&rc_string_buffer[1024]||
           (DWORD)rc_number_buffer>=(DWORD)&rc_string_buffer[2045]){ __asm int 3 }
        macro=rc_macro_frames[rc_expanding_macro].macro;
        if(!macro||!macro->text||((DWORD)rc_source_cursor&1)){ __asm int 3 }
        if((DWORD)rc_source_cursor>(DWORD)macro->text&&
           (DWORD)rc_source_cursor<=(DWORD)(macro->text+wcslen(macro->text))){
            if(rc_source_cursor[-1]!=first||wcslen(macro->text)+1>
               (unsigned)(&rc_string_buffer[2045]-rc_number_buffer)){ __asm int 3 }
        }else{
            RcMacroFrame *frame=&rc_macro_frames[rc_expanding_macro];
            WCHAR *record=frame->output,*end,*scan;
            unsigned index=frame->expanded,bytes;
            if(!index||index>frame->arguments||
               ((DWORD)rc_macro_output_cursor&1)||
               (DWORD)rc_macro_output_cursor<(DWORD)rc_string_buffer||
               (DWORD)rc_macro_output_cursor>(DWORD)&rc_string_buffer[1024]){ __asm int 3 }
            for(;;){
                if(((DWORD)record&1)||
                   (DWORD)record<(DWORD)rc_string_buffer||
                   (DWORD)record>(DWORD)rc_macro_output_cursor-6){ __asm int 3 }
                bytes=*record;
                if(bytes<6||(bytes&1)||bytes>
                   (DWORD)rc_macro_output_cursor-(DWORD)record){ __asm int 3 }
                end=record+bytes/2;
                if(end[-2]!=0||end[-1]!=L'A'){ __asm int 3 }
                if(!--index)break;
                record=end;
            }
            /* The producer initializes payload, terminator and marker. Do not
             * admit arbitrary lower-workspace addresses or earlier NUL tails. */
            for(scan=record+1;scan<end-2&&*scan;++scan);
            if(scan!=end-2||
               (DWORD)rc_source_cursor<(DWORD)(record+2)||
               (DWORD)rc_source_cursor>(DWORD)(end-2)||
               rc_source_cursor[-1]!=first||bytes/2-2>
               (unsigned)(&rc_string_buffer[2045]-rc_number_buffer)){ __asm int 3 }
        }
    }
#endif
    begin=rc_number_buffer_active?rc_number_buffer:rc_macro_name;
    cursor=begin;
    if(first==L'0'){
        first=rc_get_logical_character();
        if(towupper(first)==L'X'){
            radix=16;
            if(rc_cpp_active){*cursor++=L'0';*cursor++=L'x';}
            for(;;){
                first=rc_get_logical_character();
                if(!(rc_character_class(first)&8))break;
                *cursor++=first;
            }
            if(cursor==rc_macro_name&&!rc_number_buffer_active){
                strcpy(rc_error_text,rc_message(2153));
                rc_input_error(2153);
                begin=rc_macro_name;
            }
            goto suffix;
        }
        radix=8;
        *cursor++=L'0';
    }else radix=10;
    while(rc_character_class(first)&4){
        *cursor++=first;
        first=rc_get_logical_character();
    }
    if(first==L'.'||towupper(first)==L'E'){
        --rc_source_cursor;
        return rc_read_fraction(cursor);
    }
suffix:
    if(towupper(first)==L'L'){
        if(rc_cpp_active)*cursor++=first;
        character=rc_get_logical_character();
        if(towupper(character)==L'U'){
            if(rc_cpp_active)*cursor++=character;
            token=10;
        }else{
            token=8;
            --rc_source_cursor;
        }
    }else if(towupper(first)==L'U'){
        if(rc_cpp_active)*cursor++=first;
        character=rc_get_logical_character();
        if(towupper(character)==L'L'){
            if(rc_cpp_active)*cursor++=character;
            token=10;
        }else{
            --rc_source_cursor;
            token=9;
        }
    }else --rc_source_cursor;
    *cursor=0;
    if(begin==rc_number_buffer){
        rc_number_buffer=cursor;
        return 1;
    }
    if(rc_cpp_active){
        rc_write_preprocessed(rc_macro_name,(unsigned)(cursor-rc_macro_name)*2,1,rc_cpp_output_file);
        return 1;
    }
    value[0]=rc_parse_integer_value(rc_macro_name,radix);
    switch(token){
    case 7:token=radix==10?rc_decimal_integer_kind(value[0]):rc_based_integer_kind(value[0]);break;
    case 8:token=rc_long_integer_kind(value[0]);break;
    case 9:token=rc_unsigned_integer_kind(value[0]);break;
    }
    rc_token_value=rc_store_integer(token,value);
    return token;
}

unsigned char __stdcall rc_read_fraction(WCHAR *cursor)
{
    WCHAR character=rc_get_logical_character();
    unsigned char token;

    if(rc_cpp_xc&&!rc_number_buffer_active){
        strcpy(rc_error_text,rc_message(4012));
        rc_input_warning(4012);
        rc_cpp_xc=0;
    }
    if((rc_character_class(character)&4)||character==L'.'){
        do{
            *cursor++=character;
            character=rc_get_logical_character();
        }while(rc_character_class(character)&4);
    }
    if(towupper(character)==L'E'){
        *cursor++=character;
        character=rc_get_logical_character();
        if(character==L'+'||character==L'-'){
            *cursor++=character;
            character=rc_get_logical_character();
        }
        if(!(rc_character_class(character)&4)){
            if(!rc_cpp_R){
                if(!rc_number_buffer_active){
                    rc_error_template=rc_message(2021);
                    rc_format(rc_error_text,2048,rc_error_template,character);
                    rc_input_error(2021);
                }
                *cursor++=L'0';
            }
        }else{
            do{
                *cursor++=character;
                character=rc_get_logical_character();
            }while(rc_character_class(character)&4);
        }
    }
    if(towupper(character)==L'F')token=4;
    else if(towupper(character)==L'L')token=6;
    else{
        --rc_source_cursor;
        token=5;
        goto terminated;
    }
    if(rc_cpp_active)*cursor++=character;
terminated:
    *cursor=0;
    if(rc_number_buffer_active>0){
        rc_number_buffer=cursor;
        return 1;
    }
    if(rc_cpp_active){
        rc_write_preprocessed(rc_macro_name,(unsigned)(cursor-rc_macro_name)*2,
            1,rc_cpp_output_file);
        return 1;
    }
    return token;
}
