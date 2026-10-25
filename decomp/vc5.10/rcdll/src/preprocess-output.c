#include <string.h>
#include <wchar.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

void __cdecl rc_run_preprocessor(void)
{
    unsigned char token;
    const WCHAR *text;

    for(;;){
        token=rc_next_token();
        if(!token)return;
        if(token==1)continue;
        if(!rc_token_kind){
            strcpy(rc_error_text,rc_message(1011));
            rc_input_fatal(1011);
        }
        text=rc_token_info[rc_token_kind].description;
#ifdef RECON_NATIVE_TEST
        /* Unavailable descriptor rows remain unsupported, not empty strings. */
        if(!text){ __asm int 3 }
#endif
        rc_write_preprocessed(text,wcslen(text)*2,1,rc_cpp_output_file);
    }
}
