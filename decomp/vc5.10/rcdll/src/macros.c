#include <string.h>
#include <wchar.h>
#ifdef _MSC_VER
#include <excpt.h>
#endif
#include "command.h"
#include "preprocess.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

WCHAR *__stdcall rc_copy_macro_text(const WCHAR *value,unsigned length)
{
    WCHAR *result,*cursor;

    result=rc_allocate(length*sizeof(WCHAR)+sizeof(WCHAR));
    cursor=result;
    if(!result){
        strcpy(rc_error_text,rc_message(1002));
        rc_input_error(1002);
        return NULL;
    }
    /* Windows handles an unreadable source by zeroing the uncopied suffix.
     * Native cases currently prove every source read is valid; a native fault
     * stops the run rather than pretending to execute the Windows handler. */
#ifdef _MSC_VER
    __try {
#endif
        while(length--) *cursor++=*value++;
#ifdef _MSC_VER
    } __except(1) {
        ++length;
        while(length--) *cursor++=0;
    }
#endif
    *cursor=0;
    return result;
}

int rc_macro_listing_readonly,rc_macro_listing_hidden;
WCHAR rc_macro_listing_buffer[2048];

void __stdcall rc_list_macro(RcMacro *macro)
{
    int length;

    if(!rc_macro_listing_enabled || !rc_cpp_output_file) return;
    if(!wcscmp(macro->name,L"APSTUDIO_READONLY_SYMBOLS"))
        rc_macro_listing_readonly=1;
    else if(!wcscmp(macro->name,L"APSTUDIO_HIDDEN_SYMBOLS"))
        rc_macro_listing_hidden=1;
    else if(!rc_macro_listing_hidden && macro->text){
        length=swprintf(rc_macro_listing_buffer,L"%c%s%c%s%c%d%c%c%c",
                        0xe000,macro->name,0xe002,macro->text,0xe002,
                        rc_preparing_line,0xe002,
                        (rc_macro_listing_readonly!=0)|0xffffff80,0xe002);
        rc_write_preprocessed(rc_macro_listing_buffer,length*sizeof(WCHAR),1,
                              rc_cpp_output_file);
    }
}
