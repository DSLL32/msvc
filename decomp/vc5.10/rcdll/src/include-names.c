#include <string.h>
#include <wctype.h>
#include <wchar.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

WCHAR *__cdecl rc_parse_angle_name(void)
{
    WCHAR *text=rc_macro_name;
    WCHAR character=rc_read_nonspace();
    unsigned error;
    if(character==L'\n'){
        error=2012;
        goto bad_name;
    }
    while(character!=L'>'){
        if(character==L'\n'){
            error=2013;
            goto bad_name;
        }
#ifdef RECON_NATIVE_TEST
        /* The original has no bound; larger scratch writes remain unproved. */
        if(text>=rc_macro_name+511){ __asm int 3 }
#endif
        *text++=character;
        character=rc_get_logical_character();
    }
    while(text>rc_macro_name && iswctype(text[-1],8)) --text;
    *text=0;
    return rc_macro_name;
bad_name:
    --rc_source_cursor;
    strcpy(rc_error_text,rc_message(error));
    rc_input_error(error);
    return NULL;
}

void __cdecl rc_search_include_directories(void)
{
    int index,remaining=rc_cpp_includes.remaining;
    WCHAR *path,*prefix,*destination;
#ifdef RECON_NATIVE_TEST
    if(remaining<0 || remaining>100){ __asm int 3 }
#endif
    for(index=99;index>=remaining;--index){
        path=rc_token_value;
        prefix=rc_cpp_includes.items[index];
        if(prefix && wcscmp(prefix,L"./")){
#ifdef RECON_NATIVE_TEST
            /* Only the identified interior work area and nonempty owned
             * directory names are available to native concatenation. */
            if(!*prefix || ((DWORD)rc_number_buffer&1) ||
               rc_number_buffer<&rc_string_buffer[1024] ||
               rc_number_buffer>=&rc_string_buffer[2045] ||
               wcslen(prefix)+wcslen(path)+2>
                   (unsigned)(&rc_string_buffer[2045]-rc_number_buffer)){
                __asm int 3
            }
#endif
            destination=rc_number_buffer;
            do *destination++=*prefix; while(*prefix++);
            if(!wcschr(rc_cpp_path_characters,prefix[-2]))
                destination[-1]=L'/';
            else --destination;
            do *destination++=*path; while(*path++);
            path=rc_number_buffer;
        }
        if(rc_open_source(path,0))return;
    }
    rc_error_template=rc_message(1015);
    rc_format(rc_error_text,2048,rc_error_template,rc_token_value);
    rc_input_fatal(1015);
}
