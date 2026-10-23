#include <string.h>
#include <wctype.h>
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
