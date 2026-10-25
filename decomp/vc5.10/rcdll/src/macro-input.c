#include <string.h>
#include <wchar.h>
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

int __cdecl rc_finish_macro_frame(void)
{
#ifdef RECON_NATIVE_TEST
    if(rc_number_buffer_active!=1||rc_expanding_macro<1||rc_expanding_macro>=64||
       ((DWORD)rc_number_buffer&1)||
       (DWORD)rc_number_buffer<(DWORD)&rc_string_buffer[1024]||
       (DWORD)rc_number_buffer>=(DWORD)&rc_string_buffer[2045]){ __asm int 3 }
#endif
    --rc_number_buffer_active;
    if(!rc_number_buffer_active){
        if(rc_number_buffer>=&rc_string_buffer[2043])rc_macro_buffer_fatal(10056);
        if(rc_apstudio&&!rc_in_condition_expression&&
           !rc_macro_frames[rc_expanding_macro].macro->parameters)
            rc_emit_macro_reference(rc_macro_frames[rc_expanding_macro].macro);
        *rc_number_buffer++=0;
        *rc_number_buffer++=L'R';
        rc_source_cursor=rc_macro_frames[rc_expanding_macro].number;
        return 1;
    }
    rc_source_cursor=rc_macro_frames[rc_expanding_macro].source;
    rc_macro_output_cursor=rc_macro_frames[rc_expanding_macro].output;
    --rc_macro_frames[rc_expanding_macro].macro->flags;
    --rc_expanding_macro;
    return 0;
}

void __stdcall rc_pack_macro_argument(const WCHAR *begin,const WCHAR *end)
{
    int length;
    WCHAR *cursor;

#ifdef RECON_NATIVE_TEST
    /* The encountered argument spelling is in the upper work area; packed
     * records occupy the separate lower half. Odd record alignment is unproved. */
    if(((DWORD)begin&1) || ((DWORD)end&1) ||
       (DWORD)begin<(DWORD)&rc_string_buffer[1024] ||
       (DWORD)end<(DWORD)begin ||
       (DWORD)end>(DWORD)&rc_string_buffer[2045] ||
       ((DWORD)rc_macro_output_cursor&1) ||
       (DWORD)rc_macro_output_cursor<(DWORD)rc_string_buffer ||
       (DWORD)rc_macro_output_cursor>(DWORD)&rc_string_buffer[1023]){ __asm int 3 }
#endif
    length=end-begin;
    if(rc_macro_output_cursor+length>=&rc_string_buffer[1022])
        rc_macro_buffer_fatal(10056);
    wcsncpy(rc_macro_output_cursor+1,begin,length);
    cursor=rc_macro_output_cursor+length+1;
    *cursor=0;
    if((DWORD)cursor&1)*++cursor=L'P';
    *++cursor=L'A';
    ++cursor;
    *rc_macro_output_cursor=(WCHAR)((cursor-rc_macro_output_cursor)*sizeof(WCHAR));
    rc_macro_output_cursor=cursor;
}
