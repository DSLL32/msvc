#include <string.h>
#include <wchar.h>
#include "command.h"
#include "macro-expansion.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#pragma intrinsic(memcpy)
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
    if((rc_number_buffer_active!=1 &&
        !(rc_number_buffer_active==2&&rc_expanding_macro==2))||
       rc_expanding_macro<1||rc_expanding_macro>=64||
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

void __cdecl rc_advance_macro_text(void)
{
    RcMacroFrame *frame;
#ifdef RECON_NATIVE_TEST
    if(rc_expanding_macro<1 || rc_expanding_macro>=64){ __asm int 3 }
#endif
    frame=&rc_macro_frames[rc_expanding_macro];
    if(frame->arguments>frame->expanded)
        rc_select_macro_argument(++frame->expanded);
    else rc_select_macro_text();
}

#ifdef RECON_NATIVE_TEST
#define ARGUMENT_RECORD(cursor) do { \
    int bytes; \
    if(((DWORD)(cursor)&1) || \
       (DWORD)(cursor)<(DWORD)rc_string_buffer || \
       (DWORD)(cursor)>=(DWORD)&rc_string_buffer[1024] || \
       (DWORD)(cursor+3)>(DWORD)rc_macro_output_cursor){ __asm int 3 } \
    bytes=*(short *)(cursor); \
    if(bytes<6 || (bytes&1) || \
       (DWORD)(cursor)+bytes>(DWORD)rc_macro_output_cursor || \
       (cursor)[bytes/2-2]!=0 || (cursor)[bytes/2-1]!=L'A'){ __asm int 3 } \
} while(0)
#else
#define ARGUMENT_RECORD(cursor)
#endif

void __stdcall rc_select_macro_argument(unsigned char index)
{
    WCHAR *cursor;
#ifdef RECON_NATIVE_TEST
    if(rc_expanding_macro<1 || rc_expanding_macro>=64 || !index ||
       index>rc_macro_frames[rc_expanding_macro].arguments ||
       ((DWORD)rc_macro_output_cursor&1) ||
       (DWORD)rc_macro_output_cursor<(DWORD)rc_string_buffer ||
       (DWORD)rc_macro_output_cursor>(DWORD)&rc_string_buffer[1024]){ __asm int 3 }
#endif
    cursor=rc_macro_frames[rc_expanding_macro].output;
    --index;
    while(index--){
        ARGUMENT_RECORD(cursor);
        cursor=(WCHAR *)((char *)cursor+*(short *)cursor);
    }
    ARGUMENT_RECORD(cursor);
    rc_source_cursor=cursor+1;
}

const WCHAR *__stdcall rc_substitute_macro_argument(int *expanded)
{
    unsigned short index;
    WCHAR *cursor;
    int follows;
#ifdef RECON_NATIVE_TEST
    RcMacro *macro;
    if(rc_expanding_macro<1||rc_expanding_macro>=64||!expanded||
       ((DWORD)rc_source_cursor&1)){ __asm int 3 }
    macro=rc_macro_frames[rc_expanding_macro].macro;
    if(!macro||!macro->text||
       (DWORD)rc_source_cursor<(DWORD)macro->text||
       (DWORD)rc_source_cursor>=(DWORD)(macro->text+wcslen(macro->text))){ __asm int 3 }
#endif
    index=*rc_source_cursor++;
    cursor=rc_macro_frames[rc_expanding_macro].output;
    if(index>rc_macro_frames[rc_expanding_macro].arguments)return NULL;
    follows=rc_macro_identifier_follows();
    if(!*expanded&&!follows)
        index+=rc_macro_frames[rc_expanding_macro].arguments;
    *expanded=follows;
    if(index){
        while(--index){
            ARGUMENT_RECORD(cursor);
            cursor=(WCHAR *)((char *)cursor+*(short *)cursor);
        }
    }
    ARGUMENT_RECORD(cursor);
    return cursor+1;
}

const WCHAR *__cdecl rc_macro_argument_text(void)
{
    unsigned short index;
    WCHAR *cursor;
#ifdef RECON_NATIVE_TEST
    RcMacro *macro;
    if(rc_expanding_macro<1||rc_expanding_macro>=64||
       ((DWORD)rc_source_cursor&1)){ __asm int 3 }
    macro=rc_macro_frames[rc_expanding_macro].macro;
    if(!macro||!macro->text||
       (DWORD)rc_source_cursor<(DWORD)macro->text||
       (DWORD)rc_source_cursor>=(DWORD)(macro->text+wcslen(macro->text))){ __asm int 3 }
#endif
    index=*rc_source_cursor++;
    cursor=rc_macro_frames[rc_expanding_macro].output;
    if(index>rc_macro_frames[rc_expanding_macro].arguments)return NULL;
    if(index){
        while(--index){
            ARGUMENT_RECORD(cursor);
            cursor=(WCHAR *)((char *)cursor+*(short *)cursor);
        }
    }
    ARGUMENT_RECORD(cursor);
    return cursor+1;
}

#undef ARGUMENT_RECORD

#ifdef RECON_NATIVE_TEST
#define IDENTIFIER_SPACE() \
    if(((DWORD)rc_number_buffer&1) || \
       (DWORD)rc_number_buffer<(DWORD)&rc_string_buffer[1024] || \
       (DWORD)rc_number_buffer>(DWORD)&rc_string_buffer[2044]){ __asm int 3 }
#else
#define IDENTIFIER_SPACE()
#endif

int __stdcall rc_collect_macro_identifier(WCHAR first)
{
    WCHAR *begin=rc_number_buffer,character;
    const WCHAR *text;
    int valid=1,argument_mode=0;
    unsigned blocked_length=0;

    IDENTIFIER_SPACE();
copy_part:
    if(rc_character_token(first)==36){
        text=rc_substitute_macro_argument(&argument_mode);
        if(text){
            while(*text){
                if(!(rc_character_class(*text)&16))valid=0;
                if(rc_number_buffer>=&rc_string_buffer[2043])
                    rc_macro_buffer_fatal(10056);
                IDENTIFIER_SPACE();
                *rc_number_buffer++=*text++;
            }
        }
    }else{
        IDENTIFIER_SPACE();
        *rc_number_buffer++=first;
    }
    for(;;){
        character=*rc_source_cursor++;
        if(rc_character_class(character)&16){
            if(rc_number_buffer>=&rc_string_buffer[2043])
                rc_macro_buffer_fatal(10056);
            IDENTIFIER_SPACE();
            *rc_number_buffer++=character;
            continue;
        }
        if(rc_character_token(character)==39){
            blocked_length=*rc_source_cursor++;
            continue;
        }
        if(rc_character_token(character)==36){
            argument_mode=1;
            first=character;
            goto copy_part;
        }
        --rc_source_cursor;
        break;
    }
    if(valid && blocked_length==(unsigned)(rc_number_buffer-begin))valid=0;
    if(valid){
        if((int)(rc_number_buffer-begin)*2>62 && !rc_cpp_active){
            rc_number_buffer=begin+31;
            IDENTIFIER_SPACE();
            *rc_number_buffer=0;
            rc_error_template=rc_message(4011);
            rc_format(rc_error_text,2048,rc_error_template,begin);
            rc_input_warning(4011);
        }else{
            IDENTIFIER_SPACE();
            *rc_number_buffer=0;
        }
        if((unsigned)(rc_number_buffer-begin)>512){
            rc_number_buffer=begin+31;
            IDENTIFIER_SPACE();
            *rc_number_buffer=0;
            rc_error_template=rc_message(4011);
            rc_format(rc_error_text,2048,rc_error_template,begin);
            rc_input_warning(4011);
        }
#ifdef RECON_NATIVE_TEST
        /* The original permits 512 characters plus a terminator. That extra
         * destination word is outside the encountered name storage. */
        if(rc_number_buffer<begin || rc_number_buffer-begin+1>512){ __asm int 3 }
#endif
        memcpy(rc_macro_name,begin,(rc_number_buffer-begin+1)*sizeof(WCHAR));
        rc_macro_hash=rc_hash_macro_name(rc_macro_name);
        rc_macro_name_length=rc_number_buffer-begin+1;
    }
    return valid;
}

#undef IDENTIFIER_SPACE

int __cdecl rc_macro_identifier_follows(void)
{
    WCHAR character;
#ifdef RECON_NATIVE_TEST
    RcMacro *macro;
    if(rc_expanding_macro<1||rc_expanding_macro>=64||
       ((DWORD)rc_source_cursor&1)){ __asm int 3 }
    macro=rc_macro_frames[rc_expanding_macro].macro;
    if(!macro||!macro->text||
       (DWORD)rc_source_cursor<(DWORD)macro->text||
       (DWORD)rc_source_cursor>(DWORD)(macro->text+wcslen(macro->text))){ __asm int 3 }
#endif
    character=*rc_source_cursor++;
    if(rc_character_token(character)==36 || rc_character_class(character)&16){
        --rc_source_cursor;
        return 1;
    }
    --rc_source_cursor;
    return 0;
}
