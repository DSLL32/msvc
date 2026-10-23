#include <string.h>
#include "command.h"
#include "preprocess.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strlen)
#endif

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
