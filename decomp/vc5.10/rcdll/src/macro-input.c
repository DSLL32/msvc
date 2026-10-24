#include <string.h>
#include "command.h"
#include "macro-expansion.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

int __cdecl rc_resume_input(void)
{
    int depth;
    WCHAR marker;

    if(rc_source_cursor[-1]==L'\\')return rc_splice_line()?1:16;
    depth=rc_expanding_macro;
    if(!depth){
        if(rc_refill_source()){
            strcpy(rc_error_text,rc_message(1004));
            rc_input_fatal(1004);
        }
        return 1;
    }
    for(;;){
        marker=*rc_source_cursor++;
        switch(marker){
        case L'A':
#ifdef RECON_NATIVE_TEST
            if(depth<0||depth>=64){ __asm int 3 }
#endif
            rc_pack_macro_argument(rc_macro_frames[depth].number,rc_number_buffer);
            rc_number_buffer=rc_macro_frames[rc_expanding_macro].number;
            rc_advance_macro_text();
            return 2;
        case L'D':return rc_finish_macro_frame()?8:4;
        case L'P':continue;
        case L'R':
#ifdef RECON_NATIVE_TEST
            if(depth<0||depth>=64){ __asm int 3 }
#endif
            rc_source_cursor=rc_macro_frames[depth].source;
            --rc_macro_frames[depth].macro->flags;
            if(rc_expanding_macro==1){
                rc_macro_output_cursor=rc_string_buffer;
                rc_number_buffer=&rc_string_buffer[1024];
            }
            --rc_expanding_macro;
            return 4;
        default:return 1;
        }
    }
}
