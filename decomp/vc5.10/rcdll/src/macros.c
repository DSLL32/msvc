#include <string.h>
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
