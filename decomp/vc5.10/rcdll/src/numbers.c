#include <string.h>
#include <wctype.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

int rc_number_buffer_active;
/* Address identity only. The original alternate buffer's contents and capacity
 * are not reconstructed; native execution holds before that mode can write it. */
WCHAR rc_number_spelling_address_identity;
WCHAR *rc_number_buffer=&rc_number_spelling_address_identity;

unsigned char __stdcall rc_read_number(WCHAR first)
{
    WCHAR *begin,*cursor,character;
    unsigned radix,value[2];
    unsigned char token=7;

#ifdef RECON_NATIVE_TEST
    if(rc_number_buffer_active){ __asm int 3 }
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
