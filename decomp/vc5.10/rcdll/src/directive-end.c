#include "command.h"
#include "directives.h"
#include <recon.h>

void __stdcall rc_expect_end(const WCHAR *word)
{
    if(rc_read_nonspace()==L'\n')
        --rc_source_cursor;
    else{
        rc_error_template=rc_message(4067);
        rc_format(rc_error_text,2048,rc_error_template,word);
        rc_input_warning(4067);
        rc_skip_line();
    }
}
