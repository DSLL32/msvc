#include <string.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

unsigned __stdcall rc_parse_integer_value(const WCHAR *text,int radix)
{
    const WCHAR *cursor=text;
    unsigned value=0,previous=0;
    int digit;

    while(*cursor){
        value*=radix;
        digit=rc_digit_value(*cursor);
        if(digit>=radix&&!rc_cpp_active){
            rc_error_template=rc_message(2020);
            rc_format(rc_error_text,2048,rc_error_template,*cursor,radix);
            rc_input_error(2020);
        }
        value+=digit;
        ++cursor;
        if(radix==10){
            if((int)value<(int)previous){--cursor;break;}
        }else if(*cursor){
            if(value&(radix==8?0xe0000000U:0xf0000000U))break;
        }
        previous=value;
    }
    if(*cursor){
        strcpy(rc_error_text,rc_message(2177));
        rc_input_error(2177);
        value=0;
    }
    return value;
}
