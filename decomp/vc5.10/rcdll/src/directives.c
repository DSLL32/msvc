#include <string.h>
#include <wchar.h>
#include "command.h"
#include "preprocess.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

int rc_expanding_macro,rc_in_directive,rc_in_condition_expression,rc_header_mode;
int rc_condition_state[32];
/* The lexer starts immediately after a newline token. */
unsigned char rc_token_kind=1;
RcTokenExtra rc_token_extra;
int rc_keep_internal_marker;
void *rc_token_value;

void __cdecl rc_handle_directive(void)
{
    WCHAR character,*text;
    const WCHAR *word=rc_macro_name;
    unsigned char directive;
    unsigned short token;
    int saved_output,condition,skipped,level;
    UINT codepage;
    unsigned message;

    if(rc_expanding_macro) return;
    character=rc_read_nonspace();
    token=rc_character_token(character);
    if(token==5){
        --rc_source_cursor;
        return;
    }
    if(token!=40){
        rc_error_template=rc_message(2019);
        rc_format(rc_error_text,2048,rc_error_template,character);
        rc_input_error(2019);
        rc_skip_line();
        return;
    }
    rc_read_identifier(character);
    rc_in_directive=1;
redispatch:
    directive=rc_directive_code(word);
    switch(directive){
    case PP_IFDEF:
    case PP_IFNDEF:
        character=rc_read_nonspace();
        if(rc_character_token(character)!=40){
            strcpy(rc_error_text,rc_message(1016));
            rc_input_fatal(1016);
        }
        rc_read_identifier(character);
        condition=rc_find_macro()!=NULL;
        rc_expect_end(directive==PP_IFDEF ? L"#ifdef" : L"#ifndef");
        if(directive==PP_IFNDEF) condition=!condition;
        if(!condition){
            skipped=rc_skip_inactive(5);
            if(skipped!=3){
                if(skipped==2){word=L"if";goto redispatch;}
                break;
            }
        }
        level=rc_push_condition();
        rc_condition_state[level]=1;
        break;
    case PP_IF:
        saved_output=rc_cpp_active;
        rc_cpp_active=0;
        rc_in_condition_expression=1;
        condition=rc_evaluate_condition();
        rc_in_condition_expression=0;
        rc_cpp_active=saved_output;
        rc_expect_end(L"#if/#elif");
        if(!condition){
            skipped=rc_skip_inactive(5);
            if(skipped!=3){
                if(rc_cpp_E) rc_emit_source_line();
                if(skipped==2){word=L"if";goto redispatch;}
                break;
            }
        }
        level=rc_push_condition();
        rc_condition_state[level]=1;
        if(rc_cpp_E && !condition) rc_emit_source_line();
        break;
    case PP_DEFINE:
        rc_parse_define();
        break;
    case PP_LINE:
        saved_output=rc_cpp_active;
        rc_cpp_active=0;
        rc_next_token();
        if(rc_token_kind!=7){
            rc_error_template=rc_message(2005);
            rc_format(rc_error_text,2048,rc_error_template,
                      rc_token_info[rc_token_kind].description);
            rc_input_error(2005);
            rc_cpp_active=saved_output;
            goto discard;
        }
        rc_preparing_line=((RcIntegerToken *)rc_token_value)->value-1;
        rc_next_token();
        rc_cpp_active=saved_output;
        if(rc_token_kind==3){
            if(wcscmp(rc_filename,rc_token_value))
                wcsncpy(rc_filename,rc_token_value,257);
        }else if(rc_token_kind!=1){
            rc_error_template=rc_message(2130);
            rc_format(rc_error_text,2048,rc_error_template,
                      rc_token_info[rc_token_kind].description);
            rc_input_error(2130);
            goto discard;
        }
        rc_emit_source_line();
        rc_expect_end(L"#line");
        break;
    case PP_INCLUDE:
        saved_output=rc_cpp_active;
        rc_cpp_active=0;
        rc_header_mode=1;
        rc_next_token();
        rc_header_mode=0;
        rc_cpp_active=saved_output;
        if(rc_token_kind!=3){
            if(rc_token_kind!=63){
                rc_error_template=rc_message(2006);
                rc_format(rc_error_text,2048,rc_error_template,
                          rc_token_info[rc_token_kind].description);
                rc_input_error(2006);
                goto discard;
            }
            if(!rc_parse_angle_name()) goto discard;
            rc_token_value=rc_macro_name;
        }
#ifdef RECON_NATIVE_TEST
        /* Hold unknown longer names before exceeding the contextual model. */
        if(wcslen(rc_token_value)>=257){ __asm int 3 }
#endif
        wcscpy(rc_source_marker,rc_token_value);
        rc_expect_end(L"#include");
        text=rc_token_value;
        if(wcschr(rc_cpp_path_characters,*text) ||
           (wcschr(rc_cpp_path_characters,L':') && text[1]==L':')){
            if(!rc_open_source(text,0)){
                rc_error_template=rc_message(1015);
                rc_format(rc_error_text,2048,rc_error_template,text);
                rc_input_fatal(1015);
            }
        }else if(rc_token_kind!=3 || !rc_search_current_directory())
            rc_search_include_directories();
        break;
    case PP_ELIF:
        level=rc_cpp_nesting_level--;
        if(level<0){
            strcpy(rc_error_text,rc_message(1018));
            rc_input_fatal(1018);
        }
        do skipped=rc_skip_inactive(5);while(skipped!=4);
        if(rc_cpp_E) rc_emit_source_line();
        break;
    case PP_ELSE:
        condition=0;
        if(rc_cpp_nesting_level>=0){
            level=rc_cpp_nesting_level--;
            condition=rc_condition_state[level]==1;
        }
        if(!condition){
            strcpy(rc_error_text,rc_message(1019));
            rc_input_fatal(1019);
        }
        rc_expect_end(L"#else");
        rc_skip_inactive(6);
        if(rc_cpp_E) rc_emit_source_line();
        break;
    case PP_ENDIF:
        level=rc_cpp_nesting_level--;
        if(level<0){
            strcpy(rc_error_text,rc_message(1020));
            rc_input_fatal(1020);
        }
        if(rc_cpp_E) rc_emit_source_line();
        rc_expect_end(L"#endif");
        break;
    case PP_PRAGMA:
        codepage=rc_parse_pragma();
        if(!codepage) break;
        if(codepage==1200) message=4213;
        else if(!IsValidCodePage(codepage)) message=4214;
        else{
            if(codepage==rc_codepage) break;
            if(!rc_change_codepage(codepage)){
                strcpy(rc_error_text,rc_message(1121));
                rc_input_fatal(1121);
            }
            rc_codepage=codepage;
            break;
        }
        strcpy(rc_error_text,rc_message(message));
        if(rc_warnings) rc_input_warning(message);
        else rc_input_fatal(message);
        break;
    case PP_UNDEF:
        character=rc_read_nonspace();
        if(rc_character_token(character)==40){
            rc_read_identifier(character);
            rc_undefine_macro();
        }else{
            strcpy(rc_error_text,rc_message(4006));
            rc_input_warning(4006);
        }
        rc_expect_end(L"#undef");
        break;
    case PP_ERROR:
        text=rc_macro_name;
        for(;;){
            character=rc_get_logical_character();
            if(character==3) break;
            if(character==10){--rc_source_cursor;break;}
            *text++=character;
        }
        *text=0;
        rc_error_template=rc_message(2189);
        rc_format(rc_error_text,2048,rc_error_template,rc_macro_name);
        rc_input_error(2188);
        rc_expect_end(L"#error");
        break;
    case PP_IDENT:
        saved_output=rc_cpp_active;
        rc_cpp_active=0;
        rc_next_token();
        rc_cpp_active=saved_output;
        if(rc_token_kind!=3){
            rc_error_template=rc_message(4079);
            rc_format(rc_error_text,2048,rc_error_template,
                      rc_token_info[rc_token_kind].description);
            rc_input_warning(4079);
        }
        rc_expect_end(L"#error");
        break;
    case PP_UNKNOWN:
        rc_error_template=rc_message(1021);
        rc_format(rc_error_text,2048,rc_error_template,word);
        rc_input_fatal(1021);
        break;
    }
    rc_in_directive=0;
    return;
discard:
    rc_skip_line();
    rc_in_directive=0;
}

/* Balance is reset before each preprocessing constant expression. */
int rc_expression_parentheses;

int __cdecl rc_evaluate_condition(void)
{
    int value;
    rc_expression_parentheses=0;
    rc_token_kind=1;
    value=rc_parse_conditional_expression();
    if(rc_token_kind==121){
        if(!rc_expression_parentheses--){
            rc_error_template=rc_message(1012);
            rc_format(rc_error_text,2048,rc_error_template,L"(");
            rc_input_fatal(1012);
        }
    }else if(rc_token_kind!=1){
        rc_error_template=rc_message(4067);
        rc_format(rc_error_text,2048,rc_error_template,L"#if/#elif");
        rc_input_warning(4067);
    }
    if(rc_expression_parentheses>0){
        rc_error_template=rc_message(4012);
        rc_format(rc_error_text,2048,rc_error_template,L")");
        rc_input_fatal(4012);
    }
    return value;
}

int __cdecl rc_parse_conditional_expression(void)
{
    int value,selected,alternate;
    value=rc_parse_logical_or();
    if(rc_accept_expression_token(81)){
        selected=rc_parse_logical_or();
        if(rc_accept_expression_token(49))
            alternate=rc_parse_logical_or();
        value=value ? selected : alternate;
    }
    return value;
}

int __cdecl rc_parse_logical_or(void)
{
    int value,right;
    value=rc_parse_logical_and();
    while(rc_accept_expression_token(74)){
        right=rc_parse_logical_and();
        if(right || value)value=1;
    }
    return value;
}

int __cdecl rc_parse_logical_and(void)
{
    int value,right;
    value=rc_parse_bitwise_or();
    while(rc_accept_expression_token(45)){
        right=rc_parse_bitwise_or();
        value=right && value;
    }
    return value;
}
