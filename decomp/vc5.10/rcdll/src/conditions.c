#include <string.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

int __cdecl rc_push_condition(void)
{
    ++rc_cpp_nesting_level;
    if(rc_cpp_nesting_level>=32){
        strcpy(rc_error_text,rc_message(1068));
        rc_input_fatal(1068);
    }
    return rc_cpp_nesting_level;
}
