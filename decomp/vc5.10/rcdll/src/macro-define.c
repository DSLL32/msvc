#include <string.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

int rc_macro_parameter_count;
WCHAR *rc_macro_replacement;

void __cdecl rc_parse_define(void)
{
    WCHAR character;
    unsigned length;
    int parameters;

    character=rc_read_nonspace();
    if(rc_character_token(character)!=40){
        strcpy(rc_error_text,rc_message(2007));
        rc_input_error(2007);
        rc_skip_line();
        return;
    }
    rc_read_identifier(character);
    rc_macro_replacement=rc_string_buffer;
    rc_macro_parameter_count=0;
    for(;;){
        character=*rc_source_cursor++;
        switch(rc_character_token(character)){
        case 1: continue;
        case 3:
            if(rc_resume_input()!=16)continue;
            break;
        case 0: goto read_replacement;
        case 2:
            if(rc_scan_comment())goto read_replacement;
            break;
        case 5:
            --rc_source_cursor;
            rc_define_macro(NULL,0,0);
            return;
        case 32:
            rc_parse_macro_parameters();
            if(!rc_macro_parameter_count)rc_macro_parameter_count=-1;
            goto read_replacement;
        }
        rc_error_template=rc_message(2008);
        rc_format(rc_error_text,2048,rc_error_template,character);
        rc_input_error(2008);
read_replacement:
        parameters=rc_macro_parameter_count;
        length=rc_read_macro_replacement();
        rc_define_macro(rc_macro_replacement,length,parameters);
        return;
    }
}
