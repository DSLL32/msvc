#include "command.h"
#include "preprocess.h"
#include <recon.h>

int __stdcall rc_preprocess(int count,char **arguments,int unused)
{
    WCHAR **wide_arguments=rc_widen_arguments(count,arguments);
    return rc_preprocessor_main(count,wide_arguments);
}
