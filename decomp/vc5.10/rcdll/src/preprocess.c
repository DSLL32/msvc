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
