#include <string.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

/* Character classes come from the preprocessor's contextual character table. */
#define NEXT_CLASS() rc_character_token(*rc_source_cursor++)
#define RETURN_OPERATOR(code) { \
    rc_token_kind=(code); \
    *(unsigned char *)&rc_token_value=(code); \
    return (unsigned char)rc_token_info[(code)].kind; \
}

#define RETURN_TOKEN(code) { rc_token_kind=(code); \
    *(unsigned char *)&rc_token_value=(code); return (code); }

unsigned char __cdecl rc_next_token(void)
{
    WCHAR character,escaped;
    unsigned short first,next,dispatch;
    unsigned char token;
    RcMacro *macro;

read_character:
    character=*rc_source_cursor++;
    first=rc_character_token(character);
    dispatch=first;
redispatch:
    switch(dispatch){
    case 0:                         /* Horizontal whitespace. */
        if(!rc_cpp_active){
            do {character=*rc_source_cursor++;}
            while(rc_character_class(character)&64);
        }else{
            character=rc_source_cursor[-1];
            do {
                rc_write_preprocessed(&character,2,1,rc_cpp_output_file);
                character=*rc_source_cursor++;
            }while(rc_character_class(character)&64);
        }
        --rc_source_cursor;
        goto read_character;
    case 1: goto read_character;
    case 2:                         /* Slash, comment or /=. */
        first=NEXT_CLASS();
        if(first==2){rc_line_comment();goto read_character;}
        if(first==4){rc_block_comment();goto read_character;}
        if(first==13){RETURN_OPERATOR(0x35)}
        token=0x34;break;
    case 3:                         /* Source boundary or a line splice. */
        if(rc_source_cursor[-1]==L'\\') goto splice;
        if(!rc_source_depth){
            if(rc_refill_source()&&!rc_open_source_frame()){
                RETURN_OPERATOR(0x66)
            }
        }else rc_resume_input();
        goto read_character;
    case 4:                         /* Star or an unmatched comment end. */
        first=NEXT_CLASS();
        if(first==2){
            if(rc_cpp_active){
                rc_write_preprocessed(L"*/",4,1,rc_cpp_output_file);
                goto read_character;
            }
            strcpy(rc_error_text,rc_message(2138));
            rc_input_error(2138);
            goto read_character;
        }
        if(first==13){RETURN_OPERATOR(0x46)}
        token=0x45;break;
    case 5:                         /* Newline. */
        if(rc_in_directive){
            --rc_source_cursor;
            rc_token_kind=1;
            return 1;
        }
        if(rc_cpp_active)rc_write_preprocessed(L"\r\n",4,1,rc_cpp_output_file);
scan_line:
        rc_scan_line_start();
        goto read_character;
    case 6:
splice:
        if(rc_cpp_active){
            rc_write_preprocessed(L"\\",2,1,rc_cpp_output_file);
            escaped=rc_get_logical_character();
            rc_write_preprocessed(&escaped,2,1,rc_cpp_output_file);
            goto read_character;
        }
        if(rc_splice_line())goto read_character;
        strcpy(rc_error_text,rc_message(2017));
        rc_input_error(2017);
        goto read_character;
    case 7:                         /* Character constant. */
        if(rc_cpp_active){rc_copy_quoted(L'\'');goto read_character;}
        token=rc_read_character_constant();
        goto parsed_token;
    case 8:                         /* String constant. */
        if(rc_cpp_active){rc_copy_quoted(L'"');goto read_character;}
        rc_read_string();
        rc_token_kind=3;
        return 3;
    case 9:                         /* Dot, decimal fraction or ellipsis. */
dot_character:
        next=NEXT_CLASS();
        if(next==3){
            if(rc_resume_input()==16)goto dot_done;
            goto dot_character;
        }
        if(next==6){
            if(rc_splice_line())goto dot_character;
            --rc_source_cursor;
            goto dot_done;
        }
        if(next==29){
            rc_macro_name[0]=L'.';
            rc_macro_name[1]=rc_source_cursor[-1];
            token=rc_read_fraction(rc_macro_name+2);
            goto parsed_token;
        }
        if(next==9){
            if(!rc_match_logical_character(L'.')){
                strcpy(rc_error_text,rc_message(2142));
                rc_input_error(2142);
            }
            rc_token_kind=99;
            *(unsigned char *)&rc_token_value=99;
            return 99;
        }
dot_done:
        --rc_source_cursor;
        RETURN_OPERATOR(0x4b)
    case 10:
        first=NEXT_CLASS();
        if(first==13){RETURN_OPERATOR(0x47)}
        token=0x37;break;
    case 11:
        if(rc_cpp_active){rc_write_preprocessed(L"#",2,1,rc_cpp_output_file);goto read_character;}
        strcpy(rc_error_text,rc_message(2014));
        rc_input_error(2014);
        --rc_source_cursor;
        --rc_preparing_line;
        goto scan_line;
    case 12:
        first=NEXT_CLASS();
        if(first==13){RETURN_OPERATOR(0x44)}
        token=0x43;break;
    case 13:
        first=NEXT_CLASS();
        if(first==13){RETURN_OPERATOR(0x36)}
        token=0x2f;break;
    case 14:
        first=NEXT_CLASS();
        if(first==13){RETURN_OPERATOR(0x57)}
        token=0x56;break;
    case 15:
        first=NEXT_CLASS();
        if(first==13){RETURN_OPERATOR(0x49)}
        if(first==15){RETURN_OPERATOR(0x4a)}
        token=0x48;break;
    case 16:
        first=NEXT_CLASS();
        if(first==13){RETURN_OPERATOR(0x2e)}
        if(first==16){RETURN_OPERATOR(0x2d)}
        token=0x2c;break;
    case 17:
        first=NEXT_CLASS();
        if(first==13){RETURN_OPERATOR(0x4d)}
        if(first==17){RETURN_OPERATOR(0x3b)}
        token=0x4c;break;
    case 18:
        first=NEXT_CLASS();
        if(first==13){RETURN_OPERATOR(0x42)}
        if(first==18){RETURN_OPERATOR(0x33)}
        if(first==20){RETURN_OPERATOR(0x4e)}
        token=0x41;break;
    case 19:
        first=NEXT_CLASS();
        if(first==13){RETURN_OPERATOR(0x40)}
        if(first==19){
            first=NEXT_CLASS();
            if(first==13){RETURN_OPERATOR(0x3d)}
            token=0x3e;dispatch=21;
        }else token=0x3f;
        break;
    case 20:
        first=NEXT_CLASS();
        if(first==13){RETURN_OPERATOR(0x3a)}
        if(first==20){
            first=NEXT_CLASS();
            if(first==13){RETURN_OPERATOR(0x52)}
            token=0x53;dispatch=22;
        }else token=0x39;
        break;
    case 21:
        next=NEXT_CLASS();
        if(next==13){RETURN_OPERATOR(0x3d)}
        --rc_source_cursor;
        RETURN_OPERATOR(0x3e)
    case 22:
        next=NEXT_CLASS();
        if(next==13){RETURN_OPERATOR(0x52)}
        --rc_source_cursor;
        RETURN_OPERATOR(0x53)
    case 23:
        if(rc_cpp_active){
            rc_write_preprocessed(rc_source_cursor-1,2,1,rc_cpp_output_file);
            goto read_character;
        }
        goto invalid_character;
    case 24: RETURN_TOKEN(0x76)
    case 25: RETURN_TOKEN(0x75)
    case 26: RETURN_TOKEN(0x31)
    case 27: RETURN_TOKEN(0x32)
    case 28: RETURN_TOKEN(0x79)
    case 29:
        token=rc_read_number(rc_source_cursor[-1]);
        goto parsed_token;
    case 30: RETURN_TOKEN(0x6f)
    case 31: RETURN_TOKEN(0x3c)
    case 32: RETURN_TOKEN(0x71)
    case 33: RETURN_TOKEN(0x51)
    case 34: RETURN_TOKEN(0x7b)
    case 35: RETURN_TOKEN(0x54)
    case 39:
        ++rc_source_cursor;
        goto read_character;
    case 40:                        /* Identifier, possibly a macro. */
        if(rc_source_depth<1)rc_read_identifier(rc_source_cursor[-1]);
        else if(!rc_read_macro_identifier(rc_source_cursor[-1]))goto identifier;
        macro=rc_find_macro();
        if(macro&&!macro->flags&&rc_expand_macro(macro))goto read_character;
identifier:
        if(rc_cpp_active){
            rc_write_preprocessed(rc_macro_name,rc_macro_name_length*2-2,1,rc_cpp_output_file);
            rc_token_kind=1;
            return 1;
        }
        rc_token_value=rc_macro_name;
        rc_token_hash=rc_macro_hash;
        rc_token_length=(unsigned char)rc_macro_name_length;
        return 2;
    case 41:
    case 43:                        /* Internal preprocessor markers. */
        character=rc_source_cursor[-1];
        if((!rc_keep_internal_marker||character!=0xe001)&&character!=0xe000&&character!=0xe002)
            goto invalid_character;
        rc_write_preprocessed(rc_source_cursor-1,2,1,rc_cpp_output_file);
        goto read_character;
    }
    if(first==3&&rc_resume_input()!=16)goto redispatch;
    --rc_source_cursor;
    rc_token_kind=token;
    *(unsigned char *)&rc_token_value=token;
#ifdef RECON_NATIVE_TEST
    if(!rc_token_info[token].description){ __asm int 3 }
#endif
    return (unsigned char)rc_token_info[token].kind;
parsed_token:
    rc_token_kind=token;
#ifdef RECON_NATIVE_TEST
    if(!rc_token_info[token].description){ __asm int 3 }
#endif
    return (unsigned char)rc_token_info[token].kind;
invalid_character:
    rc_error_template=rc_message(2018);
    rc_format(rc_error_text,2048,rc_error_template,rc_source_cursor[-1]);
    rc_input_error(2018);
    goto read_character;
}
