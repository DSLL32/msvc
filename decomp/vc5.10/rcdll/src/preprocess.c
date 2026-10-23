#include <string.h>
#include "command.h"
#include "preprocess.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strlen)
#pragma intrinsic(strcpy)
#endif

int rc_cpp_remaining,rc_cpp_active;
WCHAR **rc_cpp_arguments,*rc_cpp_bad_argument;
WCHAR *rc_cpp_option_argument;

int __stdcall rc_preprocess(int count,char **arguments,int unused)
{
    WCHAR **wide_arguments=rc_widen_arguments(count,arguments);
    return rc_preprocessor_main(count,wide_arguments);
}

WCHAR **__stdcall rc_widen_arguments(int count,char **arguments)
{
    unsigned bytes=0,length;
    int i;
    WCHAR **result,**slot,*text;

    for(i=0;i<count;++i) bytes+=strlen(arguments[i])+1;
    result=rc_allocate(count*sizeof(WCHAR *)+sizeof(WCHAR *)+bytes*sizeof(WCHAR));
    if(result){
        slot=result;
        text=(WCHAR *)(result+count+1);
        for(i=0;i<count;++i){
            *slot++=text;
            length=strlen(arguments[i])+1;
            MultiByteToWideChar(0,1,arguments[i],length,text,length);
            text+=length;
        }
        *slot=NULL;
    }
    return result;
}

int __cdecl rc_preprocessor_main(int count,WCHAR **arguments)
{
    WCHAR *argument;

    rc_cpp_remaining=count;
    rc_cpp_arguments=arguments;
    if(!arguments){
        strcpy(rc_error_text,rc_message(1002));
        rc_input_fatal(1007);
    }
    argument=rc_next_argument();
    while(rc_scan_options(rc_cpp_options,argument,rc_next_argument,0))
        argument=rc_next_argument();
    if(rc_cpp_bad_argument){
        rc_error_template=rc_message(1007);
        rc_format(rc_error_text,2048,rc_error_template,rc_cpp_bad_argument,"c1");
        rc_input_fatal(1007);
    }
    if(!rc_cpp_input){
        strcpy(rc_error_text,rc_message(1008));
        rc_input_fatal(1008);
    }
    if(!rc_cpp_output){
        strcpy(rc_error_text,rc_message(1010));
        rc_input_fatal(1010);
    }
    rc_cpp_active=1;
    if(!rc_cpp_E&&!rc_cpp_P) rc_cpp_E=1;
    wcsncpy(rc_filename,rc_cpp_input,128);
    rc_open_preprocessor(rc_cpp_input,rc_cpp_output,&rc_cpp_defines,&rc_cpp_undefines);
    rc_run_preprocessor();
    if(rc_cpp_nesting_level>=0){
        strcpy(rc_error_text,rc_message(1022));
        rc_input_fatal(1022);
    }
    rc_close_preprocessor();
    return rc_result;
}

WCHAR *__cdecl rc_next_argument(void)
{
    --rc_cpp_remaining;
    if(rc_cpp_remaining>0){
        ++rc_cpp_arguments;
        return *rc_cpp_arguments;
    }
    return NULL;
}

int __stdcall rc_scan_options(const RcOption *options,WCHAR *argument,
                             RcNextArgument next,int duplicate)
{
    const WCHAR *pattern;
    WCHAR *operand;

    if(!argument) return 0;
    rc_cpp_option_argument=argument;
    for(;options->action;++options){
        pattern=options->pattern;
        operand=argument;
        for(;;){
            if(!*pattern){
                if(*operand) goto next_option;
                operand=(options->action&0x20)?next():NULL;
                break;
            }
            if(*pattern==L'#'){
                if(!*operand) operand=next();
                break;
            }
            if(*pattern==L'*'){
                if(*operand&&rc_match_option(pattern,operand)) break;
                goto next_option;
            }
            if(*pattern!=*operand) goto next_option;
            ++pattern;
            ++operand;
        }
        rc_apply_option(options,operand,duplicate);
        return options->keep_scanning;
next_option:;
    }
    return 0;
}

void __stdcall rc_apply_option(const RcOption *option,WCHAR *operand,int duplicate)
{
    RcOptionList *list;
    RcLetterOption *letter;
    WCHAR *copy;
    unsigned message;

    switch(option->action){
    case 1:
        *(int *)option->destination=1;
        break;
    case 5:
        *(int *)option->destination=0;
        break;
    case 0x23:
        for(;*operand;++operand){
            letter=option->destination;
            while(letter->letter&&(WCHAR)letter->letter!=*operand) ++letter;
            if(letter->letter){
                if(letter->action==1) *letter->destination=1;
                else if(letter->action==5) *letter->destination=0;
            }
            if(!letter->letter){
                rc_error_template=rc_message(1048);
                rc_format(rc_error_text,2048,rc_error_template,(unsigned)*operand,
                          rc_cpp_option_argument);
                rc_input_fatal(1048);
            }
        }
        break;
    case 0x24:
        *(int *)option->destination=rc_option_number(operand);
        break;
    case 0x26:
        list=option->destination;
        if(list->remaining>0){
            if(duplicate) operand=_wcsdup(operand);
            --list->remaining;
            list->items[list->remaining]=operand;
            break;
        }
        message=1047;
        rc_error_template=rc_message(message);
        rc_format(rc_error_text,2048,rc_error_template,option->pattern,operand);
fatal_option:
        rc_input_fatal(message);
        break;
    case 0x27:
        if(*(int *)option->destination){
            copy=_wcsdup(option->pattern);
            copy[wcslen(copy)-1]=0;
            message=1000;
            rc_error_template=rc_message(message);
            goto fatal_option;
        }
        /* The first occurrence stores its operand like an ordinary string. */
    case 0x22:
        if(duplicate) operand=_wcsdup(operand);
        *(WCHAR **)option->destination=operand;
        break;
    }
}
